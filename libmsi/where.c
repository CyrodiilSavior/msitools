/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2002 Mike McCormack for CodeWeavers
 * Copyright 2011 Bernhard Loos
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
#include <assert.h>

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
typedef struct tagMSIROWENTRY
{
    struct tagMSIWHEREVIEW *wv; /* used during sorting */
    unsigned values[1];
} MSIROWENTRY;

typedef struct tagJOINTABLE
{
    struct tagJOINTABLE *next;
    MSIVIEW *view;
    unsigned col_count;
    unsigned row_count;
    unsigned table_index;
} JOINTABLE;

typedef struct tagMSIORDERINFO
{
    unsigned col_count;
    unsigned error;
    union ext_column columns[1];
} MSIORDERINFO;

typedef struct tagMSIWHEREVIEW
{
    MSIVIEW        view;
    MSIDATABASE   *db;
    JOINTABLE     *tables;
    unsigned           row_count;
    unsigned           col_count;
    unsigned           table_count;
    MSIROWENTRY  **reorder;
    unsigned           reorder_size; /* number of entries available in reorder */
    struct expr   *cond;
    unsigned           rec_index;
    MSIORDERINFO  *order_info;
} MSIWHEREVIEW;

static unsigned WHERE_evaluate( MSIWHEREVIEW *wv, const unsigned rows[],
                            struct expr *cond, int *val, MSIRECORD *record );

#define INITIAL_REORDER_SIZE 16

#define INVALID_ROW_INDEX (-1)

static void free_reorder(MSIWHEREVIEW *wv)
{
    unsigned i;

    if (!wv->reorder)
        return;

    for (i = 0; i < wv->row_count; i++)
        msi_free(wv->reorder[i]);

    msi_free( wv->reorder );
    wv->reorder = NULL;
    wv->reorder_size = 0;
    wv->row_count = 0;
}

static unsigned init_reorder(MSIWHEREVIEW *wv)
{
    MSIROWENTRY **new = msi_alloc_zero(sizeof(MSIROWENTRY *) * INITIAL_REORDER_SIZE);
    if (!new)
        return ERROR_OUTOFMEMORY;

    free_reorder(wv);

    wv->reorder = new;
    wv->reorder_size = INITIAL_REORDER_SIZE;

    return ERROR_SUCCESS;
}

static inline unsigned find_row(MSIWHEREVIEW *wv, unsigned row, unsigned *(values[]))
{
    if (row >= wv->row_count)
        return ERROR_NO_MORE_ITEMS;

    *values = wv->reorder[row]->values;

    return ERROR_SUCCESS;
}

static unsigned add_row(MSIWHEREVIEW *wv, unsigned vals[])
{
    MSIROWENTRY *new;

    if (wv->reorder_size <= wv->row_count)
    {
        MSIROWENTRY **new_reorder;
        unsigned newsize = wv->reorder_size * 2;

        new_reorder = msi_realloc_zero(wv->reorder, sizeof(MSIROWENTRY *) * newsize);
        if (!new_reorder)
            return ERROR_OUTOFMEMORY;

        wv->reorder = new_reorder;
        wv->reorder_size = newsize;
    }

    new = msi_alloc(FIELD_OFFSET( MSIROWENTRY, values[wv->table_count] ));

    if (!new)
        return ERROR_OUTOFMEMORY;

    wv->reorder[wv->row_count++] = new;

    memcpy(new->values, vals, wv->table_count * sizeof(unsigned));
    new->wv = wv;

    return ERROR_SUCCESS;
}

static JOINTABLE *find_table(MSIWHEREVIEW *wv, unsigned col, unsigned *table_col)
{
    JOINTABLE *table = wv->tables;

    if(col == 0 || col > wv->col_count)
         return NULL;

    while (col > table->col_count)
    {
        col -= table->col_count;
        table = table->next;
        assert(table);
    }

    *table_col = col;
    return table;
}

