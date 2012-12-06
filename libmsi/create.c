/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2002-2004 Mike McCormack for CodeWeavers
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

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "debug.h"
#include "unicode.h"
#include "libmsi.h"
#include "objbase.h"
#include "objidl.h"
#include "msipriv.h"
#include "winnls.h"

#include "query.h"



/* below is the query interface to a table */

typedef struct LibmsiCreateView
{
    LibmsiView          view;
    LibmsiDatabase     *db;
    const WCHAR *         name;
    bool             bIsTemp;
    bool             hold;
    column_info     *col_info;
} LibmsiCreateView;

static unsigned CREATE_fetch_int( LibmsiView *view, unsigned row, unsigned col, unsigned *val )
{
    LibmsiCreateView *cv = (LibmsiCreateView*)view;

    TRACE("%p %d %d %p\n", cv, row, col, val );

    return ERROR_FUNCTION_FAILED;
}

static unsigned CREATE_execute( LibmsiView *view, LibmsiRecord *record )
{
    LibmsiCreateView *cv = (LibmsiCreateView*)view;
    bool persist = (cv->bIsTemp) ? LIBMSI_CONDITION_FALSE : LIBMSI_CONDITION_TRUE;

    TRACE("%p Table %s (%s)\n", cv, debugstr_w(cv->name),
          cv->bIsTemp?"temporary":"permanent");

    if (cv->bIsTemp && !cv->hold)
        return ERROR_SUCCESS;

    return msi_create_table( cv->db, cv->name, cv->col_info, persist );
}

static unsigned CREATE_close( LibmsiView *view )
{
    LibmsiCreateView *cv = (LibmsiCreateView*)view;

    TRACE("%p\n", cv);

    return ERROR_SUCCESS;
}

static unsigned CREATE_get_dimensions( LibmsiView *view, unsigned *rows, unsigned *cols )
{
    LibmsiCreateView *cv = (LibmsiCreateView*)view;

    TRACE("%p %p %p\n", cv, rows, cols );

    return ERROR_FUNCTION_FAILED;
}

static unsigned CREATE_get_column_info( LibmsiView *view, unsigned n, const WCHAR **name,
                                    unsigned *type, bool *temporary, const WCHAR **table_name )
{
    LibmsiCreateView *cv = (LibmsiCreateView*)view;

    TRACE("%p %d %p %p %p %p\n", cv, n, name, type, temporary, table_name );

    return ERROR_FUNCTION_FAILED;
}

static unsigned CREATE_modify( LibmsiView *view, LibmsiModify eModifyMode,
                           LibmsiRecord *rec, unsigned row)
{
    LibmsiCreateView *cv = (LibmsiCreateView*)view;

    TRACE("%p %d %p\n", cv, eModifyMode, rec );

    return ERROR_FUNCTION_FAILED;
}

static unsigned CREATE_delete( LibmsiView *view )
{
    LibmsiCreateView *cv = (LibmsiCreateView*)view;

    TRACE("%p\n", cv );

    msiobj_release( &cv->db->hdr );
    msi_free( cv );

    return ERROR_SUCCESS;
}

static const LibmsiViewOPS create_ops =
{
    CREATE_fetch_int,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    CREATE_execute,
    CREATE_close,
    CREATE_get_dimensions,
    CREATE_get_column_info,
    CREATE_modify,
    CREATE_delete,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

static unsigned check_columns( const column_info *col_info )
{
    const column_info *c1, *c2;

    /* check for two columns with the same name */
    for( c1 = col_info; c1; c1 = c1->next )
        for( c2 = c1->next; c2; c2 = c2->next )
            if (!strcmpW( c1->column, c2->column ))
                return ERROR_BAD_QUERY_SYNTAX;

    return ERROR_SUCCESS;
}

unsigned CREATE_CreateView( LibmsiDatabase *db, LibmsiView **view, const WCHAR *table,
                        column_info *col_info, bool hold )
{
    LibmsiCreateView *cv = NULL;
    unsigned r;
    column_info *col;
    bool temp = true;
    bool tempprim = false;

    TRACE("%p\n", cv );

    r = check_columns( col_info );
    if( r != ERROR_SUCCESS )
        return r;

    cv = alloc_msiobject( sizeof *cv, NULL );
    if( !cv )
        return ERROR_FUNCTION_FAILED;

    for( col = col_info; col; col = col->next )
    {
        if (!col->table)
            col->table = table;

        if( !col->temporary )
            temp = false;
        else if ( col->type & MSITYPE_KEY )
            tempprim = true;
    }

    if ( !temp && tempprim )
    {
        msi_free( cv );
        return ERROR_FUNCTION_FAILED;
    }

    /* fill the structure */
    cv->view.ops = &create_ops;
    msiobj_addref( &db->hdr );
    cv->db = db;
    cv->name = table;
    cv->col_info = col_info;
    cv->bIsTemp = temp;
    cv->hold = hold;
    *view = (LibmsiView*) cv;

    return ERROR_SUCCESS;
}
