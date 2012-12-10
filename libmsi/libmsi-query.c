/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2002-2005 Mike McCormack for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "debug.h"
#include "libmsi.h"
#include "msipriv.h"

#include "query.h"


static void _libmsi_query_destroy( LibmsiObject *arg )
{
    LibmsiQuery *query = (LibmsiQuery*) arg;
    struct list *ptr, *t;

    if( query->view && query->view->ops->delete )
        query->view->ops->delete( query->view );
    msiobj_release( &query->db->hdr );

    LIST_FOR_EACH_SAFE( ptr, t, &query->mem )
    {
        msi_free( ptr );
    }
}

unsigned _libmsi_view_find_column( LibmsiView *table, const char *name, const char *table_name, unsigned *n )
{
    const char *col_name;
    const char *haystack_table_name;
    unsigned i, count, r;

    r = table->ops->get_dimensions( table, NULL, &count );
    if( r != LIBMSI_RESULT_SUCCESS )
        return r;

    for( i=1; i<=count; i++ )
    {
        int x;

        r = table->ops->get_column_info( table, i, &col_name, NULL,
                                         NULL, &haystack_table_name );
        if( r != LIBMSI_RESULT_SUCCESS )
            return r;
        x = strcmp( name, col_name );
        if( table_name )
            x |= strcmp( table_name, haystack_table_name );
        if( !x )
        {
            *n = i;
            return LIBMSI_RESULT_SUCCESS;
        }
    }
    return LIBMSI_RESULT_INVALID_PARAMETER;
}

LibmsiResult libmsi_database_open_query(LibmsiDatabase *db,
              const char *szQuery, LibmsiQuery **pQuery)
{
    unsigned r;

    TRACE("%d %s %p\n", db, debugstr_a(szQuery), pQuery);

    if( !db )
        return LIBMSI_RESULT_INVALID_HANDLE;
    msiobj_addref( &db->hdr);

    r = _libmsi_database_open_query( db, szQuery, pQuery );
    msiobj_release( &db->hdr );

    return r;
}

unsigned _libmsi_database_open_query(LibmsiDatabase *db,
              const char *szQuery, LibmsiQuery **pView)
{
    LibmsiQuery *query;
    unsigned r;

    TRACE("%s %p\n", debugstr_a(szQuery), pView);

    if( !szQuery)
        return LIBMSI_RESULT_INVALID_PARAMETER;

    /* pre allocate a handle to hold a pointer to the view */
    query = alloc_msiobject( sizeof (LibmsiQuery), _libmsi_query_destroy );
    if( !query )
        return LIBMSI_RESULT_FUNCTION_FAILED;

    msiobj_addref( &db->hdr );
    query->db = db;
    list_init( &query->mem );

    r = _libmsi_parse_sql( db, szQuery, &query->view, &query->mem );
    if( r == LIBMSI_RESULT_SUCCESS )
    {
        msiobj_addref( &query->hdr );
        *pView = query;
    }

    msiobj_release( &query->hdr );
    return r;
}

unsigned _libmsi_query_open( LibmsiDatabase *db, LibmsiQuery **view, const char *fmt, ... )
{
    unsigned r;
    int size = 100, res;
    char *query;

    /* construct the string */
    for (;;)
    {
        va_list va;
        query = msi_alloc( size*sizeof(char) );
        va_start(va, fmt);
        res = vsnprintf(query, size, fmt, va);
        va_end(va);
        if (res == -1) size *= 2;
        else if (res >= size) size = res + 1;
        else break;
        msi_free( query );
    }
    /* perform the query */
    r = _libmsi_database_open_query(db, query, view);
    msi_free(query);
    return r;
}

unsigned _libmsi_query_iterate_records( LibmsiQuery *view, unsigned *count,
                         record_func func, void *param )
{
    LibmsiRecord *rec = NULL;
    unsigned r, n = 0, max = 0;

    r = _libmsi_query_execute( view, NULL );
    if( r != LIBMSI_RESULT_SUCCESS )
        return r;

    if( count )
        max = *count;

    /* iterate a query */
    for( n = 0; (max == 0) || (n < max); n++ )
    {
        r = _libmsi_query_fetch( view, &rec );
        if( r != LIBMSI_RESULT_SUCCESS )
            break;
        if (func)
            r = func( rec, param );
        msiobj_release( &rec->hdr );
        if( r != LIBMSI_RESULT_SUCCESS )
            break;
    }

    libmsi_query_close( view );

    if( count )
        *count = n;

    if( r == LIBMSI_RESULT_NO_MORE_ITEMS )
        r = LIBMSI_RESULT_SUCCESS;

    return r;
}

