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

struct tagMSIOBJECT;
typedef struct tagMSIOBJECT MSIOBJECT;

typedef enum tagMSICONDITION
{
    MSICONDITION_FALSE = 0,
    MSICONDITION_TRUE  = 1,
    MSICONDITION_NONE  = 2,
    MSICONDITION_ERROR = 3,
} MSICONDITION;

#define MSI_NULL_INTEGER 0x80000000

typedef enum tagMSICOLINFO
{
    MSICOLINFO_NAMES = 0,
    MSICOLINFO_TYPES = 1
} MSICOLINFO;

typedef enum tagMSIMODIFY
{
    MSIMODIFY_SEEK = -1,
    MSIMODIFY_REFRESH = 0,
    MSIMODIFY_INSERT = 1,
    MSIMODIFY_UPDATE = 2,
    MSIMODIFY_ASSIGN = 3,
    MSIMODIFY_REPLACE = 4,
    MSIMODIFY_MERGE = 5,
    MSIMODIFY_DELETE = 6,
    MSIMODIFY_INSERT_TEMPORARY = 7,
    MSIMODIFY_VALIDATE = 8,
    MSIMODIFY_VALIDATE_NEW = 9,
    MSIMODIFY_VALIDATE_FIELD = 10,
    MSIMODIFY_VALIDATE_DELETE = 11
} MSIMODIFY;

#define MSIDBOPEN_READONLY (LPCTSTR)0
#define MSIDBOPEN_TRANSACT (LPCTSTR)1
#define MSIDBOPEN_DIRECT   (LPCTSTR)2
#define MSIDBOPEN_CREATE   (LPCTSTR)3
#define MSIDBOPEN_CREATEDIRECT (LPCTSTR)4

#define MSIDBOPEN_PATCHFILE 32 / sizeof(*MSIDBOPEN_READONLY)

typedef enum tagMSIDBERROR
{
    MSIDBERROR_INVALIDARG = -3,
    MSIDBERROR_MOREDATA = -2,
    MSIDBERROR_FUNCTIONERROR = -1,
    MSIDBERROR_NOERROR = 0,
    MSIDBERROR_DUPLICATEKEY = 1,
    MSIDBERROR_REQUIRED = 2,
    MSIDBERROR_BADLINK = 3,
    MSIDBERROR_OVERFLOW = 4,
    MSIDBERROR_UNDERFLOW = 5,
    MSIDBERROR_NOTINSET = 6,
    MSIDBERROR_BADVERSION = 7,
    MSIDBERROR_BADCASE = 8,
    MSIDBERROR_BADGUID = 9,
    MSIDBERROR_BADWILDCARD = 10,
    MSIDBERROR_BADIDENTIFIER = 11,
    MSIDBERROR_BADLANGUAGE = 12,
    MSIDBERROR_BADFILENAME = 13,
    MSIDBERROR_BADPATH = 14,
    MSIDBERROR_BADCONDITION = 15,
    MSIDBERROR_BADFORMATTED = 16,
    MSIDBERROR_BADTEMPLATE = 17,
    MSIDBERROR_BADDEFAULTDIR = 18,
    MSIDBERROR_BADREGPATH = 19,
    MSIDBERROR_BADCUSTOMSOURCE = 20,
    MSIDBERROR_BADPROPERTY = 21,
    MSIDBERROR_MISSINGDATA = 22,
    MSIDBERROR_BADCATEGORY = 23,
    MSIDBERROR_BADKEYTABLE = 24,
    MSIDBERROR_BADMAXMINVALUES = 25,
    MSIDBERROR_BADCABINET = 26,
    MSIDBERROR_BADSHORTCUT= 27,
    MSIDBERROR_STRINGOVERFLOW = 28,
    MSIDBERROR_BADLOCALIZEATTRIB = 29
} MSIDBERROR; 

