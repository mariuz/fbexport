///////////////////////////////////////////////////////////////////////////////
//
//  File        : fbexport.cpp
//  Author      : Milan Babuskov (mbabuskov@yahoo.com)
//  Purpose     : This is the main file that does just about everything.
//                All code is encapsulated into FBExport class
//
//  Note        : Uses version 2 of IBPP library (also released under MPL)
//                Check http://ibpp.sourceforge.net for more info
//
///////////////////////////////////////////////////////////////////////////////
//
//  The contents of this file are subject to the Mozilla Public License
//  Version 1.0 (the "License"); you may not use this file except in
//  compliance with the License. You may obtain a copy of the License at
//  http://www.mozilla.org/MPL/
//
//  Software distributed under the License is distributed on an "AS IS"
//  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
//  License for the specific language governing rights and limitations
//  under the License.
//
//  The Original Code is "FBExport 1.0" and all its associated documentation.
//
//  The Initial Developer of the Original Code is Milan Babuskov.
//
//  Contributor(s): Istvan Matyasi, Alex Edelev.
//
///////////////////////////////////////////////////////////////////////////////
#ifdef IBPP_WINDOWS
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#define INT64FORMAT "%Li"
#define atoll(x) _atoi64(x)
#endif

#ifdef IBPP_LINUX
#define __cdecl /**/
#define INT64FORMAT "%lli"
#define strnicmp(a, b, c) strncasecmp( (a), (b), (c) )
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
#include <cstring>