static unsigned parse_column(MSIWHEREVIEW *wv, union ext_column *column,
                         unsigned *column_type)
{
    JOINTABLE *table = wv->tables;
    unsigned i, r;

    do
    {
        const WCHAR *table_name;

        if (column->unparsed.table)
        {
            r = table->view->ops->get_column_info(table->view, 1, NULL, NULL,
                                                  NULL, &table_name);
            if (r != ERROR_SUCCESS)
                return r;
            if (strcmpW(table_name, column->unparsed.table) != 0)
                continue;
        }

        for(i = 1; i <= table->col_count; i++)
        {
            const WCHAR *col_name;

            r = table->view->ops->get_column_info(table->view, i, &col_name, column_type,
                                                  NULL, NULL);
            if(r != ERROR_SUCCESS )
                return r;

            if(strcmpW(col_name, column->unparsed.column))
                continue;
            column->parsed.column = i;
            column->parsed.table = table;
            return ERROR_SUCCESS;
        }
    }
    while ((table = table->next));

    WARN("Couldn't find column %s.%s\n", debugstr_w( column->unparsed.table ), debugstr_w( column->unparsed.column ) );
    return ERROR_BAD_QUERY_SYNTAX;
}

static unsigned WHERE_fetch_int( MSIVIEW *view, unsigned row, unsigned col, unsigned *val )
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW*)view;
    JOINTABLE *table;
    unsigned *rows;
    unsigned r;

    TRACE("%p %d %d %p\n", wv, row, col, val );

    if( !wv->tables )
        return ERROR_FUNCTION_FAILED;

    r = find_row(wv, row, &rows);
    if (r != ERROR_SUCCESS)
        return r;

    table = find_table(wv, col, &col);
    if (!table)
        return ERROR_FUNCTION_FAILED;

    return table->view->ops->fetch_int(table->view, rows[table->table_index], col, val);
}

static unsigned WHERE_fetch_stream( MSIVIEW *view, unsigned row, unsigned col, IStream **stm )
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW*)view;
    JOINTABLE *table;
    unsigned *rows;
    unsigned r;

    TRACE("%p %d %d %p\n", wv, row, col, stm );

    if( !wv->tables )
        return ERROR_FUNCTION_FAILED;

    r = find_row(wv, row, &rows);
    if (r != ERROR_SUCCESS)
        return r;

    table = find_table(wv, col, &col);
    if (!table)
        return ERROR_FUNCTION_FAILED;

    return table->view->ops->fetch_stream( table->view, rows[table->table_index], col, stm );
}

static unsigned WHERE_get_row( MSIVIEW *view, unsigned row, MSIRECORD **rec )
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW *)view;

    TRACE("%p %d %p\n", wv, row, rec );

    if (!wv->tables)
        return ERROR_FUNCTION_FAILED;

    return msi_view_get_row( wv->db, view, row, rec );
}

static unsigned WHERE_set_row( MSIVIEW *view, unsigned row, MSIRECORD *rec, unsigned mask )
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW*)view;
    unsigned i, r, offset = 0;
    JOINTABLE *table = wv->tables;
    unsigned *rows;
    unsigned mask_copy = mask;

    TRACE("%p %d %p %08x\n", wv, row, rec, mask );

    if( !wv->tables )
         return ERROR_FUNCTION_FAILED;

    r = find_row(wv, row, &rows);
    if (r != ERROR_SUCCESS)
        return r;

    if (mask >= 1 << wv->col_count)
        return ERROR_INVALID_PARAMETER;

    do
    {
        for (i = 0; i < table->col_count; i++) {
            unsigned type;

            if (!(mask_copy & (1 << i)))
                continue;
            r = table->view->ops->get_column_info(table->view, i + 1, NULL,
                                            &type, NULL, NULL );
            if (r != ERROR_SUCCESS)
                return r;
            if (type & MSITYPE_KEY)
                return ERROR_FUNCTION_FAILED;
        }
        mask_copy >>= table->col_count;
    }
    while (mask_copy && (table = table->next));

    table = wv->tables;

    do
    {
        const unsigned col_count = table->col_count;
        unsigned i;
        MSIRECORD *reduced;
        unsigned reduced_mask = (mask >> offset) & ((1 << col_count) - 1);

        if (!reduced_mask)
        {
            offset += col_count;
            continue;
        }

        reduced = MSI_CreateRecord(col_count);
        if (!reduced)
            return ERROR_FUNCTION_FAILED;

        for (i = 1; i <= col_count; i++)
        {
            r = MSI_RecordCopyField(rec, i + offset, reduced, i);
            if (r != ERROR_SUCCESS)
                break;
        }

        offset += col_count;

        if (r == ERROR_SUCCESS)
            r = table->view->ops->set_row(table->view, rows[table->table_index], reduced, reduced_mask);

        msiobj_release(&reduced->hdr);
    }
    while ((table = table->next));
    return r;
}

