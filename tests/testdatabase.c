/*
 * Copyright (C) 2005 Mike McCormack for CodeWeavers
 *
 * A test program for MSI database files.
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

#define COBJMACROS

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include <windows.h>
#include <libmsi.h>

#include <objidl.h>

#include "test.h"

static const char *msifile = "winetest-db.msi";
static const char *msifile2 = "winetst2-db.msi";
static const char *mstfile = "winetst-db.mst";
static const WCHAR msifileW[] = {'w','i','n','e','t','e','s','t','-','d','b','.','m','s','i',0};

static void test_msidatabase(void)
{
    LibmsiObject *hdb = 0;
    LibmsiObject *hdb2 = 0;
    unsigned res;

    DeleteFile(msifile);

    res = MsiOpenDatabase( msifile, msifile2, &hdb );
    ok( res == ERROR_OPEN_FAILED, "expected failure\n");

    res = MsiOpenDatabase( msifile, (LPSTR) 0xff, &hdb );
    ok( res == ERROR_INVALID_PARAMETER, "expected failure\n");

    res = MsiCloseHandle( hdb );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );

    /* create an empty database */
    res = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_CREATE, &hdb );
    ok( res == ERROR_SUCCESS , "Failed to create database\n" );

    res = MsiDatabaseCommit( hdb );
    ok( res == ERROR_SUCCESS , "Failed to commit database\n" );

    ok( INVALID_FILE_ATTRIBUTES != GetFileAttributes( msifile ), "database should exist\n");

    res = MsiCloseHandle( hdb );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );
    res = MsiOpenDatabase( msifile, msifile2, &hdb2 );
    ok( res == ERROR_SUCCESS , "Failed to open database\n" );

    ok( INVALID_FILE_ATTRIBUTES != GetFileAttributes( msifile2 ), "database should exist\n");

    res = MsiDatabaseCommit( hdb2 );
    ok( res == ERROR_SUCCESS , "Failed to commit database\n" );

    res = MsiCloseHandle( hdb2 );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );

    res = MsiOpenDatabase( msifile, msifile2, &hdb2 );
    ok( res == ERROR_SUCCESS , "Failed to open database\n" );

    res = MsiCloseHandle( hdb2 );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );

    ok( INVALID_FILE_ATTRIBUTES == GetFileAttributes( msifile2 ), "uncommitted database should not exist\n");

    res = MsiOpenDatabase( msifile, msifile2, &hdb2 );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );

    res = MsiDatabaseCommit( hdb2 );
    ok( res == ERROR_SUCCESS , "Failed to commit database\n" );

    res = MsiCloseHandle( hdb2 );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );

    ok( INVALID_FILE_ATTRIBUTES != GetFileAttributes( msifile2 ), "committed database should exist\n");

    res = MsiOpenDatabase( msifile, LIBMSI_DB_OPEN_READONLY, &hdb );
    ok( res == ERROR_SUCCESS , "Failed to open database\n" );

    res = MsiDatabaseCommit( hdb );
    ok( res == ERROR_SUCCESS , "Failed to commit database\n" );

    res = MsiCloseHandle( hdb );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );

    res = MsiOpenDatabase( msifile, LIBMSI_DB_OPEN_DIRECT, &hdb );
    ok( res == ERROR_SUCCESS , "Failed to open database\n" );

    res = MsiCloseHandle( hdb );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );

    res = MsiOpenDatabase( msifile, LIBMSI_DB_OPEN_TRANSACT, &hdb );
    ok( res == ERROR_SUCCESS , "Failed to open database\n" );

    res = MsiCloseHandle( hdb );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );
    ok( INVALID_FILE_ATTRIBUTES != GetFileAttributes( msifile ), "database should exist\n");

    /* LIBMSI_DB_OPEN_CREATE deletes the database if MsiCommitDatabase isn't called */
    res = MsiOpenDatabase( msifile, LIBMSI_DB_OPEN_CREATE, &hdb );
    ok( res == ERROR_SUCCESS , "Failed to open database\n" );

    ok( INVALID_FILE_ATTRIBUTES != GetFileAttributes( msifile ), "database should exist\n");

    res = MsiCloseHandle( hdb );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );

    ok( INVALID_FILE_ATTRIBUTES == GetFileAttributes( msifile ), "database should exist\n");

    res = MsiOpenDatabase( msifile, LIBMSI_DB_OPEN_CREATE, &hdb );
    ok( res == ERROR_SUCCESS , "Failed to open database\n" );

    res = MsiDatabaseCommit( hdb );
    ok( res == ERROR_SUCCESS , "Failed to commit database\n" );

    ok( INVALID_FILE_ATTRIBUTES != GetFileAttributes( msifile ), "database should exist\n");

    res = MsiCloseHandle( hdb );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );

    res = DeleteFile( msifile2 );
    ok( res == true, "Failed to delete database\n" );

    res = DeleteFile( msifile );
    ok( res == true, "Failed to delete database\n" );
}

static unsigned do_query(LibmsiObject *hdb, const char *sql, LibmsiObject **phrec)
{
    LibmsiObject *hquery = 0;
    unsigned r, ret;

    if (phrec)
        *phrec = 0;

    /* open a select query */
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    if (r != ERROR_SUCCESS)
        return r;
    r = MsiQueryExecute(hquery, 0);
    if (r != ERROR_SUCCESS)
        return r;
    ret = MsiQueryFetch(hquery, phrec);
    r = MsiQueryClose(hquery);
    if (r != ERROR_SUCCESS)
        return r;
    r = MsiCloseHandle(hquery);
    if (r != ERROR_SUCCESS)
        return r;
    return ret;
}

static unsigned run_query( LibmsiObject *hdb, LibmsiObject *hrec, const char *sql )
{
    LibmsiObject *hquery = 0;
    unsigned r;

    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    if( r != ERROR_SUCCESS )
        return r;

    r = MsiQueryExecute(hquery, hrec);
    if( r == ERROR_SUCCESS )
        r = MsiQueryClose(hquery);
    MsiCloseHandle(hquery);
    return r;
}

static unsigned create_component_table( LibmsiObject *hdb )
{
    return run_query( hdb, 0,
            "CREATE TABLE `Component` ( "
            "`Component` CHAR(72) NOT NULL, "
            "`ComponentId` CHAR(38), "
            "`Directory_` CHAR(72) NOT NULL, "
            "`Attributes` SHORT NOT NULL, "
            "`Condition` CHAR(255), "
            "`KeyPath` CHAR(72) "
            "PRIMARY KEY `Component`)" );
}

static unsigned create_custom_action_table( LibmsiObject *hdb )
{
    return run_query( hdb, 0,
            "CREATE TABLE `CustomAction` ( "
            "`Action` CHAR(72) NOT NULL, "
            "`Type` SHORT NOT NULL, "
            "`Source` CHAR(72), "
            "`Target` CHAR(255) "
            "PRIMARY KEY `Action`)" );
}

static unsigned create_directory_table( LibmsiObject *hdb )
{
    return run_query( hdb, 0,
            "CREATE TABLE `Directory` ( "
            "`Directory` CHAR(255) NOT NULL, "
            "`Directory_Parent` CHAR(255), "
            "`DefaultDir` CHAR(255) NOT NULL "
            "PRIMARY KEY `Directory`)" );
}

static unsigned create_feature_components_table( LibmsiObject *hdb )
{
    return run_query( hdb, 0,
            "CREATE TABLE `FeatureComponents` ( "
            "`Feature_` CHAR(38) NOT NULL, "
            "`Component_` CHAR(72) NOT NULL "
            "PRIMARY KEY `Feature_`, `Component_` )" );
}

static unsigned create_std_dlls_table( LibmsiObject *hdb )
{
    return run_query( hdb, 0,
            "CREATE TABLE `StdDlls` ( "
            "`File` CHAR(255) NOT NULL, "
            "`Binary_` CHAR(72) NOT NULL "
            "PRIMARY KEY `File` )" );
}

static unsigned create_binary_table( LibmsiObject *hdb )
{
    return run_query( hdb, 0,
            "CREATE TABLE `Binary` ( "
            "`Name` CHAR(72) NOT NULL, "
            "`Data` CHAR(72) NOT NULL "
            "PRIMARY KEY `Name` )" );
}

#define make_add_entry(type, qtext) \
    static unsigned add##_##type##_##entry( LibmsiObject *hdb, const char *values ) \
    { \
        char insert[] = qtext; \
        char *sql; \
        unsigned sz, r; \
        sz = strlen(values) + sizeof insert; \
        sql = HeapAlloc(GetProcessHeap(),0,sz); \
        sprintf(sql,insert,values); \
        r = run_query( hdb, 0, sql ); \
        HeapFree(GetProcessHeap(), 0, sql); \
        return r; \
    }

make_add_entry(component,
               "INSERT INTO `Component`  "
               "(`Component`, `ComponentId`, `Directory_`, "
               "`Attributes`, `Condition`, `KeyPath`) VALUES( %s )")

make_add_entry(custom_action,
               "INSERT INTO `CustomAction`  "
               "(`Action`, `Type`, `Source`, `Target`) VALUES( %s )")

make_add_entry(feature_components,
               "INSERT INTO `FeatureComponents` "
               "(`Feature_`, `Component_`) VALUES( %s )")

make_add_entry(std_dlls,
               "INSERT INTO `StdDlls` (`File`, `Binary_`) VALUES( %s )")

make_add_entry(binary,
               "INSERT INTO `Binary` (`Name`, `Data`) VALUES( %s )")

static void test_msiinsert(void)
{
    LibmsiObject *hdb = 0;
    LibmsiObject *hquery = 0;
    LibmsiObject *hquery2 = 0;
    LibmsiObject *hrec = 0;
    unsigned r;
    const char *sql;
    char buf[80];
    unsigned sz;

    DeleteFile(msifile);

    /* just MsiOpenDatabase should not create a file */
    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    /* create a table */
    sql = "CREATE TABLE `phone` ( "
            "`id` INT, `name` CHAR(32), `number` CHAR(32) "
            "PRIMARY KEY `id`)";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenQuery failed\n");
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");
    r = MsiQueryClose(hquery);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(hquery);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    sql = "SELECT * FROM phone WHERE number = '8675309'";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery2);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenQuery failed\n");
    r = MsiQueryExecute(hquery2, 0);
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");
    r = MsiQueryFetch(hquery2, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "MsiQueryFetch produced items\n");

    /* insert a value into it */
    sql = "INSERT INTO `phone` ( `id`, `name`, `number` )"
        "VALUES('1', 'Abe', '8675309')";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenQuery failed\n");
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");
    r = MsiQueryClose(hquery);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(hquery);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = MsiQueryFetch(hquery2, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "MsiQueryFetch produced items\n");
    r = MsiQueryExecute(hquery2, 0);
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");
    r = MsiQueryFetch(hquery2, &hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed: %u\n", r);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");
    r = MsiQueryClose(hquery2);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(hquery2);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    sql = "SELECT * FROM `phone` WHERE `id` = 1";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed\n");

    /* check the record contains what we put in it */
    r = MsiRecordGetFieldCount(hrec);
    ok(r == 3, "record count wrong\n");

    r = MsiRecordIsNull(hrec, 0);
    ok(r == false, "field 0 not null\n");

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 1, "field 1 contents wrong\n");
    sz = sizeof buf;
    r = MsiRecordGetString(hrec, 2, buf, &sz);
    ok(r == ERROR_SUCCESS, "field 2 content fetch failed\n");
    ok(!strcmp(buf,"Abe"), "field 2 content incorrect\n");
    sz = sizeof buf;
    r = MsiRecordGetString(hrec, 3, buf, &sz);
    ok(r == ERROR_SUCCESS, "field 3 content fetch failed\n");
    ok(!strcmp(buf,"8675309"), "field 3 content incorrect\n");

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* open a select query */
    hrec = 100;
    sql = "SELECT * FROM `phone` WHERE `id` >= 10";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "MsiQueryFetch failed\n");
    ok(hrec == 0, "hrec should be null\n");

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    sql = "SELECT * FROM `phone` WHERE `id` < 0";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "MsiQueryFetch failed\n");

    sql = "SELECT * FROM `phone` WHERE `id` <= 0";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "MsiQueryFetch failed\n");

    sql = "SELECT * FROM `phone` WHERE `id` <> 1";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "MsiQueryFetch failed\n");

    sql = "SELECT * FROM `phone` WHERE `id` > 10";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "MsiQueryFetch failed\n");

    /* now try a few bad INSERT xqueries */
    sql = "INSERT INTO `phone` ( `id`, `name`, `number` )"
        "VALUES(?, ?)";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "MsiDatabaseOpenQuery failed\n");

    /* construct a record to insert */
    hrec = MsiCreateRecord(4);
    r = MsiRecordSetInteger(hrec, 1, 2);
    ok(r == ERROR_SUCCESS, "MsiRecordSetInteger failed\n");
    r = MsiRecordSetString(hrec, 2, "Adam");
    ok(r == ERROR_SUCCESS, "MsiRecordSetString failed\n");
    r = MsiRecordSetString(hrec, 3, "96905305");
    ok(r == ERROR_SUCCESS, "MsiRecordSetString failed\n");

    /* insert another value, using a record and wildcards */
    sql = "INSERT INTO `phone` ( `id`, `name`, `number` )"
        "VALUES(?, ?, ?)";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenQuery failed\n");

    if (r == ERROR_SUCCESS)
    {
        r = MsiQueryExecute(hquery, hrec);
        ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");
        r = MsiQueryClose(hquery);
        ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
        r = MsiCloseHandle(hquery);
        ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");
    }
    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = MsiQueryFetch(0, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "MsiQueryFetch failed\n");

    r = MsiDatabaseCommit(hdb);
    ok(r == ERROR_SUCCESS, "MsiDatabaseCommit failed\n");

    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = DeleteFile(msifile);
    ok(r == true, "file didn't exist after commit\n");
}

static unsigned try_query_param( LibmsiObject *hdb, const char *szQuery, LibmsiObject *hrec )
{
    LibmsiObject *htab = 0;
    unsigned res;

    res = MsiDatabaseOpenQuery( hdb, szQuery, &htab );
    if(res == ERROR_SUCCESS )
    {
        unsigned r;

        r = MsiQueryExecute( htab, hrec );
        if(r != ERROR_SUCCESS )
            res = r;

        r = MsiQueryClose( htab );
        if(r != ERROR_SUCCESS )
            res = r;

        r = MsiCloseHandle( htab );
        if(r != ERROR_SUCCESS )
            res = r;
    }
    return res;
}

static unsigned try_query( LibmsiObject *hdb, const char *szQuery )
{
    return try_query_param( hdb, szQuery, 0 );
}

static unsigned try_insert_query( LibmsiObject *hdb, const char *szQuery )
{
    LibmsiObject *hrec = 0;
    unsigned r;

    hrec = MsiCreateRecord( 1 );
    MsiRecordSetString( hrec, 1, "Hello");

    r = try_query_param( hdb, szQuery, hrec );

    MsiCloseHandle( hrec );
    return r;
}

static void test_msibadqueries(void)
{
    LibmsiObject *hdb = 0;
    unsigned r;

    DeleteFile(msifile);

    /* just MsiOpenDatabase should not create a file */
    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    r = MsiDatabaseCommit( hdb );
    ok(r == ERROR_SUCCESS , "Failed to commit database\n");

    r = MsiCloseHandle( hdb );
    ok(r == ERROR_SUCCESS , "Failed to close database\n");

    /* open it readonly */
    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_READONLY, &hdb );
    ok(r == ERROR_SUCCESS , "Failed to open database r/o\n");

    /* add a table to it */
    r = try_query( hdb, "select * from _Tables");
    ok(r == ERROR_SUCCESS , "query 1 failed\n");

    r = MsiCloseHandle( hdb );
    ok(r == ERROR_SUCCESS , "Failed to close database r/o\n");

    /* open it read/write */
    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_TRANSACT, &hdb );
    ok(r == ERROR_SUCCESS , "Failed to open database r/w\n");

    /* a bunch of test queries that fail with the native MSI */

    r = try_query( hdb, "CREATE TABLE");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2a return code\n");

    r = try_query( hdb, "CREATE TABLE `a`");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2b return code\n");

    r = try_query( hdb, "CREATE TABLE `a` ()");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2c return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b`)");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2d return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(72) )");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2e return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(72) NOT NULL)");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2f return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(72) NOT NULL PRIMARY)");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2g return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(72) NOT NULL PRIMARY KEY)");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2h return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(72) NOT NULL PRIMARY KEY)");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2i return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(72) NOT NULL PRIMARY KEY 'b')");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2j return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(72) NOT NULL PRIMARY KEY `b')");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2k return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(72) NOT NULL PRIMARY KEY `b')");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2l return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHA(72) NOT NULL PRIMARY KEY `b`)");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2m return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(-1) NOT NULL PRIMARY KEY `b`)");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2n return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(720) NOT NULL PRIMARY KEY `b`)");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2o return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(72) NOT NULL KEY `b`)");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2p return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`` CHAR(72) NOT NULL PRIMARY KEY `b`)");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2p return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(72) NOT NULL PRIMARY KEY `b`)");
    ok(r == ERROR_SUCCESS , "valid query 2z failed\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(72) NOT NULL PRIMARY KEY `b`)");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "created same table again\n");

    r = try_query( hdb, "CREATE TABLE `aa` (`b` CHAR(72) NOT NULL, `c` "
          "CHAR(72), `d` CHAR(255) NOT NULL LOCALIZABLE PRIMARY KEY `b`)");
    ok(r == ERROR_SUCCESS , "query 4 failed\n");

    r = MsiDatabaseCommit( hdb );
    ok(r == ERROR_SUCCESS , "Failed to commit database after write\n");

    r = try_query( hdb, "CREATE TABLE `blah` (`foo` CHAR(72) NOT NULL "
                          "PRIMARY KEY `foo`)");
    ok(r == ERROR_SUCCESS , "query 4 failed\n");

    r = try_insert_query( hdb, "insert into a  ( `b` ) VALUES ( ? )");
    ok(r == ERROR_SUCCESS , "failed to insert record in db\n");

    r = MsiDatabaseCommit( hdb );
    ok(r == ERROR_SUCCESS , "Failed to commit database after write\n");

    r = try_query( hdb, "CREATE TABLE `boo` (`foo` CHAR(72) NOT NULL "
                          "PRIMARY KEY `ba`)");
    ok(r != ERROR_SUCCESS , "query 5 succeeded\n");

    r = try_query( hdb,"CREATE TABLE `bee` (`foo` CHAR(72) NOT NULL )");
    ok(r != ERROR_SUCCESS , "query 6 succeeded\n");

    r = try_query( hdb, "CREATE TABLE `temp` (`t` CHAR(72) NOT NULL "
                          "PRIMARY KEY `t`)");
    ok(r == ERROR_SUCCESS , "query 7 failed\n");

    r = try_query( hdb, "CREATE TABLE `c` (`b` CHAR NOT NULL PRIMARY KEY `b`)");
    ok(r == ERROR_SUCCESS , "query 8 failed\n");

    r = try_query( hdb, "select * from c");
    ok(r == ERROR_SUCCESS , "query failed\n");

    r = try_query( hdb, "select * from c where b = 'x");
    ok(r == ERROR_BAD_QUERY_SYNTAX, "query failed\n");

    r = try_query( hdb, "select * from c where b = 'x'");
    ok(r == ERROR_SUCCESS, "query failed\n");

    r = try_query( hdb, "select * from 'c'");
    ok(r == ERROR_BAD_QUERY_SYNTAX, "query failed\n");

    r = try_query( hdb, "select * from ''");
    ok(r == ERROR_BAD_QUERY_SYNTAX, "query failed\n");

    r = try_query( hdb, "select * from c where b = x");
    ok(r == ERROR_BAD_QUERY_SYNTAX, "query failed\n");

    r = try_query( hdb, "select * from c where b = \"x\"");
    ok(r == ERROR_BAD_QUERY_SYNTAX, "query failed\n");

    r = try_query( hdb, "select * from c where b = 'x'");
    ok(r == ERROR_SUCCESS, "query failed\n");

    r = try_query( hdb, "select * from c where b = '\"x'");
    ok(r == ERROR_SUCCESS, "query failed\n");

    if (0)  /* FIXME: this query causes trouble with other tests */
    {
        r = try_query( hdb, "select * from c where b = '\\\'x'");
        ok(r == ERROR_BAD_QUERY_SYNTAX, "query failed\n");
    }

    r = try_query( hdb, "select * from 'c'");
    ok(r == ERROR_BAD_QUERY_SYNTAX, "query failed\n");

    r = try_query( hdb, "select `c`.`b` from `c` order by `c`.`order`");
    ok( r == ERROR_BAD_QUERY_SYNTAX, "query failed: %u\n", r );

    r = try_query( hdb, "select `c`.b` from `c`");
    ok( r == ERROR_SUCCESS, "query failed: %u\n", r );

    r = try_query( hdb, "select `c`.`b from `c`");
    ok( r == ERROR_BAD_QUERY_SYNTAX, "query failed: %u\n", r );

    r = try_query( hdb, "select `c`.b from `c`");
    ok( r == ERROR_SUCCESS, "query failed: %u\n", r );

    r = try_query( hdb, "select `c.`b` from `c`");
    ok( r == ERROR_BAD_QUERY_SYNTAX, "query failed: %u\n", r );

    r = try_query( hdb, "select c`.`b` from `c`");
    ok( r == ERROR_SUCCESS, "query failed: %u\n", r );

    r = try_query( hdb, "select c.`b` from `c`");
    ok( r == ERROR_SUCCESS, "query failed: %u\n", r );

    r = try_query( hdb, "select `c`.`b` from c`");
    ok( r == ERROR_SUCCESS, "query failed: %u\n", r );

    r = try_query( hdb, "select `c`.`b` from `c");
    ok( r == ERROR_BAD_QUERY_SYNTAX, "query failed: %u\n", r );

    r = try_query( hdb, "select `c`.`b` from c");
    ok( r == ERROR_SUCCESS, "query failed: %u\n", r );

    r = try_query( hdb, "CREATE TABLE `\5a` (`b` CHAR NOT NULL PRIMARY KEY `b`)" );
    ok( r == ERROR_SUCCESS , "query failed: %u\n", r );

    r = try_query( hdb, "SELECT * FROM \5a" );
    ok( r == ERROR_SUCCESS , "query failed: %u\n", r );

    r = try_query( hdb, "CREATE TABLE `a\5` (`b` CHAR NOT NULL PRIMARY KEY `b`)" );
    ok( r == ERROR_SUCCESS , "query failed: %u\n", r );

    r = try_query( hdb, "SELECT * FROM a\5" );
    ok( r == ERROR_SUCCESS , "query failed: %u\n", r );

    r = try_query( hdb, "CREATE TABLE `-a` (`b` CHAR NOT NULL PRIMARY KEY `b`)" );
    ok( r == ERROR_SUCCESS , "query failed: %u\n", r );

    r = try_query( hdb, "SELECT * FROM -a" );
    todo_wine ok( r == ERROR_SUCCESS , "query failed: %u\n", r );

    r = try_query( hdb, "CREATE TABLE `a-` (`b` CHAR NOT NULL PRIMARY KEY `b`)" );
    ok( r == ERROR_SUCCESS , "query failed: %u\n", r );

    r = try_query( hdb, "SELECT * FROM a-" );
    ok( r == ERROR_SUCCESS , "query failed: %u\n", r );

    r = MsiCloseHandle( hdb );
    ok(r == ERROR_SUCCESS , "Failed to close database transact\n");

    r = DeleteFile( msifile );
    ok(r == true, "file didn't exist after commit\n");
}

static void test_querymodify(void)
{
    LibmsiObject *hdb = 0;
    LibmsiObject *hquery = 0;
    LibmsiObject *hrec = 0;
    unsigned r;
    LibmsiDBError err;
    const char *sql;
    char buffer[0x100];
    unsigned sz;

    DeleteFile(msifile);

    /* just MsiOpenDatabase should not create a file */
    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    sql = "CREATE TABLE `phone` ( "
            "`id` INT, `name` CHAR(32), `number` CHAR(32) "
            "PRIMARY KEY `id`)";
    r = run_query( hdb, 0, sql );
    ok(r == ERROR_SUCCESS, "query failed\n");

    sql = "CREATE TABLE `_Validation` ( "
            "`Table` CHAR(32) NOT NULL, `Column` CHAR(32) NOT NULL, "
            "`Nullable` CHAR(4) NOT NULL, `MinValue` INT, `MaxValue` INT, "
            "`KeyTable` CHAR(255), `KeyColumn` SHORT, `Category` CHAR(32), "
            "`Set` CHAR(255), `Description` CHAR(255) PRIMARY KEY `Table`, `Column`)";
    r = run_query( hdb, 0, sql );
    ok(r == ERROR_SUCCESS, "query failed\n");

    sql = "INSERT INTO `_Validation` ( `Table`, `Column`, `Nullable` ) "
            "VALUES('phone', 'id', 'N')";
    r = run_query( hdb, 0, sql );
    ok(r == ERROR_SUCCESS, "query failed\n");

    /* check what the error function reports without doing anything */
    sz = 0;
    /* passing NULL as the 3rd param make function to crash on older platforms */
    err = MsiQueryGetError( 0, NULL, &sz );
    ok(err == LIBMSI_DB_ERROR_INVALIDARG, "MsiQueryGetError return\n");

    /* open a query */
    sql = "SELECT * FROM `phone`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenQuery failed\n");

    /* see what happens with a good hquery and bad args */
    err = MsiQueryGetError( hquery, NULL, NULL );
    ok(err == LIBMSI_DB_ERROR_INVALIDARG || err == LIBMSI_DB_ERROR_NOERROR,
       "MsiQueryGetError returns %u (expected -3)\n", err);
    err = MsiQueryGetError( hquery, buffer, NULL );
    ok(err == LIBMSI_DB_ERROR_INVALIDARG, "MsiQueryGetError return\n");

    /* see what happens with a zero length buffer */
    sz = 0;
    buffer[0] = 'x';
    err = MsiQueryGetError( hquery, buffer, &sz );
    ok(err == LIBMSI_DB_ERROR_MOREDATA, "MsiQueryGetError return\n");
    ok(buffer[0] == 'x', "buffer cleared\n");
    ok(sz == 0, "size not zero\n");

    /* ok this one is strange */
    sz = 0;
    err = MsiQueryGetError( hquery, NULL, &sz );
    ok(err == LIBMSI_DB_ERROR_NOERROR, "MsiQueryGetError return\n");
    ok(sz == 0, "size not zero\n");

    /* see if it really has an error */
    sz = sizeof buffer;
    buffer[0] = 'x';
    err = MsiQueryGetError( hquery, buffer, &sz );
    ok(err == LIBMSI_DB_ERROR_NOERROR, "MsiQueryGetError return\n");
    ok(buffer[0] == 0, "buffer not cleared\n");
    ok(sz == 0, "size not zero\n");

    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");

    /* try some invalid records */
    r = MsiQueryModify(hquery, LIBMSI_MODIFY_INSERT, 0 );
    ok(r == ERROR_INVALID_HANDLE, "MsiQueryModify failed\n");
    r = MsiQueryModify(hquery, -1, 0 );
    ok(r == ERROR_INVALID_HANDLE, "MsiQueryModify failed\n");

    /* try an small record */
    hrec = MsiCreateRecord(1);
    r = MsiQueryModify(hquery, -1, hrec );
    ok(r == ERROR_INVALID_DATA, "MsiQueryModify failed\n");

    sz = sizeof buffer;
    buffer[0] = 'x';
    err = MsiQueryGetError( hquery, buffer, &sz );
    ok(err == LIBMSI_DB_ERROR_NOERROR, "MsiQueryGetError return\n");
    ok(buffer[0] == 0, "buffer not cleared\n");
    ok(sz == 0, "size not zero\n");

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    /* insert a valid record */
    hrec = MsiCreateRecord(3);

    r = MsiRecordSetInteger(hrec, 1, 1);
    ok(r == ERROR_SUCCESS, "failed to set integer\n");
    r = MsiRecordSetString(hrec, 2, "bob");
    ok(r == ERROR_SUCCESS, "failed to set string\n");
    r = MsiRecordSetString(hrec, 3, "7654321");
    ok(r == ERROR_SUCCESS, "failed to set string\n");

    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");
    r = MsiQueryModify(hquery, LIBMSI_MODIFY_INSERT_TEMPORARY, hrec );
    ok(r == ERROR_SUCCESS, "MsiQueryModify failed\n");

    /* validate it */
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");

    r = MsiQueryModify(hquery, LIBMSI_MODIFY_VALIDATE_NEW, hrec );
    ok(r == ERROR_INVALID_DATA, "MsiQueryModify failed %u\n", r);

    sz = sizeof buffer;
    buffer[0] = 'x';
    err = MsiQueryGetError( hquery, buffer, &sz );
    ok(err == LIBMSI_DB_ERROR_DUPLICATEKEY, "MsiQueryGetError returned %u\n", err);
    ok(!strcmp(buffer, "id"), "expected \"id\" c, got \"%s\"\n", buffer);
    ok(sz == 2, "size not 2\n");

    /* insert the same thing again */
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");

    /* should fail ... */
    r = MsiQueryModify(hquery, LIBMSI_MODIFY_INSERT_TEMPORARY, hrec );
    ok(r == ERROR_FUNCTION_FAILED, "MsiQueryModify failed\n");

    /* try to merge the same record */
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");
    r = MsiQueryModify(hquery, LIBMSI_MODIFY_MERGE, hrec );
    ok(r == ERROR_SUCCESS, "MsiQueryModify failed\n");

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    /* try merging a new record */
    hrec = MsiCreateRecord(3);

    r = MsiRecordSetInteger(hrec, 1, 10);
    ok(r == ERROR_SUCCESS, "failed to set integer\n");
    r = MsiRecordSetString(hrec, 2, "pepe");
    ok(r == ERROR_SUCCESS, "failed to set string\n");
    r = MsiRecordSetString(hrec, 3, "7654321");
    ok(r == ERROR_SUCCESS, "failed to set string\n");

    r = MsiQueryModify(hquery, LIBMSI_MODIFY_MERGE, hrec );
    ok(r == ERROR_SUCCESS, "MsiQueryModify failed\n");
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiQueryClose(hquery);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(hquery);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    sql = "SELECT * FROM `phone`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenQuery failed\n");

    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed\n");

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 1, "Expected 1, got %d\n", r);

    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 2, buffer, &sz);
    ok(r == ERROR_SUCCESS, "MsiRecordGetString failed\n");
    ok(!strcmp(buffer, "bob"), "Expected bob, got %s\n", buffer);

    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 3, buffer, &sz);
    ok(r == ERROR_SUCCESS, "MsiRecordGetString failed\n");
    ok(!strcmp(buffer, "7654321"), "Expected 7654321, got %s\n", buffer);

    /* update the query, non-primary key */
    r = MsiRecordSetString(hrec, 3, "3141592");
    ok(r == ERROR_SUCCESS, "MsiRecordSetString failed\n");

    r = MsiQueryModify(hquery, LIBMSI_MODIFY_UPDATE, hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryModify failed\n");

    /* do it again */
    r = MsiQueryModify(hquery, LIBMSI_MODIFY_UPDATE, hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryModify failed: %d\n", r);

    /* update the query, primary key */
    r = MsiRecordSetInteger(hrec, 1, 5);
    ok(r == ERROR_SUCCESS, "MsiRecordSetInteger failed\n");

    r = MsiQueryModify(hquery, LIBMSI_MODIFY_UPDATE, hrec);
    ok(r == ERROR_FUNCTION_FAILED, "MsiQueryModify failed\n");

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiQueryClose(hquery);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(hquery);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    sql = "SELECT * FROM `phone`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenQuery failed\n");

    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed\n");

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 1, "Expected 1, got %d\n", r);

    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 2, buffer, &sz);
    ok(r == ERROR_SUCCESS, "MsiRecordGetString failed\n");
    ok(!strcmp(buffer, "bob"), "Expected bob, got %s\n", buffer);

    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 3, buffer, &sz);
    ok(r == ERROR_SUCCESS, "MsiRecordGetString failed\n");
    ok(!strcmp(buffer, "3141592"), "Expected 3141592, got %s\n", buffer);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    /* use a record that doesn't come from a query fetch */
    hrec = MsiCreateRecord(3);
    ok(hrec != 0, "MsiCreateRecord failed\n");

    r = MsiRecordSetInteger(hrec, 1, 3);
    ok(r == ERROR_SUCCESS, "failed to set integer\n");
    r = MsiRecordSetString(hrec, 2, "jane");
    ok(r == ERROR_SUCCESS, "failed to set string\n");
    r = MsiRecordSetString(hrec, 3, "112358");
    ok(r == ERROR_SUCCESS, "failed to set string\n");

    r = MsiQueryModify(hquery, LIBMSI_MODIFY_UPDATE, hrec);
    ok(r == ERROR_FUNCTION_FAILED, "Expected ERROR_FUNCTION_FAILED, got %d\n", r);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    /* use a record that doesn't come from a query fetch, primary key matches */
    hrec = MsiCreateRecord(3);
    ok(hrec != 0, "MsiCreateRecord failed\n");

    r = MsiRecordSetInteger(hrec, 1, 1);
    ok(r == ERROR_SUCCESS, "failed to set integer\n");
    r = MsiRecordSetString(hrec, 2, "jane");
    ok(r == ERROR_SUCCESS, "failed to set string\n");
    r = MsiRecordSetString(hrec, 3, "112358");
    ok(r == ERROR_SUCCESS, "failed to set string\n");

    r = MsiQueryModify(hquery, LIBMSI_MODIFY_UPDATE, hrec);
    ok(r == ERROR_FUNCTION_FAILED, "MsiQueryModify failed\n");

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    hrec = MsiCreateRecord(3);

    r = MsiRecordSetInteger(hrec, 1, 2);
    ok(r == ERROR_SUCCESS, "failed to set integer\n");
    r = MsiRecordSetString(hrec, 2, "nick");
    ok(r == ERROR_SUCCESS, "failed to set string\n");
    r = MsiRecordSetString(hrec, 3, "141421");
    ok(r == ERROR_SUCCESS, "failed to set string\n");

    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");
    r = MsiQueryModify(hquery, LIBMSI_MODIFY_INSERT_TEMPORARY, hrec );
    ok(r == ERROR_SUCCESS, "MsiQueryModify failed\n");

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");
    r = MsiQueryClose(hquery);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(hquery);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    sql = "SELECT * FROM `phone` WHERE `id` = 1";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenQuery failed\n");
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");
    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed\n");

    /* change the id to match the second row */
    r = MsiRecordSetInteger(hrec, 1, 2);
    ok(r == ERROR_SUCCESS, "failed to set integer\n");
    r = MsiRecordSetString(hrec, 2, "jerry");
    ok(r == ERROR_SUCCESS, "failed to set string\n");

    r = MsiQueryModify(hquery, LIBMSI_MODIFY_UPDATE, hrec);
    ok(r == ERROR_FUNCTION_FAILED, "MsiQueryModify failed\n");

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");
    r = MsiQueryClose(hquery);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(hquery);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* broader search */
    sql = "SELECT * FROM `phone` ORDER BY `id`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenQuery failed\n");
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");
    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed\n");

    /* change the id to match the second row */
    r = MsiRecordSetInteger(hrec, 1, 2);
    ok(r == ERROR_SUCCESS, "failed to set integer\n");
    r = MsiRecordSetString(hrec, 2, "jerry");
    ok(r == ERROR_SUCCESS, "failed to set string\n");

    r = MsiQueryModify(hquery, LIBMSI_MODIFY_UPDATE, hrec);
    ok(r == ERROR_FUNCTION_FAILED, "MsiQueryModify failed\n");

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");
    r = MsiQueryClose(hquery);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(hquery);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = MsiCloseHandle( hdb );
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase close failed\n");
}

