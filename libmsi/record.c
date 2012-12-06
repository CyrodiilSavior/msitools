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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "debug.h"
#include "unicode.h"
#include "libmsi.h"
#include "msipriv.h"
#include "objidl.h"
#include "winnls.h"
#include "ole2.h"

#include "shlwapi.h"

#include "query.h"


#define MSIFIELD_NULL   0
#define MSIFIELD_INT    1
#define MSIFIELD_WSTR   3
#define MSIFIELD_STREAM 4
#define MSIFIELD_INTPTR 5

static void MSI_FreeField( MSIFIELD *field )
{
    switch( field->type )
    {
    case MSIFIELD_NULL:
    case MSIFIELD_INT:
    case MSIFIELD_INTPTR:
        break;
    case MSIFIELD_WSTR:
        msi_free( field->u.szwVal);
        break;
    case MSIFIELD_STREAM:
        IStream_Release( field->u.stream );
        break;
    default:
        ERR("Invalid field type %d\n", field->type);
    }
}

void MSI_CloseRecord( MSIOBJECT *arg )
{
    MSIRECORD *rec = (MSIRECORD *) arg;
    unsigned i;

    for( i=0; i<=rec->count; i++ )
        MSI_FreeField( &rec->fields[i] );
}

MSIRECORD *MSI_CreateRecord( unsigned cParams )
{
    MSIRECORD *rec;
    unsigned len;

    TRACE("%d\n", cParams);

    if( cParams>65535 )
        return NULL;

    len = sizeof (MSIRECORD) + sizeof (MSIFIELD)*cParams;
    rec = alloc_msiobject( MSIOBJECTTYPE_RECORD, len, MSI_CloseRecord );
    if( rec )
        rec->count = cParams;
    return rec;
}

MSIOBJECT * MsiCreateRecord( unsigned cParams )
{
    MSIRECORD *rec;

    TRACE("%d\n", cParams);

    rec = MSI_CreateRecord( cParams );
    return &rec->hdr;
}

unsigned MSI_RecordGetFieldCount( const MSIRECORD *rec )
{
    return rec->count;
}

unsigned MsiRecordGetFieldCount( MSIOBJECT *handle )
{
    MSIRECORD *rec;
    unsigned ret;

    TRACE("%d\n", handle );

    rec = msihandle2msiinfo( handle, MSIOBJECTTYPE_RECORD );
    if( !rec )
        return -1;

    msiobj_lock( &rec->hdr );
    ret = MSI_RecordGetFieldCount( rec );
    msiobj_unlock( &rec->hdr );
    msiobj_release( &rec->hdr );

    return ret;
}

static bool string2intW( const WCHAR *str, int *out )
{
    int x = 0;
    const WCHAR *p = str;

    if( *p == '-' ) /* skip the minus sign */
        p++;
    while ( *p )
    {
        if( (*p < '0') || (*p > '9') )
            return false;
        x *= 10;
        x += (*p - '0');
        p++;
    }

    if( str[0] == '-' ) /* check if it's negative */
        x = -x;
    *out = x; 

    return true;
}

unsigned MSI_RecordCopyField( MSIRECORD *in_rec, unsigned in_n,
                          MSIRECORD *out_rec, unsigned out_n )
{
    unsigned r = ERROR_SUCCESS;

    msiobj_lock( &in_rec->hdr );

    if ( in_n > in_rec->count || out_n > out_rec->count )
        r = ERROR_FUNCTION_FAILED;
    else if ( in_rec != out_rec || in_n != out_n )
    {
        WCHAR *str;
        MSIFIELD *in, *out;

        in = &in_rec->fields[in_n];
        out = &out_rec->fields[out_n];

        switch ( in->type )
        {
        case MSIFIELD_NULL:
            break;
        case MSIFIELD_INT:
            out->u.iVal = in->u.iVal;
            break;
        case MSIFIELD_INTPTR:
            out->u.pVal = in->u.pVal;
            break;
        case MSIFIELD_WSTR:
            str = strdupW( in->u.szwVal );
            if ( !str )
                r = ERROR_OUTOFMEMORY;
            else
                out->u.szwVal = str;
            break;
        case MSIFIELD_STREAM:
            IStream_AddRef( in->u.stream );
            out->u.stream = in->u.stream;
            break;
        default:
            ERR("invalid field type %d\n", in->type);
        }
        if (r == ERROR_SUCCESS)
            out->type = in->type;
    }

    msiobj_unlock( &in_rec->hdr );

    return r;
}