static unsigned WHERE_delete_row(MSIVIEW *view, unsigned row)
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW *)view;
    unsigned r;
    unsigned *rows;

    TRACE("(%p %d)\n", view, row);

    if (!wv->tables)
        return ERROR_FUNCTION_FAILED;

    r = find_row(wv, row, &rows);
    if ( r != ERROR_SUCCESS )
        return r;

    if (wv->table_count > 1)
        return ERROR_CALL_NOT_IMPLEMENTED;

    return wv->tables->view->ops->delete_row(wv->tables->view, rows[0]);
}

static int INT_evaluate_binary( MSIWHEREVIEW *wv, const unsigned rows[],
                                const struct complex_expr *expr, int *val, MSIRECORD *record )
{
    unsigned rl, rr;
    int lval, rval;

    rl = WHERE_evaluate(wv, rows, expr->left, &lval, record);
    if (rl != ERROR_SUCCESS && rl != ERROR_CONTINUE)
        return rl;
    rr = WHERE_evaluate(wv, rows, expr->right, &rval, record);
    if (rr != ERROR_SUCCESS && rr != ERROR_CONTINUE)
        return rr;

    if (rl == ERROR_CONTINUE || rr == ERROR_CONTINUE)
    {
        if (rl == rr)
        {
            *val = true;
            return ERROR_CONTINUE;
        }

        if (expr->op == OP_AND)
        {
            if ((rl == ERROR_CONTINUE && !rval) || (rr == ERROR_CONTINUE && !lval))
            {
                *val = false;
                return ERROR_SUCCESS;
            }
        }
        else if (expr->op == OP_OR)
        {
            if ((rl == ERROR_CONTINUE && rval) || (rr == ERROR_CONTINUE && lval))
            {
                *val = true;
                return ERROR_SUCCESS;
            }
        }

        *val = true;
        return ERROR_CONTINUE;
    }

    switch( expr->op )
    {
    case OP_EQ:
        *val = ( lval == rval );
        break;
    case OP_AND:
        *val = ( lval && rval );
        break;
    case OP_OR:
        *val = ( lval || rval );
        break;
    case OP_GT:
        *val = ( lval > rval );
        break;
    case OP_LT:
        *val = ( lval < rval );
        break;
    case OP_LE:
        *val = ( lval <= rval );
        break;
    case OP_GE:
        *val = ( lval >= rval );
        break;
    case OP_NE:
        *val = ( lval != rval );
        break;
    default:
        ERR("Unknown operator %d\n", expr->op );
        return ERROR_FUNCTION_FAILED;
    }

    return ERROR_SUCCESS;
}

static inline unsigned expr_fetch_value(const union ext_column *expr, const unsigned rows[], unsigned *val)
{
    JOINTABLE *table = expr->parsed.table;

    if( rows[table->table_index] == INVALID_ROW_INDEX )
    {
        *val = 1;
        return ERROR_CONTINUE;
    }
    return table->view->ops->fetch_int(table->view, rows[table->table_index],
                                        expr->parsed.column, val);
}


static unsigned INT_evaluate_unary( MSIWHEREVIEW *wv, const unsigned rows[],
                                const struct complex_expr *expr, int *val, MSIRECORD *record )
{
    unsigned r;
    unsigned lval;

    r = expr_fetch_value(&expr->left->u.column, rows, &lval);
    if(r != ERROR_SUCCESS)
        return r;

    switch( expr->op )
    {
    case OP_ISNULL:
        *val = !lval;
        break;
    case OP_NOTNULL:
        *val = lval;
        break;
    default:
        ERR("Unknown operator %d\n", expr->op );
        return ERROR_FUNCTION_FAILED;
    }
    return ERROR_SUCCESS;
}

static unsigned STRING_evaluate( MSIWHEREVIEW *wv, const unsigned rows[],
                                     const struct expr *expr,
                                     const MSIRECORD *record,
                                     const WCHAR **str )
{
    unsigned val = 0, r = ERROR_SUCCESS;

    switch( expr->type )
    {
    case EXPR_COL_NUMBER_STRING:
        r = expr_fetch_value(&expr->u.column, rows, &val);
        if (r == ERROR_SUCCESS)
            *str =  msi_string_lookup_id(wv->db->strings, val);
        else
            *str = NULL;
        break;

    case EXPR_SVAL:
        *str = expr->u.sval;
        break;

    case EXPR_WILDCARD:
        *str = MSI_RecordGetString(record, ++wv->rec_index);
        break;

    default:
        ERR("Invalid expression type\n");
        r = ERROR_FUNCTION_FAILED;
        *str = NULL;
        break;
    }
    return r;
}