static LibmsiObject *create_db(void)
{
    LibmsiObject *hdb = 0;
    unsigned res;

    DeleteFile(msifile);

    /* create an empty database */
    res = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_CREATE, &hdb );
    ok( res == ERROR_SUCCESS , "Failed to create database\n" );
    if( res != ERROR_SUCCESS )
        return hdb;

    res = MsiDatabaseCommit( hdb );
    ok( res == ERROR_SUCCESS , "Failed to commit database\n" );

    return hdb;
}

static void test_getcolinfo(void)
{
    LibmsiObject *hdb;
    LibmsiObject *hquery = 0;
    LibmsiObject *rec = 0;
    unsigned r;
    unsigned sz;
    char buffer[0x20];

    /* create an empty db */
    hdb = create_db();
    ok( hdb, "failed to create db\n");

    /* tables should be present */
    r = MsiDatabaseOpenQuery(hdb, "select * from _Tables", &hquery);
    ok( r == ERROR_SUCCESS, "failed to open query\n");

    r = MsiQueryExecute(hquery, 0);
    ok( r == ERROR_SUCCESS, "failed to execute query\n");

    /* check that NAMES works */
    rec = 0;
    r = MsiQueryGetColumnInfo( hquery, LIBMSI_COL_INFO_NAMES, &rec );
    ok( r == ERROR_SUCCESS, "failed to get names\n");
    sz = sizeof buffer;
    r = MsiRecordGetString(rec, 1, buffer, &sz );
    ok( r == ERROR_SUCCESS, "failed to get string\n");
    ok( !strcmp(buffer,"Name"), "_Tables has wrong column name\n");
    r = MsiCloseHandle( rec );
    ok( r == ERROR_SUCCESS, "failed to close record handle\n");

    /* check that TYPES works */
    rec = 0;
    r = MsiQueryGetColumnInfo( hquery, LIBMSI_COL_INFO_TYPES, &rec );
    ok( r == ERROR_SUCCESS, "failed to get names\n");
    sz = sizeof buffer;
    r = MsiRecordGetString(rec, 1, buffer, &sz );
    ok( r == ERROR_SUCCESS, "failed to get string\n");
    ok( !strcmp(buffer,"s64"), "_Tables has wrong column type\n");
    r = MsiCloseHandle( rec );
    ok( r == ERROR_SUCCESS, "failed to close record handle\n");

    /* check that invalid values fail */
    rec = 0;
    r = MsiQueryGetColumnInfo( hquery, 100, &rec );
    ok( r == ERROR_INVALID_PARAMETER, "wrong error code\n");
    ok( rec == 0, "returned a record\n");

    r = MsiQueryGetColumnInfo( hquery, LIBMSI_COL_INFO_TYPES, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "wrong error code\n");

    r = MsiQueryGetColumnInfo( 0, LIBMSI_COL_INFO_TYPES, &rec );
    ok( r == ERROR_INVALID_HANDLE, "wrong error code\n");

    r = MsiQueryClose(hquery);
    ok( r == ERROR_SUCCESS, "failed to close query\n");
    r = MsiCloseHandle(hquery);
    ok( r == ERROR_SUCCESS, "failed to close query handle\n");
    r = MsiCloseHandle(hdb);
    ok( r == ERROR_SUCCESS, "failed to close database\n");
}

static LibmsiObject *get_column_info(LibmsiObject *hdb, const char *sql, LibmsiColInfo type)
{
    LibmsiObject *hquery = 0;
    LibmsiObject *rec = 0;
    unsigned r;

    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    if( r != ERROR_SUCCESS )
        return rec;

    r = MsiQueryExecute(hquery, 0);
    if( r == ERROR_SUCCESS )
    {
        MsiQueryGetColumnInfo( hquery, type, &rec );
    }
    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);
    return rec;
}

static unsigned get_columns_table_type(LibmsiObject *hdb, const char *table, unsigned field)
{
    LibmsiObject *hquery = 0;
    LibmsiObject *rec = 0;
    unsigned r, type = 0;
    char sql[0x100];

    sprintf(sql, "select * from `_Columns` where  `Table` = '%s'", table );

    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    if( r != ERROR_SUCCESS )
        return r;

    r = MsiQueryExecute(hquery, 0);
    if( r == ERROR_SUCCESS )
    {
        while (1)
        {
            r = MsiQueryFetch( hquery, &rec );
            if( r != ERROR_SUCCESS)
                break;
            r = MsiRecordGetInteger( rec, 2 );
            if (r == field)
                type = MsiRecordGetInteger( rec, 4 );
            MsiCloseHandle( rec );
        }
    }
    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);
    return type;
}

static bool check_record( LibmsiObject *rec, unsigned field, const char *val )
{
    char buffer[0x20];
    unsigned r;
    unsigned sz;

    sz = sizeof buffer;
    r = MsiRecordGetString( rec, field, buffer, &sz );
    return (r == ERROR_SUCCESS ) && !strcmp(val, buffer);
}

static void test_querygetcolumninfo(void)
{
    LibmsiObject *hdb = 0;
    LibmsiObject *rec;
    unsigned r;

    hdb = create_db();
    ok( hdb, "failed to create db\n");

    r = run_query( hdb, 0,
            "CREATE TABLE `Properties` "
            "( `Property` CHAR(255), "
	    "  `Value` CHAR(1), "
	    "  `Intvalue` INT, "
	    "  `Integervalue` INTEGER, "
	    "  `Shortvalue` SHORT, "
	    "  `Longvalue` LONG, "
	    "  `Longcharvalue` LONGCHAR "
	    "  PRIMARY KEY `Property`)" );
    ok( r == ERROR_SUCCESS , "Failed to create table\n" );

    /* check the column types */
    rec = get_column_info( hdb, "select * from `Properties`", LIBMSI_COL_INFO_TYPES );
    ok( rec, "failed to get column info record\n" );

    ok( check_record( rec, 1, "S255"), "wrong record type\n");
    ok( check_record( rec, 2, "S1"), "wrong record type\n");
    ok( check_record( rec, 3, "I2"), "wrong record type\n");
    ok( check_record( rec, 4, "I2"), "wrong record type\n");
    ok( check_record( rec, 5, "I2"), "wrong record type\n");
    ok( check_record( rec, 6, "I4"), "wrong record type\n");
    ok( check_record( rec, 7, "S0"), "wrong record type\n");

    MsiCloseHandle( rec );

    /* check the type in _Columns */
    ok( 0x3dff == get_columns_table_type(hdb, "Properties", 1 ), "_columns table wrong\n");
    ok( 0x1d01 == get_columns_table_type(hdb, "Properties", 2 ), "_columns table wrong\n");
    ok( 0x1502 == get_columns_table_type(hdb, "Properties", 3 ), "_columns table wrong\n");
    ok( 0x1502 == get_columns_table_type(hdb, "Properties", 4 ), "_columns table wrong\n");
    ok( 0x1502 == get_columns_table_type(hdb, "Properties", 5 ), "_columns table wrong\n");
    ok( 0x1104 == get_columns_table_type(hdb, "Properties", 6 ), "_columns table wrong\n");
    ok( 0x1d00 == get_columns_table_type(hdb, "Properties", 7 ), "_columns table wrong\n");

    /* now try the names */
    rec = get_column_info( hdb, "select * from `Properties`", LIBMSI_COL_INFO_NAMES );
    ok( rec, "failed to get column info record\n" );

    ok( check_record( rec, 1, "Property"), "wrong record type\n");
    ok( check_record( rec, 2, "Value"), "wrong record type\n");
    ok( check_record( rec, 3, "Intvalue"), "wrong record type\n");
    ok( check_record( rec, 4, "Integervalue"), "wrong record type\n");
    ok( check_record( rec, 5, "Shortvalue"), "wrong record type\n");
    ok( check_record( rec, 6, "Longvalue"), "wrong record type\n");
    ok( check_record( rec, 7, "Longcharvalue"), "wrong record type\n");

    MsiCloseHandle( rec );

    r = run_query( hdb, 0,
            "CREATE TABLE `Binary` "
            "( `Name` CHAR(255), `Data` OBJECT  PRIMARY KEY `Name`)" );
    ok( r == ERROR_SUCCESS , "Failed to create table\n" );

    /* check the column types */
    rec = get_column_info( hdb, "select * from `Binary`", LIBMSI_COL_INFO_TYPES );
    ok( rec, "failed to get column info record\n" );

    ok( check_record( rec, 1, "S255"), "wrong record type\n");
    ok( check_record( rec, 2, "V0"), "wrong record type\n");

    MsiCloseHandle( rec );

    /* check the type in _Columns */
    ok( 0x3dff == get_columns_table_type(hdb, "Binary", 1 ), "_columns table wrong\n");
    ok( 0x1900 == get_columns_table_type(hdb, "Binary", 2 ), "_columns table wrong\n");

    /* now try the names */
    rec = get_column_info( hdb, "select * from `Binary`", LIBMSI_COL_INFO_NAMES );
    ok( rec, "failed to get column info record\n" );

    ok( check_record( rec, 1, "Name"), "wrong record type\n");
    ok( check_record( rec, 2, "Data"), "wrong record type\n");
    MsiCloseHandle( rec );

    r = run_query( hdb, 0,
            "CREATE TABLE `UIText` "
            "( `Key` CHAR(72) NOT NULL, `Text` CHAR(255) LOCALIZABLE PRIMARY KEY `Key`)" );
    ok( r == ERROR_SUCCESS , "Failed to create table\n" );

    ok( 0x2d48 == get_columns_table_type(hdb, "UIText", 1 ), "_columns table wrong\n");
    ok( 0x1fff == get_columns_table_type(hdb, "UIText", 2 ), "_columns table wrong\n");

    rec = get_column_info( hdb, "select * from `UIText`", LIBMSI_COL_INFO_NAMES );
    ok( rec, "failed to get column info record\n" );
    ok( check_record( rec, 1, "Key"), "wrong record type\n");
    ok( check_record( rec, 2, "Text"), "wrong record type\n");
    MsiCloseHandle( rec );

    rec = get_column_info( hdb, "select * from `UIText`", LIBMSI_COL_INFO_TYPES );
    ok( rec, "failed to get column info record\n" );
    ok( check_record( rec, 1, "s72"), "wrong record type\n");
    ok( check_record( rec, 2, "L255"), "wrong record type\n");
    MsiCloseHandle( rec );

    MsiCloseHandle( hdb );
}

