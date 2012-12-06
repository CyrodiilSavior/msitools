/*
 * Copyright (C) 2002,2003 Mike McCormack
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

#ifndef _LIBMSI_H
#define _LIBMSI_H

#include <stdint.h>
#include <stdbool.h>

struct LibmsiObject;
typedef struct LibmsiObject LibmsiObject;

typedef enum LibmsiCondition
{
    LIBMSI_CONDITION_FALSE = 0,
    LIBMSI_CONDITION_TRUE  = 1,
    LIBMSI_CONDITION_NONE  = 2,
    LIBMSI_CONDITION_ERROR = 3,
} LibmsiCondition;

#define MSI_NULL_INTEGER 0x80000000

typedef enum LibmsiColInfo
{
    LIBMSI_COL_INFO_NAMES = 0,
    LIBMSI_COL_INFO_TYPES = 1
} LibmsiColInfo;

typedef enum LibmsiModify
{
    LIBMSI_MODIFY_SEEK = -1,
    LIBMSI_MODIFY_REFRESH = 0,
    LIBMSI_MODIFY_INSERT = 1,
    LIBMSI_MODIFY_UPDATE = 2,
    LIBMSI_MODIFY_ASSIGN = 3,
    LIBMSI_MODIFY_REPLACE = 4,
    LIBMSI_MODIFY_MERGE = 5,
    LIBMSI_MODIFY_DELETE = 6,
    LIBMSI_MODIFY_INSERT_TEMPORARY = 7,
    LIBMSI_MODIFY_VALIDATE = 8,
    LIBMSI_MODIFY_VALIDATE_NEW = 9,
    LIBMSI_MODIFY_VALIDATE_FIELD = 10,
    LIBMSI_MODIFY_VALIDATE_DELETE = 11
} LibmsiModify;

#define LIBMSI_DB_OPEN_READONLY (LPCTSTR)0
#define LIBMSI_DB_OPEN_TRANSACT (LPCTSTR)1
#define LIBMSI_DB_OPEN_DIRECT   (LPCTSTR)2
#define LIBMSI_DB_OPEN_CREATE   (LPCTSTR)3
#define LIBMSI_DB_OPEN_CREATEDIRECT (LPCTSTR)4

#define LIBMSI_DB_OPEN_PATCHFILE 32 / sizeof(*LIBMSI_DB_OPEN_READONLY)

typedef enum LibmsiDBError
{
    LIBMSI_DB_ERROR_INVALIDARG = -3,
    LIBMSI_DB_ERROR_MOREDATA = -2,
    LIBMSI_DB_ERROR_FUNCTIONERROR = -1,
    LIBMSI_DB_ERROR_NOERROR = 0,
    LIBMSI_DB_ERROR_DUPLICATEKEY = 1,
    LIBMSI_DB_ERROR_REQUIRED = 2,
    LIBMSI_DB_ERROR_BADLINK = 3,
    LIBMSI_DB_ERROR_OVERFLOW = 4,
    LIBMSI_DB_ERROR_UNDERFLOW = 5,
    LIBMSI_DB_ERROR_NOTINSET = 6,
    LIBMSI_DB_ERROR_BADVERSION = 7,
    LIBMSI_DB_ERROR_BADCASE = 8,
    LIBMSI_DB_ERROR_BADGUID = 9,
    LIBMSI_DB_ERROR_BADWILDCARD = 10,
    LIBMSI_DB_ERROR_BADIDENTIFIER = 11,
    LIBMSI_DB_ERROR_BADLANGUAGE = 12,
    LIBMSI_DB_ERROR_BADFILENAME = 13,
    LIBMSI_DB_ERROR_BADPATH = 14,
    LIBMSI_DB_ERROR_BADCONDITION = 15,
    LIBMSI_DB_ERROR_BADFORMATTED = 16,
    LIBMSI_DB_ERROR_BADTEMPLATE = 17,
    LIBMSI_DB_ERROR_BADDEFAULTDIR = 18,
    LIBMSI_DB_ERROR_BADREGPATH = 19,
    LIBMSI_DB_ERROR_BADCUSTOMSOURCE = 20,
    LIBMSI_DB_ERROR_BADPROPERTY = 21,
    LIBMSI_DB_ERROR_MISSINGDATA = 22,
    LIBMSI_DB_ERROR_BADCATEGORY = 23,
    LIBMSI_DB_ERROR_BADKEYTABLE = 24,
    LIBMSI_DB_ERROR_BADMAXMINVALUES = 25,
    LIBMSI_DB_ERROR_BADCABINET = 26,
    LIBMSI_DB_ERROR_BADSHORTCUT= 27,
    LIBMSI_DB_ERROR_STRINGOVERFLOW = 28,
    LIBMSI_DB_ERROR_BADLOCALIZEATTRIB = 29
} LibmsiDBError; 

typedef enum LibmsiDBState
{
    LIBMSI_DB_STATE_ERROR = -1,
    LIBMSI_DB_STATE_READ = 0,
    LIBMSI_DB_STATE_WRITE = 1
} LibmsiDBState;


#ifdef __cplusplus
extern "C" {
#endif

#ifndef WINELIB_NAME_AW
#ifdef UNICODE
#define WINELIB_NAME_AW(x) x##W
#else
#define WINELIB_NAME_AW(x) x##A
#endif
#endif

#define MSI_PID_DICTIONARY (0)
#define MSI_PID_CODEPAGE (0x1)
#define MSI_PID_FIRST_USABLE 2
#define MSI_PID_TITLE 2
#define MSI_PID_SUBJECT 3
#define MSI_PID_AUTHOR 4
#define MSI_PID_KEYWORDS 5
#define MSI_PID_COMMENTS 6
#define MSI_PID_TEMPLATE 7
#define MSI_PID_LASTAUTHOR 8
#define MSI_PID_REVNUMBER 9
#define MSI_PID_EDITTIME 10
#define MSI_PID_LASTPRINTED 11
#define MSI_PID_CREATE_DTM 12
#define MSI_PID_LASTSAVE_DTM 13
#define MSI_PID_PAGECOUNT 14
#define MSI_PID_WORDCOUNT 15
#define MSI_PID_CHARCOUNT 16
#define MSI_PID_THUMBNAIL 17
#define MSI_PID_APPNAME 18
#define MSI_PID_SECURITY 19

#define MSI_PID_MSIVERSION MSI_PID_PAGECOUNT
#define MSI_PID_MSISOURCE MSI_PID_WORDCOUNT
#define MSI_PID_MSIRESTRICT MSI_PID_CHARCOUNT


/* view manipulation */
unsigned MsiViewFetch(LibmsiObject *,LibmsiObject **);
unsigned MsiViewExecute(LibmsiObject *,LibmsiObject *);
unsigned MsiViewClose(LibmsiObject *);
unsigned MsiDatabaseOpenViewA(LibmsiObject *,const char *,LibmsiObject **);
unsigned MsiDatabaseOpenViewW(LibmsiObject *,const WCHAR *,LibmsiObject **);
#define     MsiDatabaseOpenView WINELIB_NAME_AW(MsiDatabaseOpenView)
LibmsiDBError MsiViewGetErrorA(LibmsiObject *,char *,unsigned *);
LibmsiDBError MsiViewGetErrorW(LibmsiObject *,WCHAR *,unsigned *);
#define     MsiViewGetError WINELIB_NAME_AW(MsiViewGetError)