intptr_t MSI_RecordGetIntPtr( MSIRECORD *rec, unsigned iField )
{
    int ret;

    TRACE( "%p %d\n", rec, iField );

    if( iField > rec->count )
        return INTPTR_MIN;

    switch( rec->fields[iField].type )
    {
    case MSIFIELD_INT:
        return rec->fields[iField].u.iVal;
    case MSIFIELD_INTPTR:
        return rec->fields[iField].u.pVal;
    case MSIFIELD_WSTR:
        if( string2intW( rec->fields[iField].u.szwVal, &ret ) )
            return ret;
        return INTPTR_MIN;
    default:
        break;
    }

    return INTPTR_MIN;
}

int MSI_RecordGetInteger( MSIRECORD *rec, unsigned iField)
{
    int ret = 0;

    TRACE("%p %d\n", rec, iField );

    if( iField > rec->count )
        return MSI_NULL_INTEGER;

    switch( rec->fields[iField].type )
    {
    case MSIFIELD_INT:
        return rec->fields[iField].u.iVal;
    case MSIFIELD_INTPTR:
        return rec->fields[iField].u.pVal;
    case MSIFIELD_WSTR:
        if( string2intW( rec->fields[iField].u.szwVal, &ret ) )
            return ret;
        return MSI_NULL_INTEGER;
    default:
        break;
    }

    return MSI_NULL_INTEGER;
}

int MsiRecordGetInteger( MSIOBJECT *handle, unsigned iField)
{
    MSIRECORD *rec;
    unsigned ret;

    TRACE("%d %d\n", handle, iField );

    rec = msihandle2msiinfo( handle, MSIOBJECTTYPE_RECORD );
    if( !rec )
        return MSI_NULL_INTEGER;

    msiobj_lock( &rec->hdr );
    ret = MSI_RecordGetInteger( rec, iField );
    msiobj_unlock( &rec->hdr );
    msiobj_release( &rec->hdr );

    return ret;
}

unsigned MsiRecordClearData( MSIOBJECT *handle )
{
    MSIRECORD *rec;
    unsigned i;

    TRACE("%d\n", handle );

    rec = msihandle2msiinfo( handle, MSIOBJECTTYPE_RECORD );
    if( !rec )
        return ERROR_INVALID_HANDLE;

    msiobj_lock( &rec->hdr );
    for( i=0; i<=rec->count; i++)
    {
        MSI_FreeField( &rec->fields[i] );
        rec->fields[i].type = MSIFIELD_NULL;
        rec->fields[i].u.iVal = 0;
    }
    msiobj_unlock( &rec->hdr );
    msiobj_release( &rec->hdr );

    return ERROR_SUCCESS;
}

unsigned MSI_RecordSetIntPtr( MSIRECORD *rec, unsigned iField, intptr_t pVal )
{
    TRACE("%p %u %ld\n", rec, iField, pVal);

    if( iField > rec->count )
        return ERROR_INVALID_PARAMETER;

    MSI_FreeField( &rec->fields[iField] );
    rec->fields[iField].type = MSIFIELD_INTPTR;
    rec->fields[iField].u.pVal = pVal;

    return ERROR_SUCCESS;
}

unsigned MSI_RecordSetInteger( MSIRECORD *rec, unsigned iField, int iVal )
{
    TRACE("%p %u %d\n", rec, iField, iVal);

    if( iField > rec->count )
        return ERROR_INVALID_PARAMETER;

    MSI_FreeField( &rec->fields[iField] );
    rec->fields[iField].type = MSIFIELD_INT;
    rec->fields[iField].u.iVal = iVal;

    return ERROR_SUCCESS;
}

unsigned MsiRecordSetInteger( MSIOBJECT *handle, unsigned iField, int iVal )
{
    MSIRECORD *rec;
    unsigned ret;

    TRACE("%d %u %d\n", handle, iField, iVal);

    rec = msihandle2msiinfo( handle, MSIOBJECTTYPE_RECORD );
    if( !rec )
        return ERROR_INVALID_HANDLE;

    msiobj_lock( &rec->hdr );
    ret = MSI_RecordSetInteger( rec, iField, iVal );
    msiobj_unlock( &rec->hdr );
    msiobj_release( &rec->hdr );
    return ret;
}