static void test_msiexport(void)
{
    LibmsiObject *hdb = 0;
    LibmsiObject *hquery = 0;
    unsigned r;
    const char *sql;
    int fd;
    const char file[] = "phone.txt";
    HANDLE handle;
    char buffer[0x100];
    unsigned length;
    const char expected[] =
        "id\tname\tnumber\r\n"
        "I2\tS32\tS32\r\n"
        "phone\tid\r\n"
        "1\tAbe\t8675309\r\n";

    DeleteFile(msifile);

    /* just MsiOpenDatabase should not create a file */
    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    /* create a table */
    sql = "CREATE TABLE `phone` ( "
            "`id` INT, `name` CHAR(32), `number` CHAR(32) "
            "PRIMARY KEY `id`)";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenQuery failed\n");
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");
    r = MsiQueryClose(hquery);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(hquery);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* insert a value into it */
    sql = "INSERT INTO `phone` ( `id`, `name`, `number` )"
        "VALUES('1', 'Abe', '8675309')";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenQuery failed\n");
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");
    r = MsiQueryClose(hquery);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(hquery);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    fd = open(file, O_WRONLY | O_BINARY | O_CREAT, 0644);
    ok(fd != -1, "open failed\n");

    r = MsiDatabaseExport(hdb, "phone", fd);
    ok(r == ERROR_SUCCESS, "MsiDatabaseExport failed\n");

    close(fd);

    MsiCloseHandle(hdb);

    /* check the data that was written */
    length = 0;
    memset(buffer, 0, sizeof buffer);
    handle = CreateFile(file, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (handle != INVALID_HANDLE_VALUE)
    {
        ReadFile(handle, buffer, sizeof buffer, &length, NULL);
        CloseHandle(handle);
        DeleteFile(file);
    }
    else
        ok(0, "failed to open file %s\n", file);

    ok( length == strlen(expected), "length of data wrong\n");
    ok( !strcmp(buffer, expected), "data doesn't match\n");
    DeleteFile(msifile);
}

static void test_longstrings(void)
{
    const char insert_query[] = 
        "INSERT INTO `strings` ( `id`, `val` ) VALUES('1', 'Z')";
    char *str;
    LibmsiObject *hdb = 0;
    LibmsiObject *hquery = 0;
    LibmsiObject *hrec = 0;
    unsigned len;
    unsigned r;
    const unsigned STRING_LENGTH = 0x10005;

    DeleteFile(msifile);
    /* just MsiOpenDatabase should not create a file */
    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    /* create a table */
    r = try_query( hdb, 
        "CREATE TABLE `strings` ( `id` INT, `val` CHAR(0) PRIMARY KEY `id`)");
    ok(r == ERROR_SUCCESS, "query failed\n");

    /* try a insert a very long string */
    str = HeapAlloc(GetProcessHeap(), 0, STRING_LENGTH+sizeof insert_query);
    len = strchr(insert_query, 'Z') - insert_query;
    strcpy(str, insert_query);
    memset(str+len, 'Z', STRING_LENGTH);
    strcpy(str+len+STRING_LENGTH, insert_query+len+1);
    r = try_query( hdb, str );
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenQuery failed\n");

    HeapFree(GetProcessHeap(), 0, str);

    r = MsiDatabaseCommit(hdb);
    ok(r == ERROR_SUCCESS, "MsiDatabaseCommit failed\n");
    MsiCloseHandle(hdb);

    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_READONLY, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    r = MsiDatabaseOpenQuery(hdb, "select * from `strings` where `id` = 1", &hquery);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenQuery failed\n");

    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed\n");

    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    r = MsiRecordGetString(hrec, 2, NULL, &len);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed\n");
    ok(len == STRING_LENGTH, "string length wrong\n");

    MsiCloseHandle(hrec);
    MsiCloseHandle(hdb);
    DeleteFile(msifile);
}

static void create_file_data(const char *name, const char *data, unsigned size)
{
    HANDLE file;
    unsigned written;

    file = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return;

    WriteFile(file, data, strlen(data), &written, NULL);
    WriteFile(file, "\n", strlen("\n"), &written, NULL);

    if (size)
    {
        SetFilePointer(file, size, NULL, FILE_BEGIN);
        SetEndOfFile(file);
    }

    CloseHandle(file);
}

#define create_file(name) create_file_data(name, name, 0)
 
static void test_streamtable(void)
{
    LibmsiObject *hdb = 0;
    LibmsiObject *rec;
    LibmsiObject *query;
    LibmsiObject *hsi;
    char file[MAX_PATH];
    char buf[MAX_PATH];
    unsigned size;
    unsigned r;

    hdb = create_db();
    ok( hdb, "failed to create db\n");

    r = run_query( hdb, 0,
            "CREATE TABLE `Properties` "
            "( `Property` CHAR(255), `Value` CHAR(1)  PRIMARY KEY `Property`)" );
    ok( r == ERROR_SUCCESS , "Failed to create table\n" );

    r = run_query( hdb, 0,
            "INSERT INTO `Properties` "
            "( `Value`, `Property` ) VALUES ( 'Prop', 'value' )" );
    ok( r == ERROR_SUCCESS, "Failed to add to table\n" );

    r = MsiDatabaseCommit( hdb );
    ok( r == ERROR_SUCCESS , "Failed to commit database\n" );

    MsiCloseHandle( hdb );

    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_TRANSACT, &hdb );
    ok( r == ERROR_SUCCESS , "Failed to open database\n" );

    /* check the column types */
    rec = get_column_info( hdb, "select * from `_Streams`", LIBMSI_COL_INFO_TYPES );
    ok( rec, "failed to get column info record\n" );

    ok( check_record( rec, 1, "s62"), "wrong record type\n");
    ok( check_record( rec, 2, "V0"), "wrong record type\n");

    MsiCloseHandle( rec );

    /* now try the names */
    rec = get_column_info( hdb, "select * from `_Streams`", LIBMSI_COL_INFO_NAMES );
    ok( rec, "failed to get column info record\n" );

    ok( check_record( rec, 1, "Name"), "wrong record type\n");
    ok( check_record( rec, 2, "Data"), "wrong record type\n");

    MsiCloseHandle( rec );

    query = NULL;
    r = MsiDatabaseOpenQuery( hdb,
            "SELECT * FROM `_Streams` WHERE `Name` = '\5SummaryInformation'", &query );
    ok( r == ERROR_SUCCESS, "Failed to open database query: %u\n", r );

    r = MsiQueryExecute( query, 0 );
    ok( r == ERROR_SUCCESS, "Failed to execute query: %u\n", r );

    r = MsiQueryFetch( query, &rec );
    ok( r == ERROR_NO_MORE_ITEMS, "Unexpected result: %u\n", r );

    MsiCloseHandle( rec );
    MsiQueryClose( query );
    MsiCloseHandle( query );

    /* create a summary information stream */
    r = MsiGetSummaryInformation( hdb, 1, &hsi );
    ok( r == ERROR_SUCCESS, "Failed to get summary information handle: %u\n", r );

    r = MsiSummaryInfoSetProperty( hsi, MSI_PID_SECURITY, VT_I4, 2, NULL, NULL );
    ok( r == ERROR_SUCCESS, "Failed to set property: %u\n", r );

    r = MsiSummaryInfoPersist( hsi );
    ok( r == ERROR_SUCCESS, "Failed to save summary information: %u\n", r );

    MsiCloseHandle( hsi );

    query = NULL;
    r = MsiDatabaseOpenQuery( hdb,
            "SELECT * FROM `_Streams` WHERE `Name` = '\5SummaryInformation'", &query );
    ok( r == ERROR_SUCCESS, "Failed to open database query: %u\n", r );

    r = MsiQueryExecute( query, 0 );
    ok( r == ERROR_SUCCESS, "Failed to execute query: %u\n", r );

    r = MsiQueryFetch( query, &rec );
    ok( r == ERROR_SUCCESS, "Unexpected result: %u\n", r );

    MsiCloseHandle( rec );
    MsiQueryClose( query );
    MsiCloseHandle( query );

    /* insert a file into the _Streams table */
    create_file( "test.txt" );

    rec = MsiCreateRecord( 2 );
    MsiRecordSetString( rec, 1, "data" );

    r = MsiRecordSetStream( rec, 2, "test.txt" );
    ok( r == ERROR_SUCCESS, "Failed to add stream data to the record: %d\n", r);

    DeleteFile("test.txt");

    query = NULL;
    r = MsiDatabaseOpenQuery( hdb,
            "INSERT INTO `_Streams` ( `Name`, `Data` ) VALUES ( ?, ? )", &query );
    ok( r == ERROR_SUCCESS, "Failed to open database query: %d\n", r);

    r = MsiQueryExecute( query, rec );
    ok( r == ERROR_SUCCESS, "Failed to execute query: %d\n", r);

    MsiCloseHandle( rec );
    MsiQueryClose( query );
    MsiCloseHandle( query );

    /* insert another one */
    create_file( "test1.txt" );

    rec = MsiCreateRecord( 2 );
    MsiRecordSetString( rec, 1, "data1" );

    r = MsiRecordSetStream( rec, 2, "test1.txt" );
    ok( r == ERROR_SUCCESS, "Failed to add stream data to the record: %d\n", r);

    DeleteFile("test1.txt");

    query = NULL;
    r = MsiDatabaseOpenQuery( hdb,
            "INSERT INTO `_Streams` ( `Name`, `Data` ) VALUES ( ?, ? )", &query );
    ok( r == ERROR_SUCCESS, "Failed to open database query: %d\n", r);

    r = MsiQueryExecute( query, rec );
    ok( r == ERROR_SUCCESS, "Failed to execute query: %d\n", r);

    MsiCloseHandle( rec );
    MsiQueryClose( query );
    MsiCloseHandle( query );

    query = NULL;
    r = MsiDatabaseOpenQuery( hdb,
            "SELECT `Name`, `Data` FROM `_Streams` WHERE `Name` = 'data'", &query );
    ok( r == ERROR_SUCCESS, "Failed to open database query: %d\n", r);

    r = MsiQueryExecute( query, 0 );
    ok( r == ERROR_SUCCESS, "Failed to execute query: %d\n", r);

    r = MsiQueryFetch( query, &rec );
    ok( r == ERROR_SUCCESS, "Failed to fetch record: %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString( rec, 1, file, &size );
    ok( r == ERROR_SUCCESS, "Failed to get string: %d\n", r);
    ok( !strcmp(file, "data"), "Expected 'data', got %s\n", file);

    size = MAX_PATH;
    memset(buf, 0, MAX_PATH);
    r = MsiRecordReadStream( rec, 2, buf, &size );
    ok( r == ERROR_SUCCESS, "Failed to get stream: %d\n", r);
    ok( !strcmp(buf, "test.txt\n"), "Expected 'test.txt\\n', got %s\n", buf);

    MsiCloseHandle( rec );
    MsiQueryClose( query );
    MsiCloseHandle( query );

    query = NULL;
    r = MsiDatabaseOpenQuery( hdb,
            "SELECT `Name`, `Data` FROM `_Streams` WHERE `Name` = 'data1'", &query );
    ok( r == ERROR_SUCCESS, "Failed to open database query: %d\n", r);

    r = MsiQueryExecute( query, 0 );
    ok( r == ERROR_SUCCESS, "Failed to execute query: %d\n", r);

    r = MsiQueryFetch( query, &rec );
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString( rec, 1, file, &size );
    ok( r == ERROR_SUCCESS, "Failed to get string: %d\n", r);
    ok( !strcmp(file, "data1"), "Expected 'data1', got %s\n", file);

    size = MAX_PATH;
    memset(buf, 0, MAX_PATH);
    r = MsiRecordReadStream( rec, 2, buf, &size );
    ok( r == ERROR_SUCCESS, "Failed to get stream: %d\n", r);
    ok( !strcmp(buf, "test1.txt\n"), "Expected 'test1.txt\\n', got %s\n", buf);

    MsiCloseHandle( rec );
    MsiQueryClose( query );
    MsiCloseHandle( query );

    /* perform an update */
    create_file( "test2.txt" );
    rec = MsiCreateRecord( 1 );

    r = MsiRecordSetStream( rec, 1, "test2.txt" );
    ok( r == ERROR_SUCCESS, "Failed to add stream data to the record: %d\n", r);

    DeleteFile("test2.txt");

    query = NULL;
    r = MsiDatabaseOpenQuery( hdb,
            "UPDATE `_Streams` SET `Data` = ? WHERE `Name` = 'data1'", &query );
    ok( r == ERROR_SUCCESS, "Failed to open database query: %d\n", r);

    r = MsiQueryExecute( query, rec );
    ok( r == ERROR_SUCCESS, "Failed to execute query: %d\n", r);

    MsiCloseHandle( rec );
    MsiQueryClose( query );
    MsiCloseHandle( query );

    query = NULL;
    r = MsiDatabaseOpenQuery( hdb,
            "SELECT `Name`, `Data` FROM `_Streams` WHERE `Name` = 'data1'", &query );
    ok( r == ERROR_SUCCESS, "Failed to open database query: %d\n", r);

    r = MsiQueryExecute( query, 0 );
    ok( r == ERROR_SUCCESS, "Failed to execute query: %d\n", r);

    r = MsiQueryFetch( query, &rec );
    ok( r == ERROR_SUCCESS, "Failed to fetch record: %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString( rec, 1, file, &size );
    ok( r == ERROR_SUCCESS, "Failed to get string: %d\n", r);
    ok( !strcmp(file, "data1"), "Expected 'data1', got %s\n", file);

    size = MAX_PATH;
    memset(buf, 0, MAX_PATH);
    r = MsiRecordReadStream( rec, 2, buf, &size );
    ok( r == ERROR_SUCCESS, "Failed to get stream: %d\n", r);
    todo_wine ok( !strcmp(buf, "test2.txt\n"), "Expected 'test2.txt\\n', got %s\n", buf);

    MsiCloseHandle( rec );
    MsiQueryClose( query );
    MsiCloseHandle( query );
    MsiCloseHandle( hdb );
    DeleteFile(msifile);
}

static void test_binary(void)
{
    LibmsiObject *hdb = 0;
    LibmsiObject *rec;
    char file[MAX_PATH];
    char buf[MAX_PATH];
    unsigned size;
    const char *sql;
    unsigned r;

    /* insert a file into the Binary table */
    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_CREATE, &hdb );
    ok( r == ERROR_SUCCESS , "Failed to open database\n" );

    sql = "CREATE TABLE `Binary` ( `Name` CHAR(72) NOT NULL, `ID` INT NOT NULL, `Data` OBJECT  PRIMARY KEY `Name`, `ID`)";
    r = run_query( hdb, 0, sql );
    ok( r == ERROR_SUCCESS, "Cannot create Binary table: %d\n", r );

    create_file( "test.txt" );
    rec = MsiCreateRecord( 1 );
    r = MsiRecordSetStream( rec, 1, "test.txt" );
    ok( r == ERROR_SUCCESS, "Failed to add stream data to the record: %d\n", r);
    DeleteFile( "test.txt" );

    sql = "INSERT INTO `Binary` ( `Name`, `ID`, `Data` ) VALUES ( 'filename1', 1, ? )";
    r = run_query( hdb, rec, sql );
    ok( r == ERROR_SUCCESS, "Insert into Binary table failed: %d\n", r );

    r = MsiCloseHandle( rec );
    ok( r == ERROR_SUCCESS , "Failed to close record handle\n" );

    r = MsiDatabaseCommit( hdb );
    ok( r == ERROR_SUCCESS , "Failed to commit database\n" );

    r = MsiCloseHandle( hdb );
    ok( r == ERROR_SUCCESS , "Failed to close database\n" );

    /* read file from the Stream table */
    r = MsiOpenDatabase( msifile, LIBMSI_DB_OPEN_READONLY, &hdb );
    ok( r == ERROR_SUCCESS , "Failed to open database\n" );

    sql = "SELECT * FROM `_Streams`";
    r = do_query( hdb, sql, &rec );
    ok( r == ERROR_SUCCESS, "SELECT query failed: %d\n", r );

    size = MAX_PATH;
    r = MsiRecordGetString( rec, 1, file, &size );
    ok( r == ERROR_SUCCESS, "Failed to get string: %d\n", r );
    ok( !strcmp(file, "Binary.filename1.1"), "Expected 'Binary.filename1.1', got %s\n", file );

    size = MAX_PATH;
    memset( buf, 0, MAX_PATH );
    r = MsiRecordReadStream( rec, 2, buf, &size );
    ok( r == ERROR_SUCCESS, "Failed to get stream: %d\n", r );
    ok( !strcmp(buf, "test.txt\n"), "Expected 'test.txt\\n', got %s\n", buf );

    r = MsiCloseHandle( rec );
    ok( r == ERROR_SUCCESS , "Failed to close record handle\n" );

    /* read file from the Binary table */
    sql = "SELECT * FROM `Binary`";
    r = do_query( hdb, sql, &rec );
    ok( r == ERROR_SUCCESS, "SELECT query failed: %d\n", r );

    size = MAX_PATH;
    r = MsiRecordGetString( rec, 1, file, &size );
    ok( r == ERROR_SUCCESS, "Failed to get string: %d\n", r );
    ok( !strcmp(file, "filename1"), "Expected 'filename1', got %s\n", file );

    size = MAX_PATH;
    memset( buf, 0, MAX_PATH );
    r = MsiRecordReadStream( rec, 3, buf, &size );
    ok( r == ERROR_SUCCESS, "Failed to get stream: %d\n", r );
    ok( !strcmp(buf, "test.txt\n"), "Expected 'test.txt\\n', got %s\n", buf );

    r = MsiCloseHandle( rec );
    ok( r == ERROR_SUCCESS , "Failed to close record handle\n" );

    r = MsiCloseHandle( hdb );
    ok( r == ERROR_SUCCESS , "Failed to close database\n" );

    DeleteFile( msifile );
}

static void test_where_not_in_selected(void)
{
    LibmsiObject *hdb = 0;
    LibmsiObject *rec;
    LibmsiObject *query;
    const char *sql;
    unsigned r;

    hdb = create_db();
    ok( hdb, "failed to create db\n");

    r = run_query(hdb, 0,
            "CREATE TABLE `IESTable` ("
            "`Action` CHAR(64), "
            "`Condition` CHAR(64), "
            "`Sequence` LONG PRIMARY KEY `Sequence`)");
    ok( r == S_OK, "Cannot create IESTable table: %d\n", r);

    r = run_query(hdb, 0,
            "CREATE TABLE `CATable` ("
            "`Action` CHAR(64), "
            "`Type` LONG PRIMARY KEY `Type`)");
    ok( r == S_OK, "Cannot create CATable table: %d\n", r);

    r = run_query(hdb, 0, "INSERT INTO `IESTable` "
            "( `Action`, `Condition`, `Sequence`) "
            "VALUES ( 'clean', 'cond4', 4)");
    ok( r == S_OK, "cannot add entry to IESTable table:%d\n", r );

    r = run_query(hdb, 0, "INSERT INTO `IESTable` "
            "( `Action`, `Condition`, `Sequence`) "
            "VALUES ( 'depends', 'cond1', 1)");
    ok( r == S_OK, "cannot add entry to IESTable table:%d\n", r );

    r = run_query(hdb, 0, "INSERT INTO `IESTable` "
            "( `Action`, `Condition`, `Sequence`) "
            "VALUES ( 'build', 'cond2', 2)");
    ok( r == S_OK, "cannot add entry to IESTable table:%d\n", r );

    r = run_query(hdb, 0, "INSERT INTO `IESTable` "
            "( `Action`, `Condition`, `Sequence`) "
            "VALUES ( 'build2', 'cond6', 6)");
    ok( r == S_OK, "cannot add entry to IESTable table:%d\n", r );

    r = run_query(hdb, 0, "INSERT INTO `IESTable` "
            "( `Action`, `Condition`, `Sequence`) "
            "VALUES ( 'build', 'cond3', 3)");
    ok(r == S_OK, "cannot add entry to IESTable table:%d\n", r );

    r = run_query(hdb, 0, "INSERT INTO `CATable` "
            "( `Action`, `Type` ) "
            "VALUES ( 'build', 32)");
    ok(r == S_OK, "cannot add entry to CATable table:%d\n", r );

    r = run_query(hdb, 0, "INSERT INTO `CATable` "
            "( `Action`, `Type` ) "
            "VALUES ( 'depends', 64)");
    ok(r == S_OK, "cannot add entry to CATable table:%d\n", r );

    r = run_query(hdb, 0, "INSERT INTO `CATable` "
            "( `Action`, `Type` ) "
            "VALUES ( 'clean', 63)");
    ok(r == S_OK, "cannot add entry to CATable table:%d\n", r );

    r = run_query(hdb, 0, "INSERT INTO `CATable` "
            "( `Action`, `Type` ) "
            "VALUES ( 'build2', 34)");
    ok(r == S_OK, "cannot add entry to CATable table:%d\n", r );

    query = NULL;
    sql = "Select IESTable.Condition from CATable, IESTable where "
            "CATable.Action = IESTable.Action and CATable.Type = 32";
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok( r == ERROR_SUCCESS, "failed to open query: %d\n", r );

    r = MsiQueryExecute(query, 0);
    ok( r == ERROR_SUCCESS, "failed to execute query: %d\n", r );

    r = MsiQueryFetch(query, &rec);
    ok( r == ERROR_SUCCESS, "failed to fetch query: %d\n", r );

    ok( check_record( rec, 1, "cond2"), "wrong condition\n");

    MsiCloseHandle( rec );
    r = MsiQueryFetch(query, &rec);
    ok( r == ERROR_SUCCESS, "failed to fetch query: %d\n", r );

    ok( check_record( rec, 1, "cond3"), "wrong condition\n");

    MsiCloseHandle( rec );
    MsiQueryClose(query);
    MsiCloseHandle(query);

    MsiCloseHandle( hdb );
    DeleteFile(msifile);

}


static void test_where(void)
{
    LibmsiObject *hdb = 0;
    LibmsiObject *rec;
    LibmsiObject *query;
    const char *sql;
    unsigned r;
    unsigned size;
    char buf[MAX_PATH];
    unsigned count;

    hdb = create_db();
    ok( hdb, "failed to create db\n");

    r = run_query( hdb, 0,
            "CREATE TABLE `Media` ("
            "`DiskId` SHORT NOT NULL, "
            "`LastSequence` LONG, "
            "`DiskPrompt` CHAR(64) LOCALIZABLE, "
            "`Cabinet` CHAR(255), "
            "`VolumeLabel` CHAR(32), "
            "`Source` CHAR(72) "
            "PRIMARY KEY `DiskId`)" );
    ok( r == S_OK, "cannot create Media table: %d\n", r );

    r = run_query( hdb, 0, "INSERT INTO `Media` "
            "( `DiskId`, `LastSequence`, `DiskPrompt`, `Cabinet`, `VolumeLabel`, `Source` ) "
            "VALUES ( 1, 0, '', 'zero.cab', '', '' )" );
    ok( r == S_OK, "cannot add file to the Media table: %d\n", r );

    r = run_query( hdb, 0, "INSERT INTO `Media` "
            "( `DiskId`, `LastSequence`, `DiskPrompt`, `Cabinet`, `VolumeLabel`, `Source` ) "
            "VALUES ( 2, 1, '', 'one.cab', '', '' )" );
    ok( r == S_OK, "cannot add file to the Media table: %d\n", r );

    r = run_query( hdb, 0, "INSERT INTO `Media` "
            "( `DiskId`, `LastSequence`, `DiskPrompt`, `Cabinet`, `VolumeLabel`, `Source` ) "
            "VALUES ( 3, 2, '', 'two.cab', '', '' )" );
    ok( r == S_OK, "cannot add file to the Media table: %d\n", r );

    sql = "SELECT * FROM `Media`";
    r = do_query(hdb, sql, &rec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed: %d\n", r);
    ok( check_record( rec, 4, "zero.cab"), "wrong cabinet\n");
    MsiCloseHandle( rec );

    sql = "SELECT * FROM `Media` WHERE `LastSequence` >= 1";
    r = do_query(hdb, sql, &rec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed: %d\n", r);
    ok( check_record( rec, 4, "one.cab"), "wrong cabinet\n");

    r = MsiRecordGetInteger(rec, 1);
    ok( 2 == r, "field wrong\n");
    r = MsiRecordGetInteger(rec, 2);
    ok( 1 == r, "field wrong\n");
    MsiCloseHandle( rec );

    sql = "SELECT `DiskId` FROM `Media` WHERE `LastSequence` >= 1 AND DiskId >= 0";
    query = NULL;
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok( r == ERROR_SUCCESS, "failed to open query: %d\n", r );

    r = MsiQueryExecute(query, 0);
    ok( r == ERROR_SUCCESS, "failed to execute query: %d\n", r );

    r = MsiQueryFetch(query, &rec);
    ok( r == ERROR_SUCCESS, "failed to fetch query: %d\n", r );

    count = MsiRecordGetFieldCount( rec );
    ok( count == 1, "Expected 1 record fields, got %d\n", count );

    size = MAX_PATH;
    r = MsiRecordGetString( rec, 1, buf, &size );
    ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
    ok( !strcmp( buf, "2" ),
        "For (row %d, column 1) expected '%d', got %s\n", 0, 2, buf );
    MsiCloseHandle( rec );

    r = MsiQueryFetch(query, &rec);
    ok( r == ERROR_SUCCESS, "failed to fetch query: %d\n", r );

    size = MAX_PATH;
    r = MsiRecordGetString( rec, 1, buf, &size );
    ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
    ok( !strcmp( buf, "3" ),
        "For (row %d, column 1) expected '%d', got %s\n", 1, 3, buf );
    MsiCloseHandle( rec );

    r = MsiQueryFetch(query, &rec);
    ok( r == ERROR_NO_MORE_ITEMS, "expected no more items: %d\n", r );

    MsiQueryClose(query);
    MsiCloseHandle(query);

    MsiCloseHandle( rec );

    rec = 0;
    sql = "SELECT * FROM `Media` WHERE `DiskPrompt` IS NULL";
    r = do_query(hdb, sql, &rec);
    ok( r == ERROR_SUCCESS, "query failed: %d\n", r );
    MsiCloseHandle( rec );

    rec = 0;
    sql = "SELECT * FROM `Media` WHERE `DiskPrompt` < 'Cabinet'";
    r = do_query(hdb, sql, &rec);
    ok( r == ERROR_BAD_QUERY_SYNTAX, "query failed: %d\n", r );
    MsiCloseHandle( rec );

    rec = 0;
    sql = "SELECT * FROM `Media` WHERE `DiskPrompt` > 'Cabinet'";
    r = do_query(hdb, sql, &rec);
    ok( r == ERROR_BAD_QUERY_SYNTAX, "query failed: %d\n", r );
    MsiCloseHandle( rec );

    rec = 0;
    sql = "SELECT * FROM `Media` WHERE `DiskPrompt` <> 'Cabinet'";
    r = do_query(hdb, sql, &rec);
    ok( r == ERROR_SUCCESS, "query failed: %d\n", r );
    MsiCloseHandle( rec );

    rec = 0;
    sql = "SELECT * FROM `Media` WHERE `DiskPrompt` = 'Cabinet'";
    r = do_query(hdb, sql, &rec);
    ok( r == ERROR_NO_MORE_ITEMS, "query failed: %d\n", r );
    MsiCloseHandle( rec );

    rec = MsiCreateRecord(1);
    MsiRecordSetString(rec, 1, "");

    sql = "SELECT * FROM `Media` WHERE `DiskPrompt` = ?";
    query = NULL;
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryExecute(query, rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(rec);

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(rec);
    MsiQueryClose(query);
    MsiCloseHandle(query);

    MsiCloseHandle( hdb );
    DeleteFile(msifile);
}

static char CURR_DIR[MAX_PATH];

static const char test_data[] = "FirstPrimaryColumn\tSecondPrimaryColumn\tShortInt\tShortIntNullable\tLongInt\tLongIntNullable\tString\tLocalizableString\tLocalizableStringNullable\n"
                                "s255\ti2\ti2\tI2\ti4\tI4\tS255\tS0\ts0\n"
                                "TestTable\tFirstPrimaryColumn\n"
                                "stringage\t5\t2\t\t2147483640\t-2147483640\tanother string\tlocalizable\tduh\n";

static const char two_primary[] = "PrimaryOne\tPrimaryTwo\n"
                                  "s255\ts255\n"
                                  "TwoPrimary\tPrimaryOne\tPrimaryTwo\n"
                                  "papaya\tleaf\n"
                                  "papaya\tflower\n";

static const char endlines1[] = "A\tB\tC\tD\tE\tF\r\n"
                                "s72\ts72\ts72\ts72\ts72\ts72\n"
                                "Table\tA\r\n"
                                "a\tb\tc\td\te\tf\n"
                                "g\th\ti\t\rj\tk\tl\r\n";

static const char endlines2[] = "A\tB\tC\tD\tE\tF\r"
                                "s72\ts72\ts72\ts72\ts72\ts72\n"
                                "Table2\tA\r\n"
                                "a\tb\tc\td\te\tf\n"
                                "g\th\ti\tj\tk\tl\r\n";

static const char suminfo[] = "PropertyId\tValue\n"
                              "i2\tl255\n"
                              "_SummaryInformation\tPropertyId\n"
                              "1\t1252\n"
                              "2\tInstaller Database\n"
                              "3\tInstaller description\n"
                              "4\tWineHQ\n"
                              "5\tInstaller\n"
                              "6\tInstaller comments\n"
                              "7\tIntel;1033,2057\n"
                              "9\t{12345678-1234-1234-1234-123456789012}\n"
                              "12\t2009/04/12 15:46:11\n"
                              "13\t2009/04/12 15:46:11\n"
                              "14\t200\n"
                              "15\t2\n"
                              "18\tVim\n"
                              "19\t2\n";

static void write_file(const char *filename, const char *data, int data_size)
{
    unsigned size;

    HANDLE hf = CreateFile(filename, GENERIC_WRITE, 0, NULL,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    WriteFile(hf, data, data_size, &size, NULL);
    CloseHandle(hf);
}

static unsigned add_table_to_db(LibmsiObject *hdb, const char *table_data)
{
    unsigned r;

    write_file("temp_file", table_data, (strlen(table_data) - 1) * sizeof(char));
    r = MsiDatabaseImport(hdb, CURR_DIR, "temp_file");
    DeleteFileA("temp_file");

    return r;
}

static void test_suminfo_import(void)
{
    LibmsiObject *hdb;
    LibmsiObject *hsi;
    LibmsiObject *query = 0;
    const char *sql;
    unsigned r, count, size, type;
    char str_value[50];
    int int_value;
    uint64_t ft_value;

    GetCurrentDirectoryA(MAX_PATH, CURR_DIR);

    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = add_table_to_db(hdb, suminfo);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    /* _SummaryInformation is not imported as a regular table... */

    query = NULL;
    sql = "SELECT * FROM `_SummaryInformation`";
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %u\n", r);
    MsiCloseHandle(query);

    /* ...its data is added to the special summary information stream */

    r = MsiGetSummaryInformation(hdb, 0, &hsi);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiSummaryInfoGetPropertyCount(hsi, &count);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(count == 14, "Expected 14, got %u\n", count);

    r = MsiSummaryInfoGetProperty(hsi, MSI_PID_CODEPAGE, &type, &int_value, NULL, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(type ==  VT_I2, "Expected VT_I2, got %u\n", type);
    ok(int_value == 1252, "Expected 1252, got %d\n", int_value);

    size = sizeof(str_value);
    r = MsiSummaryInfoGetProperty(hsi, MSI_PID_TITLE, &type, NULL, NULL, str_value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(type == VT_LPSTR, "Expected VT_LPSTR, got %u\n", type);
    ok(size == 18, "Expected 18, got %u\n", size);
    ok(!strcmp(str_value, "Installer Database"),
       "Expected \"Installer Database\", got %s\n", str_value);

    size = sizeof(str_value);
    r = MsiSummaryInfoGetProperty(hsi, MSI_PID_SUBJECT, &type, NULL, NULL, str_value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(type == VT_LPSTR, "Expected VT_LPSTR, got %u\n", type);
    ok(!strcmp(str_value, "Installer description"),
       "Expected \"Installer description\", got %s\n", str_value);

    size = sizeof(str_value);
    r = MsiSummaryInfoGetProperty(hsi, MSI_PID_AUTHOR, &type, NULL, NULL, str_value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(type == VT_LPSTR, "Expected VT_LPSTR, got %u\n", type);
    ok(!strcmp(str_value, "WineHQ"),
       "Expected \"WineHQ\", got %s\n", str_value);

    size = sizeof(str_value);
    r = MsiSummaryInfoGetProperty(hsi, MSI_PID_KEYWORDS, &type, NULL, NULL, str_value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(type == VT_LPSTR, "Expected VT_LPSTR, got %u\n", type);
    ok(!strcmp(str_value, "Installer"),
       "Expected \"Installer\", got %s\n", str_value);

    size = sizeof(str_value);
    r = MsiSummaryInfoGetProperty(hsi, MSI_PID_COMMENTS, &type, NULL, NULL, str_value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(type == VT_LPSTR, "Expected VT_LPSTR, got %u\n", type);
    ok(!strcmp(str_value, "Installer comments"),
       "Expected \"Installer comments\", got %s\n", str_value);

    size = sizeof(str_value);
    r = MsiSummaryInfoGetProperty(hsi, MSI_PID_TEMPLATE, &type, NULL, NULL, str_value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(type == VT_LPSTR, "Expected VT_LPSTR, got %u\n", type);
    ok(!strcmp(str_value, "Intel;1033,2057"),
       "Expected \"Intel;1033,2057\", got %s\n", str_value);

    size = sizeof(str_value);
    r = MsiSummaryInfoGetProperty(hsi, MSI_PID_REVNUMBER, &type, NULL, NULL, str_value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(type == VT_LPSTR, "Expected VT_LPSTR, got %u\n", type);
    ok(!strcmp(str_value, "{12345678-1234-1234-1234-123456789012}"),
       "Expected \"{12345678-1234-1234-1234-123456789012}\", got %s\n", str_value);

    r = MsiSummaryInfoGetProperty(hsi, MSI_PID_CREATE_DTM, &type, NULL, &ft_value, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(type == VT_FILETIME, "Expected VT_FILETIME, got %u\n", type);

    r = MsiSummaryInfoGetProperty(hsi, MSI_PID_LASTSAVE_DTM, &type, NULL, &ft_value, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(type == VT_FILETIME, "Expected VT_FILETIME, got %u\n", type);

    r = MsiSummaryInfoGetProperty(hsi, MSI_PID_PAGECOUNT, &type, &int_value, NULL, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(type ==  VT_I4, "Expected VT_I4, got %u\n", type);
    ok(int_value == 200, "Expected 200, got %d\n", int_value);

    r = MsiSummaryInfoGetProperty(hsi, MSI_PID_WORDCOUNT, &type, &int_value, NULL, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(type ==  VT_I4, "Expected VT_I4, got %u\n", type);
    ok(int_value == 2, "Expected 2, got %d\n", int_value);

    r = MsiSummaryInfoGetProperty(hsi, MSI_PID_SECURITY, &type, &int_value, NULL, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(type ==  VT_I4, "Expected VT_I4, got %u\n", type);
    ok(int_value == 2, "Expected 2, got %d\n", int_value);

    size = sizeof(str_value);
    r = MsiSummaryInfoGetProperty(hsi, MSI_PID_APPNAME, &type, NULL, NULL, str_value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(type == VT_LPSTR, "Expected VT_LPSTR, got %u\n", type);
    ok(!strcmp(str_value, "Vim"), "Expected \"Vim\", got %s\n", str_value);

    MsiCloseHandle(hsi);
    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

static void test_msiimport(void)
{
    LibmsiObject *hdb;
    LibmsiObject *query;
    LibmsiObject *rec;
    const char *sql;
    unsigned r, count;
    signed int i;

    GetCurrentDirectoryA(MAX_PATH, CURR_DIR);

    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_table_to_db(hdb, test_data);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_table_to_db(hdb, two_primary);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_table_to_db(hdb, endlines1);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_table_to_db(hdb, endlines2);
    ok(r == ERROR_FUNCTION_FAILED, "Expected ERROR_FUNCTION_FAILED, got %d\n", r);

    query = NULL;
    sql = "SELECT * FROM `TestTable`";
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    rec = NULL;
    r = MsiQueryGetColumnInfo(query, LIBMSI_COL_INFO_NAMES, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    count = MsiRecordGetFieldCount(rec);
    ok(count == 9, "Expected 9, got %d\n", count);
    ok(check_record(rec, 1, "FirstPrimaryColumn"), "Expected FirstPrimaryColumn\n");
    ok(check_record(rec, 2, "SecondPrimaryColumn"), "Expected SecondPrimaryColumn\n");
    ok(check_record(rec, 3, "ShortInt"), "Expected ShortInt\n");
    ok(check_record(rec, 4, "ShortIntNullable"), "Expected ShortIntNullalble\n");
    ok(check_record(rec, 5, "LongInt"), "Expected LongInt\n");
    ok(check_record(rec, 6, "LongIntNullable"), "Expected LongIntNullalble\n");
    ok(check_record(rec, 7, "String"), "Expected String\n");
    ok(check_record(rec, 8, "LocalizableString"), "Expected LocalizableString\n");
    ok(check_record(rec, 9, "LocalizableStringNullable"), "Expected LocalizableStringNullable\n");
    MsiCloseHandle(rec);

    rec = NULL;
    r = MsiQueryGetColumnInfo(query, LIBMSI_COL_INFO_TYPES, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    count = MsiRecordGetFieldCount(rec);
    ok(count == 9, "Expected 9, got %d\n", count);
    ok(check_record(rec, 1, "s255"), "Expected s255\n");
    ok(check_record(rec, 2, "i2"), "Expected i2\n");
    ok(check_record(rec, 3, "i2"), "Expected i2\n");
    ok(check_record(rec, 4, "I2"), "Expected I2\n");
    ok(check_record(rec, 5, "i4"), "Expected i4\n");
    ok(check_record(rec, 6, "I4"), "Expected I4\n");
    ok(check_record(rec, 7, "S255"), "Expected S255\n");
    ok(check_record(rec, 8, "S0"), "Expected S0\n");
    ok(check_record(rec, 9, "s0"), "Expected s0\n");
    MsiCloseHandle(rec);

    sql = "SELECT * FROM `TestTable`";
    r = do_query(hdb, sql, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(check_record(rec, 1, "stringage"), "Expected 'stringage'\n");
    ok(check_record(rec, 7, "another string"), "Expected 'another string'\n");
    ok(check_record(rec, 8, "localizable"), "Expected 'localizable'\n");
    ok(check_record(rec, 9, "duh"), "Expected 'duh'\n");

    i = MsiRecordGetInteger(rec, 2);
    ok(i == 5, "Expected 5, got %d\n", i);

    i = MsiRecordGetInteger(rec, 3);
    ok(i == 2, "Expected 2, got %d\n", i);

    i = MsiRecordGetInteger(rec, 4);
    ok(i == MSI_NULL_INTEGER, "Expected MSI_NULL_INTEGER, got %d\n", i);

    i = MsiRecordGetInteger(rec, 5);
    ok(i == 2147483640, "Expected 2147483640, got %d\n", i);

    i = MsiRecordGetInteger(rec, 6);
    ok(i == -2147483640, "Expected -2147483640, got %d\n", i);

    MsiCloseHandle(rec);
    MsiQueryClose(query);
    MsiCloseHandle(query);

    query = NULL;
    sql = "SELECT * FROM `TwoPrimary`";
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    rec = NULL;
    r = MsiQueryGetColumnInfo(query, LIBMSI_COL_INFO_NAMES, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    count = MsiRecordGetFieldCount(rec);
    ok(count == 2, "Expected 2, got %d\n", count);
    ok(check_record(rec, 1, "PrimaryOne"), "Expected PrimaryOne\n");
    ok(check_record(rec, 2, "PrimaryTwo"), "Expected PrimaryTwo\n");

    MsiCloseHandle(rec);

    rec = NULL;
    r = MsiQueryGetColumnInfo(query, LIBMSI_COL_INFO_TYPES, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    count = MsiRecordGetFieldCount(rec);
    ok(count == 2, "Expected 2, got %d\n", count);
    ok(check_record(rec, 1, "s255"), "Expected s255\n");
    ok(check_record(rec, 2, "s255"), "Expected s255\n");
    MsiCloseHandle(rec);

    r = MsiQueryExecute(query, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    ok(check_record(rec, 1, "papaya"), "Expected 'papaya'\n");
    ok(check_record(rec, 2, "leaf"), "Expected 'leaf'\n");

    MsiCloseHandle(rec);

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    ok(check_record(rec, 1, "papaya"), "Expected 'papaya'\n");
    ok(check_record(rec, 2, "flower"), "Expected 'flower'\n");

    MsiCloseHandle(rec);

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_NO_MORE_ITEMS,
       "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    r = MsiQueryClose(query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(query);

    query = NULL;
    sql = "SELECT * FROM `Table`";
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    rec = NULL;
    r = MsiQueryGetColumnInfo(query, LIBMSI_COL_INFO_NAMES, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    count = MsiRecordGetFieldCount(rec);
    ok(count == 6, "Expected 6, got %d\n", count);
    ok(check_record(rec, 1, "A"), "Expected A\n");
    ok(check_record(rec, 2, "B"), "Expected B\n");
    ok(check_record(rec, 3, "C"), "Expected C\n");
    ok(check_record(rec, 4, "D"), "Expected D\n");
    ok(check_record(rec, 5, "E"), "Expected E\n");
    ok(check_record(rec, 6, "F"), "Expected F\n");
    MsiCloseHandle(rec);

    rec = NULL;
    r = MsiQueryGetColumnInfo(query, LIBMSI_COL_INFO_TYPES, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    count = MsiRecordGetFieldCount(rec);
    ok(count == 6, "Expected 6, got %d\n", count);
    ok(check_record(rec, 1, "s72"), "Expected s72\n");
    ok(check_record(rec, 2, "s72"), "Expected s72\n");
    ok(check_record(rec, 3, "s72"), "Expected s72\n");
    ok(check_record(rec, 4, "s72"), "Expected s72\n");
    ok(check_record(rec, 5, "s72"), "Expected s72\n");
    ok(check_record(rec, 6, "s72"), "Expected s72\n");
    MsiCloseHandle(rec);

    MsiQueryClose(query);
    MsiCloseHandle(query);

    query = NULL;
    sql = "SELECT * FROM `Table`";
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryExecute(query, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(check_record(rec, 1, "a"), "Expected 'a'\n");
    ok(check_record(rec, 2, "b"), "Expected 'b'\n");
    ok(check_record(rec, 3, "c"), "Expected 'c'\n");
    ok(check_record(rec, 4, "d"), "Expected 'd'\n");
    ok(check_record(rec, 5, "e"), "Expected 'e'\n");
    ok(check_record(rec, 6, "f"), "Expected 'f'\n");

    MsiCloseHandle(rec);

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(check_record(rec, 1, "g"), "Expected 'g'\n");
    ok(check_record(rec, 2, "h"), "Expected 'h'\n");
    ok(check_record(rec, 3, "i"), "Expected 'i'\n");
    ok(check_record(rec, 4, "j"), "Expected 'j'\n");
    ok(check_record(rec, 5, "k"), "Expected 'k'\n");
    ok(check_record(rec, 6, "l"), "Expected 'l'\n");

    MsiCloseHandle(rec);

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_NO_MORE_ITEMS,
       "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiQueryClose(query);
    MsiCloseHandle(query);
    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

static const char bin_import_dat[] = "Name\tData\r\n"
                                     "s72\tV0\r\n"
                                     "Binary\tName\r\n"
                                     "filename1\tfilename1.ibd\r\n";

static void test_binary_import(void)
{
    LibmsiObject *hdb = 0;
    LibmsiObject *rec;
    char file[MAX_PATH];
    char buf[MAX_PATH];
    char path[MAX_PATH];
    unsigned size;
    const char *sql;
    unsigned r;

    /* create files to import */
    write_file("bin_import.idt", bin_import_dat,
          (sizeof(bin_import_dat) - 1) * sizeof(char));
    CreateDirectory("bin_import", NULL);
    create_file_data("bin_import/filename1.ibd", "just some words", 15);

    /* import files into database */
    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_CREATE, &hdb);
    ok( r == ERROR_SUCCESS , "Failed to open database\n");

    GetCurrentDirectory(MAX_PATH, path);
    r = MsiDatabaseImport(hdb, path, "bin_import.idt");
    ok(r == ERROR_SUCCESS , "Failed to import Binary table\n");

    /* read file from the Binary table */
    sql = "SELECT * FROM `Binary`";
    r = do_query(hdb, sql, &rec);
    ok(r == ERROR_SUCCESS, "SELECT query failed: %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(rec, 1, file, &size);
    ok(r == ERROR_SUCCESS, "Failed to get string: %d\n", r);
    ok(!strcmp(file, "filename1"), "Expected 'filename1', got %s\n", file);

    size = MAX_PATH;
    memset(buf, 0, MAX_PATH);
    r = MsiRecordReadStream(rec, 2, buf, &size);
    ok(r == ERROR_SUCCESS, "Failed to get stream: %d\n", r);
    ok(!strcmp(buf, "just some words"),
        "Expected 'just some words', got %s\n", buf);

    r = MsiCloseHandle(rec);
    ok(r == ERROR_SUCCESS , "Failed to close record handle\n");

    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS , "Failed to close database\n");

    DeleteFile("bin_import/filename1.ibd");
    RemoveDirectory("bin_import");
    DeleteFile("bin_import.idt");
}

static void test_markers(void)
{
    LibmsiObject *hdb;
    LibmsiObject *rec;
    const char *sql;
    unsigned r;

    hdb = create_db();
    ok( hdb, "failed to create db\n");

    rec = MsiCreateRecord(3);
    MsiRecordSetString(rec, 1, "Table");
    MsiRecordSetString(rec, 2, "Apples");
    MsiRecordSetString(rec, 3, "Oranges");

    /* try a legit create */
    sql = "CREATE TABLE `Table` ( `One` SHORT NOT NULL, `Two` CHAR(255) PRIMARY KEY `One`)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    MsiCloseHandle(rec);

    /* try table name as marker */
    rec = MsiCreateRecord(1);
    MsiRecordSetString(rec, 1, "Fable");
    sql = "CREATE TABLE `?` ( `One` SHORT NOT NULL, `Two` CHAR(255) PRIMARY KEY `One`)";
    r = run_query(hdb, rec, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* verify that we just created a table called '?', not 'Fable' */
    r = try_query(hdb, "SELECT * from `Fable`");
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    r = try_query(hdb, "SELECT * from `?`");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* try table name as marker without backticks */
    MsiRecordSetString(rec, 1, "Mable");
    sql = "CREATE TABLE ? ( `One` SHORT NOT NULL, `Two` CHAR(255) PRIMARY KEY `One`)";
    r = run_query(hdb, rec, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    /* try one column name as marker */
    MsiRecordSetString(rec, 1, "One");
    sql = "CREATE TABLE `Mable` ( `?` SHORT NOT NULL, `Two` CHAR(255) PRIMARY KEY `One`)";
    r = run_query(hdb, rec, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);
    MsiCloseHandle(rec);

    /* try column names as markers */
    rec = MsiCreateRecord(2);
    MsiRecordSetString(rec, 1, "One");
    MsiRecordSetString(rec, 2, "Two");
    sql = "CREATE TABLE `Mable` ( `?` SHORT NOT NULL, `?` CHAR(255) PRIMARY KEY `One`)";
    r = run_query(hdb, rec, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);
    MsiCloseHandle(rec);

    /* try names with backticks */
    rec = MsiCreateRecord(3);
    MsiRecordSetString(rec, 1, "One");
    MsiRecordSetString(rec, 2, "Two");
    MsiRecordSetString(rec, 3, "One");
    sql = "CREATE TABLE `Mable` ( `?` SHORT NOT NULL, `?` CHAR(255) PRIMARY KEY `?`)";
    r = run_query(hdb, rec, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    /* try names with backticks, minus definitions */
    sql = "CREATE TABLE `Mable` ( `?`, `?` PRIMARY KEY `?`)";
    r = run_query(hdb, rec, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    /* try names without backticks */
    sql = "CREATE TABLE `Mable` ( ? SHORT NOT NULL, ? CHAR(255) PRIMARY KEY ?)";
    r = run_query(hdb, rec, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);
    MsiCloseHandle(rec);

    /* try one long marker */
    rec = MsiCreateRecord(1);
    MsiRecordSetString(rec, 1, "`One` SHORT NOT NULL, `Two` CHAR(255) PRIMARY KEY `One`");
    sql = "CREATE TABLE `Mable` ( ? )";
    r = run_query(hdb, rec, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);
    MsiCloseHandle(rec);

    /* try all names as markers */
    rec = MsiCreateRecord(4);
    MsiRecordSetString(rec, 1, "Mable");
    MsiRecordSetString(rec, 2, "One");
    MsiRecordSetString(rec, 3, "Two");
    MsiRecordSetString(rec, 4, "One");
    sql = "CREATE TABLE `?` ( `?` SHORT NOT NULL, `?` CHAR(255) PRIMARY KEY `?`)";
    r = run_query(hdb, rec, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);
    MsiCloseHandle(rec);

    /* try a legit insert */
    sql = "INSERT INTO `Table` ( `One`, `Two` ) VALUES ( 5, 'hello' )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = try_query(hdb, "SELECT * from `Table`");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* try values as markers */
    rec = MsiCreateRecord(2);
    MsiRecordSetInteger(rec, 1, 4);
    MsiRecordSetString(rec, 2, "hi");
    sql = "INSERT INTO `Table` ( `One`, `Two` ) VALUES ( ?, '?' )";
    r = run_query(hdb, rec, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    MsiCloseHandle(rec);

    /* try column names and values as markers */
    rec = MsiCreateRecord(4);
    MsiRecordSetString(rec, 1, "One");
    MsiRecordSetString(rec, 2, "Two");
    MsiRecordSetInteger(rec, 3, 5);
    MsiRecordSetString(rec, 4, "hi");
    sql = "INSERT INTO `Table` ( `?`, `?` ) VALUES ( ?, '?' )";
    r = run_query(hdb, rec, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);
    MsiCloseHandle(rec);

    /* try column names as markers */
    rec = MsiCreateRecord(2);
    MsiRecordSetString(rec, 1, "One");
    MsiRecordSetString(rec, 2, "Two");
    sql = "INSERT INTO `Table` ( `?`, `?` ) VALUES ( 3, 'yellow' )";
    r = run_query(hdb, rec, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);
    MsiCloseHandle(rec);

    /* try table name as a marker */
    rec = MsiCreateRecord(1);
    MsiRecordSetString(rec, 1, "Table");
    sql = "INSERT INTO `?` ( `One`, `Two` ) VALUES ( 2, 'green' )";
    r = run_query(hdb, rec, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    MsiCloseHandle(rec);

    /* try table name and values as markers */
    rec = MsiCreateRecord(3);
    MsiRecordSetString(rec, 1, "Table");
    MsiRecordSetInteger(rec, 2, 10);
    MsiRecordSetString(rec, 3, "haha");
    sql = "INSERT INTO `?` ( `One`, `Two` ) VALUES ( ?, '?' )";
    r = run_query(hdb, rec, sql);
    ok(r == ERROR_FUNCTION_FAILED, "Expected ERROR_FUNCTION_FAILED, got %d\n", r);
    MsiCloseHandle(rec);

    /* try all markers */
    rec = MsiCreateRecord(5);
    MsiRecordSetString(rec, 1, "Table");
    MsiRecordSetString(rec, 1, "One");
    MsiRecordSetString(rec, 1, "Two");
    MsiRecordSetInteger(rec, 2, 10);
    MsiRecordSetString(rec, 3, "haha");
    sql = "INSERT INTO `?` ( `?`, `?` ) VALUES ( ?, '?' )";
    r = run_query(hdb, rec, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);
    MsiCloseHandle(rec);

    /* insert an integer as a string */
    rec = MsiCreateRecord(2);
    MsiRecordSetString(rec, 1, "11");
    MsiRecordSetString(rec, 2, "hi");
    sql = "INSERT INTO `Table` ( `One`, `Two` ) VALUES ( ?, '?' )";
    r = run_query(hdb, rec, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    MsiCloseHandle(rec);

    /* leave off the '' for the string */
    rec = MsiCreateRecord(2);
    MsiRecordSetInteger(rec, 1, 12);
    MsiRecordSetString(rec, 2, "hi");
    sql = "INSERT INTO `Table` ( `One`, `Two` ) VALUES ( ?, ? )";
    r = run_query(hdb, rec, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    MsiCloseHandle(rec);

    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

#define MY_NQUERIES 4000    /* Largest installer I've seen uses < 2k */
static void test_handle_limit(void)
{
    int i;
    LibmsiObject *hdb;
    LibmsiObject *hqueries[MY_NQUERIES];
    unsigned r;

    /* create an empty db */
    hdb = create_db();
    ok( hdb, "failed to create db\n");

    memset(hqueries, 0, sizeof(hqueries));

    for (i=0; i<MY_NQUERIES; i++) {
        static char szQueryBuf[256] = "SELECT * from `_Tables`";
        hqueries[i] = 0xdeadbeeb;
        r = MsiDatabaseOpenQuery(hdb, szQueryBuf, &hqueries[i]);
        if( r != ERROR_SUCCESS || hqueries[i] == 0xdeadbeeb || 
            hqueries[i] == 0 || (i && (hqueries[i] == hqueries[i-1])))
            break;
    }

    ok( i == MY_NQUERIES, "problem opening queries\n");

    for (i=0; i<MY_NQUERIES; i++) {
        if (hqueries[i] != 0 && hqueries[i] != 0xdeadbeeb) {
            MsiQueryClose(hqueries[i]);
            r = MsiCloseHandle(hqueries[i]);
            if (r != ERROR_SUCCESS)
                break;
        }
    }

    ok( i == MY_NQUERIES, "problem closing queries\n");

    r = MsiCloseHandle(hdb);
    ok( r == ERROR_SUCCESS, "failed to close database\n");
}

/* data for generating a transform */

/* tables transform names - encoded as they would be in an msi database file */
static const WCHAR name1[] = { 0x4840, 0x3a8a, 0x481b, 0 }; /* AAR */
static const WCHAR name2[] = { 0x4840, 0x3b3f, 0x43f2, 0x4438, 0x45b1, 0 }; /* _Columns */
static const WCHAR name3[] = { 0x4840, 0x3f7f, 0x4164, 0x422f, 0x4836, 0 }; /* _Tables */
static const WCHAR name4[] = { 0x4840, 0x3f3f, 0x4577, 0x446c, 0x3b6a, 0x45e4, 0x4824, 0 }; /* _StringData */
static const WCHAR name5[] = { 0x4840, 0x3f3f, 0x4577, 0x446c, 0x3e6a, 0x44b2, 0x482f, 0 }; /* _StringPool */
static const WCHAR name6[] = { 0x4840, 0x3e16, 0x4818, 0}; /* MOO */
static const WCHAR name7[] = { 0x4840, 0x3c8b, 0x3a97, 0x409b, 0 }; /* BINARY */
static const WCHAR name8[] = { 0x3c8b, 0x3a97, 0x409b, 0x387e, 0 }; /* BINARY.1 */
static const WCHAR name9[] = { 0x4840, 0x4559, 0x44f2, 0x4568, 0x4737, 0 }; /* Property */

/* data in each table */
static const WCHAR data1[] = { /* AAR */
    0x0201, 0x0008, 0x8001,  /* 0x0201 = add row (1), two shorts */
    0x0201, 0x0009, 0x8002,
};
static const WCHAR data2[] = { /* _Columns */
    0x0401, 0x0001, 0x8003, 0x0002, 0x9502,
    0x0401, 0x0001, 0x8004, 0x0003, 0x9502,
    0x0401, 0x0005, 0x0000, 0x0006, 0xbdff,  /* 0x0401 = add row (1), 4 shorts */
    0x0401, 0x0005, 0x0000, 0x0007, 0x8502,
    0x0401, 0x000a, 0x0000, 0x000a, 0xad48,
    0x0401, 0x000a, 0x0000, 0x000b, 0x9d00,
};
static const WCHAR data3[] = { /* _Tables */
    0x0101, 0x0005, /* 0x0101 = add row (1), 1 short */
    0x0101, 0x000a,
};
static const char data4[] = /* _StringData */
    "MOOCOWPIGcAARCARBARvwbmwPropertyValuepropval";  /* all the strings squashed together */
static const WCHAR data5[] = { /* _StringPool */
/*  len, refs */
    0,   0,    /* string 0 ''    */
    3,   2,    /* string 1 'MOO' */
    3,   1,    /* string 2 'COW' */
    3,   1,    /* string 3 'PIG' */
    1,   1,    /* string 4 'c'   */
    3,   3,    /* string 5 'AAR' */
    3,   1,    /* string 6 'CAR' */
    3,   1,    /* string 7 'BAR' */
    2,   1,    /* string 8 'vw'  */
    3,   1,    /* string 9 'bmw' */
    8,   4,    /* string 10 'Property' */
    5,   1,    /* string 11 'Value' */
    4,   1,    /* string 12 'prop' */
    3,   1,    /* string 13 'val' */
};
/* update row, 0x0002 is a bitmask of present column data, keys are excluded */
static const WCHAR data6[] = { /* MOO */
    0x000a, 0x8001, 0x0004, 0x8005, /* update row */
    0x0000, 0x8003,         /* delete row */
};

static const WCHAR data7[] = { /* BINARY */
    0x0201, 0x8001, 0x0001,
};

static const char data8[] =  /* stream data for the BINARY table */
    "naengmyon";

static const WCHAR data9[] = { /* Property */
    0x0201, 0x000c, 0x000d,
};

static const struct {
    const WCHAR *name;
    const void *data;
    unsigned size;
} table_transform_data[] =
{
    { name1, data1, sizeof data1 },
    { name2, data2, sizeof data2 },
    { name3, data3, sizeof data3 },
    { name4, data4, sizeof data4 - 1 },
    { name5, data5, sizeof data5 },
    { name6, data6, sizeof data6 },
    { name7, data7, sizeof data7 },
    { name8, data8, sizeof data8 - 1 },
    { name9, data9, sizeof data9 },
};

#define NUM_TRANSFORM_TABLES (sizeof table_transform_data/sizeof table_transform_data[0])

static void generate_transform_manual(void)
{
    IStorage *stg = NULL;
    IStream *stm;
    WCHAR name[0x20];
    HRESULT r;
    unsigned i, count;
    const unsigned mode = STGM_CREATE|STGM_READWRITE|STGM_DIRECT|STGM_SHARE_EXCLUSIVE;

    const CLSID CLSID_MsiTransform = { 0xc1082,0,0,{0xc0,0,0,0,0,0,0,0x46}};

    MultiByteToWideChar(CP_ACP, 0, mstfile, -1, name, 0x20);

    r = StgCreateDocfile(name, mode, 0, &stg);
    ok(r == S_OK, "failed to create storage\n");
    if (!stg)
        return;

    r = IStorage_SetClass( stg, &CLSID_MsiTransform );
    ok(r == S_OK, "failed to set storage type\n");

    for (i=0; i<NUM_TRANSFORM_TABLES; i++)
    {
        r = IStorage_CreateStream( stg, table_transform_data[i].name,
                            STGM_WRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &stm );
        if (FAILED(r))
        {
            ok(0, "failed to create stream %08x\n", r);
            continue;
        }

        r = IStream_Write( stm, table_transform_data[i].data,
                          table_transform_data[i].size, &count );
        if (FAILED(r) || count != table_transform_data[i].size)
            ok(0, "failed to write stream\n");
        IStream_Release(stm);
    }

    IStorage_Release(stg);
}

static unsigned set_summary_info(LibmsiObject *hdb)
{
    unsigned res;
    LibmsiObject *suminfo;

    /* build summary info */
    res = MsiGetSummaryInformation(hdb, 7, &suminfo);
    ok( res == ERROR_SUCCESS , "Failed to open summaryinfo\n" );

    res = MsiSummaryInfoSetProperty(suminfo,2, VT_LPSTR, 0,NULL,
                        "Installation Database");
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetProperty(suminfo,3, VT_LPSTR, 0,NULL,
                        "Installation Database");
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetProperty(suminfo,4, VT_LPSTR, 0,NULL,
                        "Wine Hackers");
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetProperty(suminfo,7, VT_LPSTR, 0,NULL,
                    ";1033,2057");
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetProperty(suminfo,9, VT_LPSTR, 0,NULL,
                    "{913B8D18-FBB6-4CAC-A239-C74C11E3FA74}");
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetProperty(suminfo, 14, VT_I4, 100, NULL, NULL);
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetProperty(suminfo, 15, VT_I4, 0, NULL, NULL);
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoPersist(suminfo);
    ok( res == ERROR_SUCCESS , "Failed to make summary info persist\n" );

    res = MsiCloseHandle( suminfo);
    ok( res == ERROR_SUCCESS , "Failed to close suminfo\n" );

    return res;
}

static LibmsiObject *create_package_db(const char *filename)
{
    LibmsiObject *hdb = 0;
    unsigned res;

    DeleteFile(msifile);

    /* create an empty database */
    res = MsiOpenDatabase(filename, LIBMSI_DB_OPEN_CREATE, &hdb );
    ok( res == ERROR_SUCCESS , "Failed to create database\n" );
    if( res != ERROR_SUCCESS )
        return hdb;

    res = MsiDatabaseCommit( hdb );
    ok( res == ERROR_SUCCESS , "Failed to commit database\n" );

    res = set_summary_info(hdb);
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = create_directory_table(hdb);
    ok( res == ERROR_SUCCESS , "Failed to create directory table\n" );

    return hdb;
}

static void test_try_transform(void)
{
    LibmsiObject *hdb;
    LibmsiObject *hquery;
    LibmsiObject *hrec;
    LibmsiObject *hpkg = 0;
    const char *sql;
    unsigned r;
    unsigned sz;
    char buffer[MAX_PATH];

    DeleteFile(msifile);
    DeleteFile(mstfile);

    /* create the database */
    hdb = create_package_db(msifile);
    ok(hdb, "Failed to create package db\n");

    sql = "CREATE TABLE `MOO` ( `NOO` SHORT NOT NULL, `OOO` CHAR(255) PRIMARY KEY `NOO`)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "failed to add table\n");

    sql = "INSERT INTO `MOO` ( `NOO`, `OOO` ) VALUES ( 1, 'a' )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "failed to add row\n");

    sql = "INSERT INTO `MOO` ( `NOO`, `OOO` ) VALUES ( 2, 'b' )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "failed to add row\n");

    sql = "INSERT INTO `MOO` ( `NOO`, `OOO` ) VALUES ( 3, 'c' )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "failed to add row\n");

    sql = "CREATE TABLE `BINARY` ( `ID` SHORT NOT NULL, `BLOB` OBJECT PRIMARY KEY `ID`)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "failed to add table\n");

    hrec = MsiCreateRecord(2);
    r = MsiRecordSetInteger(hrec, 1, 2);
    ok(r == ERROR_SUCCESS, "failed to set integer\n");

    write_file("testdata.bin", "lamyon", 6);
    r = MsiRecordSetStream(hrec, 2, "testdata.bin");
    ok(r == ERROR_SUCCESS, "failed to set stream\n");

    sql = "INSERT INTO `BINARY` ( `ID`, `BLOB` ) VALUES ( ?, ? )";
    r = run_query(hdb, hrec, sql);
    ok(r == ERROR_SUCCESS, "failed to add row with blob\n");

    MsiCloseHandle(hrec);

    r = MsiDatabaseCommit( hdb );
    ok( r == ERROR_SUCCESS , "Failed to commit database\n" );

    MsiCloseHandle( hdb );
    DeleteFileA("testdata.bin");

    generate_transform_manual();

    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_DIRECT, &hdb );
    ok( r == ERROR_SUCCESS , "Failed to create database\n" );

    r = MsiDatabaseApplyTransform( hdb, mstfile, 0 );
    ok( r == ERROR_SUCCESS, "return code %d, should be ERROR_SUCCESS\n", r );

    MsiDatabaseCommit( hdb );

    /* check new values */
    hrec = 0;
    sql = "select `BAR`,`CAR` from `AAR` where `BAR` = 1 AND `CAR` = 'vw'";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_SUCCESS, "select query failed\n");
    MsiCloseHandle(hrec);

    sql = "select `BAR`,`CAR` from `AAR` where `BAR` = 2 AND `CAR` = 'bmw'";
    hrec = 0;
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_SUCCESS, "select query failed\n");
    MsiCloseHandle(hrec);

    /* check updated values */
    hrec = 0;
    sql = "select `NOO`,`OOO` from `MOO` where `NOO` = 1 AND `OOO` = 'c'";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_SUCCESS, "select query failed\n");
    MsiCloseHandle(hrec);

    /* check unchanged value */
    hrec = 0;
    sql = "select `NOO`,`OOO` from `MOO` where `NOO` = 2 AND `OOO` = 'b'";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_SUCCESS, "select query failed\n");
    MsiCloseHandle(hrec);

    /* check deleted value */
    hrec = 0;
    sql = "select * from `MOO` where `NOO` = 3";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "select query failed\n");
    if (hrec) MsiCloseHandle(hrec);

    /* check added stream */
    hrec = 0;
    sql = "select `BLOB` from `BINARY` where `ID` = 1";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_SUCCESS, "select query failed\n");

    /* check the contents of the stream */
    sz = sizeof buffer;
    r = MsiRecordReadStream( hrec, 1, buffer, &sz );
    ok(r == ERROR_SUCCESS, "read stream failed\n");
    ok(!memcmp(buffer, "naengmyon", 9), "stream data was wrong\n");
    ok(sz == 9, "stream data was wrong size\n");
    if (hrec) MsiCloseHandle(hrec);

    /* check the validity of the table with a deleted row */
    hrec = 0;
    sql = "select * from `MOO`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "open query failed\n");

    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "query execute failed\n");

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "query fetch failed\n");

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 1, "Expected 1, got %d\n", r);

    sz = sizeof buffer;
    r = MsiRecordGetString(hrec, 2, buffer, &sz);
    ok(r == ERROR_SUCCESS, "record get string failed\n");
    ok(!strcmp(buffer, "c"), "Expected c, got %s\n", buffer);

    r = MsiRecordGetInteger(hrec, 3);
    ok(r == 0x80000000, "Expected 0x80000000, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 4);
    ok(r == 5, "Expected 5, got %d\n", r);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "query fetch failed\n");

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 2, "Expected 2, got %d\n", r);

    sz = sizeof buffer;
    r = MsiRecordGetString(hrec, 2, buffer, &sz);
    ok(r == ERROR_SUCCESS, "record get string failed\n");
    ok(!strcmp(buffer, "b"), "Expected b, got %s\n", buffer);

    r = MsiRecordGetInteger(hrec, 3);
    ok(r == 0x80000000, "Expected 0x80000000, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 4);
    ok(r == 0x80000000, "Expected 0x80000000, got %d\n", r);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "query fetch succeeded\n");

    MsiCloseHandle(hrec);
    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

#if 0
    /* check that the property was added */
    r = package_from_db(hdb, &hpkg);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    sz = MAX_PATH;
    r = MsiGetProperty(hpkg, "prop", buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "val"), "Expected val, got %s\n", buffer);
#endif

    MsiCloseHandle(hpkg);

error:
    MsiCloseHandle(hdb);
    DeleteFile(msifile);
    DeleteFile(mstfile);
}

struct join_res
{
    const char one[MAX_PATH];
    const char two[MAX_PATH];
};

struct join_res_4col
{
    const char one[MAX_PATH];
    const char two[MAX_PATH];
    const char three[MAX_PATH];
    const char four[MAX_PATH];
};

struct join_res_uint
{
    unsigned one;
    unsigned two;
    unsigned three;
    unsigned four;
    unsigned five;
    unsigned six;
};

static const struct join_res join_res_first[] =
{
    { "alveolar", "procerus" },
    { "septum", "procerus" },
    { "septum", "nasalis" },
    { "ramus", "nasalis" },
    { "malar", "mentalis" },
};

static const struct join_res join_res_second[] =
{
    { "nasal", "septum" },
    { "mandible", "ramus" },
};

static const struct join_res join_res_third[] =
{
    { "msvcp.dll", "abcdefgh" },
    { "msvcr.dll", "ijklmnop" },
};

static const struct join_res join_res_fourth[] =
{
    { "msvcp.dll.01234", "single.dll.31415" },
};

static const struct join_res join_res_fifth[] =
{
    { "malar", "procerus" },
};

static const struct join_res join_res_sixth[] =
{
    { "malar", "procerus" },
    { "malar", "procerus" },
    { "malar", "nasalis" },
    { "malar", "nasalis" },
    { "malar", "nasalis" },
    { "malar", "mentalis" },
};

static const struct join_res join_res_seventh[] =
{
    { "malar", "nasalis" },
    { "malar", "nasalis" },
    { "malar", "nasalis" },
};

static const struct join_res_4col join_res_eighth[] =
{
    { "msvcp.dll", "msvcp.dll.01234", "msvcp.dll.01234", "abcdefgh" },
    { "msvcr.dll", "msvcr.dll.56789", "msvcp.dll.01234", "abcdefgh" },
    { "msvcp.dll", "msvcp.dll.01234", "msvcr.dll.56789", "ijklmnop" },
    { "msvcr.dll", "msvcr.dll.56789", "msvcr.dll.56789", "ijklmnop" },
    { "msvcp.dll", "msvcp.dll.01234", "single.dll.31415", "msvcp.dll" },
    { "msvcr.dll", "msvcr.dll.56789", "single.dll.31415", "msvcp.dll" },
};

static const struct join_res_uint join_res_ninth[] =
{
    { 1, 2, 3, 4, 7, 8 },
    { 1, 2, 5, 6, 7, 8 },
    { 1, 2, 3, 4, 9, 10 },
    { 1, 2, 5, 6, 9, 10 },
    { 1, 2, 3, 4, 11, 12 },
    { 1, 2, 5, 6, 11, 12 },
};

static void test_join(void)
{
    LibmsiObject *hdb;
    LibmsiObject *hquery;
    LibmsiObject *hrec;
    const char *sql;
    char buf[MAX_PATH];
    unsigned r, count;
    unsigned size, i;
    bool data_correct;

    hdb = create_db();
    ok( hdb, "failed to create db\n");

    r = create_component_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create Component table: %d\n", r );

    r = add_component_entry( hdb, "'zygomatic', 'malar', 'INSTALLDIR', 0, '', ''" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    r = add_component_entry( hdb, "'maxilla', 'alveolar', 'INSTALLDIR', 0, '', ''" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    r = add_component_entry( hdb, "'nasal', 'septum', 'INSTALLDIR', 0, '', ''" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    r = add_component_entry( hdb, "'mandible', 'ramus', 'INSTALLDIR', 0, '', ''" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    r = create_feature_components_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create FeatureComponents table: %d\n", r );

    r = add_feature_components_entry( hdb, "'procerus', 'maxilla'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'procerus', 'nasal'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'nasalis', 'nasal'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'nasalis', 'mandible'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'nasalis', 'notacomponent'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'mentalis', 'zygomatic'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = create_std_dlls_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create StdDlls table: %d\n", r );

    r = add_std_dlls_entry( hdb, "'msvcp.dll', 'msvcp.dll.01234'" );
    ok( r == ERROR_SUCCESS, "cannot add std dlls: %d\n", r );

    r = add_std_dlls_entry( hdb, "'msvcr.dll', 'msvcr.dll.56789'" );
    ok( r == ERROR_SUCCESS, "cannot add std dlls: %d\n", r );

    r = create_binary_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create Binary table: %d\n", r );

    r = add_binary_entry( hdb, "'msvcp.dll.01234', 'abcdefgh'" );
    ok( r == ERROR_SUCCESS, "cannot add binary: %d\n", r );

    r = add_binary_entry( hdb, "'msvcr.dll.56789', 'ijklmnop'" );
    ok( r == ERROR_SUCCESS, "cannot add binary: %d\n", r );

    r = add_binary_entry( hdb, "'single.dll.31415', 'msvcp.dll'" );
    ok( r == ERROR_SUCCESS, "cannot add binary: %d\n", r );

    sql = "CREATE TABLE `One` (`A` SHORT, `B` SHORT PRIMARY KEY `A`)";
    r = run_query( hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "cannot create table: %d\n", r );

    sql = "CREATE TABLE `Two` (`C` SHORT, `D` SHORT PRIMARY KEY `C`)";
    r = run_query( hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "cannot create table: %d\n", r );

    sql = "CREATE TABLE `Three` (`E` SHORT, `F` SHORT PRIMARY KEY `E`)";
    r = run_query( hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "cannot create table: %d\n", r );

    sql = "INSERT INTO `One` (`A`, `B`) VALUES (1, 2)";
    r = run_query( hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "cannot insert into table: %d\n", r );

    sql = "INSERT INTO `Two` (`C`, `D`) VALUES (3, 4)";
    r = run_query( hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "cannot insert into table: %d\n", r );

    sql = "INSERT INTO `Two` (`C`, `D`) VALUES (5, 6)";
    r = run_query( hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "cannot insert into table: %d\n", r );

    sql = "INSERT INTO `Three` (`E`, `F`) VALUES (7, 8)";
    r = run_query( hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "cannot insert into table: %d\n", r );

    sql = "INSERT INTO `Three` (`E`, `F`) VALUES (9, 10)";
    r = run_query( hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "cannot insert into table: %d\n", r );

    sql = "INSERT INTO `Three` (`E`, `F`) VALUES (11, 12)";
    r = run_query( hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "cannot insert into table: %d\n", r );

    sql = "CREATE TABLE `Four` (`G` SHORT, `H` SHORT PRIMARY KEY `G`)";
    r = run_query( hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "cannot create table: %d\n", r );

    sql = "CREATE TABLE `Five` (`I` SHORT, `J` SHORT PRIMARY KEY `I`)";
    r = run_query( hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "cannot create table: %d\n", r );

    sql = "INSERT INTO `Five` (`I`, `J`) VALUES (13, 14)";
    r = run_query( hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "cannot insert into table: %d\n", r );

    sql = "INSERT INTO `Five` (`I`, `J`) VALUES (15, 16)";
    r = run_query( hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "cannot insert into table: %d\n", r );

    sql = "SELECT `Component`.`ComponentId`, `FeatureComponents`.`Feature_` "
            "FROM `Component`, `FeatureComponents` "
            "WHERE `Component`.`Component` = `FeatureComponents`.`Component_` "
            "ORDER BY `Feature_`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok( r == ERROR_SUCCESS, "failed to open query: %d\n", r );

    r = MsiQueryExecute(hquery, 0);
    ok( r == ERROR_SUCCESS, "failed to execute query: %d\n", r );

    i = 0;
    while ((r = MsiQueryFetch(hquery, &hrec)) == ERROR_SUCCESS)
    {
        count = MsiRecordGetFieldCount( hrec );
        ok( count == 2, "Expected 2 record fields, got %d\n", count );

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 1, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        ok( !strcmp( buf, join_res_first[i].one ),
            "For (row %d, column 1) expected '%s', got %s\n", i, join_res_first[i].one, buf );

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 2, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        ok( !strcmp( buf, join_res_first[i].two ),
            "For (row %d, column 2) expected '%s', got %s\n", i, join_res_first[i].two, buf );

        i++;
        MsiCloseHandle(hrec);
    }

    ok( i == 5, "Expected 5 rows, got %d\n", i );
    ok( r == ERROR_NO_MORE_ITEMS, "expected no more items: %d\n", r );

    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    /* try a join without a WHERE condition */
    sql = "SELECT `Component`.`ComponentId`, `FeatureComponents`.`Feature_` "
            "FROM `Component`, `FeatureComponents` ";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok( r == ERROR_SUCCESS, "failed to open query: %d\n", r );

    r = MsiQueryExecute(hquery, 0);
    ok( r == ERROR_SUCCESS, "failed to execute query: %d\n", r );

    i = 0;
    while ((r = MsiQueryFetch(hquery, &hrec)) == ERROR_SUCCESS)
    {
        i++;
        MsiCloseHandle(hrec);
    }
    ok( i == 24, "Expected 24 rows, got %d\n", i );

    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    sql = "SELECT DISTINCT Component, ComponentId FROM FeatureComponents, Component "
            "WHERE FeatureComponents.Component_=Component.Component "
            "AND (Feature_='nasalis') ORDER BY Feature_";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok( r == ERROR_SUCCESS, "failed to open query: %d\n", r );

    r = MsiQueryExecute(hquery, 0);
    ok( r == ERROR_SUCCESS, "failed to execute query: %d\n", r );

    i = 0;
    data_correct = true;
    while ((r = MsiQueryFetch(hquery, &hrec)) == ERROR_SUCCESS)
    {
        count = MsiRecordGetFieldCount( hrec );
        ok( count == 2, "Expected 2 record fields, got %d\n", count );

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 1, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( strcmp( buf, join_res_second[i].one ))
            data_correct = false;

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 2, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( strcmp( buf, join_res_second[i].two ))
            data_correct = false;

        i++;
        MsiCloseHandle(hrec);
    }

    ok( data_correct, "data returned in the wrong order\n");

    ok( i == 2, "Expected 2 rows, got %d\n", i );
    ok( r == ERROR_NO_MORE_ITEMS, "expected no more items: %d\n", r );

    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    sql = "SELECT `StdDlls`.`File`, `Binary`.`Data` "
            "FROM `StdDlls`, `Binary` "
            "WHERE `StdDlls`.`Binary_` = `Binary`.`Name` "
            "ORDER BY `File`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok( r == ERROR_SUCCESS, "failed to open query: %d\n", r );

    r = MsiQueryExecute(hquery, 0);
    ok( r == ERROR_SUCCESS, "failed to execute query: %d\n", r );

    i = 0;
    data_correct = true;
    while ((r = MsiQueryFetch(hquery, &hrec)) == ERROR_SUCCESS)
    {
        count = MsiRecordGetFieldCount( hrec );
        ok( count == 2, "Expected 2 record fields, got %d\n", count );

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 1, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( strcmp( buf, join_res_third[i].one ) )
            data_correct = false;

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 2, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( strcmp( buf, join_res_third[i].two ) )
            data_correct = false;

        i++;
        MsiCloseHandle(hrec);
    }
    ok( data_correct, "data returned in the wrong order\n");

    ok( i == 2, "Expected 2 rows, got %d\n", i );

    ok( r == ERROR_NO_MORE_ITEMS, "expected no more items: %d\n", r );

    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    sql = "SELECT `StdDlls`.`Binary_`, `Binary`.`Name` "
            "FROM `StdDlls`, `Binary` "
            "WHERE `StdDlls`.`File` = `Binary`.`Data` "
            "ORDER BY `Name`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok( r == ERROR_SUCCESS, "failed to open query: %d\n", r );

    r = MsiQueryExecute(hquery, 0);
    ok( r == ERROR_SUCCESS, "failed to execute query: %d\n", r );

    i = 0;
    data_correct = true;
    while ((r = MsiQueryFetch(hquery, &hrec)) == ERROR_SUCCESS)
    {
        count = MsiRecordGetFieldCount( hrec );
        ok( count == 2, "Expected 2 record fields, got %d\n", count );

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 1, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( strcmp( buf, join_res_fourth[i].one ))
            data_correct = false;

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 2, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( strcmp( buf, join_res_fourth[i].two ))
            data_correct = false;

        i++;
        MsiCloseHandle(hrec);
    }
    ok( data_correct, "data returned in the wrong order\n");

    ok( i == 1, "Expected 1 rows, got %d\n", i );
    ok( r == ERROR_NO_MORE_ITEMS, "expected no more items: %d\n", r );

    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    sql = "SELECT `Component`.`ComponentId`, `FeatureComponents`.`Feature_` "
            "FROM `Component`, `FeatureComponents` "
            "WHERE `Component`.`Component` = 'zygomatic' "
            "AND `FeatureComponents`.`Component_` = 'maxilla' "
            "ORDER BY `Feature_`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok( r == ERROR_SUCCESS, "failed to open query: %d\n", r );

    r = MsiQueryExecute(hquery, 0);
    ok( r == ERROR_SUCCESS, "failed to execute query: %d\n", r );

    i = 0;
    data_correct = true;
    while ((r = MsiQueryFetch(hquery, &hrec)) == ERROR_SUCCESS)
    {
        count = MsiRecordGetFieldCount( hrec );
        ok( count == 2, "Expected 2 record fields, got %d\n", count );

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 1, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( strcmp( buf, join_res_fifth[i].one ))
            data_correct = false;

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 2, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( strcmp( buf, join_res_fifth[i].two ))
            data_correct = false;

        i++;
        MsiCloseHandle(hrec);
    }
    ok( data_correct, "data returned in the wrong order\n");

    ok( i == 1, "Expected 1 rows, got %d\n", i );
    ok( r == ERROR_NO_MORE_ITEMS, "expected no more items: %d\n", r );

    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    sql = "SELECT `Component`.`ComponentId`, `FeatureComponents`.`Feature_` "
            "FROM `Component`, `FeatureComponents` "
            "WHERE `Component` = 'zygomatic' "
            "ORDER BY `Feature_`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok( r == ERROR_SUCCESS, "failed to open query: %d\n", r );

    r = MsiQueryExecute(hquery, 0);
    ok( r == ERROR_SUCCESS, "failed to execute query: %d\n", r );

    i = 0;
    data_correct = true;
    while ((r = MsiQueryFetch(hquery, &hrec)) == ERROR_SUCCESS)
    {
        count = MsiRecordGetFieldCount( hrec );
        ok( count == 2, "Expected 2 record fields, got %d\n", count );

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 1, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( strcmp( buf, join_res_sixth[i].one ))
            data_correct = false;

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 2, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( strcmp( buf, join_res_sixth[i].two ))
            data_correct = false;

        i++;
        MsiCloseHandle(hrec);
    }
    ok( data_correct, "data returned in the wrong order\n");

    ok( i == 6, "Expected 6 rows, got %d\n", i );
    ok( r == ERROR_NO_MORE_ITEMS, "expected no more items: %d\n", r );

    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    sql = "SELECT `Component`.`ComponentId`, `FeatureComponents`.`Feature_` "
            "FROM `Component`, `FeatureComponents` "
            "WHERE `Component` = 'zygomatic' "
            "AND `Feature_` = 'nasalis' "
            "ORDER BY `Feature_`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok( r == ERROR_SUCCESS, "failed to open query: %d\n", r );

    r = MsiQueryExecute(hquery, 0);
    ok( r == ERROR_SUCCESS, "failed to execute query: %d\n", r );

    i = 0;
    data_correct = true;
    while ((r = MsiQueryFetch(hquery, &hrec)) == ERROR_SUCCESS)
    {
        count = MsiRecordGetFieldCount( hrec );
        ok( count == 2, "Expected 2 record fields, got %d\n", count );

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 1, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( strcmp( buf, join_res_seventh[i].one ))
            data_correct = false;

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 2, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( strcmp( buf, join_res_seventh[i].two ))
            data_correct = false;

        i++;
        MsiCloseHandle(hrec);
    }

    ok( data_correct, "data returned in the wrong order\n");
    ok( i == 3, "Expected 3 rows, got %d\n", i );
    ok( r == ERROR_NO_MORE_ITEMS, "expected no more items: %d\n", r );

    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    sql = "SELECT `StdDlls`.`File`, `Binary`.`Data` "
            "FROM `StdDlls`, `Binary` ";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok( r == ERROR_SUCCESS, "failed to open query: %d\n", r );

    r = MsiQueryExecute(hquery, 0);
    ok( r == ERROR_SUCCESS, "failed to execute query: %d\n", r );

    i = 0;
    data_correct = true;
    while ((r = MsiQueryFetch(hquery, &hrec)) == ERROR_SUCCESS)
    {
        count = MsiRecordGetFieldCount( hrec );
        ok( count == 2, "Expected 2 record fields, got %d\n", count );

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 1, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( strcmp( buf, join_res_eighth[i].one ))
            data_correct = false;

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 2, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( strcmp( buf, join_res_eighth[i].four ))
            data_correct = false;

        i++;
        MsiCloseHandle(hrec);
    }

    ok( data_correct, "data returned in the wrong order\n");
    ok( i == 6, "Expected 6 rows, got %d\n", i );
    ok( r == ERROR_NO_MORE_ITEMS, "expected no more items: %d\n", r );

    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    sql = "SELECT * FROM `StdDlls`, `Binary` ";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok( r == ERROR_SUCCESS, "failed to open query: %d\n", r );

    r = MsiQueryExecute(hquery, 0);
    ok( r == ERROR_SUCCESS, "failed to execute query: %d\n", r );

    i = 0;
    data_correct = true;
    while ((r = MsiQueryFetch(hquery, &hrec)) == ERROR_SUCCESS)
    {
        count = MsiRecordGetFieldCount( hrec );
        ok( count == 4, "Expected 4 record fields, got %d\n", count );

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 1, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( strcmp( buf, join_res_eighth[i].one ))
            data_correct = false;

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 2, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( strcmp( buf, join_res_eighth[i].two ))
            data_correct = false;

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 3, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( strcmp( buf, join_res_eighth[i].three ))
            data_correct = false;

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 4, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( strcmp( buf, join_res_eighth[i].four ))
            data_correct = false;

        i++;
        MsiCloseHandle(hrec);
    }
    ok( data_correct, "data returned in the wrong order\n");

    ok( i == 6, "Expected 6 rows, got %d\n", i );
    ok( r == ERROR_NO_MORE_ITEMS, "expected no more items: %d\n", r );

    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    sql = "SELECT * FROM `One`, `Two`, `Three` ";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok( r == ERROR_SUCCESS, "failed to open query: %d\n", r );

    r = MsiQueryExecute(hquery, 0);
    ok( r == ERROR_SUCCESS, "failed to execute query: %d\n", r );

    i = 0;
    data_correct = true;
    while ((r = MsiQueryFetch(hquery, &hrec)) == ERROR_SUCCESS)
    {
        count = MsiRecordGetFieldCount( hrec );
        ok( count == 6, "Expected 6 record fields, got %d\n", count );

        r = MsiRecordGetInteger( hrec, 1 );
        if( r != join_res_ninth[i].one )
            data_correct = false;

        r = MsiRecordGetInteger( hrec, 2 );
        if( r != join_res_ninth[i].two )
            data_correct = false;

        r = MsiRecordGetInteger( hrec, 3 );
        if( r != join_res_ninth[i].three )
            data_correct = false;

        r = MsiRecordGetInteger( hrec, 4 );
        if( r != join_res_ninth[i].four )
            data_correct = false;

        r = MsiRecordGetInteger( hrec, 5 );
        if( r != join_res_ninth[i].five )
            data_correct = false;

        r = MsiRecordGetInteger( hrec, 6);
        if( r != join_res_ninth[i].six )
            data_correct = false;

        i++;
        MsiCloseHandle(hrec);
    }
    ok( data_correct, "data returned in the wrong order\n");

    ok( i == 6, "Expected 6 rows, got %d\n", i );
    ok( r == ERROR_NO_MORE_ITEMS, "expected no more items: %d\n", r );

    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    sql = "SELECT * FROM `Four`, `Five`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok( r == ERROR_SUCCESS, "failed to open query: %d\n", r );

    r = MsiQueryExecute(hquery, 0);
    ok( r == ERROR_SUCCESS, "failed to execute query: %d\n", r );

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    sql = "SELECT * FROM `Nonexistent`, `One`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok( r == ERROR_BAD_QUERY_SYNTAX,
        "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r );

    /* try updating a row in a join table */
    sql = "SELECT `Component`.`ComponentId`, `FeatureComponents`.`Feature_` "
            "FROM `Component`, `FeatureComponents` "
            "WHERE `Component`.`Component` = `FeatureComponents`.`Component_` "
            "ORDER BY `Feature_`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok( r == ERROR_SUCCESS, "failed to open query: %d\n", r );

    r = MsiQueryExecute(hquery, 0);
    ok( r == ERROR_SUCCESS, "failed to execute query: %d\n", r );

    r = MsiQueryFetch(hquery, &hrec);
    ok( r == ERROR_SUCCESS, "failed to fetch query: %d\n", r );

    r = MsiRecordSetString( hrec, 1, "epicranius" );
    ok( r == ERROR_SUCCESS, "failed to set string: %d\n", r );

    r = MsiQueryModify(hquery, LIBMSI_MODIFY_UPDATE, hrec);
    ok( r == ERROR_SUCCESS, "failed to update row: %d\n", r );

    /* try another valid operation for joins */
    r = MsiQueryModify(hquery, LIBMSI_MODIFY_REFRESH, hrec);
    todo_wine ok( r == ERROR_SUCCESS, "failed to refresh row: %d\n", r );

    /* try an invalid operation for joins */
    r = MsiQueryModify(hquery, LIBMSI_MODIFY_DELETE, hrec);
    ok( r == ERROR_FUNCTION_FAILED, "unexpected result: %d\n", r );

    r = MsiRecordSetString( hrec, 2, "epicranius" );
    ok( r == ERROR_SUCCESS, "failed to set string: %d\n", r );

    /* primary key cannot be updated */
    r = MsiQueryModify(hquery, LIBMSI_MODIFY_UPDATE, hrec);
    ok( r == ERROR_FUNCTION_FAILED, "failed to update row: %d\n", r );

    MsiCloseHandle(hrec);
    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenQuery failed\n");

    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed\n");

    size = MAX_PATH;
    r = MsiRecordGetString( hrec, 1, buf, &size );
    ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
    ok( !strcmp( buf, "epicranius" ), "expected 'epicranius', got %s\n", buf );

    MsiCloseHandle(hrec);
    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    MsiCloseHandle(hdb);
    DeleteFile(msifile);
}

static void test_temporary_table(void)
{
    LibmsiCondition cond;
    LibmsiObject *hdb = 0;
    LibmsiObject *query = 0;
    LibmsiObject *rec;
    const char *sql;
    unsigned r;
    char buf[0x10];
    unsigned sz;

    cond = MsiDatabaseIsTablePersistent(0, NULL);
    ok( cond == LIBMSI_CONDITION_ERROR, "wrong return condition\n");

    hdb = create_db();
    ok( hdb, "failed to create db\n");

    cond = MsiDatabaseIsTablePersistent(hdb, NULL);
    ok( cond == LIBMSI_CONDITION_ERROR, "wrong return condition\n");

    cond = MsiDatabaseIsTablePersistent(hdb, "_Tables");
    ok( cond == LIBMSI_CONDITION_NONE, "wrong return condition\n");

    cond = MsiDatabaseIsTablePersistent(hdb, "_Columns");
    ok( cond == LIBMSI_CONDITION_NONE, "wrong return condition\n");

    cond = MsiDatabaseIsTablePersistent(hdb, "_Storages");
    ok( cond == LIBMSI_CONDITION_NONE, "wrong return condition\n");

    cond = MsiDatabaseIsTablePersistent(hdb, "_Streams");
    ok( cond == LIBMSI_CONDITION_NONE, "wrong return condition\n");

    sql = "CREATE TABLE `P` ( `B` SHORT NOT NULL, `C` CHAR(255) PRIMARY KEY `C`)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "failed to add table\n");

    cond = MsiDatabaseIsTablePersistent(hdb, "P");
    ok( cond == LIBMSI_CONDITION_TRUE, "wrong return condition\n");

    sql = "CREATE TABLE `P2` ( `B` SHORT NOT NULL, `C` CHAR(255) PRIMARY KEY `C`) HOLD";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "failed to add table\n");

    cond = MsiDatabaseIsTablePersistent(hdb, "P2");
    ok( cond == LIBMSI_CONDITION_TRUE, "wrong return condition\n");

    sql = "CREATE TABLE `T` ( `B` SHORT NOT NULL TEMPORARY, `C` CHAR(255) TEMPORARY PRIMARY KEY `C`) HOLD";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "failed to add table\n");

    cond = MsiDatabaseIsTablePersistent(hdb, "T");
    ok( cond == LIBMSI_CONDITION_FALSE, "wrong return condition\n");

    sql = "CREATE TABLE `T2` ( `B` SHORT NOT NULL TEMPORARY, `C` CHAR(255) TEMPORARY PRIMARY KEY `C`)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "failed to add table\n");

    query = NULL;
    sql = "SELECT * FROM `T2`";
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    cond = MsiDatabaseIsTablePersistent(hdb, "T2");
    ok( cond == LIBMSI_CONDITION_NONE, "wrong return condition\n");

    sql = "CREATE TABLE `T3` ( `B` SHORT NOT NULL TEMPORARY, `C` CHAR(255) PRIMARY KEY `C`)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "failed to add table\n");

    cond = MsiDatabaseIsTablePersistent(hdb, "T3");
    ok( cond == LIBMSI_CONDITION_TRUE, "wrong return condition\n");

    sql = "CREATE TABLE `T4` ( `B` SHORT NOT NULL, `C` CHAR(255) TEMPORARY PRIMARY KEY `C`)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_FUNCTION_FAILED, "failed to add table\n");

    cond = MsiDatabaseIsTablePersistent(hdb, "T4");
    ok( cond == LIBMSI_CONDITION_NONE, "wrong return condition\n");

    sql = "CREATE TABLE `T5` ( `B` SHORT NOT NULL TEMP, `C` CHAR(255) TEMP PRIMARY KEY `C`) HOLD";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "failed to add table\n");

    query = NULL;
    sql = "select * from `T`";
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok(r == ERROR_SUCCESS, "failed to query table\n");

    rec = NULL;
    r = MsiQueryGetColumnInfo(query, LIBMSI_COL_INFO_TYPES, &rec);
    ok(r == ERROR_SUCCESS, "failed to get column info\n");

    sz = sizeof buf;
    r = MsiRecordGetString(rec, 1, buf, &sz);
    ok(r == ERROR_SUCCESS, "failed to get string\n");
    ok( 0 == strcmp("G255", buf), "wrong column type\n");

    sz = sizeof buf;
    r = MsiRecordGetString(rec, 2, buf, &sz);
    ok(r == ERROR_SUCCESS, "failed to get string\n");
    ok( 0 == strcmp("j2", buf), "wrong column type\n");

    MsiCloseHandle( rec );
    MsiQueryClose( query );
    MsiCloseHandle( query );

    /* query the table data */
    rec = 0;
    r = do_query(hdb, "select * from `_Tables` where `Name` = 'T'", &rec);
    ok( r == ERROR_SUCCESS, "temporary table exists in _Tables\n");
    MsiCloseHandle( rec );

    /* query the column data */
    rec = 0;
    r = do_query(hdb, "select * from `_Columns` where `Table` = 'T' AND `Name` = 'B'", &rec);
    ok( r == ERROR_NO_MORE_ITEMS, "temporary table exists in _Columns\n");
    if (rec) MsiCloseHandle( rec );

    r = do_query(hdb, "select * from `_Columns` where `Table` = 'T' AND `Name` = 'C'", &rec);
    ok( r == ERROR_NO_MORE_ITEMS, "temporary table exists in _Columns\n");
    if (rec) MsiCloseHandle( rec );

    MsiCloseHandle( hdb );

    DeleteFile(msifile);
}

static void test_alter(void)
{
    LibmsiCondition cond;
    LibmsiObject *hdb = 0;
    const char *sql;
    unsigned r;

    hdb = create_db();
    ok( hdb, "failed to create db\n");

    sql = "CREATE TABLE `T` ( `B` SHORT NOT NULL TEMPORARY, `C` CHAR(255) TEMPORARY PRIMARY KEY `C`) HOLD";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "failed to add table\n");

    cond = MsiDatabaseIsTablePersistent(hdb, "T");
    ok( cond == LIBMSI_CONDITION_FALSE, "wrong return condition\n");

    sql = "ALTER TABLE `T` HOLD";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "failed to hold table %d\n", r);

    sql = "ALTER TABLE `T` FREE";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "failed to free table\n");

    sql = "ALTER TABLE `T` FREE";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "failed to free table\n");

    sql = "ALTER TABLE `T` FREE";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "failed to free table\n");

    sql = "ALTER TABLE `T` HOLD";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "failed to hold table %d\n", r);

    /* table T is removed */
    sql = "SELECT * FROM `T`";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    /* create the table again */
    sql = "CREATE TABLE `U` ( `A` INTEGER, `B` INTEGER PRIMARY KEY `B`)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* up the ref count */
    sql = "ALTER TABLE `U` HOLD";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "failed to free table\n");

    /* add column, no data type */
    sql = "ALTER TABLE `U` ADD `C`";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "ALTER TABLE `U` ADD `C` INTEGER";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* add column C again */
    sql = "ALTER TABLE `U` ADD `C` INTEGER";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "ALTER TABLE `U` ADD `D` INTEGER TEMPORARY";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `U` ( `A`, `B`, `C`, `D` ) VALUES ( 1, 2, 3, 4 )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "ALTER TABLE `U` ADD `D` INTEGER TEMPORARY HOLD";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "INSERT INTO `U` ( `A`, `B`, `C`, `D` ) VALUES ( 5, 6, 7, 8 )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "SELECT * FROM `U` WHERE `D` = 8";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "ALTER TABLE `U` ADD `D` INTEGER TEMPORARY FREE";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "ALTER COLUMN `D` FREE";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    /* drop the ref count */
    sql = "ALTER TABLE `U` FREE";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* table is not empty */
    sql = "SELECT * FROM `U`";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* column D is removed */
    sql = "SELECT * FROM `U` WHERE `D` = 8";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "INSERT INTO `U` ( `A`, `B`, `C`, `D` ) VALUES ( 9, 10, 11, 12 )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    /* add the column again */
    sql = "ALTER TABLE `U` ADD `E` INTEGER TEMPORARY HOLD";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* up the ref count */
    sql = "ALTER TABLE `U` HOLD";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `U` ( `A`, `B`, `C`, `E` ) VALUES ( 13, 14, 15, 16 )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "SELECT * FROM `U` WHERE `E` = 16";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* drop the ref count */
    sql = "ALTER TABLE `U` FREE";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `U` ( `A`, `B`, `C`, `E` ) VALUES ( 17, 18, 19, 20 )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "SELECT * FROM `U` WHERE `E` = 20";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* drop the ref count */
    sql = "ALTER TABLE `U` FREE";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* table still exists */
    sql = "SELECT * FROM `U`";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* col E is removed */
    sql = "SELECT * FROM `U` WHERE `E` = 20";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "INSERT INTO `U` ( `A`, `B`, `C`, `E` ) VALUES ( 20, 21, 22, 23 )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    /* drop the ref count once more */
    sql = "ALTER TABLE `U` FREE";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* table still exists */
    sql = "SELECT * FROM `U`";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle( hdb );
    DeleteFile(msifile);
}

static void test_integers(void)
{
    LibmsiObject *hdb = 0;
    LibmsiObject *query = 0;
    LibmsiObject *rec = 0;
    unsigned count, i;
    const char *sql;
    unsigned r;

    /* just MsiOpenDatabase should not create a file */
    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    /* create a table */
    sql = "CREATE TABLE `integers` ( "
            "`one` SHORT, `two` INT, `three` INTEGER, `four` LONG, "
            "`five` SHORT NOT NULL, `six` INT NOT NULL, "
            "`seven` INTEGER NOT NULL, `eight` LONG NOT NULL "
            "PRIMARY KEY `one`)";
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenQuery failed\n");
    r = MsiQueryExecute(query, 0);
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");
    r = MsiQueryClose(query);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(query);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    sql = "SELECT * FROM `integers`";
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    rec = NULL;
    r = MsiQueryGetColumnInfo(query, LIBMSI_COL_INFO_NAMES, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    count = MsiRecordGetFieldCount(rec);
    ok(count == 8, "Expected 8, got %d\n", count);
    ok(check_record(rec, 1, "one"), "Expected one\n");
    ok(check_record(rec, 2, "two"), "Expected two\n");
    ok(check_record(rec, 3, "three"), "Expected three\n");
    ok(check_record(rec, 4, "four"), "Expected four\n");
    ok(check_record(rec, 5, "five"), "Expected five\n");
    ok(check_record(rec, 6, "six"), "Expected six\n");
    ok(check_record(rec, 7, "seven"), "Expected seven\n");
    ok(check_record(rec, 8, "eight"), "Expected eight\n");
    MsiCloseHandle(rec);

    rec = NULL;
    r = MsiQueryGetColumnInfo(query, LIBMSI_COL_INFO_TYPES, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    count = MsiRecordGetFieldCount(rec);
    ok(count == 8, "Expected 8, got %d\n", count);
    ok(check_record(rec, 1, "I2"), "Expected I2\n");
    ok(check_record(rec, 2, "I2"), "Expected I2\n");
    ok(check_record(rec, 3, "I2"), "Expected I2\n");
    ok(check_record(rec, 4, "I4"), "Expected I4\n");
    ok(check_record(rec, 5, "i2"), "Expected i2\n");
    ok(check_record(rec, 6, "i2"), "Expected i2\n");
    ok(check_record(rec, 7, "i2"), "Expected i2\n");
    ok(check_record(rec, 8, "i4"), "Expected i4\n");
    MsiCloseHandle(rec);

    MsiQueryClose(query);
    MsiCloseHandle(query);

    /* insert values into it, NULL where NOT NULL is specified */
    query = NULL;
    sql = "INSERT INTO `integers` ( `one`, `two`, `three`, `four`, `five`, `six`, `seven`, `eight` )"
        "VALUES('', '', '', '', '', '', '', '')";
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(query, 0);
    ok(r == ERROR_FUNCTION_FAILED, "Expected ERROR_FUNCTION_FAILED, got %d\n", r);

    MsiQueryClose(query);
    MsiCloseHandle(query);

    sql = "SELECT * FROM `integers`";
    r = do_query(hdb, sql, &rec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    r = MsiRecordGetFieldCount(rec);
    ok(r == -1, "record count wrong: %d\n", r);

    MsiCloseHandle(rec);

    /* insert legitimate values into it */
    query = NULL;
    sql = "INSERT INTO `integers` ( `one`, `two`, `three`, `four`, `five`, `six`, `seven`, `eight` )"
        "VALUES('', '2', '', '4', '5', '6', '7', '8')";
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(query, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "SELECT * FROM `integers`";
    r = do_query(hdb, sql, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetFieldCount(rec);
    ok(r == 8, "record count wrong: %d\n", r);

    i = MsiRecordGetInteger(rec, 1);
    ok(i == MSI_NULL_INTEGER, "Expected MSI_NULL_INTEGER, got %d\n", i);
    i = MsiRecordGetInteger(rec, 3);
    ok(i == MSI_NULL_INTEGER, "Expected MSI_NULL_INTEGER, got %d\n", i);
    i = MsiRecordGetInteger(rec, 2);
    ok(i == 2, "Expected 2, got %d\n", i);
    i = MsiRecordGetInteger(rec, 4);
    ok(i == 4, "Expected 4, got %d\n", i);
    i = MsiRecordGetInteger(rec, 5);
    ok(i == 5, "Expected 5, got %d\n", i);
    i = MsiRecordGetInteger(rec, 6);
    ok(i == 6, "Expected 6, got %d\n", i);
    i = MsiRecordGetInteger(rec, 7);
    ok(i == 7, "Expected 7, got %d\n", i);
    i = MsiRecordGetInteger(rec, 8);
    ok(i == 8, "Expected 8, got %d\n", i);

    MsiCloseHandle(rec);
    MsiQueryClose(query);
    MsiCloseHandle(query);

    r = MsiDatabaseCommit(hdb);
    ok(r == ERROR_SUCCESS, "MsiDatabaseCommit failed\n");

    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = DeleteFile(msifile);
    ok(r == true, "file didn't exist after commit\n");
}

static void test_update(void)
{
    LibmsiObject *hdb = 0;
    LibmsiObject *query = 0;
    LibmsiObject *rec = 0;
    char result[MAX_PATH];
    const char *sql;
    unsigned size;
    unsigned r;

    /* just MsiOpenDatabase should not create a file */
    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    /* create the Control table */
    sql = "CREATE TABLE `Control` ( "
        "`Dialog_` CHAR(72) NOT NULL, `Control` CHAR(50) NOT NULL, `Type` SHORT NOT NULL, "
        "`X` SHORT NOT NULL, `Y` SHORT NOT NULL, `Width` SHORT NOT NULL, `Height` SHORT NOT NULL,"
        "`Attributes` LONG, `Property` CHAR(50), `Text` CHAR(0) LOCALIZABLE, "
        "`Control_Next` CHAR(50), `Help` CHAR(50) LOCALIZABLE PRIMARY KEY `Dialog_`, `Control`)";
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenQuery failed\n");
    r = MsiQueryExecute(query, 0);
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");
    r = MsiQueryClose(query);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(query);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* add a control */
    query = NULL;
    sql = "INSERT INTO `Control` ( "
        "`Dialog_`, `Control`, `Type`, `X`, `Y`, `Width`, `Height`, "
        "`Property`, `Text`, `Control_Next`, `Help` )"
        "VALUES('ErrorDialog', 'ErrorText', '1', '5', '5', '5', '5', '', '', '', '')";
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(query, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryClose(query);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(query);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* add a second control */
    query = NULL;
    sql = "INSERT INTO `Control` ( "
        "`Dialog_`, `Control`, `Type`, `X`, `Y`, `Width`, `Height`, "
        "`Property`, `Text`, `Control_Next`, `Help` )"
        "VALUES('ErrorDialog', 'Button', '1', '5', '5', '5', '5', '', '', '', '')";
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(query, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryClose(query);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(query);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* add a third control */
    query = NULL;
    sql = "INSERT INTO `Control` ( "
        "`Dialog_`, `Control`, `Type`, `X`, `Y`, `Width`, `Height`, "
        "`Property`, `Text`, `Control_Next`, `Help` )"
        "VALUES('AnotherDialog', 'ErrorText', '1', '5', '5', '5', '5', '', '', '', '')";
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(query, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryClose(query);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(query);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* bad table */
    query = NULL;
    sql = "UPDATE `NotATable` SET `Text` = 'this is text' WHERE `Dialog_` = 'ErrorDialog'";
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    /* bad set column */
    query = NULL;
    sql = "UPDATE `Control` SET `NotAColumn` = 'this is text' WHERE `Dialog_` = 'ErrorDialog'";
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    /* bad where condition */
    query = NULL;
    sql = "UPDATE `Control` SET `Text` = 'this is text' WHERE `NotAColumn` = 'ErrorDialog'";
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    /* just the dialog_ specified */
    query = NULL;
    sql = "UPDATE `Control` SET `Text` = 'this is text' WHERE `Dialog_` = 'ErrorDialog'";
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(query, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryClose(query);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(query);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* check the modified text */
    query = NULL;
    sql = "SELECT `Text` FROM `Control` WHERE `Control` = 'ErrorText'";
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(query, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(rec, 1, result, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(result, "this is text"), "Expected `this is text`, got %s\n", result);

    MsiCloseHandle(rec);

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(rec, 1, result, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strlen(result), "Expected an empty string, got %s\n", result);

    MsiCloseHandle(rec);

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    r = MsiQueryClose(query);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(query);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* dialog_ and control specified */
    query = NULL;
    sql = "UPDATE `Control` SET `Text` = 'this is text' WHERE `Dialog_` = 'ErrorDialog' AND `Control` = 'ErrorText'";
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(query, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryClose(query);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(query);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* check the modified text */
    query = NULL;
    sql = "SELECT `Text` FROM `Control` WHERE `Control` = 'ErrorText'";
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(query, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(rec, 1, result, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(result, "this is text"), "Expected `this is text`, got %s\n", result);

    MsiCloseHandle(rec);

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(rec, 1, result, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strlen(result), "Expected an empty string, got %s\n", result);

    MsiCloseHandle(rec);

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    r = MsiQueryClose(query);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(query);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* no where condition */
    query = NULL;
    sql = "UPDATE `Control` SET `Text` = 'this is text'";
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(query, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryClose(query);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(query);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* check the modified text */
    query = NULL;
    sql = "SELECT `Text` FROM `Control`";
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(query, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(rec, 1, result, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(result, "this is text"), "Expected `this is text`, got %s\n", result);

    MsiCloseHandle(rec);

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(rec, 1, result, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(result, "this is text"), "Expected `this is text`, got %s\n", result);

    MsiCloseHandle(rec);

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(rec, 1, result, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(result, "this is text"), "Expected `this is text`, got %s\n", result);

    MsiCloseHandle(rec);

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    r = MsiQueryClose(query);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(query);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    sql = "CREATE TABLE `Apple` ( `Banana` CHAR(72) NOT NULL, "
        "`Orange` CHAR(72),  `Pear` INT PRIMARY KEY `Banana`)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `Apple` ( `Banana`, `Orange`, `Pear` )"
        "VALUES('one', 'two', 3)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `Apple` ( `Banana`, `Orange`, `Pear` )"
        "VALUES('three', 'four', 5)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `Apple` ( `Banana`, `Orange`, `Pear` )"
        "VALUES('six', 'two', 7)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    rec = MsiCreateRecord(2);
    MsiRecordSetInteger(rec, 1, 8);
    MsiRecordSetString(rec, 2, "two");

    sql = "UPDATE `Apple` SET `Pear` = ? WHERE `Orange` = ?";
    r = run_query(hdb, rec, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(rec);

    query = NULL;
    sql = "SELECT `Pear` FROM `Apple` ORDER BY `Orange`";
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(query, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetInteger(rec, 1);
    ok(r == 8, "Expected 8, got %d\n", r);

    MsiCloseHandle(rec);

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetInteger(rec, 1);
    ok(r == 8, "Expected 8, got %d\n", r);

    MsiCloseHandle(rec);

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetInteger(rec, 1);
    ok(r == 5, "Expected 5, got %d\n", r);

    MsiCloseHandle(rec);

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expectd ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiQueryClose(query);
    MsiCloseHandle(query);

    r = MsiDatabaseCommit(hdb);
    ok(r == ERROR_SUCCESS, "MsiDatabaseCommit failed\n");
    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    DeleteFile(msifile);
}

static void test_special_tables(void)
{
    const char *sql;
    LibmsiObject *hdb = 0;
    unsigned r;

    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    sql = "CREATE TABLE `_Properties` ( "
        "`foo` INT NOT NULL, `bar` INT LOCALIZABLE PRIMARY KEY `foo`)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "failed to create table\n");

    sql = "CREATE TABLE `_Storages` ( "
        "`foo` INT NOT NULL, `bar` INT LOCALIZABLE PRIMARY KEY `foo`)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "created _Streams table\n");

    sql = "CREATE TABLE `_Streams` ( "
        "`foo` INT NOT NULL, `bar` INT LOCALIZABLE PRIMARY KEY `foo`)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "created _Streams table\n");

    sql = "CREATE TABLE `_Tables` ( "
        "`foo` INT NOT NULL, `bar` INT LOCALIZABLE PRIMARY KEY `foo`)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "created _Tables table\n");

    sql = "CREATE TABLE `_Columns` ( "
        "`foo` INT NOT NULL, `bar` INT LOCALIZABLE PRIMARY KEY `foo`)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "created _Columns table\n");

    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");
}

static void test_tables_order(void)
{
    const char *sql;
    LibmsiObject *hdb = 0;
    LibmsiObject *hquery = 0;
    LibmsiObject *hrec = 0;
    unsigned r;
    char buffer[100];
    unsigned sz;

    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    sql = "CREATE TABLE `foo` ( "
        "`baz` INT NOT NULL PRIMARY KEY `baz`)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "failed to create table\n");

    sql = "CREATE TABLE `bar` ( "
        "`foo` INT NOT NULL PRIMARY KEY `foo`)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "failed to create table\n");

    sql = "CREATE TABLE `baz` ( "
        "`bar` INT NOT NULL, "
        "`baz` INT NOT NULL, "
        "`foo` INT NOT NULL PRIMARY KEY `bar`)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "failed to create table\n");

    /* The names of the tables in the _Tables table must
       be in the same order as these names are created in
       the strings table. */
    sql = "SELECT * FROM `_Tables`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenQuery failed\n");
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed\n");
    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 1, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "foo"), "Expected foo, got %s\n", buffer);
    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed\n");
    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 1, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "baz"), "Expected baz, got %s\n", buffer);
    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed\n");
    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 1, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "bar"), "Expected bar, got %s\n", buffer);
    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiQueryClose(hquery);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(hquery);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* The names of the tables in the _Columns table must
       be in the same order as these names are created in
       the strings table. */
    sql = "SELECT * FROM `_Columns`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenQuery failed\n");
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed\n");
    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 1, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "foo"), "Expected foo, got %s\n", buffer);
    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 3, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "baz"), "Expected baz, got %s\n", buffer);
    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed\n");
    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 1, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "baz"), "Expected baz, got %s\n", buffer);
    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 3, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "bar"), "Expected bar, got %s\n", buffer);
    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed\n");
    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 1, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "baz"), "Expected baz, got %s\n", buffer);
    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 3, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "baz"), "Expected baz, got %s\n", buffer);
    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed\n");
    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 1, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "baz"), "Expected baz, got %s\n", buffer);
    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 3, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "foo"), "Expected foo, got %s\n", buffer);
    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed\n");
    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 1, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "bar"), "Expected bar, got %s\n", buffer);
    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 3, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "foo"), "Expected foo, got %s\n", buffer);
    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiQueryClose(hquery);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(hquery);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    DeleteFile(msifile);
}

static void test_rows_order(void)
{
    const char *sql;
    LibmsiObject *hdb = 0;
    LibmsiObject *hquery = 0;
    LibmsiObject *hrec = 0;
    unsigned r;
    char buffer[100];
    unsigned sz;

    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    sql = "CREATE TABLE `foo` ( "
        "`bar` LONGCHAR NOT NULL PRIMARY KEY `bar`)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "failed to create table\n");

    r = run_query(hdb, 0, "INSERT INTO `foo` "
            "( `bar` ) VALUES ( 'A' )");
    ok(r == ERROR_SUCCESS, "cannot add value to table\n");

    r = run_query(hdb, 0, "INSERT INTO `foo` "
            "( `bar` ) VALUES ( 'B' )");
    ok(r == ERROR_SUCCESS, "cannot add value to table\n");

    r = run_query(hdb, 0, "INSERT INTO `foo` "
            "( `bar` ) VALUES ( 'C' )");
    ok(r == ERROR_SUCCESS, "cannot add value to table\n");

    r = run_query(hdb, 0, "INSERT INTO `foo` "
            "( `bar` ) VALUES ( 'D' )");
    ok(r == ERROR_SUCCESS, "cannot add value to table\n");

    r = run_query(hdb, 0, "INSERT INTO `foo` "
            "( `bar` ) VALUES ( 'E' )");
    ok(r == ERROR_SUCCESS, "cannot add value to table\n");

    r = run_query(hdb, 0, "INSERT INTO `foo` "
            "( `bar` ) VALUES ( 'F' )");
    ok(r == ERROR_SUCCESS, "cannot add value to table\n");

    sql = "CREATE TABLE `bar` ( "
        "`foo` LONGCHAR NOT NULL, "
        "`baz` LONGCHAR NOT NULL "
        "PRIMARY KEY `foo` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "failed to create table\n");

    r = run_query(hdb, 0, "INSERT INTO `bar` "
            "( `foo`, `baz` ) VALUES ( 'C', 'E' )");
    ok(r == ERROR_SUCCESS, "cannot add value to table\n");

    r = run_query(hdb, 0, "INSERT INTO `bar` "
            "( `foo`, `baz` ) VALUES ( 'F', 'A' )");
    ok(r == ERROR_SUCCESS, "cannot add value to table\n");

    r = run_query(hdb, 0, "INSERT INTO `bar` "
            "( `foo`, `baz` ) VALUES ( 'A', 'B' )");
    ok(r == ERROR_SUCCESS, "cannot add value to table\n");

    r = run_query(hdb, 0, "INSERT INTO `bar` "
            "( `foo`, `baz` ) VALUES ( 'D', 'E' )");
    ok(r == ERROR_SUCCESS, "cannot add value to table\n");

    /* The rows of the table must be ordered by the column values of
       each row. For strings, the column value is the string id
       in the string table.  */

    sql = "SELECT * FROM `bar`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenQuery failed\n");
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed\n");
    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 1, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "A"), "Expected A, got %s\n", buffer);
    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 2, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "B"), "Expected B, got %s\n", buffer);
    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed\n");
    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 1, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "C"), "Expected E, got %s\n", buffer);
    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 2, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "E"), "Expected E, got %s\n", buffer);
    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed\n");
    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 1, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "D"), "Expected D, got %s\n", buffer);
    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 2, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "E"), "Expected E, got %s\n", buffer);
    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed\n");
    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 1, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "F"), "Expected F, got %s\n", buffer);
    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 2, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "A"), "Expected A, got %s\n", buffer);
    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiQueryClose(hquery);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(hquery);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    DeleteFile(msifile);
}

