/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2008 James Hawkins
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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "ole2.h"
#include "libmsi.h"
#include "objbase.h"
#include "msipriv.h"
#include "query.h"

#include "debug.h"


#define NUM_STORAGES_COLS    2
#define MAX_STORAGES_NAME_LEN 62

typedef struct tabSTORAGE
{
    unsigned str_index;
} STORAGE;

typedef struct LibmsiStorageView
{
    LibmsiView view;
    LibmsiDatabase *db;
    STORAGE **storages;
    unsigned max_storages;
    unsigned num_rows;
    unsigned row_size;
} LibmsiStorageView;

static bool storages_set_table_size(LibmsiStorageView *sv, unsigned size)
{
    if (size >= sv->max_storages)
    {
        sv->max_storages *= 2;
        sv->storages = msi_realloc(sv->storages, sv->max_storages * sizeof(STORAGE *));
        if (!sv->storages)
            return false;
    }

    return true;
}

static STORAGE *create_storage(LibmsiStorageView *sv, const WCHAR *name)
{
    STORAGE *storage;

    storage = msi_alloc(sizeof(STORAGE));
    if (!storage)
        return NULL;

    storage->str_index = _libmsi_add_string(sv->db->strings, name, -1, 1, StringNonPersistent);

    return storage;
}

static unsigned storages_view_fetch_int(LibmsiView *view, unsigned row, unsigned col, unsigned *val)
{
    LibmsiStorageView *sv = (LibmsiStorageView *)view;

    TRACE("(%p, %d, %d, %p)\n", view, row, col, val);

    if (col != 1)
        return LIBMSI_RESULT_INVALID_PARAMETER;

    if (row >= sv->num_rows)
        return LIBMSI_RESULT_NO_MORE_ITEMS;

    *val = sv->storages[row]->str_index;

    return LIBMSI_RESULT_SUCCESS;
}

static unsigned storages_view_fetch_stream(LibmsiView *view, unsigned row, unsigned col, IStream **stm)
{
    LibmsiStorageView *sv = (LibmsiStorageView *)view;

    TRACE("(%p, %d, %d, %p)\n", view, row, col, stm);

    if (row >= sv->num_rows)
        return LIBMSI_RESULT_FUNCTION_FAILED;

    return LIBMSI_RESULT_INVALID_DATA;
}

static unsigned storages_view_get_row( LibmsiView *view, unsigned row, LibmsiRecord **rec )
{
    LibmsiStorageView *sv = (LibmsiStorageView *)view;

    FIXME("%p %d %p\n", sv, row, rec);

    return LIBMSI_RESULT_CALL_NOT_IMPLEMENTED;
}

static unsigned storages_view_set_row(LibmsiView *view, unsigned row, LibmsiRecord *rec, unsigned mask)
{
    LibmsiStorageView *sv = (LibmsiStorageView *)view;
    IStream *stm;
    WCHAR *name = NULL;
    unsigned r = LIBMSI_RESULT_FUNCTION_FAILED;

    TRACE("(%p, %p)\n", view, rec);

    if (row > sv->num_rows)
        return LIBMSI_RESULT_FUNCTION_FAILED;

    r = _libmsi_record_get_IStream(rec, 2, &stm);
    if (r != LIBMSI_RESULT_SUCCESS)
        return r;

    name = strdupW(_libmsi_record_get_string_raw(rec, 1));
    if (!name)
    {
        r = LIBMSI_RESULT_OUTOFMEMORY;
        goto done;
    }

    msi_create_storage(sv->db, name, stm);
    if (r != LIBMSI_RESULT_SUCCESS)
    {
        IStream_Release(stm);
        return r;
    }

    sv->storages[row] = create_storage(sv, name);
    if (!sv->storages[row])
        r = LIBMSI_RESULT_FUNCTION_FAILED;

done:
    msi_free(name);
    IStream_Release(stm);

    return r;
}

static unsigned storages_view_insert_row(LibmsiView *view, LibmsiRecord *rec, unsigned row, bool temporary)
{
    LibmsiStorageView *sv = (LibmsiStorageView *)view;

    if (!storages_set_table_size(sv, ++sv->num_rows))
        return LIBMSI_RESULT_FUNCTION_FAILED;

    if (row == -1)
        row = sv->num_rows - 1;

    /* FIXME have to readjust rows */

    return storages_view_set_row(view, row, rec, 0);
}

static unsigned storages_view_delete_row(LibmsiView *view, unsigned row)
{
    LibmsiStorageView *sv = (LibmsiStorageView *)view;
    const WCHAR *name;
    WCHAR *encname;
    unsigned i;

    if (row > sv->num_rows)
        return LIBMSI_RESULT_FUNCTION_FAILED;

    name = msi_string_lookup_id(sv->db->strings, sv->storages[row]->str_index);
    if (!name)
    {
        WARN("failed to retrieve storage name\n");
        return LIBMSI_RESULT_FUNCTION_FAILED;
    }

    msi_destroy_storage(sv->db, name);

    /* shift the remaining rows */
    for (i = row + 1; i < sv->num_rows; i++)
    {
        sv->storages[i - 1] = sv->storages[i];
    }
    sv->num_rows--;

    return LIBMSI_RESULT_SUCCESS;
}

static unsigned storages_view_execute(LibmsiView *view, LibmsiRecord *record)
{
    TRACE("(%p, %p)\n", view, record);
    return LIBMSI_RESULT_SUCCESS;
}