bool MSI_RecordIsNull( MSIRECORD *rec, unsigned iField )
{
    bool r = true;

    TRACE("%p %d\n", rec, iField );

    r = ( iField > rec->count ) ||
        ( rec->fields[iField].type == MSIFIELD_NULL );

    return r;
}

bool MsiRecordIsNull( MSIOBJECT *handle, unsigned iField )
{
    MSIRECORD *rec;
    unsigned ret;

    TRACE("%d %d\n", handle, iField );

    rec = msihandle2msiinfo( handle, MSIOBJECTTYPE_RECORD );
    if( !rec )
        return 0;
    msiobj_lock( &rec->hdr );
    ret = MSI_RecordIsNull( rec, iField );
    msiobj_unlock( &rec->hdr );
    msiobj_release( &rec->hdr );
    return ret;

}

unsigned MSI_RecordGetStringA(MSIRECORD *rec, unsigned iField,
               char *szValue, unsigned *pcchValue)
{
    unsigned len=0, ret;
    char buffer[16];

    TRACE("%p %d %p %p\n", rec, iField, szValue, pcchValue);

    if( iField > rec->count )
    {
        if ( szValue && *pcchValue > 0 )
            szValue[0] = 0;

        *pcchValue = 0;
        return ERROR_SUCCESS;
    }

    ret = ERROR_SUCCESS;
    switch( rec->fields[iField].type )
    {
    case MSIFIELD_INT:
        wsprintfA(buffer, "%d", rec->fields[iField].u.iVal);
        len = lstrlenA( buffer );
        if (szValue)
            lstrcpynA(szValue, buffer, *pcchValue);
        break;
    case MSIFIELD_WSTR:
        len = WideCharToMultiByte( CP_ACP, 0, rec->fields[iField].u.szwVal, -1,
                             NULL, 0 , NULL, NULL);
        if (szValue)
            WideCharToMultiByte( CP_ACP, 0, rec->fields[iField].u.szwVal, -1,
                                 szValue, *pcchValue, NULL, NULL);
        if( szValue && *pcchValue && len>*pcchValue )
            szValue[*pcchValue-1] = 0;
        if( len )
            len--;
        break;
    case MSIFIELD_NULL:
        if( szValue && *pcchValue > 0 )
            szValue[0] = 0;
        break;
    default:
        ret = ERROR_INVALID_PARAMETER;
        break;
    }

    if( szValue && *pcchValue <= len )
        ret = ERROR_MORE_DATA;
    *pcchValue = len;

    return ret;
}

unsigned MsiRecordGetStringA(MSIOBJECT *handle, unsigned iField,
               char *szValue, unsigned *pcchValue)
{
    MSIRECORD *rec;
    unsigned ret;

    TRACE("%d %d %p %p\n", handle, iField, szValue, pcchValue);

    rec = msihandle2msiinfo( handle, MSIOBJECTTYPE_RECORD );
    if( !rec )
        return ERROR_INVALID_HANDLE;
    msiobj_lock( &rec->hdr );
    ret = MSI_RecordGetStringA( rec, iField, szValue, pcchValue);
    msiobj_unlock( &rec->hdr );
    msiobj_release( &rec->hdr );
    return ret;
}

const WCHAR *MSI_RecordGetString( const MSIRECORD *rec, unsigned iField )
{
    if( iField > rec->count )
        return NULL;

    if( rec->fields[iField].type != MSIFIELD_WSTR )
        return NULL;

    return rec->fields[iField].u.szwVal;
}

unsigned MSI_RecordGetStringW(MSIRECORD *rec, unsigned iField,
               WCHAR *szValue, unsigned *pcchValue)
{
    unsigned len=0, ret;
    WCHAR buffer[16];
    static const WCHAR szFormat[] = { '%','d',0 };

    TRACE("%p %d %p %p\n", rec, iField, szValue, pcchValue);

    if( iField > rec->count )
    {
        if ( szValue && *pcchValue > 0 )
            szValue[0] = 0;

        *pcchValue = 0;
        return ERROR_SUCCESS;
    }

    ret = ERROR_SUCCESS;
    switch( rec->fields[iField].type )
    {
    case MSIFIELD_INT:
        wsprintfW(buffer, szFormat, rec->fields[iField].u.iVal);
        len = lstrlenW( buffer );
        if (szValue)
            lstrcpynW(szValue, buffer, *pcchValue);
        break;
    case MSIFIELD_WSTR:
        len = lstrlenW( rec->fields[iField].u.szwVal );
        if (szValue)
            lstrcpynW(szValue, rec->fields[iField].u.szwVal, *pcchValue);
        break;
    case MSIFIELD_NULL:
        if( szValue && *pcchValue > 0 )
            szValue[0] = 0;
        break;
    default:
        break;
    }

    if( szValue && *pcchValue <= len )
        ret = ERROR_MORE_DATA;
    *pcchValue = len;

    return ret;
}