typedef enum tagMSIDBSTATE
{
    MSIDBSTATE_ERROR = -1,
    MSIDBSTATE_READ = 0,
    MSIDBSTATE_WRITE = 1
} MSIDBSTATE;


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
UINT WINAPI MsiViewFetch(MSIOBJECT *,MSIOBJECT **);
UINT WINAPI MsiViewExecute(MSIOBJECT *,MSIOBJECT *);
UINT WINAPI MsiViewClose(MSIOBJECT *);
UINT WINAPI MsiDatabaseOpenViewA(MSIOBJECT *,const CHAR *,MSIOBJECT **);
UINT WINAPI MsiDatabaseOpenViewW(MSIOBJECT *,const WCHAR *,MSIOBJECT **);
#define     MsiDatabaseOpenView WINELIB_NAME_AW(MsiDatabaseOpenView)
MSIDBERROR WINAPI MsiViewGetErrorA(MSIOBJECT *,CHAR *,DWORD *);
MSIDBERROR WINAPI MsiViewGetErrorW(MSIOBJECT *,WCHAR *,DWORD *);
#define     MsiViewGetError WINELIB_NAME_AW(MsiViewGetError)

MSIDBSTATE WINAPI MsiGetDatabaseState(MSIOBJECT *);

/* record manipulation */
MSIOBJECT *WINAPI MsiCreateRecord(UINT);
UINT WINAPI MsiRecordClearData(MSIOBJECT *);
UINT WINAPI MsiRecordSetInteger(MSIOBJECT *,UINT,int);
UINT WINAPI MsiRecordSetStringA(MSIOBJECT *,UINT,const CHAR *);
UINT WINAPI MsiRecordSetStringW(MSIOBJECT *,UINT,const WCHAR *);
#define     MsiRecordSetString WINELIB_NAME_AW(MsiRecordSetString)
UINT WINAPI MsiRecordGetStringA(MSIOBJECT *,UINT,CHAR *,DWORD *);
UINT WINAPI MsiRecordGetStringW(MSIOBJECT *,UINT,WCHAR *,DWORD *);
#define     MsiRecordGetString WINELIB_NAME_AW(MsiRecordGetString)
UINT WINAPI MsiRecordGetFieldCount(MSIOBJECT *);
int WINAPI MsiRecordGetInteger(MSIOBJECT *,UINT);
UINT WINAPI MsiRecordDataSize(MSIOBJECT *,UINT);
BOOL WINAPI MsiRecordIsNull(MSIOBJECT *,UINT);
UINT WINAPI MsiFormatRecordA(MSIOBJECT *,MSIOBJECT *,CHAR *,DWORD *);
UINT WINAPI MsiFormatRecordW(MSIOBJECT *,MSIOBJECT *,WCHAR *,DWORD *);
#define     MsiFormatRecord WINELIB_NAME_AW(MsiFormatRecord)
UINT WINAPI MsiRecordSetStreamA(MSIOBJECT *,UINT,const CHAR *);
UINT WINAPI MsiRecordSetStreamW(MSIOBJECT *,UINT,const WCHAR *);
#define     MsiRecordSetStream WINELIB_NAME_AW(MsiRecordSetStream)
UINT WINAPI MsiRecordReadStream(MSIOBJECT *,UINT,char*,DWORD *);

UINT WINAPI MsiDatabaseGetPrimaryKeysA(MSIOBJECT *,const CHAR *,MSIOBJECT **);
UINT WINAPI MsiDatabaseGetPrimaryKeysW(MSIOBJECT *,const WCHAR *,MSIOBJECT **);
#define     MsiDatabaseGetPrimaryKeys WINELIB_NAME_AW(MsiDatabaseGetPrimaryKeys)

/* database transforms */
UINT WINAPI MsiDatabaseApplyTransformA(MSIOBJECT *,const CHAR *,int);
UINT WINAPI MsiDatabaseApplyTransformW(MSIOBJECT *,const WCHAR *,int);
#define     MsiDatabaseApplyTransform WINELIB_NAME_AW(MsiDatabaseApplyTransform)

UINT WINAPI MsiViewGetColumnInfo(MSIOBJECT *, MSICOLINFO, MSIOBJECT **);