static unsigned STRCMP_Evaluate( MSIWHEREVIEW *wv, const unsigned rows[], const struct complex_expr *expr,
                             int *val, const MSIRECORD *record )
{
    int sr;
    const WCHAR *l_str, *r_str;
    unsigned r;

    *val = true;
    r = STRING_evaluate(wv, rows, expr->left, record, &l_str);
    if (r == ERROR_CONTINUE)
        return r;
    r = STRING_evaluate(wv, rows, expr->right, record, &r_str);
    if (r == ERROR_CONTINUE)
        return r;

    if( l_str == r_str ||
        ((!l_str || !*l_str) && (!r_str || !*r_str)) )
        sr = 0;
    else if( l_str && ! r_str )
        sr = 1;
    else if( r_str && ! l_str )
        sr = -1;
    else
        sr = strcmpW( l_str, r_str );

    *val = ( expr->op == OP_EQ && ( sr == 0 ) ) ||
           ( expr->op == OP_NE && ( sr != 0 ) );

    return ERROR_SUCCESS;
}

static unsigned WHERE_evaluate( MSIWHEREVIEW *wv, const unsigned rows[],
                            struct expr *cond, int *val, MSIRECORD *record )
{
    unsigned r, tval;

    if( !cond )
    {
        *val = true;
        return ERROR_SUCCESS;
    }

    switch( cond->type )
    {
    case EXPR_COL_NUMBER:
        r = expr_fetch_value(&cond->u.column, rows, &tval);
        if( r != ERROR_SUCCESS )
            return r;
        *val = tval - 0x8000;
        return ERROR_SUCCESS;

    case EXPR_COL_NUMBER32:
        r = expr_fetch_value(&cond->u.column, rows, &tval);
        if( r != ERROR_SUCCESS )
            return r;
        *val = tval - 0x80000000;
        return r;

    case EXPR_UVAL:
        *val = cond->u.uval;
        return ERROR_SUCCESS;

    case EXPR_COMPLEX:
        return INT_evaluate_binary(wv, rows, &cond->u.expr, val, record);

    case EXPR_UNARY:
        return INT_evaluate_unary( wv, rows, &cond->u.expr, val, record );

    case EXPR_STRCMP:
        return STRCMP_Evaluate( wv, rows, &cond->u.expr, val, record );

    case EXPR_WILDCARD:
        *val = MSI_RecordGetInteger( record, ++wv->rec_index );
        return ERROR_SUCCESS;

    default:
        ERR("Invalid expression type\n");
        break;
    }

    return ERROR_SUCCESS;
}

static unsigned check_condition( MSIWHEREVIEW *wv, MSIRECORD *record, JOINTABLE **tables,
                             unsigned table_rows[] )
{
    unsigned r = ERROR_FUNCTION_FAILED;
    int val;

    for (table_rows[(*tables)->table_index] = 0;
         table_rows[(*tables)->table_index] < (*tables)->row_count;
         table_rows[(*tables)->table_index]++)
    {
        val = 0;
        wv->rec_index = 0;
        r = WHERE_evaluate( wv, table_rows, wv->cond, &val, record );
        if (r != ERROR_SUCCESS && r != ERROR_CONTINUE)
            break;
        if (val)
        {
            if (*(tables + 1))
            {
                r = check_condition(wv, record, tables + 1, table_rows);
                if (r != ERROR_SUCCESS)
                    break;
            }
            else
            {
                if (r != ERROR_SUCCESS)
                    break;
                add_row (wv, table_rows);
            }
        }
    }
    table_rows[(*tables)->table_index] = INVALID_ROW_INDEX;
    return r;
}