unsigned MsiRecordGetStringW(MSIOBJECT *handle, unsigned iField,
               WCHAR *szValue, unsigned *pcchValue)
{
    MSIRECORD *rec;
    unsigned ret;

    TRACE("%d %d %p %p\n", handle, iField, szValue, pcchValue);

    rec = msihandle2msiinfo( handle, MSIOBJECTTYPE_RECORD );
    if( !rec )
        return ERROR_INVALID_HANDLE;

    msiobj_lock( &rec->hdr );
    ret = MSI_RecordGetStringW( rec, iField, szValue, pcchValue );
    msiobj_unlock( &rec->hdr );
    msiobj_release( &rec->hdr );
    return ret;
}

static unsigned msi_get_stream_size( IStream *stm )
{
    STATSTG stat;
    HRESULT r;

    r = IStream_Stat( stm, &stat, STATFLAG_NONAME );
    if( FAILED(r) )
        return 0;
    return stat.cbSize.QuadPart;
}

static unsigned MSI_RecordDataSize(MSIRECORD *rec, unsigned iField)
{
    TRACE("%p %d\n", rec, iField);

    if( iField > rec->count )
        return 0;

    switch( rec->fields[iField].type )
    {
    case MSIFIELD_INT:
        return sizeof (int);
    case MSIFIELD_WSTR:
        return lstrlenW( rec->fields[iField].u.szwVal );
    case MSIFIELD_NULL:
        break;
    case MSIFIELD_STREAM:
        return msi_get_stream_size( rec->fields[iField].u.stream );
    }
    return 0;
}

unsigned MsiRecordDataSize(MSIOBJECT *handle, unsigned iField)
{
    MSIRECORD *rec;
    unsigned ret;

    TRACE("%d %d\n", handle, iField);

    rec = msihandle2msiinfo( handle, MSIOBJECTTYPE_RECORD );
    if( !rec )
        return 0;
    msiobj_lock( &rec->hdr );
    ret = MSI_RecordDataSize( rec, iField);
    msiobj_unlock( &rec->hdr );
    msiobj_release( &rec->hdr );
    return ret;
}

static unsigned MSI_RecordSetStringA( MSIRECORD *rec, unsigned iField, const char *szValue )
{
    WCHAR *str;

    TRACE("%p %d %s\n", rec, iField, debugstr_a(szValue));

    if( iField > rec->count )
        return ERROR_INVALID_FIELD;

    MSI_FreeField( &rec->fields[iField] );
    if( szValue && szValue[0] )
    {
        str = strdupAtoW( szValue );
        rec->fields[iField].type = MSIFIELD_WSTR;
        rec->fields[iField].u.szwVal = str;
    }
    else
    {
        rec->fields[iField].type = MSIFIELD_NULL;
        rec->fields[iField].u.szwVal = NULL;
    }

    return 0;
}

unsigned MsiRecordSetStringA( MSIOBJECT *handle, unsigned iField, const char *szValue )
{
    MSIRECORD *rec;
    unsigned ret;

    TRACE("%d %d %s\n", handle, iField, debugstr_a(szValue));

    rec = msihandle2msiinfo( handle, MSIOBJECTTYPE_RECORD );
    if( !rec )
        return ERROR_INVALID_HANDLE;
    msiobj_lock( &rec->hdr );
    ret = MSI_RecordSetStringA( rec, iField, szValue );
    msiobj_unlock( &rec->hdr );
    msiobj_release( &rec->hdr );
    return ret;
}

unsigned MSI_RecordSetStringW( MSIRECORD *rec, unsigned iField, const WCHAR *szValue )
{
    WCHAR *str;

    TRACE("%p %d %s\n", rec, iField, debugstr_w(szValue));

    if( iField > rec->count )
        return ERROR_INVALID_FIELD;

    MSI_FreeField( &rec->fields[iField] );

    if( szValue && szValue[0] )
    {
        str = strdupW( szValue );
        rec->fields[iField].type = MSIFIELD_WSTR;
        rec->fields[iField].u.szwVal = str;
    }
    else
    {
        rec->fields[iField].type = MSIFIELD_NULL;
        rec->fields[iField].u.szwVal = NULL;
    }

    return 0;
}