/* return a single record from a query */
LibmsiRecord *_libmsi_query_get_record( LibmsiDatabase *db, const char *fmt, ... )
{
    LibmsiRecord *rec = NULL;
    LibmsiQuery *view = NULL;
    unsigned r;
    int size = 100, res;
    char *query;

    /* construct the string */
    for (;;)
    {
        va_list va;
        query = msi_alloc( size*sizeof(char) );
        va_start(va, fmt);
        res = vsnprintf(query, size, fmt, va);
        va_end(va);
        if (res == -1) size *= 2;
        else if (res >= size) size = res + 1;
        else break;
        msi_free( query );
    }
    /* perform the query */
    r = _libmsi_database_open_query(db, query, &view);
    msi_free(query);

    if( r == LIBMSI_RESULT_SUCCESS )
    {
        _libmsi_query_execute( view, NULL );
        _libmsi_query_fetch( view, &rec );
        libmsi_query_close( view );
        msiobj_release( &view->hdr );
    }
    return rec;
}

unsigned msi_view_get_row(LibmsiDatabase *db, LibmsiView *view, unsigned row, LibmsiRecord **rec)
{
    unsigned row_count = 0, col_count = 0, i, ival, ret, type;

    TRACE("%p %p %d %p\n", db, view, row, rec);

    ret = view->ops->get_dimensions(view, &row_count, &col_count);
    if (ret)
        return ret;

    if (!col_count)
        return LIBMSI_RESULT_INVALID_PARAMETER;

    if (row >= row_count)
        return LIBMSI_RESULT_NO_MORE_ITEMS;

    *rec = libmsi_record_new(col_count);
    if (!*rec)
        return LIBMSI_RESULT_FUNCTION_FAILED;

    for (i = 1; i <= col_count; i++)
    {
        ret = view->ops->get_column_info(view, i, NULL, &type, NULL, NULL);
        if (ret)
        {
            ERR("Error getting column type for %d\n", i);
            continue;
        }

        if (MSITYPE_IS_BINARY(type))
        {
            GsfInput *stm = NULL;

            ret = view->ops->fetch_stream(view, row, i, &stm);
            if ((ret == LIBMSI_RESULT_SUCCESS) && stm)
            {
                _libmsi_record_set_gsf_input(*rec, i, stm);
                g_object_unref(G_OBJECT(stm));
            }
            else
                WARN("failed to get stream\n");

            continue;
        }

        ret = view->ops->fetch_int(view, row, i, &ival);
        if (ret)
        {
            ERR("Error fetching data for %d\n", i);
            continue;
        }

        if (! (type & MSITYPE_VALID))
            ERR("Invalid type!\n");

        /* check if it's nul (0) - if so, don't set anything */
        if (!ival)
            continue;

        if (type & MSITYPE_STRING)
        {
            const char *sval;

            sval = msi_string_lookup_id(db->strings, ival);
            libmsi_record_set_string(*rec, i, sval);
        }
        else
        {
            if ((type & MSI_DATASIZEMASK) == 2)
                libmsi_record_set_int(*rec, i, ival - (1<<15));
            else
                libmsi_record_set_int(*rec, i, ival - (1<<31));
        }
    }

    return LIBMSI_RESULT_SUCCESS;
}

LibmsiResult _libmsi_query_fetch(LibmsiQuery *query, LibmsiRecord **prec)
{
    LibmsiView *view;
    LibmsiResult r;

    TRACE("%p %p\n", query, prec );

    view = query->view;
    if( !view )
        return LIBMSI_RESULT_FUNCTION_FAILED;

    r = msi_view_get_row(query->db, view, query->row, prec);
    if (r == LIBMSI_RESULT_SUCCESS)
        query->row ++;

    return r;
}

LibmsiResult libmsi_query_fetch(LibmsiQuery *query, LibmsiRecord **record)
{
    unsigned ret;

    TRACE("%d %p\n", query, record);

    if( !record )
        return LIBMSI_RESULT_INVALID_PARAMETER;
    *record = 0;

    if( !query )
        return LIBMSI_RESULT_INVALID_HANDLE;
    msiobj_addref( &query->hdr);
    ret = _libmsi_query_fetch( query, record );
    msiobj_release( &query->hdr );
    return ret;
}

LibmsiResult libmsi_query_close(LibmsiQuery *query)
{
    LibmsiView *view;
    unsigned ret;

    TRACE("%p\n", query );

    if( !query )
        return LIBMSI_RESULT_INVALID_HANDLE;

    msiobj_addref( &query->hdr );
    view = query->view;
    if( !view )
        return LIBMSI_RESULT_FUNCTION_FAILED;
    if( !view->ops->close )
        return LIBMSI_RESULT_FUNCTION_FAILED;

    ret = view->ops->close( view );
    msiobj_release( &query->hdr );
    return ret;
}