static int compare_entry( const void *left, const void *right )
{
    const MSIROWENTRY *le = *(const MSIROWENTRY**)left;
    const MSIROWENTRY *re = *(const MSIROWENTRY**)right;
    const MSIWHEREVIEW *wv = le->wv;
    MSIORDERINFO *order = wv->order_info;
    unsigned i, j, r, l_val, r_val;

    assert(le->wv == re->wv);

    if (order)
    {
        for (i = 0; i < order->col_count; i++)
        {
            const union ext_column *column = &order->columns[i];

            r = column->parsed.table->view->ops->fetch_int(column->parsed.table->view,
                          le->values[column->parsed.table->table_index],
                          column->parsed.column, &l_val);
            if (r != ERROR_SUCCESS)
            {
                order->error = r;
                return 0;
            }

            r = column->parsed.table->view->ops->fetch_int(column->parsed.table->view,
                          re->values[column->parsed.table->table_index],
                          column->parsed.column, &r_val);
            if (r != ERROR_SUCCESS)
            {
                order->error = r;
                return 0;
            }

            if (l_val != r_val)
                return l_val < r_val ? -1 : 1;
        }
    }

    for (j = 0; j < wv->table_count; j++)
    {
        if (le->values[j] != re->values[j])
            return le->values[j] < re->values[j] ? -1 : 1;
    }
    return 0;
}

static void add_to_array( JOINTABLE **array, JOINTABLE *elem )
{
    while (*array && *array != elem)
        array++;
    if (!*array)
        *array = elem;
}

static bool in_array( JOINTABLE **array, JOINTABLE *elem )
{
    while (*array && *array != elem)
        array++;
    return *array != NULL;
}

#define CONST_EXPR 1 /* comparison to a constant value */
#define JOIN_TO_CONST_EXPR 0x10000 /* comparison to a table involved with
                                      a CONST_EXPR comaprison */

static unsigned reorder_check( const struct expr *expr, JOINTABLE **ordered_tables,
                           bool process_joins, JOINTABLE **lastused )
{
    unsigned res = 0;

    switch (expr->type)
    {
        case EXPR_WILDCARD:
        case EXPR_SVAL:
        case EXPR_UVAL:
            return 0;
        case EXPR_COL_NUMBER:
        case EXPR_COL_NUMBER32:
        case EXPR_COL_NUMBER_STRING:
            if (in_array(ordered_tables, expr->u.column.parsed.table))
                return JOIN_TO_CONST_EXPR;
            *lastused = expr->u.column.parsed.table;
            return CONST_EXPR;
        case EXPR_STRCMP:
        case EXPR_COMPLEX:
            res = reorder_check(expr->u.expr.right, ordered_tables, process_joins, lastused);
            /* fall through */
        case EXPR_UNARY:
            res += reorder_check(expr->u.expr.left, ordered_tables, process_joins, lastused);
            if (res == 0)
                return 0;
            if (res == CONST_EXPR)
                add_to_array(ordered_tables, *lastused);
            if (process_joins && res == JOIN_TO_CONST_EXPR + CONST_EXPR)
                add_to_array(ordered_tables, *lastused);
            return res;
        default:
            ERR("Unknown expr type: %i\n", expr->type);
            assert(0);
            return 0x1000000;
    }
}

/* reorders the tablelist in a way to evaluate the condition as fast as possible */
static JOINTABLE **ordertables( MSIWHEREVIEW *wv )
{
    JOINTABLE *table;
    JOINTABLE **tables;

    tables = msi_alloc_zero( (wv->table_count + 1) * sizeof(*tables) );

    if (wv->cond)
    {
        table = NULL;
        reorder_check(wv->cond, tables, false, &table);
        table = NULL;
        reorder_check(wv->cond, tables, true, &table);
    }

    table = wv->tables;
    while (table)
    {
        add_to_array(tables, table);
        table = table->next;
    }
    return tables;
}

static unsigned WHERE_execute( MSIVIEW *view, MSIRECORD *record )
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW*)view;
    unsigned r;
    JOINTABLE *table = wv->tables;
    unsigned *rows;
    JOINTABLE **ordered_tables;
    int i = 0;

    TRACE("%p %p\n", wv, record);

    if( !table )
         return ERROR_FUNCTION_FAILED;

    r = init_reorder(wv);
    if (r != ERROR_SUCCESS)
        return r;

    do
    {
        table->view->ops->execute(table->view, NULL);

        r = table->view->ops->get_dimensions(table->view, &table->row_count, NULL);
        if (r != ERROR_SUCCESS)
        {
            ERR("failed to get table dimensions\n");
            return r;
        }

        /* each table must have at least one row */
        if (table->row_count == 0)
            return ERROR_SUCCESS;
    }
    while ((table = table->next));

    ordered_tables = ordertables( wv );

    rows = msi_alloc( wv->table_count * sizeof(*rows) );
    for (i = 0; i < wv->table_count; i++)
        rows[i] = INVALID_ROW_INDEX;

    r =  check_condition(wv, record, ordered_tables, rows);

    if (wv->order_info)
        wv->order_info->error = ERROR_SUCCESS;

    qsort(wv->reorder, wv->row_count, sizeof(MSIROWENTRY *), compare_entry);

    if (wv->order_info)
        r = wv->order_info->error;

    msi_free( rows );
    msi_free( ordered_tables );
    return r;
}