unsigned MsiRecordSetStringW( MSIOBJECT *handle, unsigned iField, const WCHAR *szValue )
{
    MSIRECORD *rec;
    unsigned ret;

    TRACE("%d %d %s\n", handle, iField, debugstr_w(szValue));

    rec = msihandle2msiinfo( handle, MSIOBJECTTYPE_RECORD );
    if( !rec )
        return ERROR_INVALID_HANDLE;

    msiobj_lock( &rec->hdr );
    ret = MSI_RecordSetStringW( rec, iField, szValue );
    msiobj_unlock( &rec->hdr );
    msiobj_release( &rec->hdr );
    return ret;
}

/* read the data in a file into an IStream */
static unsigned RECORD_StreamFromFile(const WCHAR *szFile, IStream **pstm)
{
    unsigned sz, szHighWord = 0, read;
    HANDLE handle;
    HGLOBAL hGlob = 0;
    HRESULT hr;
    ULARGE_INTEGER ulSize;

    TRACE("reading %s\n", debugstr_w(szFile));

    /* read the file into memory */
    handle = CreateFileW(szFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if( handle == INVALID_HANDLE_VALUE )
        return GetLastError();
    sz = GetFileSize(handle, &szHighWord);
    if( sz != INVALID_FILE_SIZE && szHighWord == 0 )
    {
        hGlob = GlobalAlloc(GMEM_FIXED, sz);
        if( hGlob )
        {
            bool r = ReadFile(handle, hGlob, sz, &read, NULL);
            if( !r )
            {
                GlobalFree(hGlob);
                hGlob = 0;
            }
        }
    }
    CloseHandle(handle);
    if( !hGlob )
        return ERROR_FUNCTION_FAILED;

    /* make a stream out of it, and set the correct file size */
    hr = CreateStreamOnHGlobal(hGlob, true, pstm);
    if( FAILED( hr ) )
    {
        GlobalFree(hGlob);
        return ERROR_FUNCTION_FAILED;
    }

    /* set the correct size - CreateStreamOnHGlobal screws it up */
    ulSize.QuadPart = sz;
    IStream_SetSize(*pstm, ulSize);

    TRACE("read %s, %d bytes into IStream %p\n", debugstr_w(szFile), sz, *pstm);

    return ERROR_SUCCESS;
}

unsigned MSI_RecordSetStream(MSIRECORD *rec, unsigned iField, IStream *stream)
{
    if ( (iField == 0) || (iField > rec->count) )
        return ERROR_INVALID_PARAMETER;

    MSI_FreeField( &rec->fields[iField] );
    rec->fields[iField].type = MSIFIELD_STREAM;
    rec->fields[iField].u.stream = stream;

    return ERROR_SUCCESS;
}

unsigned MSI_RecordSetStreamFromFileW(MSIRECORD *rec, unsigned iField, const WCHAR *szFilename)
{
    IStream *stm = NULL;
    HRESULT r;

    if( (iField == 0) || (iField > rec->count) )
        return ERROR_INVALID_PARAMETER;

    /* no filename means we should seek back to the start of the stream */
    if( !szFilename )
    {
        LARGE_INTEGER ofs;
        ULARGE_INTEGER cur;

        if( rec->fields[iField].type != MSIFIELD_STREAM )
            return ERROR_INVALID_FIELD;

        stm = rec->fields[iField].u.stream;
        if( !stm )
            return ERROR_INVALID_FIELD;

        ofs.QuadPart = 0;
        r = IStream_Seek( stm, ofs, STREAM_SEEK_SET, &cur );
        if( FAILED( r ) )
            return ERROR_FUNCTION_FAILED;
    }
    else
    {
        /* read the file into a stream and save the stream in the record */
        r = RECORD_StreamFromFile(szFilename, &stm);
        if( r != ERROR_SUCCESS )
            return r;

        /* if all's good, store it in the record */
        MSI_RecordSetStream(rec, iField, stm);
    }

    return ERROR_SUCCESS;
}

unsigned MsiRecordSetStreamA(MSIOBJECT *hRecord, unsigned iField, const char *szFilename)
{
    WCHAR *wstr = NULL;
    unsigned ret;

    TRACE("%d %d %s\n", hRecord, iField, debugstr_a(szFilename));

    if( szFilename )
    {
        wstr = strdupAtoW( szFilename );
        if( !wstr )
             return ERROR_OUTOFMEMORY;
    }
    ret = MsiRecordSetStreamW(hRecord, iField, wstr);
    msi_free(wstr);

    return ret;
}

unsigned MsiRecordSetStreamW(MSIOBJECT *handle, unsigned iField, const WCHAR *szFilename)
{
    MSIRECORD *rec;
    unsigned ret;

    TRACE("%d %d %s\n", handle, iField, debugstr_w(szFilename));

    rec = msihandle2msiinfo( handle, MSIOBJECTTYPE_RECORD );
    if( !rec )
        return ERROR_INVALID_HANDLE;

    msiobj_lock( &rec->hdr );
    ret = MSI_RecordSetStreamFromFileW( rec, iField, szFilename );
    msiobj_unlock( &rec->hdr );
    msiobj_release( &rec->hdr );
    return ret;
}

unsigned MSI_RecordReadStream(MSIRECORD *rec, unsigned iField, char *buf, unsigned *sz)
{
    unsigned count;
    HRESULT r;
    IStream *stm;

    TRACE("%p %d %p %p\n", rec, iField, buf, sz);

    if( !sz )
        return ERROR_INVALID_PARAMETER;

    if( iField > rec->count)
        return ERROR_INVALID_PARAMETER;

    if ( rec->fields[iField].type == MSIFIELD_NULL )
    {
        *sz = 0;
        return ERROR_INVALID_DATA;
    }

    if( rec->fields[iField].type != MSIFIELD_STREAM )
        return ERROR_INVALID_DATATYPE;

    stm = rec->fields[iField].u.stream;
    if( !stm )
        return ERROR_INVALID_PARAMETER;

    /* if there's no buffer pointer, calculate the length to the end */
    if( !buf )
    {
        LARGE_INTEGER ofs;
        ULARGE_INTEGER end, cur;

        ofs.QuadPart = cur.QuadPart = 0;
        end.QuadPart = 0;
        IStream_Seek( stm, ofs, STREAM_SEEK_SET, &cur );
        IStream_Seek( stm, ofs, STREAM_SEEK_END, &end );
        ofs.QuadPart = cur.QuadPart;
        IStream_Seek( stm, ofs, STREAM_SEEK_SET, &cur );
        *sz = end.QuadPart - cur.QuadPart;

        return ERROR_SUCCESS;
    }

    /* read the data */
    count = 0;
    r = IStream_Read( stm, buf, *sz, &count );
    if( FAILED( r ) )
    {
        *sz = 0;
        return ERROR_FUNCTION_FAILED;
    }

    *sz = count;

    return ERROR_SUCCESS;
}

unsigned MsiRecordReadStream(MSIOBJECT *handle, unsigned iField, char *buf, unsigned *sz)
{
    MSIRECORD *rec;
    unsigned ret;

    TRACE("%d %d %p %p\n", handle, iField, buf, sz);

    rec = msihandle2msiinfo( handle, MSIOBJECTTYPE_RECORD );
    if( !rec )
        return ERROR_INVALID_HANDLE;
    msiobj_lock( &rec->hdr );
    ret = MSI_RecordReadStream( rec, iField, buf, sz );
    msiobj_unlock( &rec->hdr );
    msiobj_release( &rec->hdr );
    return ret;
}

unsigned MSI_RecordSetIStream( MSIRECORD *rec, unsigned iField, IStream *stm )
{
    TRACE("%p %d %p\n", rec, iField, stm);

    if( iField > rec->count )
        return ERROR_INVALID_FIELD;

    MSI_FreeField( &rec->fields[iField] );

    rec->fields[iField].type = MSIFIELD_STREAM;
    rec->fields[iField].u.stream = stm;
    IStream_AddRef( stm );

    return ERROR_SUCCESS;
}

unsigned MSI_RecordGetIStream( MSIRECORD *rec, unsigned iField, IStream **pstm)
{
    TRACE("%p %d %p\n", rec, iField, pstm);

    if( iField > rec->count )
        return ERROR_INVALID_FIELD;

    if( rec->fields[iField].type != MSIFIELD_STREAM )
        return ERROR_INVALID_FIELD;

    *pstm = rec->fields[iField].u.stream;
    IStream_AddRef( *pstm );

    return ERROR_SUCCESS;
}

static unsigned msi_dump_stream_to_file( IStream *stm, const WCHAR *name )
{
    ULARGE_INTEGER size;
    LARGE_INTEGER pos;
    IStream *out;
    unsigned stgm;
    HRESULT r;

    stgm = STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_FAILIFTHERE;
    r = SHCreateStreamOnFileW( name, stgm, &out );
    if( FAILED( r ) )
        return ERROR_FUNCTION_FAILED;

    pos.QuadPart = 0;
    r = IStream_Seek( stm, pos, STREAM_SEEK_END, &size );
    if( FAILED( r ) )
        goto end;

    pos.QuadPart = 0;
    r = IStream_Seek( stm, pos, STREAM_SEEK_SET, NULL );
    if( FAILED( r ) )
        goto end;

    r = IStream_CopyTo( stm, out, size, NULL, NULL );

end:
    IStream_Release( out );
    if( FAILED( r ) )
        return ERROR_FUNCTION_FAILED;
    return ERROR_SUCCESS;
}

unsigned MSI_RecordStreamToFile( MSIRECORD *rec, unsigned iField, const WCHAR *name )
{
    IStream *stm = NULL;
    unsigned r;

    TRACE("%p %u %s\n", rec, iField, debugstr_w(name));

    msiobj_lock( &rec->hdr );

    r = MSI_RecordGetIStream( rec, iField, &stm );
    if( r == ERROR_SUCCESS )
    {
        r = msi_dump_stream_to_file( stm, name );
        IStream_Release( stm );
    }

    msiobj_unlock( &rec->hdr );

    return r;
}

MSIRECORD *MSI_CloneRecord(MSIRECORD *rec)
{
    MSIRECORD *clone;
    unsigned r, i, count;

    count = MSI_RecordGetFieldCount(rec);
    clone = MSI_CreateRecord(count);
    if (!clone)
        return NULL;

    for (i = 0; i <= count; i++)
    {
        if (rec->fields[i].type == MSIFIELD_STREAM)
        {
            if (FAILED(IStream_Clone(rec->fields[i].u.stream,
                                     &clone->fields[i].u.stream)))
            {
                msiobj_release(&clone->hdr);
                return NULL;
            }
            clone->fields[i].type = MSIFIELD_STREAM;
        }
        else
        {
            r = MSI_RecordCopyField(rec, i, clone, i);
            if (r != ERROR_SUCCESS)
            {
                msiobj_release(&clone->hdr);
                return NULL;
            }
        }
    }

    return clone;
}

bool MSI_RecordsAreFieldsEqual(MSIRECORD *a, MSIRECORD *b, unsigned field)
{
    if (a->fields[field].type != b->fields[field].type)
        return false;

    switch (a->fields[field].type)
    {
        case MSIFIELD_NULL:
            break;

        case MSIFIELD_INT:
            if (a->fields[field].u.iVal != b->fields[field].u.iVal)
                return false;
            break;

        case MSIFIELD_WSTR:
            if (strcmpW(a->fields[field].u.szwVal, b->fields[field].u.szwVal))
                return false;
            break;

        case MSIFIELD_STREAM:
        default:
            return false;
    }
    return true;
}


bool MSI_RecordsAreEqual(MSIRECORD *a, MSIRECORD *b)
{
    unsigned i;

    if (a->count != b->count)
        return false;

    for (i = 0; i <= a->count; i++)
    {
        if (!MSI_RecordsAreFieldsEqual( a, b, i ))
            return false;
    }

    return true;
}

WCHAR *msi_dup_record_field( MSIRECORD *rec, int field )
{
    unsigned sz = 0;
    WCHAR *str;
    unsigned r;

    if (MSI_RecordIsNull( rec, field )) return NULL;

    r = MSI_RecordGetStringW( rec, field, NULL, &sz );
    if (r != ERROR_SUCCESS)
        return NULL;

    sz++;
    str = msi_alloc( sz * sizeof(WCHAR) );
    if (!str) return NULL;
    str[0] = 0;
    r = MSI_RecordGetStringW( rec, field, str, &sz );
    if (r != ERROR_SUCCESS)
    {
        ERR("failed to get string!\n");
        msi_free( str );
        return NULL;
    }
    return str;
}