#include "ParseArgs.h"
#include "FBExport.h"
//---------------------------------------------------------------------------------------
// Converts IBPP::SDT to unsigned char (which will be written to file)
unsigned char FBExport::SDT2uc(IBPP::SDT st)
{
    return (unsigned char)(st);
}
//---------------------------------------------------------------------------------------
// read blob data from database and Write to fbx file
void FBExport::WriteBlob(FILE *fp, IBPP::Statement& st, int col)
{
    // NULL indicator: 0 = null, 1 = not null
    if (st->IsNull(col))
    {
        fputc(0, fp);
        return;
    }
    else
        fputc(1, fp);

    IBPP::Blob b = IBPP::BlobFactory(st->DatabasePtr(), st->TransactionPtr());
    st->Get(col, b);
    b->Open();

    unsigned char buffer[8192];     // 8K block
    int size;
    do
    {
        size = b->Read(buffer, 8192);
        fprintf(fp, "%04d", size);          // write as ASCII number
        if (size > 0)
            fwrite(buffer, 1, size, fp);    // write contents to a file
    }
    while (size > 0);
    b->Close();
}
//---------------------------------------------------------------------------------------
// Read Blob from fbx file and insert into database
int FBExport::ReadBlob(FILE *fp, IBPP::Statement& st, int col, bool needed )
{
    int len = fgetc(fp);
    if (len == EOF)         // blob could be the first field of a row
        return -1;          // report EOF to the calling function

    if (len == 0)             // null value
    {
        if (needed)
            st->SetNull(col);
        return 0;               // 0 means OK.
    }

    try
    {
        IBPP::Blob b = IBPP::BlobFactory(st->DatabasePtr(), st->TransactionPtr());
        if (needed)
            b->Create();

        while (true)
        {
            // read length of the next segment
            char temp[5];
            unsigned char buffer[8192];
            len = fread(temp, 1, 4, fp);
            if (len != 4)   // fatal error, something's wrong
            {
                Printf("\nFile seems corrupt (reason 1), bailing out...\n");
                return -2;
            }

            // convert ascii to number
            temp[4] = '\0'; // terminator
            len = (int)atol(temp);
            if (len < 0)
            {
                Printf("\nFile seems corrupt (reason 2), bailing out...\n");
                return -2;
            }

            if (len == 0)   // end of blob data
                break;

            if ((unsigned int)len != fread(buffer, 1, len, fp))
            {
                Printf("\nFile seems corrupt (reason 3), bailing out...\n");
                return -2;
            }

            if (needed)
                b->Write(buffer, len);
        }
        if (needed)
        {
            b->Close();
            for (set<int>::iterator j = parmap[col].begin(); j != parmap[col].end(); j++)
                st->Set(*j, b);
        }
    }
    catch (...)
    {
        Printf("\nFatal exception caught!\n");
        return -2;
    }

    return 0;
}
//---------------------------------------------------------------------------------------
string FBExport::GetHumanDate(int year, int month, int day)
{
    string value;
    for (string::iterator c = ar->DateFormat.begin(); c != ar->DateFormat.end(); c++)
    {
        int var = -1;
        string format = "%02d";
        switch (*c)
        {
            case 'd':   format = "%d";
            case 'D':   var = day;          break;
            case 'm':   format = "%d";
            case 'M':   var = month;        break;
            case 'Y':   format = "%04d";
            case 'y':   var = year;         break;
            default:    value += *c;
        }

        if (*c == 'y')
            var %= 100;

        if (var != -1)
        {
            char str[10];
            sprintf(str, format.c_str(), var);
            value += str;
        }
    }
    return value;
}
//---------------------------------------------------------------------------------------
string FBExport::GetHumanTime(int hour, int minute, int second)
{
    string value;
    for (string::iterator c = ar->TimeFormat.begin(); c != ar->TimeFormat.end(); c++)
    {
        int var = -1;
        string format = "%02d";
        switch (*c)
        {
            case 'h':   format = "%d";
            case 'H':   var = hour;         break;
            case 'm':   format = "%d";
            case 'M':   var = minute;       break;
            case 's':   format = "%d";
            case 'S':   var = second;       break;
            default:    value += *c;
        }

        if (var != -1)
        {
            char str[10];
            sprintf(str, format.c_str(), var);
            value += str;
        }
    }
    return value;
}
//---------------------------------------------------------------------------------------
string FBExport::GetHumanTimestamp(IBPP::Timestamp ts)
{
    int year, month, day, hour, minute, second;
    ts.GetDate(year, month, day);
    ts.GetTime(hour, minute, second);

    string value;
    value = GetHumanTime(hour, minute, second);
    if (value != "")
        value = " " + value;
    return GetHumanDate(year, month, day) + value;
}
//---------------------------------------------------------------------------------------
void reScaleInt(std::string& s, int scale, int i)
{
    // find the decimal point to determine the scale of input string
    std::string::size_type p = s.find(".");

    // we are moving from one scale to another
    int needed;
    if (p != std::string::npos) // dot found
    {
        int input_scale = (s.length() - p - 1);
        if (input_scale > scale)    // we are losing precision, warn the user
        {
            char buff[300];
            sprintf(buff, "Warning: precision from file (%d) is "
                "greater than parameter %d precision (%d).\nYou might lose some "
                "significant digits.\nPlease re-export data or use CAST.\n",
                input_scale, i, scale);
            DataFormatException dfe(buff);
            throw dfe;
        }
        needed = scale - input_scale;
        s.erase(p, 1);
    }
    else
        needed = scale;
    while (needed-- > 0)
        s += "0";
}
//---------------------------------------------------------------------------------------
void scaleInt(std::string& s, int scale)
{
    if (scale == 0)
        return;
    bool minus = (s.length() > 0 && s[0] == '-');
    unsigned len = (unsigned)scale;
    if (minus)
        len++;
    while (s.length() <= len)
    {
        if (minus)
            s.insert(1, "0");
        else
            s = "0" + s;
    }
    s.insert(s.length() - scale, ".");
}
//---------------------------------------------------------------------------------------
string FBExport::CreateHumanString(IBPP::Statement& st, int col)
{
    string value;
    if (st->IsNull(col))
        return (ar->ExportFormat == xefInserts ? "NULL" : "");

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

    if (DataType == IBPP::sdDate && Dialect == 1)
        DataType = IBPP::sdTimestamp;

    value = "";
    bool numeric = false;
    switch (DataType)
    {
        case IBPP::sdString:
            st->Get(col, value);
            if (ar->TrimChars)
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
                value = GetHumanDate(year, month, day);
            break;
        case IBPP::sdTime:
            st->Get(col, t);
            IBPP::ttoi(t.GetTime(), &hour, &minute, &second, &millisec);
            value = GetHumanTime(hour, minute, second);
            break;
        case IBPP::sdTimestamp:
            st->Get(col, ts);
            value = GetHumanTimestamp(ts);
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
            Printf("\nWARNING: Datatype not supported!!!\n");
    };

    if (ar->ExportFormat == xefCSV)         // CVS format, each field's value is quoted
    {
        // escape quotes with two quotes " => ""
        string::size_type pos = string::npos;
        while (true)
        {
            pos = value.find_last_of('"', pos);
            if (pos == string::npos)
                break;
            value.insert(pos, 1, '"');
            if (pos == 0)
                break;
            pos--;
        }
        value = "\"" + value + "\"";        // except those which are NULL
    }

    if (ar->ExportFormat == xefInserts && !numeric)
    {
        string::size_type pos = string::npos;
        while (true)
        {
            pos = value.find_last_of('\'', pos);
            if (pos == string::npos)
                break;
            value.insert(pos, 1, '\'');
            if (pos == 0)
                break;
            pos--;
        }
        value = "'" + value + "'";      // INSERTs, use single quotes for non-numeric values
    }

    return value;
}
//---------------------------------------------------------------------------------------
// sets the value to string that represents value of column "col"
// returns false is value is null, true otherwise
bool FBExport::CreateString(IBPP::Statement& st, int col, string &value)
{
    if (st->IsNull(col))
        return false;

    char *c;                // temporary variables, declared here since declaring
    char str[30];           // inside "switch" isn't possible
    double dval;
    float fval;
    int64_t int64val;
    int x;
    IBPP::Date d;
    IBPP::Time t;
    IBPP::Timestamp ts;
    int year, month, day, hour, minute, second;

    IBPP::SDT DataType = st->ColumnType(col);
    if (DataType == IBPP::sdDate && Dialect == 1)
        DataType = IBPP::sdTimestamp;

    switch (DataType)
    {
        case IBPP::sdString:
            st->Get(col, value);
            if (ar->TrimChars)
                value.erase(value.find_last_not_of(' ')+1);
            return true;
        case IBPP::sdInteger:
        case IBPP::sdSmallint:
            st->Get(col, &x);
            sprintf(str, "%ld", x);
            value = str;
            scaleInt(value, st->ColumnScale(col));   // scaled integer
            return true;
        case IBPP::sdDate:
            st->Get(col, d);
            sprintf(str, "%ld", d.GetDate());
            value = str;
            return true;
        case IBPP::sdTime:
            st->Get(col, t);
            sprintf(str, "%ld", t.GetTime());
            value = str;
            return true;
        case IBPP::sdTimestamp:
            st->Get(col, ts);
            ts.GetDate(year, month, day);
            ts.GetTime(hour, minute, second);
            sprintf(str, "%04d%02d%02d%02d%02d%02d", year, month, day, hour, minute, second);
            value = str;
            return true;
        case IBPP::sdFloat:
            st->Get(col, &fval);
            gcvt(fval, 19, str);
            value = str;
            return true;
        case IBPP::sdDouble:
            st->Get(col, &dval);
            gcvt(dval, 19, str);
            value = str;
            return true;
        case IBPP::sdLargeint:
            st->Get(col, &int64val);
            sprintf(str, INT64FORMAT, int64val);
            value = str;
            scaleInt(value, st->ColumnScale(col));   // scaled integer
            return true;

        default:
            Printf("\nWARNING: Datatype not supported by this version of fbexport!!!\n");
            value = "";
    };

    return true;
}
//---------------------------------------------------------------------------------------
// statement is prepared, just need to fetch all into file
// returns: # of rows exported, -1 on error
int FBExport::Export(IBPP::Statement& st, FILE *fp)
{
    Printf("Exporting data...\n");
    time_t StartTime;
    time(&StartTime);

    // file header
    fputc(0, fp);
    fputc(FBEXPORT_FILE_VERSION, fp);   // fbexport version

    register int fc = st->Columns();

    // writes number of fields and fields' data types
    fputc((unsigned char)(fc), fp);
    for (register int i=1; i<=fc; i++)
    {
        IBPP::SDT DataType = st->ColumnType(i);
        if (DataType == IBPP::sdDate && Dialect == 1)
            DataType = IBPP::sdTimestamp;

        fputc(SDT2uc(DataType), fp);
    }

    int ret=0;
    // loop through all records in dataset, and ...
    while (st->Fetch())
    {
        CurrentData = "";
        for (register int i=1; i<=fc; i++)   // ... export all fields to file.
        {
            // if it's a BLOB, use different technique (since BLOBs can be really big!)
            if (st->ColumnType(i) == IBPP::sdBlob)
                WriteBlob(fp, st, i);
            else
            {
                // creates string representation of a field
                string value;
                bool is_null = !CreateString(st, i, value);
                unsigned int vallen = value.length();
                int len = (is_null ? 255 : vallen);

                // up to 253 chars for single byte marker, if value = 254, it's a multibyte
                // if value = 255, it's a null value
                if (vallen > 253)
                    len = 254;  // special marker

                // writes the length of it.
                if (fputc((unsigned char)len, fp) == EOF)
                {
                    Printf("Cannot write file: %s.\n", ar->Filename.c_str());
                    return -1;
                }

                if (len == 254) // real length is > 253
                {
                    fputc((unsigned char)(vallen / 256), fp);
                    fputc((unsigned char)(vallen % 256), fp);
                }

                // writes it if it's not a NULL value
                if (!is_null)
                    fprintf(fp, "%s", value.c_str());
            }
        }

        // print a checkpoint (exporting, no commit needed)
        if (ret % ar->CheckPoint == 0 && ret)
            Printf("Checkpoint at: %d lines.\n", ret);
        ret++;
    }

    time_t EndTime;
    time(&EndTime);
    Printf("\nStart   : %s", ctime(&StartTime));
    Printf("End     : %s",   ctime(&EndTime));
    Printf("Elapsed : %d seconds.\n",  (EndTime - StartTime));
    return ret;
}
//---------------------------------------------------------------------------------------
// binds string value to ibpp statement parameter
// ft variable is used to track datatype
// i  is the index of parameter
void FBExport::StringToParam(string src, IBPP::Statement& st, int i, IBPP::SDT ft)
{
    //Printf("Setting parameter: %d\n", i);

    std::string types[] = {
        "Array", "Blob", "Date", "Time", "Timestamp", "String",
        "16bit", "32bit", "64bit", "Float", "Double" };
    if (st->ParameterType(i) != ft)
    {
        char buff[300];
        sprintf(buff, "Input parameter type (%s) for column #%d does not match the\n"
            "statement parameter type (%s). Please re-export data or use CAST.\n",
            types[ft].c_str(), i, types[st->ParameterType(i)].c_str());
        DataFormatException dfe(buff);
        throw dfe;
    }

    switch (ft)
    {
        case IBPP::sdString:
            st->Set(i, src);
            break;
        case IBPP::sdInteger:
        case IBPP::sdSmallint:
        {
            reScaleInt(src, st->ParameterScale(i), i);
            int x = atol(src.c_str());
            st->Set(i, x);
            break;
        }
        case IBPP::sdDate:
        {
            IBPP::Date dat = (int)atol(src.c_str());
            st->Set(i, dat);
            break;
        }
        case IBPP::sdTime:
        {
            IBPP::Time tim = (int)atol(src.c_str());
            st->Set(i, tim);
            break;
        }
        case IBPP::sdTimestamp:
        {
            IBPP::Timestamp ts;
            int tsa[6];
            tsa[0] = (int)atol(src.substr(0, 4).c_str());
            for (int x=1; x<6; x++)
                tsa[x] = (int)atol(src.substr(x*2+2, 2).c_str());
            ts.SetDate(tsa[0], tsa[1], tsa[2]);
            ts.SetTime(tsa[3], tsa[4], tsa[5]);
            st->Set(i, ts);
            break;
        }
        case IBPP::sdFloat:
        {
            float f = atof(src.c_str());
            st->Set(i, f);
            break;
        }
        case IBPP::sdDouble:
        {
            double d = atof(src.c_str());
            st->Set(i, d);
            break;
        }
        case IBPP::sdLargeint:
        {
            reScaleInt(src, st->ParameterScale(i), i);
            int64_t int64val = atoll(src.c_str());
            st->Set(i, int64val);
            break;
        }

        default:
            Printf("\nWARNING! Unsupported datatype... value set to NULL\n");
            st->SetNull(i);
    }

}
//---------------------------------------------------------------------------------------
// imports data from "file" into database using ibpp statement "st", and "sql" insert statement
// returns number of rows inserted, or -1 if error
int FBExport::Import(IBPP::Statement& st, FILE *fp)
{
    int Errors = 0;
    Printf("Importing data...\n");
    time_t StartTime;
    time(&StartTime);

    // print SQL statement to stderr and prepare
    Printf("SQL: %s\n", ar->SQL.c_str());
    Printf("Prepare statement...");
    st->Prepare(ar->SQL);
    Printf("Done.\n");

    // read in field types - into dynamicaly created array
    IBPP::SDT *ft;
    ft = new IBPP::SDT[fieldcount];
    for (int i=0; i<fieldcount; i++)
        ft[i] = (IBPP::SDT)(fgetc(fp));

    // load data
    int ret = 0;    // number of rows entered - counter
    while (!feof(fp))
    {
        bool ErrorInHere = false;
        int size;           // size of value in bytes
        char buff[65536];     // buffer to load data from file
        CurrentData = "";

        for (int i=0; i<fieldcount; i++)    // load each field value...
        {
            bool need_this_field =  (parmap.end() != parmap.find(i+1));
            if (ft[i] == IBPP::sdBlob)
            {
                int result = ReadBlob(fp, st, i+1, need_this_field);    // try...catch..etc
                if (result == -1)       // EOF detected (important if the first column's type is Blob)
                {
                    size = EOF;
                    break;
                }
                else if (result == -2)  // Blob errors are fatal
                {                       // since we don't know where to continue reading the file
                    delete[] ft;
                    return -1;
                }
            }
            else
            {
                bool is_null = false;
                size = fgetc(fp);

                if (size == EOF)    // only breaks "for" loop - see other check
                    break;          // again - just after the "for" loop

                if (size == 255)
                    is_null = true;

                if (size == 254)    // size > 253, read it
                {
                    int first = fgetc(fp);
                    int second = fgetc(fp);
                    size = first * 256 + second;
                }

                if (!is_null)
                    fread(buff, size, 1, fp);

                if (ErrorInHere)    // make sure it reads the whole row
                    continue;       // version 1.0 had this bug.

                string S;
                if (!is_null)
                {
                    buff[size] = '\0';
                    S = buff;
                }
                else
                    S = "[null]";

                if (i)
                    CurrentData += ",";
                CurrentData += S;

                try
                {
                    if (need_this_field) // we need this field data
                    {
                        if (!is_null)
                        {
                            #pragma warn -sig           // to aviod Warning: conversion may lose significant digits
                            StringToNumParams(S, st, i+1, ft[i]);       // ... and assign it to params
                            #pragma warn +sig       // bring it back
                        }
                        else
                        {
                            for (set<int>::iterator j = parmap[i+1].begin(); j != parmap[i+1].end(); j++)
                                st->SetNull(*j);
                        }
                    }
                    //else
                    //  Printf("Field not needed\n");
                }
                catch(IBPP::Exception &e)
                {
                    Printf("\nIBPP Error: %s\n", e.ErrorMessage());
                    Printf("\nCurrent field: %d of %d.\n", i+1, fieldcount);
                    ErrorInHere = true;
                }
            }
        }

        if (size == EOF)    // if break was signaled inside for loop (above)
            break;          // see comments above

        if (!ErrorInHere)
        {
            try
            {
                st->Execute();
                ret++;          // increase row counter
            }
            catch (IBPP::Exception &e)
            {
                Printf("\nIBPP Error: %s\n", e.ErrorMessage());
                ErrorInHere = true;
            }
        }

        if (ErrorInHere)
        {
            WereErrors = true;
            if (ar->Operation == xopInsert
                            || ar->Operation == xopInsertFull ) // dump data if insert failed
                Printf("Data: %s\n", CurrentData.c_str());

            // output error
            if (ar->IgnoreErrors > -1 && ++Errors > ar->IgnoreErrors)
                break;
        }

        // print checkpoint at each "ar->Checkpoint" lines
        if (ret % ar->CheckPoint == 0 && ret)
        {
            Printf("Checkpoint at: %d lines.", ret);
            if (ar->CommitOnCheckpoint)
            {
                IBPP::Transaction t = st->TransactionPtr();
                if (t.intf())
                {
                    t->Commit();
                    Printf(" Transaction commited.");
                    t->Start();     // start new transaction
                }
            }
            Printf("\n");
        }
    }

    // free Field Types array
    delete[] ft;

    time_t EndTime;
    time(&EndTime);
    Printf("\nStart   : %s", ctime(&StartTime));
    Printf("End     : %s",   ctime(&EndTime));
    Printf("Elapsed : %d seconds.\n",  (EndTime - StartTime));

    return ret;
}
//---------------------------------------------------------------------------------------
std::string getAlign(IBPP::Statement& st, int col)
{
    switch (st->ColumnType(col))
    {
        case IBPP::sdInteger:
        case IBPP::sdSmallint:
        case IBPP::sdFloat:
        case IBPP::sdDouble:
        case IBPP::sdLargeint:
            return " align=right";
    };
    return "";
}
//---------------------------------------------------------------------------------------
// statement is prepared, just need to fetch all into file
// returns: # of rows exported, -1 on error
int FBExport::ExportHuman(IBPP::Statement& st, FILE *fp)
{
    register int fc = st->Columns();

    string prefix, suffix;
    if (ar->ExportFormat == xefCSV)
    {
        Printf("Exporting data in CSV format...\n");
        for (register int i=1; i<=fc; i++)   // output CSV header.
        {
            if (i > 1)
                fprintf(fp, ",");
            fprintf(fp, "%s", st->ColumnAlias(i));
        }
        fprintf(fp, "\n");
    }
    else if (ar->ExportFormat == xefInserts)
    {
        Printf("Exporting data as INSERT statements...\n");
        string column_list;
        for (register int i=1; i<=fc; i++)   // ... export all fields to file.
        {
            if (i > 1)
                column_list += ",";
            column_list += st->ColumnAlias(i);
        }
        prefix = "INSERT INTO " + string(st->ColumnTable(1)) + " (" + column_list + ") values (";
        suffix = ");";
    }
    else if (ar->ExportFormat == xefHTML)
    {
        Printf("Exporting data as HTML statements...\n");
        fprintf(fp, "<HTML><BODY bgcolor=white><TABLE bgcolor=black cellspacing=1 cellpadding=3 border=0><tr bgcolor=white>\n");
        for (register int i=1; i<=fc; i++)   // output CSV header.
            fprintf(fp, "<td><b>%s</b></td>", st->ColumnAlias(i));
        fprintf(fp, "</tr>\n");
        prefix = "<tr bgcolor=white>";
        suffix = "</tr>";
    }

    time_t StartTime;
    time(&StartTime);

    // loop through all records in dataset, and ...
    int ret=0;
    while (st->Fetch())
    {
        fprintf(fp, "%s", prefix.c_str());
        for (register int i=1; i<=fc; i++)   // ... export all fields to file.
        {
            if (ar->ExportFormat == xefHTML)
                fprintf(fp, "<td%s>%s</td>", getAlign(st, i).c_str(), CreateHumanString(st, i).c_str());
            else
            {
                if (i > 1)
                {
                    if (ar->ExportFormat == xefInserts)
                        fprintf(fp, ",");
                    else if (ar->ExportFormat == xefCSV)
                        fprintf(fp, "%s", ar->Separator.c_str());
                }
                fprintf(fp, "%s", CreateHumanString(st, i).c_str());
            }
        }
        fprintf(fp, "%s\n", suffix.c_str());

        // print a checkpoint
        if (ret % ar->CheckPoint == 0 && ret)
            Printf("Checkpoint at: %d lines.\n", ret);
        ret++;
    }

    if (ar->ExportFormat == xefHTML)
        fprintf(fp, "</table></body></html>\n");

    time_t EndTime;
    time(&EndTime);
    Printf("\nStart   : %s", ctime(&StartTime));
    Printf("End     : %s",   ctime(&EndTime));
    Printf("Elapsed : %d seconds.\n",  (EndTime - StartTime));

    return ret;
}
//---------------------------------------------------------------------------------------
int FBExport::Init(Arguments *a)
{
    ar = a;         // move to internal
    int retval = 0;
    WereErrors = false;

    // error while parsing, or insufficient args
    if (ar->Error != "OK")
    {
        printf("--------------------------------\n");
        printf("FBExport v%s by Milan Babuskov (mbabuskov@yahoo.com), using IBPP %d.%d.%d.%d\n",
                    FBEXPORT_VERSION,
                    IBPP::Version >> 24,
                    (IBPP::Version >> 16) % 256,
                    (IBPP::Version >> 8) % 256,
                    IBPP::Version % 256);
        printf("Tool for importing/exporting data with Firebird and InterBase databases.\n");
        printf("Usage: fbexport -[S|Sc|Si|Sh|I|If|X|L] Options\n\n");
        printf(" -S  Select = output to file  (S - binary, Si - INSERTs, Sc - CSV, Sh - HTML)\n");
        printf(" -I  Insert = input from file\n");
        printf(" -If Insert by Full SQL = input from file, by parameterized SQL\n");
        printf(" -X  eXecute SQL statement, use with -F to execute sql scripts\n");
        printf(" -L  List connected users\n\n");
        printf("Options are:                           -H Host          [LOCALHOST]\n");
        printf(" -D Database                           -U Username      [SYSDBA]\n");
        printf(" -F Filename (use - for stdout)        -O Role\n");
        printf(" -P Password                           -T Trim chars    [off]\n");
        printf(" -A \"Charset\"                          -J \"Date format\" [D.M.Y]\n");
        printf(" -Q \"SQL Query statement\"              -K \"Time format\" [H:M:S]\n");
        printf(" -C # = Checkpoint at # rows [1000]    -M Commit at each checkpoint [off]\n");
        printf(" -E # = Ignore up to # errors [0] Set to -1 to ignore all\n");
        printf(" -R Rollback transaction if any errors occur while importing [off]\n");
        printf(" -V Table = Verbatim copy of table (use -Q to set where clause if desired)\n");
        printf(" -B Separator [,] = Field separator for CSV export. Allows special value: TAB\n");
        printf("Command-line options are not case-sensitive (except the TAB setting).\n\n");
        if (ar->Count > 1)
            printf("Error: %s\n", ar->Error.c_str());

        return 1;
    }

    if (ar->CommitOnCheckpoint && ar->Rollback)
    {
        printf("Error: Cannot use options -M and -R at the same time.\n");
        return 9;
    }

    try
    {
        IBPP::Database db1;
        std::string chset(ar->Charset);
        if (ar->Operation != xopListUsers && chset == "")  // connect and read
        {
            Printf("Checking database charset...");
            db1 = IBPP::DatabaseFactory(ar->Host, ar->Database, ar->Username, ar->Password, ar->Role, "NONE", "");
            db1->Connect();
            IBPP::Transaction tr = IBPP::TransactionFactory(db1, IBPP::amRead);
            tr->Start();
            IBPP::Statement st = IBPP::StatementFactory(db1, tr);
            st->Prepare("SELECT rdb$character_set_name FROM rdb$database");
            st->Execute();
            st->Fetch();
            st->Get(1, chset);
            chset.erase(chset.find_last_not_of(" ")+1);
            Printf("%s.\n", chset.c_str());
            tr->Commit();
            db1->Disconnect();
        }
        db1 = IBPP::DatabaseFactory(ar->Host, ar->Database, ar->Username, ar->Password, ar->Role, chset, "");

        IBPP::Transaction tr1;

        Printf("Connecting to: \'%s\' as \'%s\'...", ar->Host.c_str(), ar->Username.c_str());
        db1->Connect();
        Printf("Connected.\n");

        Dialect = db1->Dialect();
        if (ar->Operation == xopListUsers)
        {
            Printf("Showing list of connected users:\n");
            std::vector<std::string> users;
            db1->Users(users);
            for (std::vector<std::string>::iterator it = users.begin(); it != users.end(); ++it)
                Printf("%s\n", (*it).c_str());
        }
        else if (ar->Operation == xopExec && !ar->Filename.empty())       // insert
        {
            Printf("Creating transaction...");
            if (ar->NoAutoUndo)
                tr1 = IBPP::TransactionFactory(db1, IBPP::amWrite, IBPP::ilConcurrency, IBPP::lrWait, IBPP::tfNoAutoUndo);
            else
                tr1 = IBPP::TransactionFactory(db1, IBPP::amWrite);
            Printf("Done.\n");

            Printf("Create statement...");
            IBPP::Statement st1 = IBPP::StatementFactory(db1, tr1);
            Printf("Done.\n");

            // we need to open stdin and stdout in binary mode on Windows
#ifdef IBPP_WINDOWS
            if (ar->Filename == "-")
                setmode(fileno(stdin), O_BINARY);
#endif

            FILE *fp = (ar->Filename == "-" ? stdin : fopen(ar->Filename.c_str(), "rb"));
            if (!fp)
            {
                Printf("Cannot open file: %s for reading.\n", ar->Filename.c_str());
                return -1;
            }
            int rows = ExecuteSqlScript(st1, tr1, fp);
            if (rows < 0)
            {
                retval = 6;
                Printf("Script execution failed.\n\nData: %s\n", CurrentData.c_str());
            }
            else
            {
                Printf("%d statements executed from ", rows);
                if (ar->Filename == "-")
                    Printf("standard input (stdin).\n");
                else
                    Printf("%s.\n", ar->Filename.c_str());
            }
            if (ar->Filename != "-")
                fclose(fp);
        }
        else
        {
            Printf("Creating and starting transaction...");
            if (ar->NoAutoUndo)
                tr1 = IBPP::TransactionFactory(db1, IBPP::amWrite, IBPP::ilConcurrency, IBPP::lrWait, IBPP::tfNoAutoUndo);
            else
                tr1 = IBPP::TransactionFactory(db1, IBPP::amWrite);
            tr1->Start();
            Printf("Done.\n");

            Printf("Create statement...");
            IBPP::Statement st1 = IBPP::StatementFactory(db1, tr1);
            Printf("Done.\n");

            // do export
            if (ar->Operation == xopSelect)
            {
                if (ar->VerbatimCopyTable != "")    // build SQL
                {
                    #pragma warn -sig
                    for (string::iterator it1=ar->VerbatimCopyTable.begin(); it1!=ar->VerbatimCopyTable.end(); it1++)
                        *it1 = ::toupper(*it1);
                    #pragma warn +sig

                    Printf("Doing verbatim export of table: %s\n", ar->VerbatimCopyTable.c_str());
                    st1->Prepare(
                        "select r.rdb$field_name "
                        "from rdb$relation_fields r "
                        "left join rdb$fields f on r.rdb$field_source = f.rdb$field_name "
                        "where r.rdb$relation_name = ? and f.rdb$computed_blr is null "
                        "order by 1");
                    st1->Set(1, ar->VerbatimCopyTable);
                    st1->Execute();

                    std::string colist;
                    while (st1->Fetch())
                    {
                        std::string temp;
                        st1->Get(1, temp);
                        if (colist != "")
                            colist += ",";
                        temp.erase(temp.find_last_not_of(' ')+1);   // trim
                        colist += temp;
                    }

                    // create SQL (select + list + where clause)
                    ar->SQL = "SELECT " + colist + " FROM " + ar->VerbatimCopyTable + " " + ar->SQL;
                    Printf("SQL: %s\n", ar->SQL.c_str());
                }

                Printf("Prepare statement...");
                st1->Prepare(ar->SQL.c_str());
                Printf("Done.\n");
                Printf("Exec statement...");
                st1->Execute();
                Printf("Done.\n");

            // we need to open stdin and stdout in binary mode on Windows
#ifdef IBPP_WINDOWS
                if (ar->Filename == "-")
                    setmode(fileno(stdout), O_BINARY);
#endif
                // if output is not binary, and filename is not given: output to stdout
                if (ar->Filename == "" && ar->ExportFormat != xefDefault)
                    ar->Filename = "-";
                FILE *fp = (ar->Filename == "-" ? stdout : fopen(ar->Filename.c_str(),
                                                                (ar->ExportFormat == xefDefault ? "w+b" : "w+t")));
                if (!fp)
                {
                    Printf("Cannot open file: %s for writing.\n", ar->Filename.c_str());
                    return -1;
                }
                int rows;
                if (ar->ExportFormat == xefDefault)
                    rows = Export(st1, fp);
                else
                    rows = ExportHuman(st1, fp);

                if (rows < 0)
                    Printf("Export failed.\n");
                else
                {
                    Printf("%d rows exported to ", rows);
                    if (ar->Filename == "-")
                        Printf("standard output (stdout).\n");
                    else
                        Printf("%s.\n", ar->Filename.c_str());
                }
                if (ar->Filename != "-")
                    fclose(fp);
            }
            //
            // IF WE ARE INSERTING
            else if (ar->Operation == xopInsert
                  || ar->Operation == xopInsertFull)    // do import
            {

                // Open input file
            // we need to open stdin and stdout in binary mode on Windows
#ifdef IBPP_WINDOWS
                if (ar->Filename == "-")
                    setmode(fileno(stdin), O_BINARY);
#endif
                FILE *fp = (ar->Filename == "-" ? stdin : fopen(ar->Filename.c_str(), "rb"));
                if (!fp)
                {
                    Printf("Cannot open file: %s for reading.\n", ar->Filename.c_str());
                    return -1;
                }

                // Substitute -Q parameter if V is on
                if (ar->VerbatimCopyTable != "")    // build SQL
                {
                    #pragma warn -sig
                    for (string::iterator it1=ar->VerbatimCopyTable.begin(); it1!=ar->VerbatimCopyTable.end(); it1++)
                        *it1 = ::toupper(*it1);
                    #pragma warn +sig

                    Printf("Doing verbatim import of table: %s\n", ar->VerbatimCopyTable.c_str());
                    st1->Prepare("select r.rdb$field_name "
                        "from rdb$relation_fields r "
                        "left join rdb$fields f on r.rdb$field_source = f.rdb$field_name "
                        "where r.rdb$relation_name = ? and f.rdb$computed_blr is null "
                        "order by 1");
                    st1->Set(1, ar->VerbatimCopyTable);
                    st1->Execute();

                    std::string colist;
                    while (st1->Fetch())
                    {
                        std::string temp;
                        st1->Get(1, temp);
                        if (colist != "")
                            colist += ",";
                        temp.erase(temp.find_last_not_of(' ')+1);   // trim
                        colist += temp;
                    }

                    // create SQL (select + list + where clause)
                    ar->SQL = "INSERT INTO " + ar->VerbatimCopyTable + " (" + colist + ") ";
                }

                // remove semi-colon (if any)
                int f = ar->SQL.find(';');
                while (f > -1)
                {
                        ar->SQL.erase(f, 1);
                        f = ar->SQL.find(';');
                }

                // read first two bytes to see if file is compatible
                int first = fgetc(fp);
                int second = fgetc(fp);

                if (first != 0 || second != FBEXPORT_FILE_VERSION)
                {
                    Printf("This file is not compatible with this version of FBExport\nPlease import the data to database with the same version you used to export.\n");
                    return -1;
                }

                // Read field count
                fieldcount = fgetc(fp);

                // add values clause to insert statement
                // only if op = 'I'
                if (ar->Operation == xopInsert)
                {
                    ar->SQL += " VALUES (";
                    char num[10];
                    for (int i=0; i<fieldcount; i++)
                    {
                        if (i)
                            ar->SQL += ",";
                        sprintf(num, "%d", i+1);
                        ar->SQL += ":" + (string)num;
                    }
                    ar->SQL += ")";
                }

                // Build parameter map
                BuildParamMap();

                int rows = Import(st1, fp);
                if (rows < 0)
                {
                    retval = 6;
                    Printf("Import failed.\n\nData: %s\n", CurrentData.c_str());
                }
                else
                {
                    Printf("%d rows imported from ", rows);
                    if (ar->Filename == "-")
                        Printf("standard input (stdin).\n");
                    else
                        Printf("%s.\n", ar->Filename.c_str());
                }

                if (ar->Filename != "-")
                    fclose(fp);
            }
            else if (ar->Operation == xopExec)       // do execute query
            {
                Printf("Prepare statement.\n");
                st1->Prepare(ar->SQL.c_str());
                Printf("Exec statement.\n");
                st1->Execute();
                Printf("Query executed.\n");
            }

            if (ar->Rollback && WereErrors)
            {
                tr1->Rollback();
                Printf("\nTransaction rolled back!\n");
            }
            else
            {
                tr1->Commit();
                Printf("\nTransaction commited.\n");
            }
        }
    }
    catch (IBPP::Exception &e)
    {
        Printf("ERROR!\n");
        Printf(e.ErrorMessage());
        retval = 3;
    }
    catch (std::exception &e)
    {
        Printf("Fatal exception occured!\n");
        Printf(e.what());
        retval = 4;
    }
    catch (...)
    {
        Printf("ERROR!\nUnknown runtime exception occured !\n\n");
        retval = 5;
    }

    return retval;
}