static unsigned WHERE_close( MSIVIEW *view )
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW*)view;
    JOINTABLE *table = wv->tables;

    TRACE("%p\n", wv );

    if (!table)
        return ERROR_FUNCTION_FAILED;

    do
        table->view->ops->close(table->view);
    while ((table = table->next));

    return ERROR_SUCCESS;
}

static unsigned WHERE_get_dimensions( MSIVIEW *view, unsigned *rows, unsigned *cols )
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW*)view;

    TRACE("%p %p %p\n", wv, rows, cols );

    if(!wv->tables)
         return ERROR_FUNCTION_FAILED;

    if (rows)
    {
        if (!wv->reorder)
            return ERROR_FUNCTION_FAILED;
        *rows = wv->row_count;
    }

    if (cols)
        *cols = wv->col_count;

    return ERROR_SUCCESS;
}

static unsigned WHERE_get_column_info( MSIVIEW *view, unsigned n, const WCHAR **name,
                                   unsigned *type, bool *temporary, const WCHAR **table_name )
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW*)view;
    JOINTABLE *table;

    TRACE("%p %d %p %p %p %p\n", wv, n, name, type, temporary, table_name );

    if(!wv->tables)
         return ERROR_FUNCTION_FAILED;

    table = find_table(wv, n, &n);
    if (!table)
        return ERROR_FUNCTION_FAILED;

    return table->view->ops->get_column_info(table->view, n, name,
                                            type, temporary, table_name);
}

static unsigned join_find_row( MSIWHEREVIEW *wv, MSIRECORD *rec, unsigned *row )
{
    const WCHAR *str;
    unsigned r, i, id, data;

    str = MSI_RecordGetString( rec, 1 );
    r = msi_string2idW( wv->db->strings, str, &id );
    if (r != ERROR_SUCCESS)
        return r;

    for (i = 0; i < wv->row_count; i++)
    {
        WHERE_fetch_int( &wv->view, i, 1, &data );

        if (data == id)
        {
            *row = i;
            return ERROR_SUCCESS;
        }
    }

    return ERROR_FUNCTION_FAILED;
}

static unsigned join_modify_update( MSIVIEW *view, MSIRECORD *rec )
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW *)view;
    unsigned r, row, i, mask = 0;
    MSIRECORD *current;


    r = join_find_row( wv, rec, &row );
    if (r != ERROR_SUCCESS)
        return r;

    r = msi_view_get_row( wv->db, view, row, &current );
    if (r != ERROR_SUCCESS)
        return r;

    assert(MSI_RecordGetFieldCount(rec) == MSI_RecordGetFieldCount(current));

    for (i = MSI_RecordGetFieldCount(rec); i > 0; i--)
    {
        if (!MSI_RecordsAreFieldsEqual(rec, current, i))
            mask |= 1 << (i - 1);
    }
     msiobj_release(&current->hdr);

    return WHERE_set_row( view, row, rec, mask );
}