LibmsiResult _libmsi_query_execute(LibmsiQuery *query, LibmsiRecord *rec )
{
    LibmsiView *view;

    TRACE("%p %p\n", query, rec);

    view = query->view;
    if( !view )
        return LIBMSI_RESULT_FUNCTION_FAILED;
    if( !view->ops->execute )
        return LIBMSI_RESULT_FUNCTION_FAILED;
    query->row = 0;

    return view->ops->execute( view, rec );
}

LibmsiResult libmsi_query_execute(LibmsiQuery *query, LibmsiRecord *rec)
{
    LibmsiResult ret;
    
    TRACE("%d %d\n", query, rec);

    if( !query )
        return LIBMSI_RESULT_INVALID_HANDLE;
    msiobj_addref( &query->hdr );

    if( rec )
        msiobj_addref( &rec->hdr );

    ret = _libmsi_query_execute( query, rec );

    msiobj_release( &query->hdr );
    if( rec )
        msiobj_release( &rec->hdr );

    return ret;
}

static unsigned msi_set_record_type_string( LibmsiRecord *rec, unsigned field,
                                        unsigned type, bool temporary )
{
    static const char fmt[] = "%d";
    char szType[0x10];

    if (MSITYPE_IS_BINARY(type))
        szType[0] = 'v';
    else if (type & MSITYPE_LOCALIZABLE)
        szType[0] = 'l';
    else if (type & MSITYPE_UNKNOWN)
        szType[0] = 'f';
    else if (type & MSITYPE_STRING)
    {
        if (temporary)
            szType[0] = 'g';
        else
          szType[0] = 's';
    }
    else
    {
        if (temporary)
            szType[0] = 'j';
        else
            szType[0] = 'i';
    }

    if (type & MSITYPE_NULLABLE)
        szType[0] &= ~0x20;

    sprintf( &szType[1], fmt, (type&0xff) );

    TRACE("type %04x -> %s\n", type, debugstr_a(szType) );

    return libmsi_record_set_string( rec, field, szType );
}

unsigned _libmsi_query_get_column_info( LibmsiQuery *query, LibmsiColInfo info, LibmsiRecord **prec )
{
    unsigned r = LIBMSI_RESULT_FUNCTION_FAILED, i, count = 0, type;
    LibmsiRecord *rec;
    LibmsiView *view = query->view;
    const char *name;
    bool temporary;

    if( !view )
        return LIBMSI_RESULT_FUNCTION_FAILED;

    if( !view->ops->get_dimensions )
        return LIBMSI_RESULT_FUNCTION_FAILED;

    r = view->ops->get_dimensions( view, NULL, &count );
    if( r != LIBMSI_RESULT_SUCCESS )
        return r;
    if( !count )
        return LIBMSI_RESULT_INVALID_PARAMETER;

    rec = libmsi_record_new( count );
    if( !rec )
        return LIBMSI_RESULT_FUNCTION_FAILED;

    for( i=0; i<count; i++ )
    {
        name = NULL;
        r = view->ops->get_column_info( view, i+1, &name, &type, &temporary, NULL );
        if( r != LIBMSI_RESULT_SUCCESS )
            continue;
        if (info == LIBMSI_COL_INFO_NAMES)
            libmsi_record_set_string( rec, i+1, name );
        else
            msi_set_record_type_string( rec, i+1, type, temporary );
    }
    *prec = rec;
    return LIBMSI_RESULT_SUCCESS;
}

LibmsiResult libmsi_query_get_column_info(LibmsiQuery *query, LibmsiColInfo info, LibmsiRecord **prec)
{
    unsigned r;

    TRACE("%d %d %p\n", query, info, prec);

    if( !prec )
        return LIBMSI_RESULT_INVALID_PARAMETER;

    if( info != LIBMSI_COL_INFO_NAMES && info != LIBMSI_COL_INFO_TYPES )
        return LIBMSI_RESULT_INVALID_PARAMETER;

    if( !query )
        return LIBMSI_RESULT_INVALID_HANDLE;

    msiobj_addref ( &query->hdr );
    r = _libmsi_query_get_column_info( query, info, prec );
    msiobj_release( &query->hdr );

    return r;
}

LibmsiDBError libmsi_query_get_error( LibmsiQuery *query, char *buffer, unsigned *buflen )
{
    const char *column;
    LibmsiDBError r;
    unsigned len;

    TRACE("%u %p %p\n", query, buffer, buflen);

    if (!buflen)
        return LIBMSI_DB_ERROR_INVALIDARG;

    if (!query)
        return LIBMSI_DB_ERROR_INVALIDARG;

    msiobj_addref( &query->hdr);
    if ((r = query->view->error)) column = query->view->error_column;
    else column = szEmpty;

    len = strlen(column);
    if (buffer)
    {
        if (*buflen >= len)
            strcpy(buffer, column);
        else
            r = LIBMSI_DB_ERROR_MOREDATA;
    }

    *buflen = len;
    msiobj_release( &query->hdr );
    return r;
}