//---------------------------------------------------------------------------------------
// Author: Istvan Matyasi
//
// Builds (incoming field position) -> (SQL parameter positions) map from ar->SQL.
// Updates ar->SQL to have question marks as params instead of coloned-style (e.g. ':5')
// Example:
// SQL = 'execute procedure xyz(:1, :5, :1, :3)'
// Map : 1 -> 1,3
//       5 -> 2
//       3 -> 4
// puts result in this->parmap
void FBExport::BuildParamMap()
{
    enum {OutSide, InsideQuotes, InsideDQuotes} state; // states for quote analysis

    string out = "", digits = "0123456789";
    string::size_type i, j, n, m=0;
    state = OutSide;

    for (i = 0; i < ar->SQL.length(); i++)
    {
        if (state == OutSide)
        {
            if ( ar->SQL[i] == ':')
            {
                j = ar->SQL.find_first_not_of(digits, i+1);
                n = atoi(ar->SQL.substr(i+1, j).c_str()); // param number
                parmap[n].insert(++m);
                ar->SQL.replace(i,j-i,"?");
            }
            else if (ar->SQL[i] == '\"')
                state = InsideDQuotes;
            else if (ar->SQL[i] == '\'')
                state = InsideQuotes;
        }
        else if (state == InsideDQuotes && ar->SQL[i] == '\"')
            state = OutSide;
        else if (state == InsideQuotes && ar->SQL[i] == '\'')
            state = OutSide;
    }
}