static unsigned WHERE_modify( MSIVIEW *view, MSIMODIFY eModifyMode,
                          MSIRECORD *rec, unsigned row )
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW*)view;
    JOINTABLE *table = wv->tables;
    unsigned r;

    TRACE("%p %d %p\n", wv, eModifyMode, rec);

    if (!table)
        return ERROR_FUNCTION_FAILED;

    if (!table->next)
    {
        unsigned *rows;

        if (find_row(wv, row - 1, &rows) == ERROR_SUCCESS)
            row = rows[0] + 1;
        else
            row = -1;

        return table->view->ops->modify(table->view, eModifyMode, rec, row);
    }

    switch (eModifyMode)
    {
    case MSIMODIFY_UPDATE:
        return join_modify_update( view, rec );

    case MSIMODIFY_ASSIGN:
    case MSIMODIFY_DELETE:
    case MSIMODIFY_INSERT:
    case MSIMODIFY_INSERT_TEMPORARY:
    case MSIMODIFY_MERGE:
    case MSIMODIFY_REPLACE:
    case MSIMODIFY_SEEK:
    case MSIMODIFY_VALIDATE:
    case MSIMODIFY_VALIDATE_DELETE:
    case MSIMODIFY_VALIDATE_FIELD:
    case MSIMODIFY_VALIDATE_NEW:
        r = ERROR_FUNCTION_FAILED;
        break;

    case MSIMODIFY_REFRESH:
        r = ERROR_CALL_NOT_IMPLEMENTED;
        break;

    default:
        WARN("%p %d %p %u - unknown mode\n", view, eModifyMode, rec, row );
        r = ERROR_INVALID_PARAMETER;
        break;
    }

    return r;
}

static unsigned WHERE_delete( MSIVIEW *view )
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW*)view;
    JOINTABLE *table = wv->tables;

    TRACE("%p\n", wv );

    while(table)
    {
        JOINTABLE *next;

        table->view->ops->delete(table->view);
        table->view = NULL;
        next = table->next;
        msi_free(table);
        table = next;
    }
    wv->tables = NULL;
    wv->table_count = 0;

    free_reorder(wv);

    msi_free(wv->order_info);
    wv->order_info = NULL;

    msiobj_release( &wv->db->hdr );
    msi_free( wv );

    return ERROR_SUCCESS;
}

static unsigned WHERE_find_matching_rows( MSIVIEW *view, unsigned col,
    unsigned val, unsigned *row, MSIITERHANDLE *handle )
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW*)view;
    unsigned i, row_value;

    TRACE("%p, %d, %u, %p\n", view, col, val, *handle);

    if (!wv->tables)
         return ERROR_FUNCTION_FAILED;

    if (col == 0 || col > wv->col_count)
        return ERROR_INVALID_PARAMETER;

    for (i = PtrToUlong(*handle); i < wv->row_count; i++)
    {
        if (view->ops->fetch_int( view, i, col, &row_value ) != ERROR_SUCCESS)
            continue;

        if (row_value == val)
        {
            *row = i;
            *handle = UlongToPtr(i + 1);
            return ERROR_SUCCESS;
        }
    }

    return ERROR_NO_MORE_ITEMS;
}

static unsigned WHERE_sort(MSIVIEW *view, column_info *columns)
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW *)view;
    JOINTABLE *table = wv->tables;
    column_info *column = columns;
    MSIORDERINFO *orderinfo;
    unsigned r, count = 0;
    int i;

    TRACE("%p %p\n", view, columns);

    if (!table)
        return ERROR_FUNCTION_FAILED;

    while (column)
    {
        count++;
        column = column->next;
    }

    if (count == 0)
        return ERROR_SUCCESS;

    orderinfo = msi_alloc(sizeof(MSIORDERINFO) + (count - 1) * sizeof(union ext_column));
    if (!orderinfo)
        return ERROR_OUTOFMEMORY;

    orderinfo->col_count = count;

    column = columns;

    for (i = 0; i < count; i++)
    {
        orderinfo->columns[i].unparsed.column = column->column;
        orderinfo->columns[i].unparsed.table = column->table;

        r = parse_column(wv, &orderinfo->columns[i], NULL);
        if (r != ERROR_SUCCESS)
            goto error;
    }

    wv->order_info = orderinfo;

    return ERROR_SUCCESS;
error:
    msi_free(orderinfo);
    return r;
}

static const MSIVIEWOPS where_ops =
{
    WHERE_fetch_int,
    WHERE_fetch_stream,
    WHERE_get_row,
    WHERE_set_row,
    NULL,
    WHERE_delete_row,
    WHERE_execute,
    WHERE_close,
    WHERE_get_dimensions,
    WHERE_get_column_info,
    WHERE_modify,
    WHERE_delete,
    WHERE_find_matching_rows,
    NULL,
    NULL,
    NULL,
    NULL,
    WHERE_sort,
    NULL,
};

