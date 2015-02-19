///////////////////////////////////////////////////////////////////////////////
//
//  File        : ParseArgs.cpp
//  Author      : Milan Babuskov (mbabuskov@yahoo.com)
//  Purpose     : Implementation of Arguments class
//                Parses command line arguments and assigns values to Argument
//                class attributes. The idea is to take all command-line
//                arguments into single class for easy management
//
//  Note:         Uses IBPP library (also released under MPL) to access Firebird
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
//  Contributor(s): Istvan Matyasi.
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#pragma hdrstop
#include "ParseArgs.h"
#include "FBExport.h"

Arguments::Arguments()
{
    Operation = xopNone;
    Error = "OK";
}

Arguments::Arguments(int argc, char **argv)
{
    Count = argc;

    if (argc < 4)
    {
        Error = "Insufficient number of arguments on the command line.";
        return;
    }

    ExportFormat = xefDefault;
    CheckPoint = 1000;  // default values (public)
    IgnoreErrors = 0;
    Rollback = false;
    TrimChars = false;
    NoAutoUndo = false;
    CommitOnCheckpoint = false;
    Host = "LOCALHOST";
    Username = "SYSDBA";

    /* read username and password from environment vars (if they are set) */
    char *usr = getenv("ISC_USER");
    char *pwd = getenv("ISC_PASSWORD");
    if (usr)
        Username = usr;
    if (pwd)
        Password = pwd;
    //fprintf(stderr, "PASS = %s\n", Password.c_str());

    Charset = "";
    SQL = "";
    Role = "";
    VerbatimCopyTable = "";
    Filename = "";              // defaults to none.
    DateFormat = "D.M.Y";
    TimeFormat = "H:M:S";
    Separator = ",";

    Error = "OK";       // initial values (local)
    string temp;
    Operation = xopNone;

    // search all important data and assign
    for (int p = 1; p < argc; p++)
    {
        if (argv[p][0] != '-')
        {
            Error = "Switches must begin with -";
            return;
        }

        char c = argv[p][1];            // allow uppercase/lowercase
        if (c >= 'a' && c <= 'z')
            c += 'A' - 'a';

        char doubles[] = "FDHUPQCEAVJKOB";
        bool in_doubles = false;
        for (int i=0; i < (sizeof(doubles)/sizeof(char)); i++)
        {
            if (c == doubles[i])
            {
                in_doubles = true;
                if (argc < p + 2 && argv[p][2] == '\0')
                {
                    Error = "Argument ";
                    Error += c;
                    Error += " needs a value.";
                    return;
                }
                break;
            }
        }

        char *arg = argv[p+1];      // next arg
        if (argv[p][2] != '\0')     // -Hhostname
        {
            if (in_doubles)
                arg = argv[p] + 2;
            else if (c != 'S' && c != 'I')
            {
                Error = "Argument ";
                Error += c;
                Error += " doesn't need a value. It should stand by itself.";
                return;
            }
        }
        else if (in_doubles)    // -H hostname    <- need to skip arg: hostname
            p++;

        switch (c)
        {
            case 'A': Charset = arg;                    break;
            case 'B': Separator = arg;
                if (Separator == "TAB")
                    Separator = "\t";
                break;
            case 'C': CheckPoint = atoi(arg);           break;
            case 'D': Database = arg;                   break;
            case 'E': IgnoreErrors = atoi(arg);         break;
            case 'F': Filename = arg;                   break;
            case 'H': Host = arg;                       break;
            case 'I': Operation = xopInsert;
                if (argv[p][2] == 'f')
                    Operation = xopInsertFull;
                break;
            case 'J': DateFormat = arg;                 break;
            case 'K': TimeFormat = arg;                 break;
            case 'L': Operation = xopListUsers;         break;
            case 'M': CommitOnCheckpoint = true;        break;
            case 'N': NoAutoUndo = true;                break;
            case 'O': Role = arg;                       break;
            case 'P': Password = arg;                   break;
            case 'Q': SQL = arg;                        break;
            case 'R': Rollback = true;                  break;
            case 'S': Operation = xopSelect;
                if (argv[p][2] == 'i')
                    ExportFormat = xefInserts;
                else if (argv[p][2] == 'c')
                    ExportFormat = xefCSV;
                else if (argv[p][2] == 'h')
                    ExportFormat = xefHTML;
                break;
            case 'T': TrimChars = true;                 break;
            case 'U': Username = arg;                   break;
            case 'V': VerbatimCopyTable = arg;          break;
            case 'X': Operation = xopExec;              break;

            default :
                Error = "Unknown switch -";
                Error += c;
                return;
        }
    }

    if (Operation == xopNone)
        Error = "You must specify operation switch. (S, I, X or L).";
}

#pragma package(smart_init)