static void test_collation(void)
{
    static const char sql1[] =
	    "INSERT INTO `bar` (`foo`, `baz`) VALUES ('a\xcc\x8a', 'C')";
    static const char sql2[] =
	    "INSERT INTO `bar` (`foo`, `baz`) VALUES ('\xc3\xa5', 'D')";
    static const char sql3[] =
	    "CREATE TABLE `baz` (`a\xcc\x8a` LONGCHAR NOT NULL, `\xc3\xa5` LONGCHAR NOT NULL PRIMARY KEY `a\xcc\x8a`)";
    static const char sql4[] =
	    "CREATE TABLE `a\xcc\x8a` (`foo` LONGCHAR NOT NULL, `\xc3\xa5` LONGCHAR NOT NULL PRIMARY KEY `foo`)";
    static const char sql5[] =
	    "CREATE TABLE `\xc3\xa5` (`foo` LONGCHAR NOT NULL, `\xc3\xa5` LONGCHAR NOT NULL PRIMARY KEY `foo`)";
    static const char sql6[] =
	    "SELECT * FROM `bar` WHERE `foo` = '\xc3\xa5'";
    static const char letter_a_ring[] = "a\xcc\x8a";
    static const char letter_a_with_ring[] = "\xc3\xa5";
    const char *sql;
    LibmsiObject *hdb = 0;
    LibmsiObject *hquery = 0;
    LibmsiObject *hrec = 0;
    unsigned r;
    char buffer[100];
    unsigned sz;

    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    sql = "CREATE TABLE `bar` ( "
        "`foo` LONGCHAR NOT NULL, "
        "`baz` LONGCHAR NOT NULL "
        "PRIMARY KEY `foo` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "failed to create table\n");

    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "wrong error %u\n", r);

    r = run_query(hdb, 0, "INSERT INTO `bar` "
            "( `foo`, `baz` ) VALUES ( '\2', 'A' )");
    ok(r == ERROR_SUCCESS, "cannot add value to table %u\n", r);

    r = run_query(hdb, 0, "INSERT INTO `bar` "
            "( `foo`, `baz` ) VALUES ( '\1', 'B' )");
    ok(r == ERROR_SUCCESS, "cannot add value to table %u\n", r);

    r = run_query(hdb, 0, sql1);
    ok(r == ERROR_SUCCESS, "cannot add value to table %u\n", r);

    r = run_query(hdb, 0, sql2);
    ok(r == ERROR_SUCCESS, "cannot add value to table %u\n", r);

    r = run_query(hdb, 0, sql3);
    ok(r == ERROR_SUCCESS, "cannot create table %u\n", r);

    r = run_query(hdb, 0, sql4);
    ok(r == ERROR_SUCCESS, "cannot create table %u\n", r);

    r = run_query(hdb, 0, sql5);
    ok(r == ERROR_SUCCESS, "cannot create table %u\n", r);

    sql = "SELECT * FROM `bar`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenQuery failed\n");
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed\n");
    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 1, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "\2"), "Expected \\2, got '%s'\n", buffer);
    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 2, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "A"), "Expected A, got '%s'\n", buffer);
    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed\n");
    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 1, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "\1"), "Expected \\1, got '%s'\n", buffer);
    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 2, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "B"), "Expected B, got '%s'\n", buffer);
    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed\n");
    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 1, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!memcmp(buffer, letter_a_ring, sizeof(letter_a_ring)),
       "Expected %s, got %s\n", letter_a_ring, buffer);
    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 2, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "C"), "Expected C, got %s\n", buffer);
    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed\n");
    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 1, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!memcmp(buffer, letter_a_with_ring, sizeof(letter_a_with_ring)),
       "Expected %s, got %s\n", letter_a_with_ring, buffer);
    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 2, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "D"), "Expected D, got %s\n", buffer);
    MsiCloseHandle(hrec);

    r = MsiQueryClose(hquery);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(hquery);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = MsiDatabaseOpenQuery(hdb, sql6, &hquery);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenQuery failed\n");
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed\n");
    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 1, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!memcmp(buffer, letter_a_with_ring, sizeof(letter_a_with_ring)),
       "Expected %s, got %s\n", letter_a_with_ring, buffer);
    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 2, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "D"), "Expected D, got %s\n", buffer);
    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "MsiQueryFetch failed\n");

    r = MsiQueryClose(hquery);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(hquery);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    DeleteFile(msifile);
}