//---------------------------------------------------------------------------------------
// Author: Istvan Matyasi
//
// Replaces all occurrences of param number i of type ft with string src in statement st
void FBExport::StringToNumParams(string src, IBPP::Statement& st, int i, IBPP::SDT ft)
{
    for (set<int>::iterator j = parmap[i].begin(); j != parmap[i].end(); j++)
        StringToParam (src, st, *j, ft);
}

//---------------------------------------------------------------------------------------
int statement_length(FILE *fp)
{
    register    int    c = 0, tmp = 0;
    register    int    l = 0;
    fpos_t pos;
    fgetpos(fp, &pos);
    bool comments = false;
    bool comments_over = false;

    while (';' != c && (c = getc(fp)) != EOF)
    {
        switch(c)
        {
            case '\r':
            case '\n':
                break;
            case '/':
                if((tmp = getc(fp)) == '*')
                    comments = true;
                else if(tmp != EOF)
                    ungetc(tmp, fp);
                break;
            case '*':
                if((tmp = getc(fp)) == '/' && comments)
                    comments_over = true;
                else if(tmp != EOF)
                    ungetc(tmp, fp);
                break;
        }
        if(!comments)
            l++;
        if (comments_over)
            comments_over = comments = false;
    }
    fsetpos(fp, &pos);

    if (EOF == c && l == 0)
        return l;
    return (ferror (fp)) ? -1 : l;
}
//------------------------------------------------------------------------------
char *read_statement(char *s, int n, FILE *fp)
{
    register    int    c = 0, tmp = 0;
    register    char   *P;
    P = s;
    bool comments = false;
    bool comments_over = false;

    while (';' != c && --n > 0 && (c = getc(fp)) != EOF)
    {
        switch(c)
        {
            case '\r':
            case '\n':
                break;
            case '/':
                if((tmp = getc(fp)) == '*')
                    comments = true;
                else if(tmp != EOF)
                    ungetc(tmp, fp);
                break;
            case '*':
                if((tmp = getc(fp)) == '/' && comments)
                    comments_over = true;
                else if(tmp != EOF)
                    ungetc(tmp, fp);
                break;
        }
        if(!comments)
            *P++ = (char)c;
        if (comments_over)
            comments_over = comments = false;
    }

    if (EOF == c && P == s)
        return( NULL );
    *P = 0;
    return (ferror (fp)) ? NULL : s;
}
//------------------------------------------------------------------------------
int FBExport::ExecuteSqlScript(IBPP::Statement& st, IBPP::Transaction &tr, FILE *fp)
{
    int Errors = 0;
    int length;
    int buffer_size = 1024;
    char *buffer;
    const char *commit_string = "COMMIT";
    int commit_string_length;
    bool transaction_started = false;
    bool byme = false;
    commit_string_length = strlen(commit_string);
    buffer = new char[buffer_size];
    if(!buffer)
        return -1;
    int result = 0;
    do
    {
        length = statement_length(fp);
        if(length < 1)
            break;
        if((length + 1) > buffer_size)
        {
            buffer_size = length + 1;
            delete[] buffer;
            buffer = new char[buffer_size];
            if(!buffer)
                return -1;
        }
        if(read_statement(buffer, buffer_size, fp))
        {
            CurrentData = buffer;
            if(strnicmp(buffer, commit_string, min(length, commit_string_length)) == 0
                && transaction_started)
            {
                tr->Commit();
                Printf("Transaction commited.\n");
                transaction_started = false;
                byme = false;
                continue;
            }
            try
            {
                if(!transaction_started)
                {
                    tr->Start();
                    transaction_started = true;
                    if (!byme)
                        Printf("Transaction started.\n");
                    byme = false;
                }
                st->Execute(CurrentData);
                if (++result % ar->CheckPoint == 0)     // increase statement counter
                {
                    if (ar->CommitOnCheckpoint)
                    {
                        tr->Commit();
                        transaction_started = false;
                        byme = true;
                        Printf("Checkpoint at: %d statements. Transaction commited.\n", result);
                    }
                    else
                        Printf("Checkpoint at: %d statements.\n", result);
                }
            }
            catch (IBPP::Exception &e)
            {
                Printf("\nIBPP Error: %s\n", e.ErrorMessage());
                WereErrors = true;
                if (ar->IgnoreErrors > -1 && ++Errors > ar->IgnoreErrors)
                    break;
            }
        }
    } while (!feof(fp));
    delete[] buffer;

    if (transaction_started)
    {
        if (ar->Rollback && WereErrors)
        {
            tr->Rollback();
            Printf("Transaction rolled back!\n");
        }
        else
        {
            tr->Commit();
            Printf("Transaction commited.\n");
        }
    }
    return result;
}
//------------------------------------------------------------------------------
//  EOF