UINT WINAPI MsiCreateTransformSummaryInfoA(MSIOBJECT *, MSIOBJECT *, const CHAR *, int, int);
UINT WINAPI MsiCreateTransformSummaryInfoW(MSIOBJECT *, MSIOBJECT *, const WCHAR *, int, int);
#define     MsiCreateTransformSummaryInfo WINELIB_NAME_AW(MsiCreateTransformSummaryInfo)

UINT WINAPI MsiGetSummaryInformationA(MSIOBJECT *, const CHAR *, UINT, MSIOBJECT **);
UINT WINAPI MsiGetSummaryInformationW(MSIOBJECT *, const WCHAR *, UINT, MSIOBJECT **);
#define     MsiGetSummaryInformation WINELIB_NAME_AW(MsiGetSummaryInformation)

UINT WINAPI MsiSummaryInfoGetPropertyA(MSIOBJECT *,UINT,UINT *,INT *,FILETIME*,CHAR *,DWORD *);
UINT WINAPI MsiSummaryInfoGetPropertyW(MSIOBJECT *,UINT,UINT *,INT *,FILETIME*,WCHAR *,DWORD *);
#define     MsiSummaryInfoGetProperty WINELIB_NAME_AW(MsiSummaryInfoGetProperty)

UINT WINAPI MsiSummaryInfoSetPropertyA(MSIOBJECT *, UINT, UINT, INT, FILETIME*, const CHAR *);
UINT WINAPI MsiSummaryInfoSetPropertyW(MSIOBJECT *, UINT, UINT, INT, FILETIME*, const WCHAR *);
#define     MsiSummaryInfoSetProperty WINELIB_NAME_AW(MsiSummaryInfoSetProperty)

UINT WINAPI MsiDatabaseExportA(MSIOBJECT *, const CHAR *, const CHAR *, const CHAR *);
UINT WINAPI MsiDatabaseExportW(MSIOBJECT *, const WCHAR *, const WCHAR *, const WCHAR *);
#define     MsiDatabaseExport WINELIB_NAME_AW(MsiDatabaseExport)

UINT WINAPI MsiDatabaseImportA(MSIOBJECT *, const CHAR *, const CHAR *);
UINT WINAPI MsiDatabaseImportW(MSIOBJECT *, const WCHAR *, const WCHAR *);
#define     MsiDatabaseImport WINELIB_NAME_AW(MsiDatabaseImport)

UINT WINAPI MsiOpenDatabaseW(const WCHAR *, const WCHAR *, MSIOBJECT **);
UINT WINAPI MsiOpenDatabaseA(const CHAR *, const CHAR *, MSIOBJECT **);
#define     MsiOpenDatabase WINELIB_NAME_AW(MsiOpenDatabase)

MSICONDITION WINAPI MsiDatabaseIsTablePersistentA(MSIOBJECT *, const CHAR *);
MSICONDITION WINAPI MsiDatabaseIsTablePersistentW(MSIOBJECT *, const WCHAR *);
#define     MsiDatabaseIsTablePersistent WINELIB_NAME_AW(MsiDatabaseIsTablePersistent)

UINT WINAPI MsiSummaryInfoPersist(MSIOBJECT *);
UINT WINAPI MsiSummaryInfoGetPropertyCount(MSIOBJECT *,UINT *);

UINT WINAPI MsiViewModify(MSIOBJECT *, MSIMODIFY, MSIOBJECT *);

UINT WINAPI MsiDatabaseMergeA(MSIOBJECT *, MSIOBJECT *, const CHAR *);
UINT WINAPI MsiDatabaseMergeW(MSIOBJECT *, MSIOBJECT *, const WCHAR *);
#define     MsiDatabaseMerge WINELIB_NAME_AW(MsiDatabaseMerge)

/* Non Unicode */
UINT WINAPI MsiDatabaseCommit(MSIOBJECT *);
UINT WINAPI MsiCloseHandle(MSIOBJECT *);

MSIOBJECT *WINAPI MsiGetLastErrorRecord(void);

#ifdef __cplusplus
}
#endif

#endif /* _LIBMSI_H */
