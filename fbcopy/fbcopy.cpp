/*

Copyright (c) 2005-2007 Milan Babuskov
Copyright (c) 2005      Thiago Borges

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/
#ifdef IBPP_WINDOWS
#include <windows.h>
#define INT64FORMAT "%Li"
#define atoll(x) _atoi64(x)
#endif

#ifdef IBPP_LINUX
#define __cdecl /**/
#define INT64FORMAT "%lli"
#endif

#include "ibpp.h"

#ifdef HAS_HDRSTOP
#pragma hdrstop
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#include <string>
#include <sstream>
#include <list>
#include <algorithm>

#include "args.h"
#include "fbcopy.h"
//---------------------------------------------------------------------------------------
int FBCopy::Run(Args *a)
{
    ar = a;         // move to internal

    // error while parsing, or insufficient args
    if (ar->Error != "OK")
    {
        fprintf(stderr, "--------------------------------\n");
        fprintf(stderr, "FBCopy v%s by Milan Babuskov (mbabuskov@yahoo.com), using IBPP %d.%d.%d.%d\n",
                    FBCOPY_VERSION,
                    IBPP::Version >> 24,
                    (IBPP::Version >> 16) % 256,
                    (IBPP::Version >> 8) % 256,
                    IBPP::Version % 256);
        fprintf(stderr, "A command-line tool to copy and compare data in Firebird databases.\n\n");
        fprintf(stderr, "Usage: fbcopy {D|C|A|S|X}[UEKNFVH1234] {source} {destination}\n\n");

        fprintf(stderr, "Source and destination format is [user:password@][host:]database[?charset]\n\n");

        fprintf(stderr, "Options:\n");
        fprintf(stderr, "D  Define - outputs a definition of fields in format:\n");
        fprintf(stderr, "   table:common fields:missing fields:extra fields[:where clause|UID]\n");
        fprintf(stderr, "C  Copy - reads that definition from stdin and does the copying\n");
        fprintf(stderr, "S  Single step (define&copy all tables in single step)\n");
        fprintf(stderr, "A  Alter - outputs ALTER TABLE script for fields missing in destination\n");
        fprintf(stderr, "X  Compare data in tables (reads definition from stdin), optionally show:\n");
        fprintf(stderr, "   1 same rows, 2 missing rows, 3 extra rows, 4 different rows\n");
        fprintf(stderr, "E  Everything in single transaction (default = transaction per table)\n");
        fprintf(stderr, "U  if insert fails, try Update statement\n");
        fprintf(stderr, "K  Keep going (default = stop on first record that cannot be copied)\n");
        fprintf(stderr, "V  Verbose, show all errors with K option (default = off)\n");
        fprintf(stderr, "F  Fire triggers (default = temporary deactivate triggers)\n");
        fprintf(stderr, "N  Nulls - used with A. Doesn't put NOT NULL in ALTER TABLE statements\n");
        fprintf(stderr, "H  Html  - used with D, A, X. Outputs differences in HTML format\n");
        fprintf(stderr, "L  Limited - if table doesn't have row to display - don't show the table\n");
        fprintf(stderr, "Options are not case-sensitive.\n\n");

        if (ar->Error != "Display help")
            fprintf(stderr, "\nError: %s\n", ar->Error.c_str());
        return 1;
    }

    if (!connect(src, ar->Src) || !connect(dest, ar->Dest))
        return 2;

    int retval = 0;
    try
    {
        disableTriggers();
        trans1 = IBPP::TransactionFactory(src,  IBPP::amRead);
        trans2 = IBPP::TransactionFactory(dest, IBPP::amWrite);
        if (ar->SingleTransaction)
        {
            trans1->Start();
            trans2->Start();
        }

        if (ar->Operation == opCopy)
            setupFromStdin(ccCopy);
        else if (ar->Operation == opCompare)
            setupFromStdin(ccCompareData);
        else
            compare();

        if (ar->SingleTransaction)
        {
            trans1->Commit();
            trans2->Commit();
        }
    }
    catch (IBPP::Exception &e)
    {
        fprintf(stderr, "ERROR!\n");
        fprintf(stderr, "%s", e.ErrorMessage());
        retval = 3;
    }
    catch (...)
    {
        fprintf(stderr, "ERROR!\nA non-IBPP C++ runtime exception occured !\n\n");
        retval = 4;
    }
    enableTriggers();
    return retval;
}
//---------------------------------------------------------------------------------------
void FBCopy::disableTriggers()
{
    if (ar->FireTriggers || ar->Operation != opCopy && ar->Operation != opSingle)
        return;

    fprintf(stderr, "Disabling triggers...");
    IBPP::Transaction tr1 = IBPP::TransactionFactory(dest);
    tr1->Start();
    IBPP::Statement st1 = IBPP::StatementFactory(dest, tr1);
    st1->Prepare(
        "select RDB$TRIGGER_NAME from RDB$TRIGGERS where "
        "(RDB$SYSTEM_FLAG is null or RDB$SYSTEM_FLAG = 0) and "
        "(RDB$TRIGGER_INACTIVE is null OR rdb$trigger_inactive = 0) "
    );
    st1->Execute();
    while (st1->Fetch())
    {
        std::string s;
        st1->Get(1, s);
        s.erase(s.find_last_not_of(" ") + 1);
        triggers.push_back(s);
    }
    for (std::vector<std::string>::const_iterator it = triggers.begin(); it!=triggers.end(); ++it)
    {
        st1->Prepare("ALTER TRIGGER " + (*it) + " INACTIVE");
        st1->Execute();
    }
    tr1->Commit();
    fprintf(stderr, "done.\n");
}
//---------------------------------------------------------------------------------------
void FBCopy::enableTriggers()
{
    if (ar->FireTriggers || ar->Operation != opCopy && ar->Operation != opSingle || triggers.empty())
        return;

    fprintf(stderr, "Enabling triggers...");
    bool ok = false;
    try
    {
        IBPP::Transaction tr1 = IBPP::TransactionFactory(dest);
        tr1->Start();
        IBPP::Statement st1 = IBPP::StatementFactory(dest, tr1);
        for (std::vector<std::string>::const_iterator it = triggers.begin(); it!=triggers.end(); ++it)
        {
            st1->Prepare("ALTER TRIGGER " + (*it) + " ACTIVE");
            st1->Execute();
        }
        tr1->Commit();
        ok = true;
        fprintf(stderr, "done.\n");
    }
    catch (IBPP::Exception &e)
    {
        fprintf(stderr, "\nERROR!\n");
        fprintf(stderr, "%s", e.ErrorMessage());
    }
    catch (...)
    {
        fprintf(stderr, "\nERROR!\nA non-IBPP C++ runtime exception occured !\n\n");
    }

    if (!ok)
    {
        printf("\nTriggers could not get enabled! Please run the following statements manually:\n");
        for (std::vector<std::string>::const_iterator it = triggers.begin(); it!=triggers.end(); ++it)
            printf("ALTER TRIGGER %s ACTIVE;\n", (*it).c_str());
    }
}
//---------------------------------------------------------------------------------------
std::vector<std::string> FBCopy::explode(const std::string& sep, const std::string& ins)
{
    std::vector<std::string> v;
    for (std::string::size_type p = 0; true; p++)
    {
        std::string::size_type q = p;
        p = ins.find(sep, p);
        if (p == std::string::npos)
        {
            v.push_back(ins.substr(q));
            break;
        }
        v.push_back(ins.substr(q, p-q));
    }
    return v;
}
//---------------------------------------------------------------------------------------
std::string FBCopy::getUpdateStatement(const std::string& table, const std::string& fields,
    std::set<std::string>& pkcols)
{
    std::string pkc;
    std::stringstream order;
    getPkInfo(table, pkc, order);

    std::vector<std::string> fvec = explode(",", fields);
    std::vector<std::string> pkvec = explode(",", pkc);

    std::set<std::string> updpairs;
    for (std::vector<std::string>::iterator it = fvec.begin(); it != fvec.end(); it++)
        updpairs.insert((*it) + " = ? ");

    std::set<std::string> pkpairs;
    for (std::vector<std::string>::iterator it = pkvec.begin(); it != pkvec.end(); it++)
    {
        pkcols.insert(*it);
        pkpairs.insert((*it) + " = ? ");
    }

    std::string retval =
        "UPDATE " + table + " SET " + join(updpairs, "", ",") +
        " WHERE " + join(pkpairs, "", " and ");
    return retval;
}
//---------------------------------------------------------------------------------------
void FBCopy::setupFromStdin(CompareOrCopy action)
{
    if (action == ccCompareData)
    {
        if (ar->Html)
        {
            printf("<HTML>\n<HEAD>\n<TITLE>FBCopy data comparison table</TITLE>\n</HEAD>\n");
            printf("<BODY>\n<B>Source database:</B> %s@%s:%s<BR><B>Destination database:</B> %s@%s:%s<BR><BR>",
                ar->Src.Username.c_str(), ar->Src.Hostname.c_str(), ar->Src.Database.c_str(),
                ar->Dest.Username.c_str(), ar->Dest.Hostname.c_str(), ar->Dest.Database.c_str());
            printf("<STYLE>\nTD { font-family: Tahoma, Helvetica; font-size: 9px }\n</STYLE>\n");
            if (!ar->DisplayDifferences)     // each database TABLE has its own HTML table
            {
                printf("<TABLE BGCOLOR=BLACK BORDER=0 CELLSPACING=1 CELLPADDING=4>\n<TR>\n");
                printf("<TD><FONT COLOR=WHITE><B>Table</B></FONT></TD>\n");
                printf("<TD><FONT COLOR=WHITE><B>Same rows</B></FONT></TD>\n");
                printf("<TD><FONT COLOR=WHITE><B>Different rows</B></FONT></TD>\n");
                printf("<TD><FONT COLOR=WHITE><B>Missing rows</B></FONT></TD>\n");
                printf("<TD><FONT COLOR=WHITE><B>Extra rows</B></FONT></TD>\n</tr>");
            }
        }
        else if (!ar->DisplayDifferences)
        {
            printf("%-32s%9s%9s%9s%9s\n", "TABLE NAME", "SAME", "DIFFER", "MISSING", "EXTRA");
            printf("-------------------------------- -------- -------- -------- --------\n");
        }
    }
    char buff[32002];
    bool firstLine = true;
    bool newFormat = false;
    while (!feof(stdin))
    {
        if (!fgets(buff, 32000, stdin))
            break;

        std::string s(buff);
        s.erase(s.find_last_of("\n"));  // remove newline at end of string

        std::string::size_type p = s.find(":");
        if (p == std::string::npos)
        {
            fprintf(stderr, "Received line without colon, ignoring.\n");
            continue;
        }

        if (firstLine)
        {
            newFormat = (p == 2 && buff[0] == '#');
            fprintf(stderr, "%s format detected.\n", newFormat ? "New":"Old");
            firstLine = false;
        }

        if (newFormat)
        {
            s = (buff+3);
            p = s.find(":");
            if (p == std::string::npos)
            {
                fprintf(stderr, "Received line without object name, ignoring.\n");
                continue;
            }
        }

        std::string table = s.substr(0, p);
        s.erase(0, p+1);

        p = s.find(":");
        if (p == std::string::npos)
        {
            fprintf(stderr, "%s for %s not terminated, ignoring.\n",
                buff[1] == 'G' ? "Generator" : "Column list",
                table.c_str());
            continue;
        }

        std::string fields, where;
        fields = s.substr(0, p);
        p = s.find_last_of(":");    // search for optional where clause
        where = " " + s.substr(p+1);
        if (fields.empty())
        {
            fprintf(stderr, "Object %s does not exist in destination db, skipping.\n", table.c_str());
            continue;
        }

        if (newFormat && buff[1] == 'G')    // generator
        {
            if (action == ccCopy)
                copyGeneratorValues(table, fields);
            else
                compareGeneratorValues(table, fields);
            continue;
        }

        if (action == ccCopy)
        {
            std::string select = "SELECT " + fields + " FROM " + table;
            if (!where.empty())
                select += where;
            std::string insert = "INSERT INTO " + table + " (" + fields + ") VALUES (" + params(fields) + ")";
            std::set<std::string> pkcols;
            std::string update = getUpdateStatement(table, fields, pkcols);
            printf("Copying table: %s\n", table.c_str());
            copy(select, insert, update, pkcols);
        }
        else    // compare records
        {
            compareData(table, fields, where);
        }
    }

    if (action == ccCompareData && ar->Html)
    {
        if (ar->DisplayDifferences)
        {
            printf("<TABLE WIDTH=99% BORDER=0 CELLSPACING=1 CELLPADDING=2>");
            printf("  <TR>");
            printf("    <TD align=right>");
            printf("      <TABLE BGCOLOR=BLACK BORDER=0 CELLSPACING=1 CELLPADDING=0 width=30 height=10>\n");
            printf("      <TR BGCOLOR=#CCFFCC><TD width=28>&nbsp; </TD></TR></TABLE>");
            printf("    </TD>");
            printf("    <TD align=left>Same records.</TD><TD align=right rowspan=4 valign=bottom>");
            printf("This document was generated with <a href=\"http://fbexport.sourceforge.net/fbcopy.html\">FBCopy</a> tool.");
            printf("\n</TD></TR>");

            printf("  <TR>");
            printf("    <TD align=right>");
            printf("      <TABLE BGCOLOR=BLACK BORDER=0 CELLSPACING=1 CELLPADDING=0 width=30 height=10>\n");
            printf("      <TR BGCOLOR=#FFFFCC><TD width=28>&nbsp; </TD></TR></TABLE>");
            printf("    </TD>");
            printf("    <TD align=left>Different records, value found in %s@%s:%s.</TD>",
                ar->Src.Username.c_str(), ar->Src.Hostname.c_str(), ar->Src.Database.c_str());

            printf("  <TR>");
            printf("    <TD align=right>");
            printf("      <TABLE BGCOLOR=BLACK BORDER=0 CELLSPACING=1 CELLPADDING=0 width=30 height=10>\n");
            printf("      <TR BGCOLOR=#DDDD99><TD width=28>&nbsp; </TD></TR></TABLE>");
            printf("    </TD>");
            printf("    <TD align=left>Different records, value found in %s@%s:%s.</TD>",
                ar->Dest.Username.c_str(), ar->Dest.Hostname.c_str(), ar->Dest.Database.c_str());

            printf("  <TR>");
            printf("    <TD align=right>");
            printf("      <TABLE BGCOLOR=BLACK BORDER=0 CELLSPACING=1 CELLPADDING=0 width=30 height=10>\n");
            printf("      <TR BGCOLOR=#FFCCCC><TD width=28>&nbsp; </TD></TR></TABLE>");
            printf("    </TD>");
            printf("    <TD align=left>Missing - Records not found in %s@%s:%s.</TD>",
                ar->Dest.Username.c_str(), ar->Dest.Hostname.c_str(), ar->Dest.Database.c_str());

            printf("  <TR>");
            printf("    <TD align=right>");
            printf("      <TABLE BGCOLOR=BLACK BORDER=0 CELLSPACING=1 CELLPADDING=0 width=30 height=10>\n");
            printf("      <TR BGCOLOR=#CCCCFF><TD width=28>&nbsp; </TD></TR></TABLE>");
            printf("    </TD>");
            printf("    <TD align=left>Extra - Records not found in %s@%s:%s.</TD>",
                ar->Src.Username.c_str(), ar->Src.Hostname.c_str(), ar->Src.Database.c_str());

            printf("</TABLE>");
        }
        printf("</TABLE>\n<BR></BODY>\n</HTML>\n");
    }
}
//---------------------------------------------------------------------------------------
std::string FBCopy::params(const std::string& fieldlist)
{
    int cnt = 0;
    for (std::string::const_iterator it = fieldlist.begin(); it != fieldlist.end(); ++it)
        if ((*it) == ',')
            cnt++;
    std::string retval;
    for (int i=0; i<=cnt; ++i)
    {
        if (i)
            retval += ",";
        retval += "?";
    }
    return retval;
}
//---------------------------------------------------------------------------------------
std::string nl2br(const std::string &input)
{
    std::string s(input);
    while (s.find("\n") != std::string::npos)
        s.replace(s.find("\n"), 1, "<BR>");
    return s;
}
//---------------------------------------------------------------------------------------
void scaleInt(std::string& s, unsigned int scale)
{
    if (scale == 0)
        return;
    while (s.length() <= scale)
        s = "0" + s;
    s.insert(s.length() - scale, ".");
}
//---------------------------------------------------------------------------------------
string createHumanString(IBPP::Statement& st, int col, bool& numeric)
{
    numeric = false;
    string value;
    if (st->IsNull(col))
        return "NULL";

    char *c;                // temporary variables, declared here since declaring
    char str[30];           // inside "switch" isn't possible
    double dval;
    float fval;
    int64_t int64val;
    int x;
    IBPP::Date d;
    IBPP::Time t;
    IBPP::Timestamp ts;
    int year, month, day, hour, minute, second, millisec;

    IBPP::SDT DataType = st->ColumnType(col);
    //if (DataType == IBPP::sdDate && Dialect == 1)
    //    DataType = IBPP::sdTimestamp;

    value = "";
    switch (DataType)
    {
        case IBPP::sdString:
            st->Get(col, value);
            value.erase(value.find_last_not_of(' ')+1);
            break;
        case IBPP::sdInteger:
        case IBPP::sdSmallint:
            st->Get(col, &x);
            sprintf(str, "%ld", x);
            value = str;
            numeric = true;
            scaleInt(value, st->ColumnScale(col));   // scaled integer
            break;
        case IBPP::sdDate:
            st->Get(col, d);
            if (IBPP::dtoi(d.GetDate(), &year, &month, &day))
            {
                sprintf(str, "%02d.%02d.%02d", day, month, year%100);
                value = str;
            }
            break;
        case IBPP::sdTime:
            st->Get(col, t);
            IBPP::ttoi(t.GetTime(), &hour, &minute, &second, &millisec);
            sprintf(str, "%02d:%02d:%02d", hour, minute, second);
            value = str;
            break;
        case IBPP::sdTimestamp:
            /*
            st->Get(col, ts);
            sprintf(str, "%02d.%02d.%02d %02d:%02d:%02d", day, month, year%100,
                );
            value = GetHumanTimestamp(ts);
            */
            break;
        case IBPP::sdFloat:
            st->Get(col, &fval);
            gcvt(fval, 19, str);
            value = str;
            numeric = true;
            break;
        case IBPP::sdDouble:
            st->Get(col, &dval);
            gcvt(dval, 19, str);
            value = str;
            numeric = true;
            break;
        case IBPP::sdLargeint:
            st->Get(col, &int64val);
            sprintf(str, INT64FORMAT, int64val);
            value = str;
            numeric = true;
            scaleInt(value, st->ColumnScale(col));   // scaled integer
            break;

        default:
            fprintf(stderr, "WARNING: Datatype not supported! Column: %s\n",
                st->ColumnName(col));
    };
    return value;
}
//---------------------------------------------------------------------------------------
void FBCopy::addRow(int& counter, int type, IBPP::Statement st1, int index,
    IBPP::Statement *st2)
{
    IBPP::Statement st3;
    if (st2)
        st3 = *(st2);
    counter++;
    if (ar->Html && (ar->DisplayDifferences & type))
    {
        std::string color;
        switch (type)
        {
            case Args::ShowMissing:     color = "#FFCCCC";   break;
            case Args::ShowExtra:       color = "#CCCCFF";   break;
            case Args::ShowCommon:      color = "#CCFFCC";   break;
            case Args::ShowDifferent:   color = "#FFFFCC";  break;
        };
        printf("<TR BGCOLOR=%s>", color.c_str());
        for (int i=index+1; i<=st1->Columns(); ++i)
        {
            bool isNumber;
            std::string s = createHumanString(st1, i, isNumber);
            if (type == Args::ShowDifferent)
            {
                if (cmpData(st1, st3, i) == 0)
                    printf("<TD rowspan=2 bgcolor=#CCFFCC%s>%s</TD>", isNumber?" align=right":"", s.c_str());
                else
                    printf("<TD bgcolor=#FFFFCC%s>%s</TD>", isNumber?" align=right":"", s.c_str());
            }
            else
                printf("<TD%s>%s</TD>", isNumber?" align=right":"", s.c_str());
        }
        printf("</TR>\n");

        if (type == Args::ShowDifferent)    // second row
        {
            printf("<TR>");
            for (int i=index+1; i<=st1->Columns(); ++i)
            {
                if (cmpData(st1, st3, i) != 0)
                {
                    bool isNumber;
                    std::string s = createHumanString(st3, i, isNumber);
                    printf("<TD bgcolor=#DDDD99%s>%s</TD>", isNumber?" align=right":"", s.c_str());
                }
            }
            printf("</TR>\n");
        }
    }
}
//---------------------------------------------------------------------------------------
void FBCopy::copyGeneratorValues(const std::string& gfrom, const std::string& gto)
{
    IBPP::Transaction tr1 = IBPP::TransactionFactory(src, IBPP::amRead);
    tr1->Start();
    IBPP::Statement st1 = IBPP::StatementFactory(src, tr1);
    st1->Prepare("select gen_id("+gfrom+", 0) from rdb$database");
    st1->Execute();
    st1->Fetch();
    int64_t x1;
    st1->Get(1, x1);
    char tbuf1[64];
    sprintf(tbuf1, INT64FORMAT, x1);

    printf("Generator %-32s -> %-32s = %s.\n", gfrom.c_str(), gto.c_str(), tbuf1);
    IBPP::Transaction tr2 = IBPP::TransactionFactory(dest, IBPP::amWrite);
    tr2->Start();
    IBPP::Statement st2 = IBPP::StatementFactory(dest, tr2);
    st2->Prepare("SET GENERATOR "+gto+" TO "+std::string(tbuf1));
    st2->Execute();
    tr1->Commit();
}
//---------------------------------------------------------------------------------------
void FBCopy::compareGeneratorValues(const std::string& gfrom, const std::string& gto)
{
    IBPP::Transaction tr1 = IBPP::TransactionFactory(src, IBPP::amRead);
    IBPP::Transaction tr2 = IBPP::TransactionFactory(dest, IBPP::amRead);
    tr1->Start();
    tr2->Start();
    IBPP::Statement st1 = IBPP::StatementFactory(src, tr1);
    IBPP::Statement st2 = IBPP::StatementFactory(dest, tr2);
    st1->Prepare("select gen_id("+gfrom+", 0) from rdb$database");
    st2->Prepare("select gen_id("+gto+", 0) from rdb$database");
    st1->Execute();
    st2->Execute();
    st1->Fetch();
    st2->Fetch();
    int64_t x1, x2;
    st1->Get(1, x1);
    st2->Get(1, x2);
    if (!ar->Html)
    {
        char tbuf1[64];
        char tbuf2[64];
        sprintf(tbuf1, INT64FORMAT, x1);
        sprintf(tbuf2, INT64FORMAT, x2);
        printf("%-32s%9s%9s\n", gfrom.c_str(), tbuf1, tbuf2);
    }
}
//---------------------------------------------------------------------------------------
int FBCopy::getPkInfo(const std::string& table, std::string& pkcols,
    std::stringstream& order)
{
    // load primary key
    IBPP::Transaction tr1 = IBPP::TransactionFactory(src, IBPP::amRead);
    tr1->Start();
    IBPP::Statement st1 = IBPP::StatementFactory(src, tr1);
    st1->Prepare(
        " select i.rdb$field_name"
        " from rdb$relation_constraints r, rdb$index_segments i "
        " where r.rdb$relation_name=? and r.rdb$index_name=i.rdb$index_name"
        " and (r.rdb$constraint_type='PRIMARY KEY') "
    );
    st1->Set(1, table);
    st1->Execute();
    int pkcnt = 0;
    while (st1->Fetch())
    {
        pkcnt++;
        std::string cname;
        st1->Get(1, cname);
        cname.erase(cname.find_last_not_of(" ")+1);
        if (pkcols != "")
        {
            pkcols += ",";
            order << ",";
        }
        pkcols += cname;
        order << pkcnt;
    }
    return pkcnt;
}
//---------------------------------------------------------------------------------------
void FBCopy::compareData(const std::string& table, const std::string& fields,
    const std::string& where)
{
    if (ar->Html && ar->DisplayDifferences)
        printf("<TABLE id=\"%s\" border=0 bgcolor=black cellspacing=1 cellpadding=3>\n", table.c_str());

    std::string pkcols;
    std::stringstream order;
    int pkcnt = getPkInfo(table, pkcols, order);

    if (pkcols == "")    // if !PK, (NOW: exit) LATER: try column with UniqueKey
    {
        if (ar->Html)
        {
          printf("<TR bgcolor=#FFBBBB><TD COLSPAN=5>Table %s doesn't have primary key. Skipping.</TD></TR>\n", table.c_str());
          if (ar->DisplayDifferences) {
            printf("</TABLE><br><BR>\n");
            if (ar->Limited) {
              printf("<STYLE> table#%s : {display:none}</STYLE>\n", table.c_str());
            }
          }
        }
        else
            fprintf(stderr, "Table %s doesn't have primary key. Skipping.\n", table.c_str());
        return;
    }

    static int color = 0;
    if (!ar->Html)
    {
        printf("%-32s", table.c_str());
        fflush(stdout);
    }

    IBPP::Transaction tr1 = IBPP::TransactionFactory(src, IBPP::amRead);
    tr1->Start();
    IBPP::Statement st1 = IBPP::StatementFactory(src, tr1);

    IBPP::Transaction tr2 = IBPP::TransactionFactory(dest, IBPP::amRead);
    tr2->Start();
    IBPP::Statement st2 = IBPP::StatementFactory(dest, tr2);
    std::string sql = "select " + pkcols + "," + fields + " from " + table
        + " " + where + " order by " + order.str();
    st1->Prepare(sql);
    st2->Prepare(sql);
    st1->Execute();
    st2->Execute();

    if (ar->Html)
    {
        if (ar->DisplayDifferences)
        {
            printf("<TR><TD colspan=%d><font size=+1 color=white><B>%s</B></font></TD></TR>\n", st1->Columns(), table.c_str());
            printf("<tr bgcolor=#666699\n");    // header
            for (int i=pkcnt+1; i<=st1->Columns(); ++i)
                printf("<td nowrap><font color=white>%s</font></td>", st1->ColumnName(i));
            printf("</tr>");
        }
        else
            printf("<TR BGCOLOR=%s><TD>%s</TD>", (color++ % 2 ? "#CCCCCC" : "silver"), table.c_str());
    }

    int same = 0;
    int different = 0;
    int missing = 0;
    int extra = 0;
    bool wasmissing = false;
    bool wasextra = false;
    bool st2has = true;
    while (wasextra || st1->Fetch())
    {
        if (!wasmissing && !st2->Fetch())  // the end
        {
            st2has = false;
            addRow(missing, Args::ShowMissing, st1, pkcnt);
            while (st1->Fetch())
                addRow(missing, Args::ShowMissing, st1, pkcnt);
            break;
        }

        // compare data
        wasmissing = wasextra = false;
        for (int col=0; col < pkcnt; ++col)
        {
            int res = cmpData(st1, st2, col+1);
            if (res < 0)    // src < dest
            {
                addRow(missing, Args::ShowMissing, st1, pkcnt);
                wasmissing = true;
                break;
            }
            else if (res > 0)    // src > dest
            {
                addRow(extra, Args::ShowExtra, st2, pkcnt);
                wasextra = true;
                break;
            }
        }
        if (wasmissing || wasextra)
            continue;

        // same PK, check other records
        bool wasdifferent = false;
        for (int col=pkcnt; col < st1->Columns(); ++col)
        {
            if (cmpData(st1, st2, col+1) != 0)  // differs
            {
                addRow(different, Args::ShowDifferent, st1, pkcnt, &st2);
                wasdifferent = true;
                break;
            }
        }

        if (!wasdifferent)
            addRow(same, Args::ShowCommon, st1, pkcnt);
    }

    while (st2has && st2->Fetch())
        addRow(extra, Args::ShowExtra, st2, pkcnt);

    if (ar->Html)
    {
        if (ar->DisplayDifferences)
            printf("<TR bgcolor=black><TD nowrap><font color=white>Same: %d, Different: %d, Missing: %d, Extra: %d.</font></TD></TR>\n</TABLE>\n<BR><BR>",
            same, different, missing, extra);
            if (ar->Limited ) {
               if (((ar->DisplayDifferences & Args::ShowMissing) && missing>0) ||
                   ((ar->DisplayDifferences & Args::ShowExtra) && extra>0) ||
                   ((ar->DisplayDifferences & Args::ShowCommon) && same>0) ||
                   ((ar->DisplayDifferences & Args::ShowDifferent) && different>0)) {
               } else {
                 printf("<STYLE> table#%s : {display:none}</STYLE>\n", table.c_str());
               }

            }
        else
            printf("<TD align=right>%d</TD><TD align=right>%d</TD><TD align=right>%d</TD><TD align=right>%d</TD></TR>\n",
            same, different, missing, extra);
    }
    else
        printf("%9d%9d%9d%9d\n", same, different, missing, extra);
}
//---------------------------------------------------------------------------------------
int FBCopy::compareGenerators(IBPP::Transaction tr1, IBPP::Transaction tr2)
{
    fprintf(stderr, "Loading list of generators...\n");
    IBPP::Statement st2 = IBPP::StatementFactory(dest, tr2);
    st2->Prepare("select 1 from RDB$GENERATORS where RDB$GENERATOR_NAME = ? ");
    IBPP::Statement st1 = IBPP::StatementFactory(src, tr1);
    st1->Prepare("select RDB$GENERATOR_NAME from RDB$GENERATORS "
        "where (RDB$SYSTEM_FLAG = 0 or RDB$SYSTEM_FLAG is null) order by 1");
    st1->Execute();
    int cnt = 0;
    while (st1->Fetch())
    {
        cnt++;
        std::string s;
        st1->Get(1, s);
        s.erase(s.find_last_not_of(" ") + 1);
        st2->Set(1, s);
        st2->Execute();
        bool has = st2->Fetch();
        if (ar->Html)
        {
            static int color = 0;
            printf("<TR BGCOLOR=%s><TD COLSPAN=4>%s</TD></TR>\n",
                has ? (color++ % 2 ? "#CCCCFF" : "#DDDDFF") : "#FFFFCC",
                s.c_str());
        }
        else
        {
            printf("#G:%s:%s:\n", s.c_str(), has ? s.c_str() : "");
        }
    }
    return cnt;
}
//---------------------------------------------------------------------------------------
void FBCopy::compare()
{
    fprintf(stderr, "Loading list of tables...\n");
    IBPP::Transaction tr1 = IBPP::TransactionFactory(src, IBPP::amRead);
    tr1->Start();
    IBPP::Statement st1 = IBPP::StatementFactory(src, tr1);
    st1->Prepare("select RDB$RELATION_NAME from RDB$RELATIONS "
        "where (RDB$SYSTEM_FLAG = 0 or RDB$SYSTEM_FLAG is null) "
        "and RDB$VIEW_SOURCE is null ORDER BY 1");
    st1->Execute();
    std::list<std::string> tables;
    while (st1->Fetch())
    {
        std::string s;
        st1->Get(1, s);
        s.erase(s.find_last_not_of(" ") + 1);
        tables.push_back(s);
    }

    fprintf(stderr, "Using foreign keys and check constraints to determine order of inserting...\n");
    setDependencies(tables);

    /* For debug only */
    //tree.printTree(&tree);

    fprintf(stderr, "Comparing fields...\n");
    IBPP::Transaction tr2 = IBPP::TransactionFactory(dest, IBPP::amRead);
    tr2->Start();
    IBPP::Statement st2 = IBPP::StatementFactory(dest, tr2);
    std::string sql =
        " SELECT r.rdb$field_name FROM rdb$relation_fields r"
        " JOIN rdb$fields f ON r.rdb$field_source = f.rdb$field_name"
        " WHERE r.rdb$relation_name = ? "
        " AND f.rdb$computed_blr is null"
        " ORDER BY 1";
    st1->Prepare(sql);
    st2->Prepare(sql);

    if (ar->Html)   // html header
    {
        printf("<HTML>\n<HEAD>\n<TITLE>FBCopy database comparison table</TITLE>\n</HEAD>\n");
        printf("<BODY>\n<B>Source database:</B> %s@%s:%s<BR><B>Destination database:</B> %s@%s:%s<BR><BR>",
            ar->Src.Username.c_str(), ar->Src.Hostname.c_str(), ar->Src.Database.c_str(),
            ar->Dest.Username.c_str(), ar->Dest.Hostname.c_str(), ar->Dest.Database.c_str());
        printf("<STYLE>\nTD { font-family: Tahoma, Helvetica; font-size: 9px }\n</STYLE>\n");
        printf("<TABLE BGCOLOR=BLACK BORDER=0 CELLSPACING=1 CELLPADDING=4 width=100%>\n<TR>\n");
        printf("<TD><FONT COLOR=WHITE><B>Table</B></FONT></TD>\n");
        printf("<TD><FONT COLOR=WHITE><B>Mutual columns</B></FONT></TD>\n");
        if (ar->Operation == opAlter)
        {
            printf("<TD colspan=2><FONT COLOR=WHITE><B>Missing tables and columns</B></FONT></TD>\n</TR>\n");
        }
        else
        {
            printf("<TD><FONT COLOR=WHITE><B>Missing in destination</B></FONT></TD>\n");
            printf("<TD><FONT COLOR=WHITE><B>Missing in source</B></FONT></TD>\n</TR>\n");
        }
    }

    preOrder(&tree, st1, st2, tr1);
    int generators = compareGenerators(tr1, tr2);

    if (ar->Html)
    {
        printf("<TR><TD COLSPAN=3><FONT COLOR=WHITE><B>Inspected %d tables and %d generators in source database.</B></FONT></TD></TR>\n",
            tables.size(), generators);
        printf("%s", "</TABLE>\n<BR>\n<TABLE WIDTH=99% BORDER=0 CELLSPACING=1 CELLPADDING=2\n<TR><TD align=right>");
        printf("<TABLE BGCOLOR=BLACK BORDER=0 CELLSPACING=1 CELLPADDING=0 width=30 height=10>\n");
        printf("<TR BGCOLOR=#FFFFCC><TD width=28>&nbsp; </TD></TR></TABLE></TD><TD align=left>\n");
        printf("This color represents the objects not found in destination database.</TD>\n<TD align=right>");
        printf("This document was generated with <a href=\"http://fbexport.sourceforge.net/fbcopy.html\">FBCopy</a> tool.</TD>");
        printf("\n</TR></TABLE>\n<BR></BODY>\n</HTML>\n");
    }

    tr1->Commit();
    tr2->Commit();
}
//---------------------------------------------------------------------------------------
std::string FBCopy::getDatatype(IBPP::Statement& st1, std::string table, std::string fieldname, bool not_nulls)
{
    st1->Set(1, table);
    st1->Set(2, fieldname);
    st1->Execute();
    st1->Fetch();
    short datatype, subtype, length, precision, scale;
    st1->Get(1, datatype);
    if (st1->IsNull(2))
        subtype = 0;
    else
        st1->Get(2, subtype);
    st1->Get(3, length);
    if (st1->IsNull(4))
        precision = 0;
    else
        st1->Get(4, precision);
    if (st1->IsNull(5))
        scale = 0;
    else
        st1->Get(5, scale);

    std::string null_flag;
    if (!st1->IsNull(6) && not_nulls)
        null_flag = " NOT NULL";

    std::ostringstream retval;      // this will be returned
    if (datatype == 27 && scale < 0)
    {
        retval << "Numeric(15," << -scale << ")" << null_flag;
        return retval.str();
    }

    // LONG&INT64: INT/SMALLINT (prec=0), DECIMAL(sub_type=2), NUMERIC(sub_type=1)
    if (datatype == 7 || datatype == 8 || datatype == 16)
    {
        if (scale == 0)
        {
            if (datatype == 7)
                retval << "Smallint";
            else if (datatype == 8)
                retval << "Integer";
            else
                retval << "Numeric(18,0)";
            retval << null_flag;
            return retval.str();
        }
        else
        {
            retval << (subtype == 2 ? "Decimal(" : "Numeric(");
            if (precision <= 0 || precision > 18)
                retval << 18;
            else
                retval << precision;
            retval << "," << -scale << ")" << null_flag;
            return retval.str();
        }
    }

    std::string names[] = {
        "Char",
        "Float",
        "Double precision",
        "Timestamp",
        "Varchar",
        "Blob",
        "Date",
        "Time"
    };
    short mapper[8] = { 14, 10, 27, 35, 37, 261, 12, 13 };
    for (int i=0; i<8; ++i)
    {
        if (mapper[i] == datatype)
        {
            retval << names[i];
            break;
        }
    }
    if (datatype == 14 || datatype == 37)   // char & varchar, add (length)
        retval << "(" << length << ")";
    if (datatype == 261)    // blob
        retval << " sub_type " << subtype;
    retval << null_flag;
    return retval.str();
}
//---------------------------------------------------------------------------------------
std::string FBCopy::join(const std::set<std::string>& s, const std::string& qualifier,
    const std::string& glue)
{
    std::string ret;
    for (std::set<std::string>::const_iterator it = s.begin(); it != s.end(); ++it)
        ret += glue + qualifier + (*it) + qualifier;
    if (!ret.empty())
        ret.erase(0, glue.length());    // remove initial ","
    return ret;
}
//---------------------------------------------------------------------------------------
int FBCopy::getPkIndex(IBPP::Statement& st1, int i, std::set<std::string>&pkcols)
{
    int col = st1->Columns();
    for (std::set<std::string>::iterator it = pkcols.begin(); it != pkcols.end(); ++it)
    {
        col++;
        if ((*it) == st1->ColumnName(i))
            return col;
    }
    return -1;
}
//---------------------------------------------------------------------------------------
bool FBCopy::copy(const std::string& select, const std::string& insert,
    const std::string& update, std::set<std::string>& pkcols)
{
    if (!ar->SingleTransaction)
    {
        trans1->Start();
        trans2->Start();
    }
    IBPP::Statement st1 = IBPP::StatementFactory(src, trans1);
    IBPP::Statement st2 = IBPP::StatementFactory(dest, trans2);
    IBPP::Statement st3;
    st1->Prepare(select);
    st2->Prepare(insert);

    if (ar->Verbose)
    {
        fprintf(stderr, "SELECT: %s\n", select.c_str());
        fprintf(stderr, "INSERT: %s\n", insert.c_str());
        fprintf(stderr, "UPDATE: %s\n", update.c_str());
    }

    if (ar->Update)
    {
        st3 = IBPP::StatementFactory(dest, trans2);
        st3->Prepare(update);
    }

    int columns = st1->Columns();

    int cnt = 0;
    int errors = 0;
    int partial = 0;
    st1->Execute();
    while (st1->Fetch())
    {
        bool allColumnsOk = true;
        for (int i=1; i<=columns; ++i)
        {
            int pkx = ar->Update ? getPkIndex(st1, i, pkcols) : -1;
            if (st1->IsNull(i))
            {
                st2->SetNull(i);
                if (ar->Update)
                {
                    st3->SetNull(i);
                    if (pkx > -1)
                        st3->SetNull(pkx);
                }
                continue;
            }
            int ok = copyData(st1, st2, i, i) ? 0 : 1;
            if (ok == 0 && ar->Update)
                ok = copyData(st1, st3, i, i) ? 0 : 2;
            if (ok == 0 && pkx > -1 && !copyData(st1, st3, i, pkx))
            {
                fprintf(stderr, "Error copying column number: %d, name: %s to primary key parameter: %d\n",
                    i, st1->ColumnName(i), pkx);
                return false;
            }
            if (ok != 0)
            {
                allColumnsOk = false;
                if (!ar->KeepGoing)
                {
                    fprintf(stderr, "Error copying column number: %d, name: %s, for operation: %s\n",
                        i, st1->ColumnName(i), ok == 1 ? "insert" : "update");
                    return false;
                }
            }
        }
        if (!allColumnsOk)
            partial++;

        if (ar->KeepGoing || ar->Update)
        {
            try
            {
                st2->Execute();
            }
            catch (IBPP::Exception& e)
            {
                if (ar->Update)
                {
                    if (ar->KeepGoing)
                    {
                        try
                        {
                            st3->Execute();
                        }
                        catch (IBPP::Exception& e2)
                        {
                            errors++;
                            if (!allColumnsOk)  // don't count as we didn't succeed anyway
                                partial--;
                            if (ar->Verbose)
                                fprintf(stderr, "Error: %s\n", e2.ErrorMessage());
                        }
                    }
                    else
                    {
                        st3->Execute();
                    }
                }
                else
                {
                    errors++;
                    if (!allColumnsOk)  // don't count as we didn't succeed anyway
                        partial--;
                    if (ar->Verbose)
                        fprintf(stderr, "Error: %s\n", e.ErrorMessage());
                }
            }
        }
        else
        {
            st2->Execute();
        }

        if (++cnt % 2000 == 0)
            fprintf(stderr, "Checkpoint at %d rows.\n", cnt);
    }
    printf("%d records copied", cnt - errors);
    if (partial > 0)
        printf(" (%d only partially)", partial);
    if (!ar->SingleTransaction)
    {
        trans1->Commit();
        trans2->Commit();
        printf(" and commited");
    }
    if (errors)
        printf(". Failed to copy %d records", errors);
    printf(".\n");
    return true;
}
//---------------------------------------------------------------------------------------
template<typename T>
int cmpval(const T& one, const T& two)
{
    if (one == two)
        return 0;
    else if (one < two)
        return -1;
    else
        return 1;
}
//---------------------------------------------------------------------------------------
// returns zero if same, -1 if src<dest and +1 if src>dest
int FBCopy::cmpData(IBPP::Statement& st1, IBPP::Statement& st2, int col)
{
    if (st1->IsNull(col) && st2->IsNull(col))
        return 0;
    if (st1->IsNull(col) && !st2->IsNull(col))
        return 1;
    if (!st1->IsNull(col) && st2->IsNull(col))
        return -1;

    string s, s2;       // temporary variables, declared here since declaring
    double dval, dval2;
    float fval, fval2;
    short sh, sh2;
    int64_t int64val, int64val2;
    int x, x2;
    IBPP::Date d, d2;
    IBPP::Time t, t2;
    IBPP::Timestamp ts, ts2;

    IBPP::SDT DataType = st1->ColumnType(col);
    if (st1->ColumnScale(col)) // FIXME: IBPP has to be changed, this is only a hack
        DataType = IBPP::sdDouble;

    //if (DataType == IBPP::sdDate && Dialect == 1)
    //DataType = IBPP::sdTimestamp;

    switch (DataType)
    {
        case IBPP::sdString:
            st1->Get(col, s);
            st2->Get(col, s2);
            return cmpval(s, s2);
        case IBPP::sdSmallint:
            st1->Get(col, sh);
            st2->Get(col, sh2);
            return cmpval(sh, sh2);
        case IBPP::sdInteger:
            st1->Get(col, x);
            st2->Get(col, x2);
            return cmpval(x, x2);
        case IBPP::sdDate:
            st1->Get(col, d);
            st2->Get(col, d2);
            return cmpval(d, d2);
        case IBPP::sdTime:
            st1->Get(col, t);
            st2->Get(col, t2);
            return cmpval(t, t2);
        case IBPP::sdTimestamp:
            st1->Get(col, ts);
            st2->Get(col, ts2);
            return cmpval(ts, ts2);
        case IBPP::sdFloat:
            st1->Get(col, fval);
            st2->Get(col, fval2);
            return cmpval(fval, fval2);
        case IBPP::sdDouble:
            st1->Get(col, dval);
            st2->Get(col, dval2);
            return cmpval(dval, dval2);
        case IBPP::sdLargeint:
            st1->Get(col, int64val);
            st2->Get(col, int64val2);
            return cmpval(int64val, int64val2);
        case IBPP::sdBlob:
        //    return 0;   // FIXME: copyBlob(st1, st2, col);

        default:
            fprintf(stderr, "WARNING: Datatype not supported! Column: %s\n",
                st1->ColumnName(col));
            return 0;
    };
}
//---------------------------------------------------------------------------------------
bool FBCopy::copyData(IBPP::Statement& st1, IBPP::Statement& st2, int srccol,
    int destcol)
{
    string s;       // temporary variables, declared here since declaring
    char str[30];   // inside "switch" isn't possible
    double dval;
    float fval;
    short sh;
    int64_t int64val;
    int x;
    IBPP::Date d;
    IBPP::Time t;
    IBPP::Timestamp ts;

    IBPP::SDT DataType = st1->ColumnType(srccol);

    // FIXME: IBPP has to be changed, this is only a hack
    if (DataType != IBPP::sdBlob && st1->ColumnScale(srccol))
        DataType = IBPP::sdDouble;

    //if (DataType == IBPP::sdDate && Dialect == 1)
    //DataType = IBPP::sdTimestamp;

    try
    {
        switch (DataType)
        {
            case IBPP::sdString:
                st1->Get(srccol, s);
                st2->Set(destcol, s);
                return true;
            case IBPP::sdSmallint:
                st1->Get(srccol, sh);
                st2->Set(destcol, sh);
                return true;
            case IBPP::sdInteger:
                st1->Get(srccol, x);
                st2->Set(destcol, x);
                return true;
            case IBPP::sdDate:
                st1->Get(srccol, d);
                st2->Set(destcol, d);
                return true;
            case IBPP::sdTime:
                st1->Get(srccol, t);
                st2->Set(destcol, t);
                return true;
            case IBPP::sdTimestamp:
                st1->Get(srccol, ts);
                st2->Set(destcol, ts);
                return true;
            case IBPP::sdFloat:
                st1->Get(srccol, fval);
                st2->Set(destcol, fval);
                return true;
            case IBPP::sdDouble:
                st1->Get(srccol, dval);
                st2->Set(destcol, dval);
                return true;
            case IBPP::sdLargeint:
                st1->Get(srccol, int64val);
                st2->Set(destcol, int64val);
                return true;
            case IBPP::sdBlob:
                return copyBlob(st1, st2, srccol);  // blob cannot be PK

            default:
                fprintf(stderr, "WARNING: Datatype not supported! Column: %s\n",
                    st1->ColumnName(srccol));
                st2->SetNull(destcol);
        };
    }
    catch (IBPP::Exception &e)
    {
        fprintf(stderr, "ERROR!\n");
        fprintf(stderr, "%s", e.ErrorMessage());
    }

    return false;
}
//---------------------------------------------------------------------------------------
bool FBCopy::copyBlob(IBPP::Statement& st1, IBPP::Statement& st2, int col)
{
    IBPP::Blob b1 = IBPP::BlobFactory(st1->DatabasePtr(), st1->TransactionPtr());
    IBPP::Blob b2 = IBPP::BlobFactory(st2->DatabasePtr(), st2->TransactionPtr());
    b2->Create();
    st1->Get(col, b1);
    b1->Open();

    unsigned char buffer[8192];     // 8K block
    while (true)
    {
        int size = b1->Read(buffer, 8192);
        if (size <= 0)
            break;
        b2->Write(buffer, size);
    }
    b1->Close();
    b2->Close();
    st2->Set(col, b2);
    return true;
}
//---------------------------------------------------------------------------------------
bool FBCopy::connect(IBPP::Database& db1, DatabaseInfo d)
{
    try
    {
        if (d.Charset == "")
            db1 = IBPP::DatabaseFactory(d.Hostname.c_str(), d.Database.c_str(), d.Username.c_str(), d.Password.c_str());
        else
            db1 = IBPP::DatabaseFactory(d.Hostname.c_str(), d.Database.c_str(), d.Username.c_str(),
                                        d.Password.c_str(), "", d.Charset.c_str(), "");

        fprintf(stderr, "Connecting to: \'%s\' as \'%s\'...", d.Hostname.c_str(), d.Username.c_str());
        db1->Connect();
        fprintf(stderr, "Connected.\n");
        if (d.Charset == "")    // read charset, and reconnect
        {
            fprintf(stderr, "Reading charset: ");
            IBPP::Transaction tr = IBPP::TransactionFactory(db1, IBPP::amRead);
            tr->Start();
            IBPP::Statement st = IBPP::StatementFactory(db1, tr);
            st->Prepare("SELECT rdb$character_set_name FROM rdb$database");
            st->Execute();
            st->Fetch();
            std::string cset;
            st->Get(1, cset);
            cset.erase(cset.find_last_not_of(" ")+1);
            fprintf(stderr, "%s", cset.c_str());
            tr->Commit();
            if (cset == "NONE")
                fprintf(stderr, ". No need for reconnecting.\n");
            else
            {
                fprintf(stderr, ", disconnecting...");
                db1->Disconnect();
                fprintf(stderr, "ok.\n");
                db1 = IBPP::DatabaseFactory(d.Hostname.c_str(), d.Database.c_str(), d.Username.c_str(),
                                            d.Password.c_str(), "", cset, "");
                fprintf(stderr, "Reconnecting with new charset...", d.Hostname.c_str(), d.Username.c_str());
                db1->Connect();
                fprintf(stderr, "Connected.\n");
            }
        }
    }
    catch (IBPP::Exception &e)
    {
        fprintf(stderr, "ERROR!\n");
        fprintf(stderr, "%s", e.ErrorMessage());
        return false;
    }
    catch (...)
    {
        fprintf(stderr, "ERROR!\nA non-IBPP C++ runtime exception occured !\n\n");
        return false;
    }
    return true;
}
//---------------------------------------------------------------------------------------
// Improved dependency sorting functions, by Thiago Borges...
//---------------------------------------------------------------------------------------
void FBCopy::addDeps(std::list<std::string>& deps, const std::string& table, IBPP::Statement& st)
{
    st->Set(1, table);
    st->Execute();
    while (st->Fetch())
    {
        std::string s;
        st->Get(1, s);
        s.erase(s.find_last_not_of(" ") + 1);
        if (s == table)
            fprintf(stderr, "WARNING: self referencing table: %s.\n", table.c_str());
        else
            deps.push_back(s);
    }
}
//---------------------------------------------------------------------------------------
// foreign keys + check_constraints
void FBCopy::getDependencies(TableDependency* dep, std::string ntable)
{
    std::list<std::string> ndeps;
    addDeps(ndeps, ntable, stDepsFK);
    addDeps(ndeps, ntable, stDepsCheck);
    ndeps.sort();

    std::list<TableDependency *>::iterator node = dep->dependencies.begin();
    for (std::list<std::string>::iterator it = ndeps.begin(); it != ndeps.end(); ++it)
    {
        std::string ndep = (*it);
        // DEBUG: printf("TABLE: %s    DEPENDS ON TABLE: %s\n", ntable.c_str(), ndep.c_str());
        if (!tree.existsInTree(&tree, ndep))
        {
            std::list<TableDependency *>::iterator depAux = dep->dependencies.insert(node, new TableDependency(ndep));
            getDependencies(*depAux, ndep);
        }
    }
}
//---------------------------------------------------------------------------------------
void FBCopy::setDependencies(std::list<std::string> tableList)
{
    /* root node */
    tree = TableDependency("root");
    std::list<TableDependency *>::iterator node = tree.dependencies.begin();
    transDep = IBPP::TransactionFactory(src, IBPP::amRead);
    transDep->Start();

    // prepare statements to be more efficient
    stDepsFK = IBPP::StatementFactory(src, transDep);
    stDepsCheck = IBPP::StatementFactory(src, transDep);
    stDepsFK->Prepare(
        "select r2.rdb$relation_name from rdb$relation_constraints r1"
        " join rdb$ref_constraints c ON r1.rdb$constraint_name = c.rdb$constraint_name"
        " join rdb$relation_constraints r2 on c.RDB$CONST_NAME_UQ  = r2.rdb$constraint_name"
        " where r1.rdb$relation_name=? "
        " and (r1.rdb$constraint_type='FOREIGN KEY') "
    );
    stDepsCheck->Prepare(
        "select distinct d.RDB$DEPENDED_ON_NAME from rdb$relation_constraints r "
        " join rdb$check_constraints c on r.rdb$constraint_name=c.rdb$constraint_name "
        "      and r.rdb$constraint_type = 'CHECK' "
        " join rdb$dependencies d on d.RDB$DEPENDENT_NAME = c.rdb$trigger_name and d.RDB$DEPENDED_ON_TYPE = 0 "
        "      and d.rdb$DEPENDENT_TYPE = 2 and d.rdb$field_name is null "
        " where r.rdb$relation_name= ? "
    );

    for (std::list<std::string>::iterator it = tableList.begin(); it != tableList.end(); ++it)
    {
        std::string table = (*it);
        if (!tree.existsInTree(&tree, table))
        {
            std::list<TableDependency *>::iterator dep = tree.dependencies.insert(node, new TableDependency(table));
            getDependencies(*dep, table);
        }
    }
}
//---------------------------------------------------------------------------------------
void FBCopy::preOrder(TableDependency* a, IBPP::Statement& st1, IBPP::Statement& st2, IBPP::Transaction& tr1)
{
    for (std::list<TableDependency *>::iterator tmp = a->dependencies.begin(); tmp != a->dependencies.end(); ++tmp)
        preOrder(*tmp, st1, st2, tr1);

    if (a->tableName != "root")
        compareTable(a->tableName, st1, st2, tr1);
}
//---------------------------------------------------------------------------------------
// moved from old compare function
//---------------------------------------------------------------------------------------
void FBCopy::compareTable(std::string table, IBPP::Statement& st1, IBPP::Statement& st2, IBPP::Transaction& tr1)
{
    std::set<std::string> srcfields, destfields;
    std::set<std::string> fields, missing, extra;
    st1->Set(1, table);             // load fields of src table
    st1->Execute();
    while (st1->Fetch())
    {
        std::string field;
        st1->Get(1, field);
        field.erase(field.find_last_not_of(" ") + 1);
        srcfields.insert(field);
    }
    st2->Set(1, table);             // load fields of dest table
    st2->Execute();
    while (st2->Fetch())
    {
        std::string field;
        st2->Get(1, field);
        field.erase(field.find_last_not_of(" ") + 1);
        destfields.insert(field);
    }
    insert_iterator<set<std::string> > fins(fields, fields.begin());
    set_intersection(srcfields.begin(), srcfields.end(), destfields.begin(), destfields.end(), fins);
    insert_iterator<set<std::string> > mins(missing, missing.begin());
    set_difference(srcfields.begin(), srcfields.end(), destfields.begin(), destfields.end(), mins);
    insert_iterator<set<std::string> > eins(extra, extra.begin());
    set_difference(destfields.begin(), destfields.end(), srcfields.begin(), srcfields.end(), eins);

    if (ar->Operation == opDefine)  // output definition line
    {
        if (ar->Html)
        {
            static int color = 0;
            printf("<TR BGCOLOR=%s><TD>%s</TD><TD>%s</TD><TD>%s</TD><TD>%s</TD></TR>\n",
                (fields.empty() && extra.empty()) ? "#FFFFCC" : (color++ % 2 ? "#CCCCCC" : "silver"),
                table.c_str(),
                join(fields, "", ", ").c_str(), join(missing, "", ", ").c_str(), join(extra, "", ", ").c_str()
            );
        }
        else
            printf("#T:%s:%s:%s:%s:\n", table.c_str(), join(fields,"\"",",").c_str(), join(missing,"\"",",").c_str(), join(extra,"\"",",").c_str());
    }
    else if (ar->Operation == opAlter)  // output alter lines
    {
        if (missing.empty())
            return;

        // getDatatype stuff
        IBPP::Statement st3 = IBPP::StatementFactory(src, tr1);
        st3->Prepare(
            "select t.rdb$type, f.rdb$field_sub_type, f.rdb$field_length,"
            "f.rdb$field_precision, f.rdb$field_scale, r.rdb$null_flag "
            "from rdb$fields f "
            "join rdb$relation_fields r on f.rdb$field_name=r.rdb$field_source "
            "join rdb$types t on f.rdb$field_type=t.rdb$type "
            "where r.rdb$relation_name = ? "
            "and r.rdb$field_name = ? "
            "and t.rdb$field_name='RDB$FIELD_TYPE' "
        );

        std::string create, alter;
        if (fields.empty() && extra.empty())    // table does not exist
        {
            create += "CREATE TABLE ";
            create += table + "\n(";
            std::set<std::string>::iterator i2 = missing.begin();
            while (true)
            {
                if (i2 == missing.end())
                    break;
                create += "\n  " + (*i2) + " " + getDatatype(st3, table, *i2);
                i2++;
                if (i2 != missing.end())
                    create += ",";
            }

            IBPP::Statement st4 = IBPP::StatementFactory(src, tr1);     // PRIMARY KEYs
            st4->Prepare(
                "select i.rdb$field_name from rdb$relation_constraints r, rdb$index_segments i "
                "where r.rdb$relation_name=? and r.rdb$index_name=i.rdb$index_name and "
                "(r.rdb$constraint_type='PRIMARY KEY')"
            );
            st4->Set(1, table);
            st4->Execute();
            std::set<std::string> pks;
            while (st4->Fetch())
            {
                std::string pkcol;
                st4->Get(1, pkcol);
                pkcol.erase(pkcol.find_last_not_of(" ") + 1);
                pks.insert(pkcol);
            }
            if (!pks.empty())
                create += ",\n  Primary Key(" + join(pks,"\"",",") + ")";
            create += "\n);";
        }
        else
        {
            for (std::set<std::string>::iterator i3 = missing.begin(); i3 != missing.end(); ++i3)
            {
                alter += "ALTER TABLE " + table + " ADD " + (*i3) + " " +
                    getDatatype(st3, table, *i3, ar->NotNulls) + ";\n";
            }
        }

        if (ar->Html)
        {
            static int color = 0;
            printf("<TR BGCOLOR=%s><TD>%s</TD><TD>%s</TD><TD COLSPAN=2>%s</TD></TR>\n",
                (fields.empty() && extra.empty()) ? "#FFFFCC" : (color++ % 2 ? "#CCCCCC" : "silver"),
                table.c_str(),
                join(fields, "", ", ").c_str(),
                nl2br(create + alter).c_str());
        }
        else
            printf("%s%s\n", create.c_str(), alter.c_str());

    }
    else if (ar->Operation == opSingle)
    {
        if (fields.empty())
        {
            printf("No common fields found for table: %s\n", table.c_str());
        }
        else
        {
            std::string select = "SELECT " + join(fields,"\"",",") + " FROM " + table;
            std::string insert = "INSERT INTO " + table + " ("
                + join(fields, "\"", ",")
                + ") VALUES (" + params(join(fields,"",",")) + ")";
            //std::string update = getUpdateStatement(table, fields, pkcols)
            printf("Copying table: %s\n", table.c_str());
            std::set<std::string> dummy;
            copy(select, insert, "", dummy);
        }
    }
}
//---------------------------------------------------------------------------------------
//  EOF