static unsigned storages_view_close(LibmsiView *view)
{
    TRACE("(%p)\n", view);
    return LIBMSI_RESULT_SUCCESS;
}

static unsigned storages_view_get_dimensions(LibmsiView *view, unsigned *rows, unsigned *cols)
{
    LibmsiStorageView *sv = (LibmsiStorageView *)view;

    TRACE("(%p, %p, %p)\n", view, rows, cols);

    if (cols) *cols = NUM_STORAGES_COLS;
    if (rows) *rows = sv->num_rows;

    return LIBMSI_RESULT_SUCCESS;
}

static unsigned storages_view_get_column_info( LibmsiView *view, unsigned n, const WCHAR **name,
                                      unsigned *type, bool *temporary, const WCHAR **table_name )
{
    TRACE("(%p, %d, %p, %p, %p, %p)\n", view, n, name, type, temporary,
          table_name);

    if (n == 0 || n > NUM_STORAGES_COLS)
        return LIBMSI_RESULT_INVALID_PARAMETER;

    switch (n)
    {
    case 1:
        if (name) *name = szName;
        if (type) *type = MSITYPE_STRING | MSITYPE_VALID | MAX_STORAGES_NAME_LEN;
        break;

    case 2:
        if (name) *name = szData;
        if (type) *type = MSITYPE_STRING | MSITYPE_VALID | MSITYPE_NULLABLE;
        break;
    }
    if (table_name) *table_name = szStorages;
    if (temporary) *temporary = false;
    return LIBMSI_RESULT_SUCCESS;
}

static unsigned storages_view_delete(LibmsiView *view)
{
    LibmsiStorageView *sv = (LibmsiStorageView *)view;
    unsigned i;

    TRACE("(%p)\n", view);

    for (i = 0; i < sv->num_rows; i++)
        msi_free(sv->storages[i]);

    msi_free(sv->storages);
    sv->storages = NULL;
    msi_free(sv);

    return LIBMSI_RESULT_SUCCESS;
}

static unsigned storages_view_find_matching_rows(LibmsiView *view, unsigned col,
                                       unsigned val, unsigned *row, MSIITERHANDLE *handle)
{
    LibmsiStorageView *sv = (LibmsiStorageView *)view;
    unsigned index = PtrToUlong(*handle);

    TRACE("(%d, %d): %d\n", *row, col, val);

    if (col == 0 || col > NUM_STORAGES_COLS)
        return LIBMSI_RESULT_INVALID_PARAMETER;

    while (index < sv->num_rows)
    {
        if (sv->storages[index]->str_index == val)
        {
            *row = index;
            break;
        }

        index++;
    }

    *handle = UlongToPtr(++index);
    if (index >= sv->num_rows)
        return LIBMSI_RESULT_NO_MORE_ITEMS;

    return LIBMSI_RESULT_SUCCESS;
}

static const LibmsiViewOps storages_ops =
{
    storages_view_fetch_int,
    storages_view_fetch_stream,
    storages_view_get_row,
    storages_view_set_row,
    storages_view_insert_row,
    storages_view_delete_row,
    storages_view_execute,
    storages_view_close,
    storages_view_get_dimensions,
    storages_view_get_column_info,
    storages_view_delete,
    storages_view_find_matching_rows,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

static int add_storages_to_table(LibmsiStorageView *sv)
{
    STORAGE *storage = NULL;
    IEnumSTATSTG *stgenum = NULL;
    STATSTG stat;
    HRESULT hr;
    unsigned r;
    unsigned count = 0, size;

    hr = IStorage_EnumElements(sv->db->infile, 0, NULL, 0, &stgenum);
    if (FAILED(hr))
        return -1;

    sv->max_storages = 1;
    sv->storages = msi_alloc(sizeof(STORAGE *));
    if (!sv->storages)
        return -1;

    while (true)
    {
        size = 0;
        hr = IEnumSTATSTG_Next(stgenum, 1, &stat, &size);
        if (FAILED(hr) || !size)
            break;

        if (stat.type != STGTY_STORAGE)
        {
            CoTaskMemFree(stat.pwcsName);
            continue;
        }

        TRACE("enumerated storage %s\n", debugstr_w(stat.pwcsName));

        r = msi_open_storage(sv->db, stat.pwcsName);
        if (r != LIBMSI_RESULT_SUCCESS)
        {
            count = -1;
            CoTaskMemFree(stat.pwcsName);
            break;
        }

        storage = create_storage(sv, stat.pwcsName);
        if (!storage)
        {
            count = -1;
            CoTaskMemFree(stat.pwcsName);
            break;
        }

        CoTaskMemFree(stat.pwcsName);

        if (!storages_set_table_size(sv, ++count))
        {
            count = -1;
            break;
        }

        sv->storages[count - 1] = storage;
    }

    IEnumSTATSTG_Release(stgenum);
    return count;
}

unsigned storages_view_create(LibmsiDatabase *db, LibmsiView **view)
{
    LibmsiStorageView *sv;
    int rows;

    TRACE("(%p, %p)\n", db, view);

    sv = alloc_msiobject( sizeof(LibmsiStorageView), NULL );
    if (!sv)
        return LIBMSI_RESULT_FUNCTION_FAILED;

    sv->view.ops = &storages_ops;
    sv->db = db;

    rows = add_storages_to_table(sv);
    if (rows < 0)
    {
        msi_free( sv );
        return LIBMSI_RESULT_FUNCTION_FAILED;
    }
    sv->num_rows = rows;

    *view = (LibmsiView *)sv;

    return LIBMSI_RESULT_SUCCESS;
}