static void test_select_markers(void)
{
    LibmsiObject *hdb = 0;
    LibmsiObject *rec;
    LibmsiObject *query;
    LibmsiObject *res;
    const char *sql;
    unsigned r;
    unsigned size;
    char buf[MAX_PATH];

    hdb = create_db();
    ok( hdb, "failed to create db\n");

    r = run_query(hdb, 0,
            "CREATE TABLE `Table` (`One` CHAR(72), `Two` CHAR(72), `Three` SHORT PRIMARY KEY `One`, `Two`, `Three`)");
    ok(r == S_OK, "cannot create table: %d\n", r);

    r = run_query(hdb, 0, "INSERT INTO `Table` "
            "( `One`, `Two`, `Three` ) VALUES ( 'apple', 'one', 1 )");
    ok(r == S_OK, "cannot add file to the Media table: %d\n", r);

    r = run_query(hdb, 0, "INSERT INTO `Table` "
            "( `One`, `Two`, `Three` ) VALUES ( 'apple', 'two', 1 )");
    ok(r == S_OK, "cannot add file to the Media table: %d\n", r);

    r = run_query(hdb, 0, "INSERT INTO `Table` "
            "( `One`, `Two`, `Three` ) VALUES ( 'apple', 'two', 2 )");
    ok(r == S_OK, "cannot add file to the Media table: %d\n", r);

    r = run_query(hdb, 0, "INSERT INTO `Table` "
            "( `One`, `Two`, `Three` ) VALUES ( 'banana', 'three', 3 )");
    ok(r == S_OK, "cannot add file to the Media table: %d\n", r);

    rec = MsiCreateRecord(2);
    MsiRecordSetString(rec, 1, "apple");
    MsiRecordSetString(rec, 2, "two");

    query = NULL;
    sql = "SELECT * FROM `Table` WHERE `One`=? AND `Two`=? ORDER BY `Three`";
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryExecute(query, rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(query, &res);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(res, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "apple"), "Expected apple, got %s\n", buf);

    size = MAX_PATH;
    r = MsiRecordGetString(res, 2, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "two"), "Expected two, got %s\n", buf);

    r = MsiRecordGetInteger(res, 3);
    ok(r == 1, "Expected 1, got %d\n", r);

    MsiCloseHandle(res);

    r = MsiQueryFetch(query, &res);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(res, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "apple"), "Expected apple, got %s\n", buf);

    size = MAX_PATH;
    r = MsiRecordGetString(res, 2, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "two"), "Expected two, got %s\n", buf);

    r = MsiRecordGetInteger(res, 3);
    ok(r == 2, "Expected 2, got %d\n", r);

    MsiCloseHandle(res);

    r = MsiQueryFetch(query, &res);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiCloseHandle(rec);
    MsiQueryClose(query);
    MsiCloseHandle(query);

    rec = MsiCreateRecord(2);
    MsiRecordSetString(rec, 1, "one");
    MsiRecordSetInteger(rec, 2, 1);

    query = NULL;
    sql = "SELECT * FROM `Table` WHERE `Two`<>? AND `Three`>? ORDER BY `Three`";
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(query, rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(query, &res);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(res, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "apple"), "Expected apple, got %s\n", buf);

    size = MAX_PATH;
    r = MsiRecordGetString(res, 2, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "two"), "Expected two, got %s\n", buf);

    r = MsiRecordGetInteger(res, 3);
    ok(r == 2, "Expected 2, got %d\n", r);

    MsiCloseHandle(res);

    r = MsiQueryFetch(query, &res);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(res, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "banana"), "Expected banana, got %s\n", buf);

    size = MAX_PATH;
    r = MsiRecordGetString(res, 2, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "three"), "Expected three, got %s\n", buf);

    r = MsiRecordGetInteger(res, 3);
    ok(r == 3, "Expected 3, got %d\n", r);

    MsiCloseHandle(res);

    r = MsiQueryFetch(query, &res);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiCloseHandle(rec);
    MsiQueryClose(query);
    MsiCloseHandle(query);
    MsiCloseHandle(hdb);
    DeleteFile(msifile);
}