static unsigned WHERE_VerifyCondition( MSIWHEREVIEW *wv, struct expr *cond,
                                   unsigned *valid )
{
    unsigned r;

    switch( cond->type )
    {
    case EXPR_COLUMN:
    {
        unsigned type;

        *valid = false;

        r = parse_column(wv, &cond->u.column, &type);
        if (r != ERROR_SUCCESS)
            break;

        if (type&MSITYPE_STRING)
            cond->type = EXPR_COL_NUMBER_STRING;
        else if ((type&0xff) == 4)
            cond->type = EXPR_COL_NUMBER32;
        else
            cond->type = EXPR_COL_NUMBER;

        *valid = true;
        break;
    }
    case EXPR_COMPLEX:
        r = WHERE_VerifyCondition( wv, cond->u.expr.left, valid );
        if( r != ERROR_SUCCESS )
            return r;
        if( !*valid )
            return ERROR_SUCCESS;
        r = WHERE_VerifyCondition( wv, cond->u.expr.right, valid );
        if( r != ERROR_SUCCESS )
            return r;

        /* check the type of the comparison */
        if( ( cond->u.expr.left->type == EXPR_SVAL ) ||
            ( cond->u.expr.left->type == EXPR_COL_NUMBER_STRING ) ||
            ( cond->u.expr.right->type == EXPR_SVAL ) ||
            ( cond->u.expr.right->type == EXPR_COL_NUMBER_STRING ) )
        {
            switch( cond->u.expr.op )
            {
            case OP_EQ:
            case OP_NE:
                break;
            default:
                *valid = false;
                return ERROR_INVALID_PARAMETER;
            }

            /* FIXME: check we're comparing a string to a column */

            cond->type = EXPR_STRCMP;
        }

        break;
    case EXPR_UNARY:
        if ( cond->u.expr.left->type != EXPR_COLUMN )
        {
            *valid = false;
            return ERROR_INVALID_PARAMETER;
        }
        r = WHERE_VerifyCondition( wv, cond->u.expr.left, valid );
        if( r != ERROR_SUCCESS )
            return r;
        break;
    case EXPR_IVAL:
        *valid = 1;
        cond->type = EXPR_UVAL;
        cond->u.uval = cond->u.ival;
        break;
    case EXPR_WILDCARD:
        *valid = 1;
        break;
    case EXPR_SVAL:
        *valid = 1;
        break;
    default:
        ERR("Invalid expression type\n");
        *valid = 0;
        break;
    }

    return ERROR_SUCCESS;
}

unsigned WHERE_CreateView( MSIDATABASE *db, MSIVIEW **view, WCHAR *tables,
                       struct expr *cond )
{
    MSIWHEREVIEW *wv = NULL;
    unsigned r, valid = 0;
    WCHAR *ptr;

    TRACE("(%s)\n", debugstr_w(tables) );

    wv = msi_alloc_zero( sizeof *wv );
    if( !wv )
        return ERROR_FUNCTION_FAILED;
    
    /* fill the structure */
    wv->view.ops = &where_ops;
    msiobj_addref( &db->hdr );
    wv->db = db;
    wv->cond = cond;

    while (*tables)
    {
        JOINTABLE *table;

        if ((ptr = strchrW(tables, ' ')))
            *ptr = '\0';

        table = msi_alloc(sizeof(JOINTABLE));
        if (!table)
        {
            r = ERROR_OUTOFMEMORY;
            goto end;
        }

        r = TABLE_CreateView(db, tables, &table->view);
        if (r != ERROR_SUCCESS)
        {
            WARN("can't create table: %s\n", debugstr_w(tables));
            msi_free(table);
            r = ERROR_BAD_QUERY_SYNTAX;
            goto end;
        }

        r = table->view->ops->get_dimensions(table->view, NULL,
                                             &table->col_count);
        if (r != ERROR_SUCCESS)
        {
            ERR("can't get table dimensions\n");
            goto end;
        }

        wv->col_count += table->col_count;
        table->table_index = wv->table_count++;

        table->next = wv->tables;
        wv->tables = table;

        if (!ptr)
            break;

        tables = ptr + 1;
    }

    if( cond )
    {
        r = WHERE_VerifyCondition( wv, cond, &valid );
        if( r != ERROR_SUCCESS )
            goto end;
        if( !valid ) {
            r = ERROR_FUNCTION_FAILED;
            goto end;
        }
    }

    *view = (MSIVIEW*) wv;

    return ERROR_SUCCESS;
end:
    WHERE_delete(&wv->view);

    return r;
}