LibmsiDBState MsiGetDatabaseState(LibmsiObject *);

/* record manipulation */
LibmsiObject * MsiCreateRecord(unsigned);
unsigned MsiRecordClearData(LibmsiObject *);
unsigned MsiRecordSetInteger(LibmsiObject *,unsigned,int);
unsigned MsiRecordSetStringA(LibmsiObject *,unsigned,const char *);
unsigned MsiRecordSetStringW(LibmsiObject *,unsigned,const WCHAR *);
#define     MsiRecordSetString WINELIB_NAME_AW(MsiRecordSetString)
unsigned MsiRecordGetStringA(LibmsiObject *,unsigned,char *,unsigned *);
unsigned MsiRecordGetStringW(LibmsiObject *,unsigned,WCHAR *,unsigned *);
#define     MsiRecordGetString WINELIB_NAME_AW(MsiRecordGetString)
unsigned MsiRecordGetFieldCount(LibmsiObject *);
int MsiRecordGetInteger(LibmsiObject *,unsigned);
unsigned MsiRecordDataSize(LibmsiObject *,unsigned);
bool MsiRecordIsNull(LibmsiObject *,unsigned);
unsigned MsiFormatRecordA(LibmsiObject *,LibmsiObject *,char *,unsigned *);
unsigned MsiFormatRecordW(LibmsiObject *,LibmsiObject *,WCHAR *,unsigned *);
#define     MsiFormatRecord WINELIB_NAME_AW(MsiFormatRecord)
unsigned MsiRecordSetStreamA(LibmsiObject *,unsigned,const char *);
unsigned MsiRecordSetStreamW(LibmsiObject *,unsigned,const WCHAR *);
#define     MsiRecordSetStream WINELIB_NAME_AW(MsiRecordSetStream)
unsigned MsiRecordReadStream(LibmsiObject *,unsigned,char*,unsigned *);