static void test_querymodify_update(void)
{
    LibmsiObject *hdb = 0;
    LibmsiObject *hquery = 0;
    LibmsiObject *hrec = 0;
    unsigned i, test_max, offset, count;
    const char *sql;
    unsigned r;

    DeleteFile(msifile);

    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    sql = "CREATE TABLE `table` (`A` INT, `B` INT PRIMARY KEY `A`)";
    r = run_query( hdb, 0, sql );
    ok(r == ERROR_SUCCESS, "query failed\n");

    sql = "INSERT INTO `table` (`A`, `B`) VALUES (1, 2)";
    r = run_query( hdb, 0, sql );
    ok(r == ERROR_SUCCESS, "query failed\n");

    sql = "INSERT INTO `table` (`A`, `B`) VALUES (3, 4)";
    r = run_query( hdb, 0, sql );
    ok(r == ERROR_SUCCESS, "query failed\n");

    sql = "INSERT INTO `table` (`A`, `B`) VALUES (5, 6)";
    r = run_query( hdb, 0, sql );
    ok(r == ERROR_SUCCESS, "query failed\n");

    sql = "SELECT `B` FROM `table`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenQuery failed\n");
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");
    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed\n");

    r = MsiRecordSetInteger(hrec, 1, 0);
    ok(r == ERROR_SUCCESS, "failed to set integer\n");

    r = MsiQueryModify(hquery, LIBMSI_MODIFY_UPDATE, hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryModify failed: %d\n", r);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiQueryClose(hquery);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(hquery);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    sql = "SELECT * FROM `table`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenQuery failed\n");
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");
    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed\n");

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 1, "Expected 1, got %d\n", r);
    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 0, "Expected 0, got %d\n", r);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed\n");

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 3, "Expected 3, got %d\n", r);
    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 4, "Expected 4, got %d\n", r);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed\n");

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 5, "Expected 5, got %d\n", r);
    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 6, "Expected 6, got %d\n", r);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    r = MsiQueryClose(hquery);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(hquery);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* loop through all elements */
    sql = "SELECT `B` FROM `table`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenQuery failed\n");
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");

    while (true)
    {
        r = MsiQueryFetch(hquery, &hrec);
        if (r != ERROR_SUCCESS)
            break;

        r = MsiRecordSetInteger(hrec, 1, 0);
        ok(r == ERROR_SUCCESS, "failed to set integer\n");

        r = MsiQueryModify(hquery, LIBMSI_MODIFY_UPDATE, hrec);
        ok(r == ERROR_SUCCESS, "MsiQueryModify failed: %d\n", r);

        r = MsiCloseHandle(hrec);
        ok(r == ERROR_SUCCESS, "failed to close record\n");
    }

    r = MsiQueryClose(hquery);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(hquery);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    sql = "SELECT * FROM `table`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenQuery failed\n");
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");
    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed\n");

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 1, "Expected 1, got %d\n", r);
    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 0, "Expected 0, got %d\n", r);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed\n");

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 3, "Expected 3, got %d\n", r);
    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 0, "Expected 0, got %d\n", r);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed\n");

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 5, "Expected 5, got %d\n", r);
    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 0, "Expected 0, got %d\n", r);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    r = MsiQueryClose(hquery);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(hquery);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    sql = "CREATE TABLE `table2` (`A` INT, `B` INT PRIMARY KEY `A`)";
    r = run_query( hdb, 0, sql );
    ok(r == ERROR_SUCCESS, "query failed\n");

    sql = "INSERT INTO `table2` (`A`, `B`) VALUES (?, ?)";
    r = MsiDatabaseOpenQuery( hdb, sql, &hquery );
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenQuery failed\n");

    test_max = 100;
    offset = 1234;
    for(i = 0; i < test_max; i++)
    {

        hrec = MsiCreateRecord( 2 );
        MsiRecordSetInteger( hrec, 1, test_max - i );
        MsiRecordSetInteger( hrec, 2, i );

        r = MsiQueryExecute( hquery, hrec );
        ok(r == ERROR_SUCCESS, "Got %d\n", r);

        r = MsiCloseHandle( hrec );
        ok(r == ERROR_SUCCESS, "Got %d\n", r);
    }

    r = MsiQueryClose( hquery );
    ok(r == ERROR_SUCCESS, "Got %d\n", r);
    r = MsiCloseHandle( hquery );
    ok(r == ERROR_SUCCESS, "Got %d\n", r);

    /* Update. */
    sql = "SELECT * FROM `table2` ORDER BY `B`";
    r = MsiDatabaseOpenQuery( hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenQuery failed\n");
    r = MsiQueryExecute( hquery, 0 );
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");

    count = 0;
    while (MsiQueryFetch( hquery, &hrec ) == ERROR_SUCCESS)
    {
        unsigned b = MsiRecordGetInteger( hrec, 2 );

        r = MsiRecordSetInteger( hrec, 2, b + offset);
        ok(r == ERROR_SUCCESS, "Got %d\n", r);

        r = MsiQueryModify( hquery, LIBMSI_MODIFY_UPDATE, hrec );
        ok(r == ERROR_SUCCESS, "Got %d\n", r);

        r = MsiCloseHandle(hrec);
        ok(r == ERROR_SUCCESS, "failed to close record\n");
        count++;
    }
    ok(count == test_max, "Got count %d\n", count);

    r = MsiQueryClose(hquery);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(hquery);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* Recheck. */
    sql = "SELECT * FROM `table2` ORDER BY `B`";
    r = MsiDatabaseOpenQuery( hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenQuery failed\n");
    r = MsiQueryExecute( hquery, 0 );
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");

    count = 0;
    while (MsiQueryFetch( hquery, &hrec ) == ERROR_SUCCESS)
    {
        unsigned a = MsiRecordGetInteger( hrec, 1 );
        unsigned b = MsiRecordGetInteger( hrec, 2 );
        ok( ( test_max - a + offset) == b, "Got (%d, %d), expected (%d, %d)\n",
            a, b, test_max - a + offset, b);

        r = MsiCloseHandle(hrec);
        ok(r == ERROR_SUCCESS, "failed to close record\n");
        count++;
    }
    ok(count == test_max, "Got count %d\n", count);

    r = MsiQueryClose(hquery);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(hquery);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = MsiCloseHandle( hdb );
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase close failed\n");
}

static void test_querymodify_assign(void)
{
    LibmsiObject *hdb = 0;
    LibmsiObject *hquery = 0;
    LibmsiObject *hrec = 0;
    const char *sql;
    unsigned r;

    /* setup database */
    DeleteFile(msifile);

    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    sql = "CREATE TABLE `table` (`A` INT, `B` INT PRIMARY KEY `A`)";
    r = run_query( hdb, 0, sql );
    ok(r == ERROR_SUCCESS, "query failed\n");

    /* assign to query, new primary key */
    sql = "SELECT * FROM `table`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenQuery failed\n");
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");

    hrec = MsiCreateRecord(2);
    ok(hrec != 0, "MsiCreateRecord failed\n");

    r = MsiRecordSetInteger(hrec, 1, 1);
    ok(r == ERROR_SUCCESS, "failed to set integer\n");
    r = MsiRecordSetInteger(hrec, 2, 2);
    ok(r == ERROR_SUCCESS, "failed to set integer\n");

    r = MsiQueryModify(hquery, LIBMSI_MODIFY_ASSIGN, hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryModify failed: %d\n", r);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiQueryClose(hquery);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(hquery);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    sql = "SELECT * FROM `table`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenQuery failed\n");
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");
    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed\n");

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 1, "Expected 1, got %d\n", r);
    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 2, "Expected 2, got %d\n", r);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    r = MsiQueryClose(hquery);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(hquery);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* assign to query, primary key matches */
    sql = "SELECT * FROM `table`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenQuery failed\n");
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");

    hrec = MsiCreateRecord(2);
    ok(hrec != 0, "MsiCreateRecord failed\n");

    r = MsiRecordSetInteger(hrec, 1, 1);
    ok(r == ERROR_SUCCESS, "failed to set integer\n");
    r = MsiRecordSetInteger(hrec, 2, 4);
    ok(r == ERROR_SUCCESS, "failed to set integer\n");

    r = MsiQueryModify(hquery, LIBMSI_MODIFY_ASSIGN, hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryModify failed: %d\n", r);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiQueryClose(hquery);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(hquery);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    sql = "SELECT * FROM `table`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenQuery failed\n");
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "MsiQueryExecute failed\n");
    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "MsiQueryFetch failed\n");

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 1, "Expected 1, got %d\n", r);
    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 4, "Expected 4, got %d\n", r);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    r = MsiQueryClose(hquery);
    ok(r == ERROR_SUCCESS, "MsiQueryClose failed\n");
    r = MsiCloseHandle(hquery);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* close database */
    r = MsiCloseHandle( hdb );
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase close failed\n");
}

static const WCHAR data10[] = { /* MOO */
    0x8001, 0x000b,
};
static const WCHAR data11[] = { /* AAR */
    0x8002, 0x8005,
    0x000c, 0x000f,
};
static const char data12[] = /* _StringData */
    "MOOABAARCDonetwofourfive";
static const WCHAR data13[] = { /* _StringPool */
/*  len, refs */
    0,   0,    /* string 0 ''     */
    0,   0,    /* string 1 ''     */
    0,   0,    /* string 2 ''     */
    0,   0,    /* string 3 ''     */
    0,   0,    /* string 4 ''     */
    3,   3,    /* string 5 'MOO'  */
    1,   1,    /* string 6 'A'    */
    1,   1,    /* string 7 'B'    */
    3,   3,    /* string 8 'AAR'  */
    1,   1,    /* string 9 'C'    */
    1,   1,    /* string a 'D'    */
    3,   1,    /* string b 'one'  */
    3,   1,    /* string c 'two'  */
    0,   0,    /* string d ''     */
    4,   1,    /* string e 'four' */
    4,   1,    /* string f 'five' */
};

static void test_stringtable(void)
{
    LibmsiObject *hdb = 0;
    LibmsiObject *hquery = 0;
    LibmsiObject *hrec = 0;
    IStorage *stg = NULL;
    IStream *stm;
    WCHAR name[0x20];
    HRESULT hr;
    const char *sql;
    char buffer[MAX_PATH];
    WCHAR data[MAX_PATH];
    unsigned sz, read;
    unsigned r;

    static const unsigned mode = STGM_DIRECT | STGM_READ | STGM_SHARE_DENY_WRITE;
    static const WCHAR stringdata[] = {0x4840, 0x3f3f, 0x4577, 0x446c, 0x3b6a, 0x45e4, 0x4824, 0}; /* _StringData */
    static const WCHAR stringpool[] = {0x4840, 0x3f3f, 0x4577, 0x446c, 0x3e6a, 0x44b2, 0x482f, 0}; /* _StringPool */
    static const WCHAR moo[] = {0x4840, 0x3e16, 0x4818, 0}; /* MOO */
    static const WCHAR aar[] = {0x4840, 0x3a8a, 0x481b, 0}; /* AAR */

    DeleteFile(msifile);

    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `MOO` (`A` INT, `B` CHAR(72) PRIMARY KEY `A`)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `AAR` (`C` INT, `D` CHAR(72) PRIMARY KEY `C`)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* insert persistent row */
    sql = "INSERT INTO `MOO` (`A`, `B`) VALUES (1, 'one')";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* insert persistent row */
    sql = "INSERT INTO `AAR` (`C`, `D`) VALUES (2, 'two')";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* open a query */
    sql = "SELECT * FROM `MOO`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    hrec = MsiCreateRecord(2);

    r = MsiRecordSetInteger(hrec, 1, 3);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiRecordSetString(hrec, 2, "three");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* insert a nonpersistent row */
    r = MsiQueryModify(hquery, LIBMSI_MODIFY_INSERT_TEMPORARY, hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryClose(hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiCloseHandle(hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* insert persistent row */
    sql = "INSERT INTO `MOO` (`A`, `B`) VALUES (4, 'four')";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* insert persistent row */
    sql = "INSERT INTO `AAR` (`C`, `D`) VALUES (5, 'five')";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiDatabaseCommit(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_READONLY, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "SELECT * FROM `MOO`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetFieldCount(hrec);
    ok(r == 2, "Expected 2, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 1, "Expected 1, got %d\n", r);

    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 2, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "one"), "Expected one, got '%s'\n", buffer);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    r = MsiQueryClose(hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiCloseHandle(hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "SELECT * FROM `AAR`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetFieldCount(hrec);
    ok(r == 2, "Expected 2, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 2, "Expected 2, got %d\n", r);

    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 2, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "two"), "Expected two, got '%s'\n", buffer);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetFieldCount(hrec);
    ok(r == 2, "Expected 2, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 5, "Expected 5, got %d\n", r);

    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 2, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "five"), "Expected five, got '%s'\n", buffer);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    r = MsiQueryClose(hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiCloseHandle(hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MultiByteToWideChar(CP_ACP, 0, msifile, -1, name, 0x20);
    hr = StgOpenStorage(name, NULL, mode, NULL, 0, &stg);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);
    ok(stg != NULL, "Expected non-NULL storage\n");

    hr = IStorage_OpenStream(stg, moo, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &stm);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);
    ok(stm != NULL, "Expected non-NULL stream\n");

    hr = IStream_Read(stm, data, MAX_PATH, &read);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);
    ok(read == 4, "Expected 4, got %d\n", read);
    todo_wine ok(!memcmp(data, data10, read), "Unexpected data\n");

    hr = IStream_Release(stm);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);

    hr = IStorage_OpenStream(stg, aar, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &stm);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);
    ok(stm != NULL, "Expected non-NULL stream\n");

    hr = IStream_Read(stm, data, MAX_PATH, &read);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);
    ok(read == 8, "Expected 8, got %d\n", read);
    todo_wine
    {
        ok(!memcmp(data, data11, read), "Unexpected data\n");
    }

    hr = IStream_Release(stm);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);

    hr = IStorage_OpenStream(stg, stringdata, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &stm);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);
    ok(stm != NULL, "Expected non-NULL stream\n");

    hr = IStream_Read(stm, buffer, MAX_PATH, &read);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);
    ok(read == 24, "Expected 24, got %d\n", read);
    ok(!memcmp(buffer, data12, read), "Unexpected data\n");

    hr = IStream_Release(stm);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);

    hr = IStorage_OpenStream(stg, stringpool, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &stm);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);
    ok(stm != NULL, "Expected non-NULL stream\n");

    hr = IStream_Read(stm, data, MAX_PATH, &read);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);
    todo_wine
    {
        ok(read == 64, "Expected 64, got %d\n", read);
        ok(!memcmp(data, data13, read), "Unexpected data\n");
    }

    hr = IStream_Release(stm);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);

    hr = IStorage_Release(stg);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);

    DeleteFileA(msifile);
}