unsigned MsiDatabaseGetPrimaryKeysA(LibmsiObject *,const char *,LibmsiObject **);
unsigned MsiDatabaseGetPrimaryKeysW(LibmsiObject *,const WCHAR *,LibmsiObject **);
#define     MsiDatabaseGetPrimaryKeys WINELIB_NAME_AW(MsiDatabaseGetPrimaryKeys)

/* database transforms */
unsigned MsiDatabaseApplyTransformA(LibmsiObject *,const char *,int);
unsigned MsiDatabaseApplyTransformW(LibmsiObject *,const WCHAR *,int);
#define     MsiDatabaseApplyTransform WINELIB_NAME_AW(MsiDatabaseApplyTransform)

unsigned MsiViewGetColumnInfo(LibmsiObject *, LibmsiColInfo, LibmsiObject **);

unsigned MsiCreateTransformSummaryInfoA(LibmsiObject *, LibmsiObject *, const char *, int, int);
unsigned MsiCreateTransformSummaryInfoW(LibmsiObject *, LibmsiObject *, const WCHAR *, int, int);
#define     MsiCreateTransformSummaryInfo WINELIB_NAME_AW(MsiCreateTransformSummaryInfo)

unsigned MsiGetSummaryInformationA(LibmsiObject *, const char *, unsigned, LibmsiObject **);
unsigned MsiGetSummaryInformationW(LibmsiObject *, const WCHAR *, unsigned, LibmsiObject **);
#define     MsiGetSummaryInformation WINELIB_NAME_AW(MsiGetSummaryInformation)

unsigned MsiSummaryInfoGetPropertyA(LibmsiObject *,unsigned,unsigned *,int *,FILETIME*,char *,unsigned *);
unsigned MsiSummaryInfoGetPropertyW(LibmsiObject *,unsigned,unsigned *,int *,FILETIME*,WCHAR *,unsigned *);
#define     MsiSummaryInfoGetProperty WINELIB_NAME_AW(MsiSummaryInfoGetProperty)

unsigned MsiSummaryInfoSetPropertyA(LibmsiObject *, unsigned, unsigned, int, FILETIME*, const char *);
unsigned MsiSummaryInfoSetPropertyW(LibmsiObject *, unsigned, unsigned, int, FILETIME*, const WCHAR *);
#define     MsiSummaryInfoSetProperty WINELIB_NAME_AW(MsiSummaryInfoSetProperty)

unsigned MsiDatabaseExportA(LibmsiObject *, const char *, const char *, const char *);
unsigned MsiDatabaseExportW(LibmsiObject *, const WCHAR *, const WCHAR *, const WCHAR *);
#define     MsiDatabaseExport WINELIB_NAME_AW(MsiDatabaseExport)

unsigned MsiDatabaseImportA(LibmsiObject *, const char *, const char *);
unsigned MsiDatabaseImportW(LibmsiObject *, const WCHAR *, const WCHAR *);
#define     MsiDatabaseImport WINELIB_NAME_AW(MsiDatabaseImport)

unsigned MsiOpenDatabaseW(const WCHAR *, const WCHAR *, LibmsiObject **);
unsigned MsiOpenDatabaseA(const char *, const char *, LibmsiObject **);
#define     MsiOpenDatabase WINELIB_NAME_AW(MsiOpenDatabase)

LibmsiCondition MsiDatabaseIsTablePersistentA(LibmsiObject *, const char *);
LibmsiCondition MsiDatabaseIsTablePersistentW(LibmsiObject *, const WCHAR *);
#define     MsiDatabaseIsTablePersistent WINELIB_NAME_AW(MsiDatabaseIsTablePersistent)

unsigned MsiSummaryInfoPersist(LibmsiObject *);
unsigned MsiSummaryInfoGetPropertyCount(LibmsiObject *,unsigned *);

unsigned MsiViewModify(LibmsiObject *, LibmsiModify, LibmsiObject *);

unsigned MsiDatabaseMergeA(LibmsiObject *, LibmsiObject *, const char *);
unsigned MsiDatabaseMergeW(LibmsiObject *, LibmsiObject *, const WCHAR *);
#define     MsiDatabaseMerge WINELIB_NAME_AW(MsiDatabaseMerge)

/* Non Unicode */
unsigned MsiDatabaseCommit(LibmsiObject *);
unsigned MsiCloseHandle(LibmsiObject *);

LibmsiObject * MsiGetLastErrorRecord(void);

#ifdef __cplusplus
}
#endif

#endif /* _LIBMSI_H */