static void test_querymodify_delete(void)
{
    LibmsiObject *hdb = 0;
    LibmsiObject *hquery = 0;
    LibmsiObject *hrec = 0;
    unsigned r;
    const char *sql;
    char buffer[0x100];
    unsigned sz;

    DeleteFile(msifile);

    /* just MsiOpenDatabase should not create a file */
    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `phone` ( "
            "`id` INT, `name` CHAR(32), `number` CHAR(32) "
            "PRIMARY KEY `id`)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `phone` ( `id`, `name`, `number` )"
        "VALUES('1', 'Alan', '5030581')";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `phone` ( `id`, `name`, `number` )"
        "VALUES('2', 'Barry', '928440')";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `phone` ( `id`, `name`, `number` )"
        "VALUES('3', 'Cindy', '2937550')";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "SELECT * FROM `phone` WHERE `id` <= 2";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* delete 1 */
    r = MsiQueryModify(hquery, LIBMSI_MODIFY_DELETE, hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* delete 2 */
    r = MsiQueryModify(hquery, LIBMSI_MODIFY_DELETE, hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryClose(hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiCloseHandle(hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "SELECT * FROM `phone`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 3, "Expected 3, got %d\n", r);

    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 2, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "Cindy"), "Expected Cindy, got %s\n", buffer);

    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 3, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "2937550"), "Expected 2937550, got %s\n", buffer);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    r = MsiQueryClose(hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiCloseHandle(hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
}

static const WCHAR _Tables[] = {0x4840, 0x3f7f, 0x4164, 0x422f, 0x4836, 0};
static const WCHAR _StringData[] = {0x4840, 0x3f3f, 0x4577, 0x446c, 0x3b6a, 0x45e4, 0x4824, 0};
static const WCHAR _StringPool[] = {0x4840, 0x3f3f, 0x4577, 0x446c, 0x3e6a, 0x44b2, 0x482f, 0};

static const WCHAR data14[] = { /* _StringPool */
/*  len, refs */
    0,   0,    /* string 0 ''    */
};

static const struct {
    const WCHAR *name;
    const void *data;
    unsigned size;
} database_table_data[] =
{
    {_Tables, NULL, 0},
    {_StringData, NULL, 0},
    {_StringPool, data14, sizeof data14},
};

static void enum_stream_names(IStorage *stg)
{
    IEnumSTATSTG *stgenum = NULL;
    IStream *stm;
    HRESULT hr;
    STATSTG stat;
    unsigned n, count;
    uint8_t data[MAX_PATH];
    uint8_t check[MAX_PATH];
    unsigned sz;

    memset(check, 'a', MAX_PATH);

    hr = IStorage_EnumElements(stg, 0, NULL, 0, &stgenum);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    n = 0;
    while(true)
    {
        count = 0;
        hr = IEnumSTATSTG_Next(stgenum, 1, &stat, &count);
        if(FAILED(hr) || !count)
            break;

        ok(!lstrcmpW(stat.pwcsName, database_table_data[n].name),
           "Expected table %d name to match\n", n);

        stm = NULL;
        hr = IStorage_OpenStream(stg, stat.pwcsName, NULL,
                                 STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &stm);
        ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
        ok(stm != NULL, "Expected non-NULL stream\n");

        CoTaskMemFree(stat.pwcsName);

        sz = MAX_PATH;
        memset(data, 'a', MAX_PATH);
        hr = IStream_Read(stm, data, sz, &count);
        ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

        ok(count == database_table_data[n].size,
           "Expected %d, got %d\n", database_table_data[n].size, count);

        if (!database_table_data[n].size)
            ok(!memcmp(data, check, MAX_PATH), "data should not be changed\n");
        else
            ok(!memcmp(data, database_table_data[n].data, database_table_data[n].size),
               "Expected table %d data to match\n", n);

        IStream_Release(stm);
        n++;
    }

    ok(n == 3, "Expected 3, got %d\n", n);

    IEnumSTATSTG_Release(stgenum);
}

static void test_defaultdatabase(void)
{
    unsigned r;
    HRESULT hr;
    LibmsiObject *hdb;
    IStorage *stg = NULL;

    DeleteFile(msifile);

    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiDatabaseCommit(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(hdb);

    hr = StgOpenStorage(msifileW, NULL, STGM_READ | STGM_SHARE_DENY_WRITE, NULL, 0, &stg);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(stg != NULL, "Expected non-NULL stg\n");

    enum_stream_names(stg);

    IStorage_Release(stg);
    DeleteFileA(msifile);
}

static void test_order(void)
{
    LibmsiObject *hdb;
    LibmsiObject *hquery;
    LibmsiObject *hrec;
    char buffer[MAX_PATH];
    const char *sql;
    unsigned r, sz;
    int val;

    hdb = create_db();
    ok(hdb, "failed to create db\n");

    sql = "CREATE TABLE `Empty` ( `A` SHORT NOT NULL PRIMARY KEY `A`)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `Mesa` ( `A` SHORT NOT NULL, `B` SHORT, `C` SHORT PRIMARY KEY `A`)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `Mesa` ( `A`, `B`, `C` ) VALUES ( 1, 2, 9 )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `Mesa` ( `A`, `B`, `C` ) VALUES ( 3, 4, 7 )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `Mesa` ( `A`, `B`, `C` ) VALUES ( 5, 6, 8 )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `Sideboard` ( `D` SHORT NOT NULL, `E` SHORT, `F` SHORT PRIMARY KEY `D`)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `Sideboard` ( `D`, `E`, `F` ) VALUES ( 10, 11, 18 )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `Sideboard` ( `D`, `E`, `F` ) VALUES ( 12, 13, 16 )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `Sideboard` ( `D`, `E`, `F` ) VALUES ( 14, 15, 17 )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "SELECT `A`, `B` FROM `Mesa` ORDER BY `C`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    val = MsiRecordGetInteger(hrec, 1);
    ok(val == 3, "Expected 3, got %d\n", val);

    val = MsiRecordGetInteger(hrec, 2);
    ok(val == 4, "Expected 3, got %d\n", val);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    val = MsiRecordGetInteger(hrec, 1);
    ok(val == 5, "Expected 5, got %d\n", val);

    val = MsiRecordGetInteger(hrec, 2);
    ok(val == 6, "Expected 6, got %d\n", val);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    val = MsiRecordGetInteger(hrec, 1);
    ok(val == 1, "Expected 1, got %d\n", val);

    val = MsiRecordGetInteger(hrec, 2);
    ok(val == 2, "Expected 2, got %d\n", val);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    sql = "SELECT `A`, `D` FROM `Mesa`, `Sideboard` ORDER BY `F`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    val = MsiRecordGetInteger(hrec, 1);
    ok(val == 1, "Expected 1, got %d\n", val);

    val = MsiRecordGetInteger(hrec, 2);
    ok(val == 12, "Expected 12, got %d\n", val);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    val = MsiRecordGetInteger(hrec, 1);
    ok(val == 3, "Expected 3, got %d\n", val);

    val = MsiRecordGetInteger(hrec, 2);
    ok(val == 12, "Expected 12, got %d\n", val);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    val = MsiRecordGetInteger(hrec, 1);
    ok(val == 5, "Expected 5, got %d\n", val);

    val = MsiRecordGetInteger(hrec, 2);
    ok(val == 12, "Expected 12, got %d\n", val);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    val = MsiRecordGetInteger(hrec, 1);
    ok(val == 1, "Expected 1, got %d\n", val);

    val = MsiRecordGetInteger(hrec, 2);
    ok(val == 14, "Expected 14, got %d\n", val);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    val = MsiRecordGetInteger(hrec, 1);
    ok(val == 3, "Expected 3, got %d\n", val);

    val = MsiRecordGetInteger(hrec, 2);
    ok(val == 14, "Expected 14, got %d\n", val);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    val = MsiRecordGetInteger(hrec, 1);
    ok(val == 5, "Expected 5, got %d\n", val);

    val = MsiRecordGetInteger(hrec, 2);
    ok(val == 14, "Expected 14, got %d\n", val);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    val = MsiRecordGetInteger(hrec, 1);
    ok(val == 1, "Expected 1, got %d\n", val);

    val = MsiRecordGetInteger(hrec, 2);
    ok(val == 10, "Expected 10, got %d\n", val);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    val = MsiRecordGetInteger(hrec, 1);
    ok(val == 3, "Expected 3, got %d\n", val);

    val = MsiRecordGetInteger(hrec, 2);
    ok(val == 10, "Expected 10, got %d\n", val);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    val = MsiRecordGetInteger(hrec, 1);
    ok(val == 5, "Expected 5, got %d\n", val);

    val = MsiRecordGetInteger(hrec, 2);
    ok(val == 10, "Expected 10, got %d\n", val);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    sql = "SELECT * FROM `Empty` ORDER BY `A`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    sql = "CREATE TABLE `Buffet` ( `One` CHAR(72), `Two` SHORT PRIMARY KEY `One`)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `Buffet` ( `One`, `Two` ) VALUES ( 'uno',  2)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `Buffet` ( `One`, `Two` ) VALUES ( 'dos',  3)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `Buffet` ( `One`, `Two` ) VALUES ( 'tres',  1)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "SELECT * FROM `Buffet` WHERE `One` = 'dos' ORDER BY `Two`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 1, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "dos"), "Expected \"dos\", got \"%s\"\n", buffer);

    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 3, "Expected 3, got %d\n", r);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);
    MsiCloseHandle(hdb);
}

static void test_querymodify_delete_temporary(void)
{
    LibmsiObject *hdb;
    LibmsiObject *hquery;
    LibmsiObject *hrec;
    const char *sql;
    unsigned r;

    DeleteFile(msifile);

    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `Table` ( `A` SHORT PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "SELECT * FROM `Table`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    hrec = MsiCreateRecord(1);
    MsiRecordSetInteger(hrec, 1, 1);

    r = MsiQueryModify(hquery, LIBMSI_MODIFY_INSERT, hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(hrec);

    hrec = MsiCreateRecord(1);
    MsiRecordSetInteger(hrec, 1, 2);

    r = MsiQueryModify(hquery, LIBMSI_MODIFY_INSERT_TEMPORARY, hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(hrec);

    hrec = MsiCreateRecord(1);
    MsiRecordSetInteger(hrec, 1, 3);

    r = MsiQueryModify(hquery, LIBMSI_MODIFY_INSERT, hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(hrec);

    hrec = MsiCreateRecord(1);
    MsiRecordSetInteger(hrec, 1, 4);

    r = MsiQueryModify(hquery, LIBMSI_MODIFY_INSERT_TEMPORARY, hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(hrec);
    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    sql = "SELECT * FROM `Table` WHERE  `A` = 2";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryModify(hquery, LIBMSI_MODIFY_DELETE, hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(hrec);
    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    sql = "SELECT * FROM `Table` WHERE  `A` = 3";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryModify(hquery, LIBMSI_MODIFY_DELETE, hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(hrec);
    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    sql = "SELECT * FROM `Table` ORDER BY `A`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 1, "Expected 1, got %d\n", r);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 4, "Expected 4, got %d\n", r);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);
    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

static void test_deleterow(void)
{
    LibmsiObject *hdb;
    LibmsiObject *hquery;
    LibmsiObject *hrec;
    const char *sql;
    char buf[MAX_PATH];
    unsigned r;
    unsigned size;

    DeleteFile(msifile);

    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `Table` ( `A` CHAR(72) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `Table` (`A`) VALUES ('one')";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `Table` (`A`) VALUES ('two')";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "DELETE FROM `Table` WHERE `A` = 'one'";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiDatabaseCommit(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(hdb);

    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_READONLY, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "SELECT * FROM `Table`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "two"), "Expected two, got %s\n", buf);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);
    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

static const char import_dat[] = "A\n"
                                 "s72\n"
                                 "Table\tA\n"
                                 "This is a new 'string' ok\n";

static void test_quotes(void)
{
    LibmsiObject *hdb;
    LibmsiObject *hquery;
    LibmsiObject *hrec;
    const char *sql;
    char buf[MAX_PATH];
    unsigned r;
    unsigned size;

    DeleteFile(msifile);

    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `Table` ( `A` CHAR(72) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `Table` ( `A` ) VALUES ( 'This is a 'string' ok' )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "INSERT INTO `Table` ( `A` ) VALUES ( \"This is a 'string' ok\" )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "INSERT INTO `Table` ( `A` ) VALUES ( \"test\" )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "INSERT INTO `Table` ( `A` ) VALUES ( 'This is a ''string'' ok' )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "INSERT INTO `Table` ( `A` ) VALUES ( 'This is a '''string''' ok' )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "INSERT INTO `Table` ( `A` ) VALUES ( 'This is a \'string\' ok' )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "INSERT INTO `Table` ( `A` ) VALUES ( 'This is a \"string\" ok' )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "SELECT * FROM `Table`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "This is a \"string\" ok"),
       "Expected \"This is a \"string\" ok\", got %s\n", buf);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    write_file("import.idt", import_dat, (sizeof(import_dat) - 1) * sizeof(char));

    r = MsiDatabaseImport(hdb, CURR_DIR, "import.idt");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    DeleteFileA("import.idt");

    sql = "SELECT * FROM `Table`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "This is a new 'string' ok"),
       "Expected \"This is a new 'string' ok\", got %s\n", buf);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);
    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

static void test_carriagereturn(void)
{
    LibmsiObject *hdb;
    LibmsiObject *hquery;
    LibmsiObject *hrec;
    const char *sql;
    char buf[MAX_PATH];
    unsigned r;
    unsigned size;

    DeleteFile(msifile);

    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `Table`\r ( `A` CHAR(72) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "CREATE TABLE `Table` \r( `A` CHAR(72) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "CREATE\r TABLE `Table` ( `A` CHAR(72) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "CREATE TABLE\r `Table` ( `A` CHAR(72) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "CREATE TABLE `Table` (\r `A` CHAR(72) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "CREATE TABLE `Table` ( `A`\r CHAR(72) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "CREATE TABLE `Table` ( `A` CHAR(72)\r NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "CREATE TABLE `Table` ( `A` CHAR(72) NOT\r NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "CREATE TABLE `Table` ( `A` CHAR(72) NOT \rNULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "CREATE TABLE `Table` ( `A` CHAR(72) NOT NULL\r PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "CREATE TABLE `Table` ( `A` CHAR(72) NOT NULL \rPRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "CREATE TABLE `Table` ( `A` CHAR(72) NOT NULL PRIMARY\r KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "CREATE TABLE `Table` ( `A` CHAR(72) NOT NULL PRIMARY \rKEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "CREATE TABLE `Table` ( `A` CHAR(72) NOT NULL PRIMARY KEY\r `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "CREATE TABLE `Table` ( `A` CHAR(72) NOT NULL PRIMARY KEY `A`\r )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "CREATE TABLE `Table` ( `A` CHAR(72) NOT NULL PRIMARY KEY `A` )\r";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "CREATE TABLE `\rOne` ( `A` CHAR(72) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `Tw\ro` ( `A` CHAR(72) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `Three\r` ( `A` CHAR(72) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `Four` ( `A\r` CHAR(72) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "CREATE TABLE `Four` ( `\rA` CHAR(72) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "CREATE TABLE `Four` ( `A` CHAR(72\r) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "CREATE TABLE `Four` ( `A` CHAR(\r72) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "CREATE TABLE `Four` ( `A` CHAR(72) NOT NULL PRIMARY KEY `\rA` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "CREATE TABLE `Four` ( `A` CHAR(72) NOT NULL PRIMARY KEY `A\r` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "CREATE TABLE `Four` ( `A` CHAR(72) NOT NULL PRIMARY KEY `A\r` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "SELECT * FROM `_Tables`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "\rOne"), "Expected \"\\rOne\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "Tw\ro"), "Expected \"Tw\\ro\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "Three\r"), "Expected \"Three\r\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

static void test_noquotes(void)
{
    LibmsiObject *hdb;
    LibmsiObject *hquery;
    LibmsiObject *hrec;
    const char *sql;
    char buf[MAX_PATH];
    unsigned r;
    unsigned size;

    DeleteFile(msifile);

    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE Table ( `A` CHAR(72) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "CREATE TABLE `Table` ( A CHAR(72) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `Table2` ( `A` CHAR(72) NOT NULL PRIMARY KEY A )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `Table3` ( A CHAR(72) NOT NULL PRIMARY KEY A )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "SELECT * FROM `_Tables`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "Table"), "Expected \"Table\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "Table2"), "Expected \"Table2\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "Table3"), "Expected \"Table3\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    sql = "SELECT * FROM `_Columns`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "Table"), "Expected \"Table\", got \"%s\"\n", buf);

    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 1, "Expected 1, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 3, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "A"), "Expected \"A\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "Table2"), "Expected \"Table2\", got \"%s\"\n", buf);

    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 1, "Expected 1, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 3, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "A"), "Expected \"A\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "Table3"), "Expected \"Table3\", got \"%s\"\n", buf);

    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 1, "Expected 1, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 3, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "A"), "Expected \"A\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    sql = "INSERT INTO Table ( `A` ) VALUES ( 'hi' )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "INSERT INTO `Table` ( A ) VALUES ( 'hi' )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `Table` ( `A` ) VALUES ( hi )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "SELECT * FROM Table WHERE `A` = 'hi'";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "SELECT * FROM `Table` WHERE `A` = hi";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "SELECT * FROM Table";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    hquery = NULL;
    sql = "SELECT * FROM Table2";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    sql = "SELECT * FROM `Table` WHERE A = 'hi'";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "hi"), "Expected \"hi\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);
    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

static void read_file_data(const char *filename, char *buffer)
{
    HANDLE file;
    unsigned read;

    file = CreateFileA( filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
    ZeroMemory(buffer, MAX_PATH);
    ReadFile(file, buffer, MAX_PATH, &read, NULL);
    CloseHandle(file);
}

static void test_forcecodepage(void)
{
    LibmsiObject *hdb;
    const char *sql;
    char buffer[MAX_PATH];
    unsigned r;
    int fd;

    DeleteFile(msifile);
    GetCurrentDirectoryA(MAX_PATH, CURR_DIR);

    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "SELECT * FROM `_ForceCodepage`";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "CREATE TABLE `Table` ( `A` CHAR(72) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "SELECT * FROM `_ForceCodepage`";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    r = MsiDatabaseCommit(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "SELECT * FROM `_ForceCodepage`";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    MsiCloseHandle(hdb);

    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_DIRECT, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "SELECT * FROM `_ForceCodepage`";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    fd = open("forcecodepage.idt", O_WRONLY | O_BINARY | O_CREAT, 0644);
    ok(fd != -1, "cannot open file\n");

    r = MsiDatabaseExport(hdb, "_ForceCodepage", fd);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    close(fd);

    read_file_data("forcecodepage.idt", buffer);
    ok(!strcmp(buffer, "\r\n\r\n0\t_ForceCodepage\r\n"),
       "Expected \"\r\n\r\n0\t_ForceCodepage\r\n\", got \"%s\"\n", buffer);

    create_file_data("forcecodepage.idt", "\r\n\r\n850\t_ForceCodepage\r\n", 0);

    r = MsiDatabaseImport(hdb, CURR_DIR, "forcecodepage.idt");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    fd = open("forcecodepage.idt", O_WRONLY | O_BINARY | O_CREAT, 0644);
    ok(fd != -1, "cannot open file\n");

    r = MsiDatabaseExport(hdb, "_ForceCodepage", fd);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    close(fd);

    read_file_data("forcecodepage.idt", buffer);
    ok(!strcmp(buffer, "\r\n\r\n850\t_ForceCodepage\r\n"),
       "Expected \"\r\n\r\n850\t_ForceCodepage\r\n\", got \"%s\"\n", buffer);

    create_file_data("forcecodepage.idt", "\r\n\r\n9999\t_ForceCodepage\r\n", 0);

    r = MsiDatabaseImport(hdb, CURR_DIR, "forcecodepage.idt");
    ok(r == ERROR_FUNCTION_FAILED, "Expected ERROR_FUNCTION_FAILED, got %d\n", r);

    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
    DeleteFileA("forcecodepage.idt");
}

static void test_querymodify_refresh(void)
{
    LibmsiObject *hdb;
    LibmsiObject *hquery;
    LibmsiObject *hrec;
    const char *sql;
    char buffer[MAX_PATH];
    unsigned r;
    unsigned size;

    DeleteFile(msifile);

    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `Table` ( `A` CHAR(72) NOT NULL, `B` INT PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `Table` ( `A`, `B` ) VALUES ( 'hi', 1 )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "SELECT * FROM `Table`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "UPDATE `Table` SET `B` = 2 WHERE `A` = 'hi'";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryModify(hquery, LIBMSI_MODIFY_REFRESH, hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 1, buffer, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "hi"), "Expected \"hi\", got \"%s\"\n", buffer);
    ok(size == 2, "Expected 2, got %d\n", size);

    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 2, "Expected 2, got %d\n", r);

    MsiCloseHandle(hrec);
    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    sql = "INSERT INTO `Table` ( `A`, `B` ) VALUES ( 'hello', 3 )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "SELECT * FROM `Table` WHERE `B` = 3";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "UPDATE `Table` SET `B` = 2 WHERE `A` = 'hello'";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `Table` ( `A`, `B` ) VALUES ( 'hithere', 3 )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryModify(hquery, LIBMSI_MODIFY_REFRESH, hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 1, buffer, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buffer, "hello"), "Expected \"hello\", got \"%s\"\n", buffer);
    ok(size == 5, "Expected 5, got %d\n", size);

    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 2, "Expected 2, got %d\n", r);

    MsiCloseHandle(hrec);
    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);
    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

static void test_where_querymodify(void)
{
    LibmsiObject *hdb;
    LibmsiObject *hquery;
    LibmsiObject *hrec;
    const char *sql;
    unsigned r;

    DeleteFile(msifile);

    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `Table` ( `A` INT, `B` INT PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `Table` ( `A`, `B` ) VALUES ( 1, 2 )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `Table` ( `A`, `B` ) VALUES ( 3, 4 )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `Table` ( `A`, `B` ) VALUES ( 5, 6 )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* `B` = 3 doesn't match, but the query shouldn't be executed */
    sql = "SELECT * FROM `Table` WHERE `B` = 3";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    hrec = MsiCreateRecord(2);
    MsiRecordSetInteger(hrec, 1, 7);
    MsiRecordSetInteger(hrec, 2, 8);

    r = MsiQueryModify(hquery, LIBMSI_MODIFY_INSERT_TEMPORARY, hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(hrec);
    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    sql = "SELECT * FROM `Table` WHERE `A` = 7";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 7, "Expected 7, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 8, "Expected 8, got %d\n", r);

    MsiRecordSetInteger(hrec, 2, 9);

    r = MsiQueryModify(hquery, LIBMSI_MODIFY_UPDATE, hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(hrec);
    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    sql = "SELECT * FROM `Table` WHERE `A` = 7";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 7, "Expected 7, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 9, "Expected 9, got %d\n", r);

    sql = "UPDATE `Table` SET `B` = 10 WHERE `A` = 7";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryModify(hquery, LIBMSI_MODIFY_REFRESH, hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 7, "Expected 7, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 10, "Expected 10, got %d\n", r);

    MsiCloseHandle(hrec);
    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);
    MsiCloseHandle(hdb);
}

static bool create_storage(const char *name)
{
    WCHAR nameW[MAX_PATH];
    IStorage *stg;
    IStream *stm;
    HRESULT hr;
    unsigned count;
    bool res = false;

    MultiByteToWideChar(CP_ACP, 0, name, -1, nameW, MAX_PATH);
    hr = StgCreateDocfile(nameW, STGM_CREATE | STGM_READWRITE |
                          STGM_DIRECT | STGM_SHARE_EXCLUSIVE, 0, &stg);
    if (FAILED(hr))
        return false;

    hr = IStorage_CreateStream(stg, nameW, STGM_WRITE | STGM_SHARE_EXCLUSIVE,
                               0, 0, &stm);
    if (FAILED(hr))
        goto done;

    hr = IStream_Write(stm, "stgdata", 8, &count);
    if (SUCCEEDED(hr))
        res = true;

done:
    IStream_Release(stm);
    IStorage_Release(stg);

    return res;
}

static void test_storages_table(void)
{
    LibmsiObject *hdb;
    LibmsiObject *hquery;
    LibmsiObject *hrec;
    IStorage *stg, *inner;
    IStream *stm;
    char file[MAX_PATH];
    char buf[MAX_PATH];
    WCHAR name[MAX_PATH];
    const char *sql;
    HRESULT hr;
    unsigned size;
    unsigned r;

    hdb = create_db();
    ok(hdb, "failed to create db\n");

    r = MsiDatabaseCommit(hdb);
    ok(r == ERROR_SUCCESS , "Failed to commit database\n");

    MsiCloseHandle(hdb);

    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_TRANSACT, &hdb);
    ok(r == ERROR_SUCCESS , "Failed to open database\n");

    /* check the column types */
    hrec = get_column_info(hdb, "SELECT * FROM `_Storages`", LIBMSI_COL_INFO_TYPES);
    ok(hrec, "failed to get column info hrecord\n");
    ok(check_record(hrec, 1, "s62"), "wrong hrecord type\n");
    ok(check_record(hrec, 2, "V0"), "wrong hrecord type\n");

    MsiCloseHandle(hrec);

    /* now try the names */
    hrec = get_column_info(hdb, "SELECT * FROM `_Storages`", LIBMSI_COL_INFO_NAMES);
    ok(hrec, "failed to get column info hrecord\n");
    ok(check_record(hrec, 1, "Name"), "wrong hrecord type\n");
    ok(check_record(hrec, 2, "Data"), "wrong hrecord type\n");

    MsiCloseHandle(hrec);

    create_storage("storage.bin");

    hrec = MsiCreateRecord(2);
    MsiRecordSetString(hrec, 1, "stgname");

    r = MsiRecordSetStream(hrec, 2, "storage.bin");
    ok(r == ERROR_SUCCESS, "Failed to add stream data to the hrecord: %d\n", r);

    DeleteFileA("storage.bin");

    sql = "INSERT INTO `_Storages` (`Name`, `Data`) VALUES (?, ?)";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "Failed to open database hquery: %d\n", r);

    r = MsiQueryExecute(hquery, hrec);
    ok(r == ERROR_SUCCESS, "Failed to execute hquery: %d\n", r);

    MsiCloseHandle(hrec);
    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    sql = "SELECT `Name`, `Data` FROM `_Storages`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "Failed to open database hquery: %d\n", r);

    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "Failed to execute hquery: %d\n", r);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Failed to fetch hrecord: %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 1, file, &size);
    ok(r == ERROR_SUCCESS, "Failed to get string: %d\n", r);
    ok(!strcmp(file, "stgname"), "Expected \"stgname\", got \"%s\"\n", file);

    size = MAX_PATH;
    strcpy(buf, "apple");
    r = MsiRecordReadStream(hrec, 2, buf, &size);
    ok(r == ERROR_INVALID_DATA, "Expected ERROR_INVALID_DATA, got %d\n", r);
    ok(!strcmp(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(size == 0, "Expected 0, got %d\n", size);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    MsiDatabaseCommit(hdb);
    MsiCloseHandle(hdb);

    MultiByteToWideChar(CP_ACP, 0, msifile, -1, name, MAX_PATH);
    hr = StgOpenStorage(name, NULL, STGM_DIRECT | STGM_READ |
                        STGM_SHARE_DENY_WRITE, NULL, 0, &stg);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(stg != NULL, "Expected non-NULL storage\n");

    MultiByteToWideChar(CP_ACP, 0, "stgname", -1, name, MAX_PATH);
    hr = IStorage_OpenStorage(stg, name, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE,
                              NULL, 0, &inner);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(inner != NULL, "Expected non-NULL storage\n");

    MultiByteToWideChar(CP_ACP, 0, "storage.bin", -1, name, MAX_PATH);
    hr = IStorage_OpenStream(inner, name, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &stm);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(stm != NULL, "Expected non-NULL stream\n");

    hr = IStream_Read(stm, buf, MAX_PATH, &size);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);
    ok(size == 8, "Expected 8, got %d\n", size);
    ok(!strcmp(buf, "stgdata"), "Expected \"stgdata\", got \"%s\"\n", buf);

    IStream_Release(stm);
    IStorage_Release(inner);

    IStorage_Release(stg);
    DeleteFileA(msifile);
}

static void test_droptable(void)
{
    LibmsiObject *hdb;
    LibmsiObject *hquery;
    LibmsiObject *hrec;
    char buf[MAX_PATH];
    const char *sql;
    unsigned size;
    unsigned r;

    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `One` ( `A` INT PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "SELECT * FROM `One`";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    sql = "SELECT * FROM `_Tables` WHERE `Name` = 'One'";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "One"), "Expected \"One\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);
    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    sql = "SELECT * FROM `_Columns` WHERE `Table` = 'One'";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "One"), "Expected \"One\", got \"%s\"\n", buf);

    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 1, "Expected 1, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 3, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "A"), "Expected \"A\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS,
       "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    sql = "DROP `One`";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "DROP TABLE";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "DROP TABLE `One`";
    hquery = 0;
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_FUNCTION_FAILED,
       "Expected ERROR_FUNCTION_FAILED, got %d\n", r);

    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    sql = "SELECT * FROM `IDontExist`";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "SELECT * FROM `One`";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "CREATE TABLE `One` ( `A` INT PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "DROP TABLE One";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "SELECT * FROM `One`";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "SELECT * FROM `_Tables` WHERE `Name` = 'One'";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    sql = "SELECT * FROM `_Columns` WHERE `Table` = 'One'";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    sql = "CREATE TABLE `One` ( `B` INT, `C` INT PRIMARY KEY `B` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "SELECT * FROM `One`";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    sql = "SELECT * FROM `_Tables` WHERE `Name` = 'One'";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "One"), "Expected \"One\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);
    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    sql = "SELECT * FROM `_Columns` WHERE `Table` = 'One'";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "One"), "Expected \"One\", got \"%s\"\n", buf);

    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 1, "Expected 1, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 3, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "B"), "Expected \"B\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "One"), "Expected \"One\", got \"%s\"\n", buf);

    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 2, "Expected 2, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 3, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "C"), "Expected \"C\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS,
       "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    sql = "DROP TABLE One";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "SELECT * FROM `One`";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "SELECT * FROM `_Tables` WHERE `Name` = 'One'";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    sql = "SELECT * FROM `_Columns` WHERE `Table` = 'One'";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

static void test_dbmerge(void)
{
    LibmsiObject *hdb;
    LibmsiObject *href;
    LibmsiObject *hquery;
    LibmsiObject *hrec;
    char buf[MAX_PATH];
    const char *sql;
    unsigned size;
    unsigned r;

    r = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiOpenDatabase("refdb.msi", LIBMSI_DB_OPEN_CREATE, &href);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* hDatabase is invalid */
    r = MsiDatabaseMerge(0, href, "MergeErrors");
    ok(r == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", r);

    /* hDatabaseMerge is invalid */
    r = MsiDatabaseMerge(hdb, 0, "MergeErrors");
    ok(r == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", r);

    /* szTableName is NULL */
    r = MsiDatabaseMerge(hdb, href, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* szTableName is empty */
    r = MsiDatabaseMerge(hdb, href, "");
    ok(r == ERROR_INVALID_TABLE, "Expected ERROR_INVALID_TABLE, got %d\n", r);

    /* both DBs empty, szTableName is valid */
    r = MsiDatabaseMerge(hdb, href, "MergeErrors");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `One` ( `A` INT PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `One` ( `A` CHAR(72) PRIMARY KEY `A` )";
    r = run_query(href, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* column types don't match */
    r = MsiDatabaseMerge(hdb, href, "MergeErrors");
    ok(r == ERROR_DATATYPE_MISMATCH,
       "Expected ERROR_DATATYPE_MISMATCH, got %d\n", r);

    /* nothing in MergeErrors */
    sql = "SELECT * FROM `MergeErrors`";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "DROP TABLE `One`";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "DROP TABLE `One`";
    r = run_query(href, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `One` ( "
        "`A` CHAR(72), "
        "`B` CHAR(56), "
        "`C` CHAR(64) LOCALIZABLE, "
        "`D` LONGCHAR, "
        "`E` CHAR(72) NOT NULL, "
        "`F` CHAR(56) NOT NULL, "
        "`G` CHAR(64) NOT NULL LOCALIZABLE, "
        "`H` LONGCHAR NOT NULL "
        "PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `One` ( "
        "`A` CHAR(64), "
        "`B` CHAR(64), "
        "`C` CHAR(64), "
        "`D` CHAR(64), "
        "`E` CHAR(64) NOT NULL, "
        "`F` CHAR(64) NOT NULL, "
        "`G` CHAR(64) NOT NULL, "
        "`H` CHAR(64) NOT NULL "
        "PRIMARY KEY `A` )";
    r = run_query(href, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* column sting types don't match exactly */
    r = MsiDatabaseMerge(hdb, href, "MergeErrors");
    ok(r == ERROR_SUCCESS,
       "Expected ERROR_SUCCESS, got %d\n", r);

    /* nothing in MergeErrors */
    sql = "SELECT * FROM `MergeErrors`";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "DROP TABLE `One`";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "DROP TABLE `One`";
    r = run_query(href, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `One` ( `A` INT, `B` INT PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `One` ( `A` INT, `C` INT PRIMARY KEY `A` )";
    r = run_query(href, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* column names don't match */
    r = MsiDatabaseMerge(hdb, href, "MergeErrors");
    ok(r == ERROR_DATATYPE_MISMATCH,
       "Expected ERROR_DATATYPE_MISMATCH, got %d\n", r);

    /* nothing in MergeErrors */
    sql = "SELECT * FROM `MergeErrors`";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "DROP TABLE `One`";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "DROP TABLE `One`";
    r = run_query(href, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `One` ( `A` INT, `B` INT PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `One` ( `A` INT, `B` INT PRIMARY KEY `B` )";
    r = run_query(href, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* primary keys don't match */
    r = MsiDatabaseMerge(hdb, href, "MergeErrors");
    ok(r == ERROR_DATATYPE_MISMATCH,
       "Expected ERROR_DATATYPE_MISMATCH, got %d\n", r);

    /* nothing in MergeErrors */
    sql = "SELECT * FROM `MergeErrors`";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "DROP TABLE `One`";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "DROP TABLE `One`";
    r = run_query(href, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `One` ( `A` INT, `B` INT PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `One` ( `A` INT, `B` INT PRIMARY KEY `A`, `B` )";
    r = run_query(href, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* number of primary keys doesn't match */
    r = MsiDatabaseMerge(hdb, href, "MergeErrors");
    ok(r == ERROR_DATATYPE_MISMATCH,
       "Expected ERROR_DATATYPE_MISMATCH, got %d\n", r);

    /* nothing in MergeErrors */
    sql = "SELECT * FROM `MergeErrors`";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "DROP TABLE `One`";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "DROP TABLE `One`";
    r = run_query(href, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `One` ( `A` INT, `B` INT, `C` INT PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `One` ( `A` INT, `B` INT PRIMARY KEY `A` )";
    r = run_query(href, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `One` ( `A`, `B` ) VALUES ( 1, 2 )";
    r = run_query(href, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* number of columns doesn't match */
    r = MsiDatabaseMerge(hdb, href, "MergeErrors");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "SELECT * FROM `One`";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 1, "Expected 1, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 2, "Expected 2, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 3);
    ok(r == MSI_NULL_INTEGER, "Expected MSI_NULL_INTEGER, got %d\n", r);

    MsiCloseHandle(hrec);

    /* nothing in MergeErrors */
    sql = "SELECT * FROM `MergeErrors`";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "DROP TABLE `One`";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "DROP TABLE `One`";
    r = run_query(href, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `One` ( `A` INT, `B` INT PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `One` ( `A` INT, `B` INT, `C` INT PRIMARY KEY `A` )";
    r = run_query(href, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `One` ( `A`, `B`, `C` ) VALUES ( 1, 2, 3 )";
    r = run_query(href, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* number of columns doesn't match */
    r = MsiDatabaseMerge(hdb, href, "MergeErrors");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "SELECT * FROM `One`";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 1, "Expected 1, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 2, "Expected 2, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 3);
    ok(r == MSI_NULL_INTEGER, "Expected MSI_NULL_INTEGER, got %d\n", r);

    MsiCloseHandle(hrec);

    /* nothing in MergeErrors */
    sql = "SELECT * FROM `MergeErrors`";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "DROP TABLE `One`";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "DROP TABLE `One`";
    r = run_query(href, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `One` ( `A` INT, `B` INT PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `One` ( `A`, `B` ) VALUES ( 1, 1 )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `One` ( `A`, `B` ) VALUES ( 2, 2 )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `One` ( `A` INT, `B` INT PRIMARY KEY `A` )";
    r = run_query(href, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `One` ( `A`, `B` ) VALUES ( 1, 2 )";
    r = run_query(href, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `One` ( `A`, `B` ) VALUES ( 2, 3 )";
    r = run_query(href, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* primary keys match, rows do not */
    r = MsiDatabaseMerge(hdb, href, "MergeErrors");
    ok(r == ERROR_FUNCTION_FAILED,
       "Expected ERROR_FUNCTION_FAILED, got %d\n", r);

    /* nothing in MergeErrors */
    sql = "SELECT * FROM `MergeErrors`";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "One"), "Expected \"One\", got \"%s\"\n", buf);

    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 2, "Expected 2, got %d\n", r);

    MsiCloseHandle(hrec);

    r = MsiDatabaseOpenQuery(hdb, "SELECT * FROM `MergeErrors`", &hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    hrec = NULL;
    r = MsiQueryGetColumnInfo(hquery, LIBMSI_COL_INFO_NAMES, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "Table"), "Expected \"Table\", got \"%s\"\n", buf);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 2, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "NumRowMergeConflicts"),
       "Expected \"NumRowMergeConflicts\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);

    hrec = NULL;
    r = MsiQueryGetColumnInfo(hquery, LIBMSI_COL_INFO_TYPES, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "s255"), "Expected \"s255\", got \"%s\"\n", buf);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 2, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "i2"), "Expected \"i2\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);
    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    sql = "DROP TABLE `MergeErrors`";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "DROP TABLE `One`";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "DROP TABLE `One`";
    r = run_query(href, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `One` ( `A` INT, `B` CHAR(72) PRIMARY KEY `A` )";
    r = run_query(href, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `One` ( `A`, `B` ) VALUES ( 1, 'hi' )";
    r = run_query(href, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* table from merged database is not in target database */
    r = MsiDatabaseMerge(hdb, href, "MergeErrors");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "SELECT * FROM `One`";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 1, "Expected 1, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 2, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "hi"), "Expected \"hi\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);

    /* nothing in MergeErrors */
    sql = "SELECT * FROM `MergeErrors`";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "DROP TABLE `One`";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "DROP TABLE `One`";
    r = run_query(href, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `One` ( "
            "`A` CHAR(72), `B` INT PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `One` ( "
            "`A` CHAR(72), `B` INT PRIMARY KEY `A` )";
    r = run_query(href, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `One` ( `A`, `B` ) VALUES ( 'hi', 1 )";
    r = run_query(href, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* primary key is string */
    r = MsiDatabaseMerge(hdb, href, "MergeErrors");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "SELECT * FROM `One`";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "hi"), "Expected \"hi\", got \"%s\"\n", buf);

    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 1, "Expected 1, got %d\n", r);

    MsiCloseHandle(hrec);

    /* nothing in MergeErrors */
    sql = "SELECT * FROM `MergeErrors`";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    create_file_data("codepage.idt", "\r\n\r\n850\t_ForceCodepage\r\n", 0);

    GetCurrentDirectoryA(MAX_PATH, buf);
    r = MsiDatabaseImport(hdb, buf, "codepage.idt");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "DROP TABLE `One`";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "DROP TABLE `One`";
    r = run_query(href, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `One` ( "
            "`A` INT, `B` CHAR(72) LOCALIZABLE PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `One` ( "
            "`A` INT, `B` CHAR(72) LOCALIZABLE PRIMARY KEY `A` )";
    r = run_query(href, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `One` ( `A`, `B` ) VALUES ( 1, 'hi' )";
    r = run_query(href, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* code page does not match */
    r = MsiDatabaseMerge(hdb, href, "MergeErrors");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "SELECT * FROM `One`";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 1, "Expected 1, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 2, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "hi"), "Expected \"hi\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);

    /* nothing in MergeErrors */
    sql = "SELECT * FROM `MergeErrors`";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "DROP TABLE `One`";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "DROP TABLE `One`";
    r = run_query(href, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `One` ( `A` INT, `B` OBJECT PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `One` ( `A` INT, `B` OBJECT PRIMARY KEY `A` )";
    r = run_query(href, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    create_file("binary.dat");
    hrec = MsiCreateRecord(1);
    MsiRecordSetStream(hrec, 1, "binary.dat");

    sql = "INSERT INTO `One` ( `A`, `B` ) VALUES ( 1, ? )";
    r = run_query(href, hrec, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(hrec);

    /* binary data to merge */
    r = MsiDatabaseMerge(hdb, href, "MergeErrors");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "SELECT * FROM `One`";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 1, "Expected 1, got %d\n", r);

    size = MAX_PATH;
    ZeroMemory(buf, MAX_PATH);
    r = MsiRecordReadStream(hrec, 2, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "binary.dat\n"),
       "Expected \"binary.dat\\n\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);

    /* nothing in MergeErrors */
    sql = "SELECT * FROM `MergeErrors`";
    r = do_query(hdb, sql, &hrec);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "DROP TABLE `One`";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "DROP TABLE `One`";
    r = run_query(href, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `One` ( `A` INT, `B` CHAR(72) PRIMARY KEY `A` )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = run_query(href, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `One` ( `A`, `B` ) VALUES ( 1, 'foo' )";
    r = run_query(href, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `One` ( `A`, `B` ) VALUES ( 2, 'bar' )";
    r = run_query(href, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiDatabaseMerge(hdb, href, "MergeErrors");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "SELECT * FROM `One`";
    r = MsiDatabaseOpenQuery(hdb, sql, &hquery);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(hquery, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 1, "Expected 1, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 2, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "foo"), "Expected \"foo\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 2, "Expected 2, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 2, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp(buf, "bar"), "Expected \"bar\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);

    r = MsiQueryFetch(hquery, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS,
       "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiQueryClose(hquery);
    MsiCloseHandle(hquery);

    MsiCloseHandle(hdb);
    MsiCloseHandle(href);
    DeleteFileA(msifile);
    DeleteFileA("refdb.msi");
    DeleteFileA("codepage.idt");
    DeleteFileA("binary.dat");
}

static void test_select_with_tablenames(void)
{
    LibmsiObject *hdb;
    LibmsiObject *query;
    LibmsiObject *rec;
    const char *sql;
    unsigned r;
    int i;

    int vals[4][2] = {
        {1,12},
        {4,12},
        {1,15},
        {4,15}};

    hdb = create_db();
    ok(hdb, "failed to create db\n");

    /* Build a pair of tables with the same column names, but unique data */
    sql = "CREATE TABLE `T1` ( `A` SHORT, `B` SHORT PRIMARY KEY `A`)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `T1` ( `A`, `B` ) VALUES ( 1, 2 )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `T1` ( `A`, `B` ) VALUES ( 4, 5 )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "CREATE TABLE `T2` ( `A` SHORT, `B` SHORT PRIMARY KEY `A`)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `T2` ( `A`, `B` ) VALUES ( 11, 12 )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `T2` ( `A`, `B` ) VALUES ( 14, 15 )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);


    /* Test that selection based on prefixing the column with the table
     * actually selects the right data */

    query = NULL;
    sql = "SELECT T1.A, T2.B FROM T1,T2";
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(query, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    for (i = 0; i < 4; i++)
    {
        r = MsiQueryFetch(query, &rec);
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

        r = MsiRecordGetInteger(rec, 1);
        ok(r == vals[i][0], "Expected %d, got %d\n", vals[i][0], r);

        r = MsiRecordGetInteger(rec, 2);
        ok(r == vals[i][1], "Expected %d, got %d\n", vals[i][1], r);

        MsiCloseHandle(rec);
    }

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiQueryClose(query);
    MsiCloseHandle(query);
    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

static const unsigned ordervals[6][3] =
{
    { MSI_NULL_INTEGER, 12, 13 },
    { 1, 2, 3 },
    { 6, 4, 5 },
    { 8, 9, 7 },
    { 10, 11, MSI_NULL_INTEGER },
    { 14, MSI_NULL_INTEGER, 15 }
};

static void test_insertorder(void)
{
    LibmsiObject *hdb;
    LibmsiObject *query;
    LibmsiObject *rec;
    const char *sql;
    unsigned r;
    int i;

    hdb = create_db();
    ok(hdb, "failed to create db\n");

    sql = "CREATE TABLE `T` ( `A` SHORT, `B` SHORT, `C` SHORT PRIMARY KEY `A`)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `T` ( `A`, `B`, `C` ) VALUES ( 1, 2, 3 )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `T` ( `B`, `C`, `A` ) VALUES ( 4, 5, 6 )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `T` ( `C`, `A`, `B` ) VALUES ( 7, 8, 9 )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `T` ( `A`, `B` ) VALUES ( 10, 11 )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `T` ( `B`, `C` ) VALUES ( 12, 13 )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* fails because the primary key already
     * has an MSI_NULL_INTEGER value set above
     */
    sql = "INSERT INTO `T` ( `C` ) VALUES ( 14 )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_FUNCTION_FAILED,
       "Expected ERROR_FUNCTION_FAILED, got %d\n", r);

    /* replicate the error where primary key is set twice */
    sql = "INSERT INTO `T` ( `A`, `C` ) VALUES ( 1, 14 )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_FUNCTION_FAILED,
       "Expected ERROR_FUNCTION_FAILED, got %d\n", r);

    sql = "INSERT INTO `T` ( `A`, `C` ) VALUES ( 14, 15 )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `T` VALUES ( 16 )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "INSERT INTO `T` VALUES ( 17, 18 )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    sql = "INSERT INTO `T` VALUES ( 19, 20, 21 )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = NULL;
    sql = "SELECT * FROM `T`";
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(query, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    for (i = 0; i < 6; i++)
    {
        r = MsiQueryFetch(query, &rec);
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

        r = MsiRecordGetInteger(rec, 1);
        ok(r == ordervals[i][0], "Expected %d, got %d\n", ordervals[i][0], r);

        r = MsiRecordGetInteger(rec, 2);
        ok(r == ordervals[i][1], "Expected %d, got %d\n", ordervals[i][1], r);

        r = MsiRecordGetInteger(rec, 3);
        ok(r == ordervals[i][2], "Expected %d, got %d\n", ordervals[i][2], r);

        MsiCloseHandle(rec);
    }

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiQueryClose(query);
    MsiCloseHandle(query);

    sql = "DELETE FROM `T` WHERE `A` IS NULL";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "INSERT INTO `T` ( `B`, `C` ) VALUES ( 12, 13 ) TEMPORARY";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = NULL;
    sql = "SELECT * FROM `T`";
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(query, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    for (i = 0; i < 6; i++)
    {
        r = MsiQueryFetch(query, &rec);
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

        r = MsiRecordGetInteger(rec, 1);
        ok(r == ordervals[i][0], "Expected %d, got %d\n", ordervals[i][0], r);

        r = MsiRecordGetInteger(rec, 2);
        ok(r == ordervals[i][1], "Expected %d, got %d\n", ordervals[i][1], r);

        r = MsiRecordGetInteger(rec, 3);
        ok(r == ordervals[i][2], "Expected %d, got %d\n", ordervals[i][2], r);

        MsiCloseHandle(rec);
    }

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiQueryClose(query);
    MsiCloseHandle(query);
    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

static void test_columnorder(void)
{
    LibmsiObject *hdb;
    LibmsiObject *query;
    LibmsiObject *rec;
    char buf[MAX_PATH];
    const char *sql;
    unsigned sz;
    unsigned r;

    hdb = create_db();
    ok(hdb, "failed to create db\n");

    /* Each column is a slot:
     * ---------------------
     * | B | C | A | E | D |
     * ---------------------
     *
     * When a column is selected as a primary key,
     * the column occupying the nth primary key slot is swapped
     * with the current position of the primary key in question:
     *
     * set primary key `D`
     * ---------------------    ---------------------
     * | B | C | A | E | D | -> | D | C | A | E | B |
     * ---------------------    ---------------------
     *
     * set primary key `E`
     * ---------------------    ---------------------
     * | D | C | A | E | B | -> | D | E | A | C | B |
     * ---------------------    ---------------------
     */

    sql = "CREATE TABLE `T` ( `B` SHORT NOT NULL, `C` SHORT NOT NULL, "
            "`A` CHAR(255), `E` INT, `D` CHAR(255) NOT NULL "
            "PRIMARY KEY `D`, `E`)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = NULL;
    sql = "SELECT * FROM `T`";
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    rec = NULL;
    r = MsiQueryGetColumnInfo(query, LIBMSI_COL_INFO_TYPES, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 1, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("s255", buf), "Expected \"s255\", got \"%s\"\n", buf);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 2, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("I2", buf), "Expected \"I2\", got \"%s\"\n", buf);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 3, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("S255", buf), "Expected \"S255\", got \"%s\"\n", buf);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 4, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("i2", buf), "Expected \"i2\", got \"%s\"\n", buf);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 5, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("i2", buf), "Expected \"i2\", got \"%s\"\n", buf);

    MsiCloseHandle(rec);

    rec = NULL;
    r = MsiQueryGetColumnInfo(query, LIBMSI_COL_INFO_NAMES, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 1, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("D", buf), "Expected \"D\", got \"%s\"\n", buf);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 2, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("E", buf), "Expected \"E\", got \"%s\"\n", buf);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 3, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("A", buf), "Expected \"A\", got \"%s\"\n", buf);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 4, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("C", buf), "Expected \"C\", got \"%s\"\n", buf);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 5, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("B", buf), "Expected \"B\", got \"%s\"\n", buf);

    MsiCloseHandle(rec);
    MsiQueryClose(query);
    MsiCloseHandle(query);

    sql = "INSERT INTO `T` ( `B`, `C`, `A`, `E`, `D` ) "
            "VALUES ( 1, 2, 'a', 3, 'bc' )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "SELECT * FROM `T`";
    r = do_query(hdb, sql, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 1, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("bc", buf), "Expected \"bc\", got \"%s\"\n", buf);

    r = MsiRecordGetInteger(rec, 2);
    ok(r == 3, "Expected 3, got %d\n", r);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 3, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("a", buf), "Expected \"a\", got \"%s\"\n", buf);

    r = MsiRecordGetInteger(rec, 4);
    ok(r == 2, "Expected 2, got %d\n", r);

    r = MsiRecordGetInteger(rec, 5);
    ok(r == 1, "Expected 1, got %d\n", r);

    MsiCloseHandle(rec);

    query = NULL;
    sql = "SELECT * FROM `_Columns` WHERE `Table` = 'T'";
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(query, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 1, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("T", buf), "Expected \"T\", got \"%s\"\n", buf);

    r = MsiRecordGetInteger(rec, 2);
    ok(r == 1, "Expected 1, got %d\n", r);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 3, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("D", buf), "Expected \"D\", got \"%s\"\n", buf);

    MsiCloseHandle(rec);

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 1, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("T", buf), "Expected \"T\", got \"%s\"\n", buf);

    r = MsiRecordGetInteger(rec, 2);
    ok(r == 2, "Expected 2, got %d\n", r);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 3, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("E", buf), "Expected \"E\", got \"%s\"\n", buf);

    MsiCloseHandle(rec);

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 1, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("T", buf), "Expected \"T\", got \"%s\"\n", buf);

    r = MsiRecordGetInteger(rec, 2);
    ok(r == 3, "Expected 3, got %d\n", r);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 3, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("A", buf), "Expected \"A\", got \"%s\"\n", buf);

    MsiCloseHandle(rec);

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 1, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("T", buf), "Expected \"T\", got \"%s\"\n", buf);

    r = MsiRecordGetInteger(rec, 2);
    ok(r == 4, "Expected 4, got %d\n", r);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 3, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("C", buf), "Expected \"C\", got \"%s\"\n", buf);

    MsiCloseHandle(rec);

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 1, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("T", buf), "Expected \"T\", got \"%s\"\n", buf);

    r = MsiRecordGetInteger(rec, 2);
    ok(r == 5, "Expected 5, got %d\n", r);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 3, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("B", buf), "Expected \"B\", got \"%s\"\n", buf);

    MsiCloseHandle(rec);

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiQueryClose(query);
    MsiCloseHandle(query);

    sql = "CREATE TABLE `Z` ( `B` SHORT NOT NULL, `C` SHORT NOT NULL, "
            "`A` CHAR(255), `E` INT, `D` CHAR(255) NOT NULL "
            "PRIMARY KEY `C`, `A`, `D`)";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = NULL;
    sql = "SELECT * FROM `Z`";
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    rec = NULL;
    r = MsiQueryGetColumnInfo(query, LIBMSI_COL_INFO_TYPES, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 1, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("i2", buf), "Expected \"i2\", got \"%s\"\n", buf);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 2, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("S255", buf), "Expected \"S255\", got \"%s\"\n", buf);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 3, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("s255", buf), "Expected \"s255\", got \"%s\"\n", buf);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 4, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("I2", buf), "Expected \"I2\", got \"%s\"\n", buf);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 5, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("i2", buf), "Expected \"i2\", got \"%s\"\n", buf);

    MsiCloseHandle(rec);

    rec = NULL;
    r = MsiQueryGetColumnInfo(query, LIBMSI_COL_INFO_NAMES, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 1, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("C", buf), "Expected \"C\", got \"%s\"\n", buf);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 2, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("A", buf), "Expected \"A\", got \"%s\"\n", buf);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 3, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("D", buf), "Expected \"D\", got \"%s\"\n", buf);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 4, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("E", buf), "Expected \"E\", got \"%s\"\n", buf);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 5, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("B", buf), "Expected \"B\", got \"%s\"\n", buf);

    MsiCloseHandle(rec);
    MsiQueryClose(query);
    MsiCloseHandle(query);

    sql = "INSERT INTO `Z` ( `B`, `C`, `A`, `E`, `D` ) "
            "VALUES ( 1, 2, 'a', 3, 'bc' )";
    r = run_query(hdb, 0, sql);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sql = "SELECT * FROM `Z`";
    r = do_query(hdb, sql, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetInteger(rec, 1);
    ok(r == 2, "Expected 2, got %d\n", r);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 2, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("a", buf), "Expected \"a\", got \"%s\"\n", buf);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 3, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("bc", buf), "Expected \"bc\", got \"%s\"\n", buf);

    r = MsiRecordGetInteger(rec, 4);
    ok(r == 3, "Expected 3, got %d\n", r);

    r = MsiRecordGetInteger(rec, 5);
    ok(r == 1, "Expected 1, got %d\n", r);

    MsiCloseHandle(rec);

    query = NULL;
    sql = "SELECT * FROM `_Columns` WHERE `Table` = 'T'";
    r = MsiDatabaseOpenQuery(hdb, sql, &query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiQueryExecute(query, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 1, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("T", buf), "Expected \"T\", got \"%s\"\n", buf);

    r = MsiRecordGetInteger(rec, 2);
    ok(r == 1, "Expected 1, got %d\n", r);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 3, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("D", buf), "Expected \"D\", got \"%s\"\n", buf);

    MsiCloseHandle(rec);

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 1, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("T", buf), "Expected \"T\", got \"%s\"\n", buf);

    r = MsiRecordGetInteger(rec, 2);
    ok(r == 2, "Expected 2, got %d\n", r);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 3, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("E", buf), "Expected \"E\", got \"%s\"\n", buf);

    MsiCloseHandle(rec);

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 1, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("T", buf), "Expected \"T\", got \"%s\"\n", buf);

    r = MsiRecordGetInteger(rec, 2);
    ok(r == 3, "Expected 3, got %d\n", r);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 3, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("A", buf), "Expected \"A\", got \"%s\"\n", buf);

    MsiCloseHandle(rec);

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 1, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("T", buf), "Expected \"T\", got \"%s\"\n", buf);

    r = MsiRecordGetInteger(rec, 2);
    ok(r == 4, "Expected 4, got %d\n", r);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 3, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("C", buf), "Expected \"C\", got \"%s\"\n", buf);

    MsiCloseHandle(rec);

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 1, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("T", buf), "Expected \"T\", got \"%s\"\n", buf);

    r = MsiRecordGetInteger(rec, 2);
    ok(r == 5, "Expected 5, got %d\n", r);

    sz = MAX_PATH;
    strcpy(buf, "kiwi");
    r = MsiRecordGetString(rec, 3, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!strcmp("B", buf), "Expected \"B\", got \"%s\"\n", buf);

    MsiCloseHandle(rec);

    r = MsiQueryFetch(query, &rec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiQueryClose(query);
    MsiCloseHandle(query);

    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

static void test_createtable(void)
{
    LibmsiObject *hdb;
    LibmsiObject *htab = 0;
    LibmsiObject *hrec = 0;
    const char *sql;
    unsigned res;
    unsigned size;
    char buffer[0x20];

    hdb = create_db();
    ok(hdb, "failed to create db\n");

    sql = "CREATE TABLE `blah` (`foo` CHAR(72) NOT NULL PRIMARY KEY `foo`)";
    res = MsiDatabaseOpenQuery( hdb, sql, &htab );
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    if(res == ERROR_SUCCESS )
    {
        res = MsiQueryExecute( htab, hrec );
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

        hrec = NULL;
        res = MsiQueryGetColumnInfo( htab, LIBMSI_COL_INFO_NAMES, &hrec );
        todo_wine ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

        size = sizeof(buffer);
        res = MsiRecordGetString(hrec, 1, buffer, &size );
        todo_wine ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
        MsiCloseHandle( hrec );

        res = MsiQueryClose( htab );
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

        res = MsiCloseHandle( htab );
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    }

    sql = "CREATE TABLE `a` (`b` INT PRIMARY KEY `b`)";
    res = MsiDatabaseOpenQuery( hdb, sql, &htab );
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    if(res == ERROR_SUCCESS )
    {
        res = MsiQueryExecute( htab, 0 );
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

        res = MsiQueryClose( htab );
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

        res = MsiCloseHandle( htab );
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

        sql = "SELECT * FROM `a`";
        res = MsiDatabaseOpenQuery( hdb, sql, &htab );
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

        hrec = NULL;
        res = MsiQueryGetColumnInfo( htab, LIBMSI_COL_INFO_NAMES, &hrec );
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

        buffer[0] = 0;
        size = sizeof(buffer);
        res = MsiRecordGetString(hrec, 1, buffer, &size );
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
        ok(!strcmp(buffer,"b"), "b != %s\n", buffer);
        MsiCloseHandle( hrec );

        res = MsiQueryClose( htab );
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

        res = MsiCloseHandle( htab );
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

        res = MsiDatabaseCommit(hdb);
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

        res = MsiCloseHandle(hdb);
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

        res = MsiOpenDatabase(msifile, LIBMSI_DB_OPEN_TRANSACT, &hdb );
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

        sql = "SELECT * FROM `a`";
        res = MsiDatabaseOpenQuery( hdb, sql, &htab );
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

        hrec = NULL;
        res = MsiQueryGetColumnInfo( htab, LIBMSI_COL_INFO_NAMES, &hrec );
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

        buffer[0] = 0;
        size = sizeof(buffer);
        res = MsiRecordGetString(hrec, 1, buffer, &size );
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
        ok(!strcmp(buffer,"b"), "b != %s\n", buffer);

        res = MsiCloseHandle( hrec );
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

        res = MsiQueryClose( htab );
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

        res = MsiCloseHandle( htab );
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    }

    res = MsiDatabaseCommit(hdb);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    res = MsiCloseHandle(hdb);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    DeleteFileA(msifile);
}

static void test_embedded_nulls(void)
{
    static const char control_table[] =
        "Dialog\tText\n"
        "s72\tL0\n"
        "Control\tDialog\n"
        "LicenseAgreementDlg\ttext\x11\x19text\0text";
    unsigned r, sz;
    LibmsiObject *hdb;
    LibmsiObject *hrec;
    char buffer[32];

    r = MsiOpenDatabase( msifile, LIBMSI_DB_OPEN_CREATE, &hdb );
    ok( r == ERROR_SUCCESS, "failed to open database %u\n", r );

    GetCurrentDirectoryA( MAX_PATH, CURR_DIR );
    write_file( "temp_file", control_table, sizeof(control_table) );
    r = MsiDatabaseImport( hdb, CURR_DIR, "temp_file" );
    ok( r == ERROR_SUCCESS, "failed to import table %u\n", r );
    DeleteFileA( "temp_file" );

    r = do_query( hdb, "SELECT `Text` FROM `Control` WHERE `Dialog` = 'LicenseAgreementDlg'", &hrec );
    ok( r == ERROR_SUCCESS, "query failed %u\n", r );

    buffer[0] = 0;
    sz = sizeof(buffer);
    r = MsiRecordGetString( hrec, 1, buffer, &sz );
    ok( r == ERROR_SUCCESS, "failed to get string %u\n", r );
    ok( !memcmp( "text\r\ntext\ntext", buffer, sizeof("text\r\ntext\ntext") - 1 ), "wrong buffer contents \"%s\"\n", buffer );

    MsiCloseHandle( hrec );
    MsiCloseHandle( hdb );
    DeleteFileA( msifile );
}

static void test_select_column_names(void)
{
    LibmsiObject *hdb = 0;
    LibmsiObject *rec;
    LibmsiObject *rec2;
    LibmsiObject *query;
    char buffer[32];
    unsigned r, size;

    DeleteFile(msifile);

    r = MsiOpenDatabase( msifile, LIBMSI_DB_OPEN_CREATE, &hdb );
    ok( r == ERROR_SUCCESS , "failed to open database: %u\n", r );

    r = try_query( hdb, "CREATE TABLE `t` (`a` CHAR NOT NULL, `b` CHAR PRIMARY KEY `a`)");
    ok( r == ERROR_SUCCESS , "query failed: %u\n", r );

    r = try_query( hdb, "SELECT `t`.`b` FROM `t` WHERE `t`.`b` = `x`" );
    ok( r == ERROR_BAD_QUERY_SYNTAX, "query failed: %u\n", r );

    r = try_query( hdb, "SELECT '', `t`.`b` FROM `t` WHERE `t`.`b` = 'x'" );
    ok( r == ERROR_SUCCESS , "query failed: %u\n", r );

    r = try_query( hdb, "SELECT *, `t`.`b` FROM `t` WHERE `t`.`b` = 'x'" );
    todo_wine ok( r == ERROR_SUCCESS, "query failed: %u\n", r );

    r = try_query( hdb, "SELECT 'b', `t`.`b` FROM `t` WHERE `t`.`b` = 'x'" );
    ok( r == ERROR_SUCCESS , "query failed: %u\n", r );

    r = try_query( hdb, "SELECT `t`.`b`, '' FROM `t` WHERE `t`.`b` = 'x'" );
    ok( r == ERROR_SUCCESS , "query failed: %u\n", r );

    r = try_query( hdb, "SELECT `t`.`b`, '' FROM `t` WHERE `t`.`b` = 'x' ORDER BY `b`" );
    ok( r == ERROR_SUCCESS , "query failed: %u\n", r );

    r = try_query( hdb, "SELECT `t`.`b`, '' FROM `t` WHERE `t`.`b` = 'x' ORDER BY 'b'" );
    ok( r == ERROR_BAD_QUERY_SYNTAX, "query failed: %u\n", r );

    r = try_query( hdb, "SELECT 't'.'b' FROM `t` WHERE `t`.`b` = 'x'" );
    ok( r == ERROR_BAD_QUERY_SYNTAX, "query failed: %u\n", r );

    r = try_query( hdb, "SELECT 'b' FROM `t` WHERE `t`.`b` = 'x'" );
    ok( r == ERROR_SUCCESS , "query failed: %u\n", r );

    r = try_query( hdb, "INSERT INTO `t` ( `a`, `b` ) VALUES( '1', '2' )" );
    ok( r == ERROR_SUCCESS , "query failed: %u\n", r );

    r = try_query( hdb, "INSERT INTO `t` ( `a`, `b` ) VALUES( '3', '4' )" );
    ok( r == ERROR_SUCCESS , "query failed: %u\n", r );

    query = NULL;
    r = MsiDatabaseOpenQuery( hdb, "SELECT '' FROM `t`", &query );
    ok( r == ERROR_SUCCESS, "failed to open database query: %u\n", r );

    r = MsiQueryExecute( query, 0 );
    ok( r == ERROR_SUCCESS, "failed to execute query: %u\n", r );

    r = MsiQueryFetch( query, &rec );
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    r = MsiRecordGetFieldCount( rec );
    ok( r == 1, "got %u\n",  r );

    rec2 = NULL;
    r = MsiQueryGetColumnInfo( query, LIBMSI_COL_INFO_NAMES, &rec2 );
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    r = MsiRecordGetFieldCount( rec2 );
    ok( r == 1, "got %u\n",  r );
    size = sizeof(buffer);
    memset( buffer, 0x55, sizeof(buffer) );
    r = MsiRecordGetString( rec2, 1, buffer, &size );
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    ok( !buffer[0], "got \"%s\"\n", buffer );
    MsiCloseHandle( rec2 );

    rec2 = NULL;
    r = MsiQueryGetColumnInfo( query, LIBMSI_COL_INFO_TYPES, &rec2 );
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    r = MsiRecordGetFieldCount( rec2 );
    ok( r == 1, "got %u\n",  r );
    size = sizeof(buffer);
    memset( buffer, 0x55, sizeof(buffer) );
    r = MsiRecordGetString( rec2, 1, buffer, &size );
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    ok( !strcmp( buffer, "f0" ), "got \"%s\"\n", buffer );
    MsiCloseHandle( rec2 );

    size = sizeof(buffer);
    memset( buffer, 0x55, sizeof(buffer) );
    r = MsiRecordGetString( rec, 1, buffer, &size );
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    ok( !buffer[0], "got \"%s\"\n", buffer );
    MsiCloseHandle( rec );

    r = MsiQueryFetch( query, &rec );
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    size = sizeof(buffer);
    memset( buffer, 0x55, sizeof(buffer) );
    r = MsiRecordGetString( rec, 1, buffer, &size );
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    ok( !buffer[0], "got \"%s\"\n", buffer );
    MsiCloseHandle( rec );

    r = MsiQueryFetch( query, &rec );
    ok( r == ERROR_NO_MORE_ITEMS, "unexpected result: %u\n", r );
    MsiCloseHandle( rec );

    MsiQueryClose( query );
    MsiCloseHandle( query );

    query = NULL;
    r = MsiDatabaseOpenQuery( hdb, "SELECT `a`, '' FROM `t`", &query );
    ok( r == ERROR_SUCCESS, "failed to open database query: %u\n", r );

    r = MsiQueryExecute( query, 0 );
    ok( r == ERROR_SUCCESS, "failed to execute query: %u\n", r );

    r = MsiQueryFetch( query, &rec );
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    r = MsiRecordGetFieldCount( rec );
    ok( r == 2, "got %u\n",  r );
    size = sizeof(buffer);
    memset( buffer, 0x55, sizeof(buffer) );
    r = MsiRecordGetString( rec, 1, buffer, &size );
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    ok( !strcmp( buffer, "1" ), "got \"%s\"\n", buffer );
    MsiCloseHandle( rec );

    r = MsiQueryFetch( query, &rec );
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    size = sizeof(buffer);
    memset( buffer, 0x55, sizeof(buffer) );
    r = MsiRecordGetString( rec, 2, buffer, &size );
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    ok( !buffer[0], "got \"%s\"\n", buffer );
    MsiCloseHandle( rec );

    r = MsiQueryFetch( query, &rec );
    ok( r == ERROR_NO_MORE_ITEMS, "unexpected result: %u\n", r );
    MsiCloseHandle( rec );

    MsiQueryClose( query );
    MsiCloseHandle( query );

    query = NULL;
    r = MsiDatabaseOpenQuery( hdb, "SELECT '', `a` FROM `t`", &query );
    ok( r == ERROR_SUCCESS, "failed to open database query: %u\n", r );

    r = MsiQueryExecute( query, 0 );
    ok( r == ERROR_SUCCESS, "failed to execute query: %u\n", r );

    r = MsiQueryFetch( query, &rec );
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    r = MsiRecordGetFieldCount( rec );
    ok( r == 2, "got %u\n",  r );
    size = sizeof(buffer);
    memset( buffer, 0x55, sizeof(buffer) );
    r = MsiRecordGetString( rec, 1, buffer, &size );
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    ok( !buffer[0], "got \"%s\"\n", buffer );
    size = sizeof(buffer);
    memset( buffer, 0x55, sizeof(buffer) );
    r = MsiRecordGetString( rec, 2, buffer, &size );
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    ok( !strcmp( buffer, "1" ), "got \"%s\"\n", buffer );
    MsiCloseHandle( rec );

    r = MsiQueryFetch( query, &rec );
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    size = sizeof(buffer);
    memset( buffer, 0x55, sizeof(buffer) );
    r = MsiRecordGetString( rec, 1, buffer, &size );
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    ok( !buffer[0], "got \"%s\"\n", buffer );
    size = sizeof(buffer);
    memset( buffer, 0x55, sizeof(buffer) );
    r = MsiRecordGetString( rec, 2, buffer, &size );
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    ok( !strcmp( buffer, "3" ), "got \"%s\"\n", buffer );
    MsiCloseHandle( rec );

    r = MsiQueryFetch( query, &rec );
    ok( r == ERROR_NO_MORE_ITEMS, "unexpected result: %u\n", r );
    MsiCloseHandle( rec );

    MsiQueryClose( query );
    MsiCloseHandle( query );

    query = NULL;
    r = MsiDatabaseOpenQuery( hdb, "SELECT `a`, '', `b` FROM `t`", &query );
    ok( r == ERROR_SUCCESS, "failed to open database query: %u\n", r );

    r = MsiQueryExecute( query, 0 );
    ok( r == ERROR_SUCCESS, "failed to execute query: %u\n", r );

    r = MsiQueryFetch( query, &rec );
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    r = MsiRecordGetFieldCount( rec );
    ok( r == 3, "got %u\n",  r );
    size = sizeof(buffer);
    memset( buffer, 0x55, sizeof(buffer) );
    r = MsiRecordGetString( rec, 1, buffer, &size );
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    ok( !strcmp( buffer, "1" ), "got \"%s\"\n", buffer );
    size = sizeof(buffer);
    memset( buffer, 0x55, sizeof(buffer) );
    r = MsiRecordGetString( rec, 2, buffer, &size );
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    ok( !buffer[0], "got \"%s\"\n", buffer );
    size = sizeof(buffer);
    memset( buffer, 0x55, sizeof(buffer) );
    r = MsiRecordGetString( rec, 3, buffer, &size );
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    ok( !strcmp( buffer, "2" ), "got \"%s\"\n", buffer );
    MsiCloseHandle( rec );

    r = MsiQueryFetch( query, &rec );
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    size = sizeof(buffer);
    memset( buffer, 0x55, sizeof(buffer) );
    r = MsiRecordGetString( rec, 1, buffer, &size );
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    ok( !strcmp( buffer, "3" ), "got \"%s\"\n", buffer );
    size = sizeof(buffer);
    memset( buffer, 0x55, sizeof(buffer) );
    r = MsiRecordGetString( rec, 2, buffer, &size );
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    ok( !buffer[0], "got \"%s\"\n", buffer );
    size = sizeof(buffer);
    memset( buffer, 0x55, sizeof(buffer) );
    r = MsiRecordGetString( rec, 3, buffer, &size );
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    ok( !strcmp( buffer, "4" ), "got \"%s\"\n", buffer );
    MsiCloseHandle( rec );

    r = MsiQueryFetch( query, &rec );
    ok( r == ERROR_NO_MORE_ITEMS, "unexpected result: %u\n", r );
    MsiCloseHandle( rec );

    MsiQueryClose( query );
    MsiCloseHandle( query );

    r = try_query( hdb, "SELECT '' FROM `t` WHERE `t`.`b` = 'x'" );
    ok( r == ERROR_SUCCESS , "query failed: %u\n", r );

    r = try_query( hdb, "SELECT `` FROM `t` WHERE `t`.`b` = 'x'" );
    todo_wine ok( r == ERROR_BAD_QUERY_SYNTAX, "query failed: %u\n", r );

    r = try_query( hdb, "SELECT `b` FROM 't' WHERE `t`.`b` = 'x'" );
    ok( r == ERROR_BAD_QUERY_SYNTAX, "query failed: %u\n", r );

    r = try_query( hdb, "SELECT `b` FROM `t` WHERE 'b' = 'x'" );
    ok( r == ERROR_BAD_QUERY_SYNTAX, "query failed: %u\n", r );

    r = try_query( hdb, "SELECT `t`.`b`, `` FROM `t` WHERE `t`.`b` = 'x'" );
    todo_wine ok( r == ERROR_BAD_QUERY_SYNTAX, "query failed: %u\n", r );

    r = MsiCloseHandle( hdb );
    ok(r == ERROR_SUCCESS , "failed to close database: %u\n", r);
}

void main()
{
    test_msidatabase();
    test_msiinsert();
    test_msibadqueries();
    test_querymodify();
    test_querygetcolumninfo();
    test_getcolinfo();
    test_msiexport();
    test_longstrings();
    test_streamtable();
    test_binary();
    test_where_not_in_selected();
    test_where();
    test_msiimport();
    test_binary_import();
    test_markers();
    test_handle_limit();
    test_try_transform();
    test_join();
    test_temporary_table();
    test_alter();
    test_integers();
    test_update();
    test_special_tables();
    test_tables_order();
    test_rows_order();
    test_select_markers();
    test_querymodify_update();
    test_querymodify_assign();
    test_stringtable();
    test_querymodify_delete();
    test_defaultdatabase();
    test_order();
    test_querymodify_delete_temporary();
    test_deleterow();
    test_quotes();
    test_carriagereturn();
    test_noquotes();
    test_forcecodepage();
    test_querymodify_refresh();
    test_where_querymodify();
    test_storages_table();
    test_droptable();
#if 0
    test_dbmerge();
#endif
    test_select_with_tablenames();
    test_insertorder();
    test_columnorder();
    test_suminfo_import();
    test_createtable();
    test_collation();
    test_embedded_nulls();
    test_select_column_names();
}
