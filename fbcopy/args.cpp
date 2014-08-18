///////////////////////////////////////////////////////////////////////////////
//
//  Author      : Milan Babuskov (mbabuskov@yahoo.com)
//  Purpose     : Implementation of Args class
//                Parses command line arguments and assigns values to Args
//                class attributes. The idea is to take all command-line
//                arguments into single class for easy management
//
///////////////////////////////////////////////////////////////////////////////
/*

Copyright (c) 2005,2006 Milan Babuskov

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
//---------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#pragma hdrstop
#include "args.h"
Args::Args(int argc, char **argv)
{
    if (argc == 1)
    {
        Error = "Display help";
        return;
    }
    if (argc != 4)
    {
        Error = "Wrong number of arguments on the command line.";
        return;
    }

    DisplayDifferences = 0;
    FireTriggers = false;
    KeepGoing = false;
    SingleTransaction = false;
    NotNulls = true;
    Html = false;
    Verbose = false;
    Update = false;
    Error = "OK";       // initial values (local)
    string temp;
    Operation = opNone;
    for (int i=0; argv[1][i]; i++)
    {
        char c = argv[1][i];            // allow uppercase/lowercase
        if (c >= 'a' && c <= 'z')
            c += 'A' - 'a';
        switch (c)
        {
            case 'D':   Operation = opDefine;       break;
            case 'A':   Operation = opAlter;        break;
            case 'C':   Operation = opCopy;         break;
            case 'S':   Operation = opSingle;       break;
            case 'X':   Operation = opCompare;      break;
            case 'K':   KeepGoing = true;           break;
            case 'E':   SingleTransaction = true;   break;
            case 'N':   NotNulls = false;           break;
            case 'H':   Html = true;                break;
            case 'F':   FireTriggers = true;        break;
            case 'V':   Verbose = true;             break;
            case 'U':   Update  = true;             break;
            case '1':   case '2':    case '3':  case '4':
                DisplayDifferences |= (1 << (c-'1'));
                break;

            default:
                fprintf(stderr, "Unknown option %c.\n", c);
                break;
        }
    }

    createDBInfo(Src, argv[2]);
    createDBInfo(Dest, argv[3]);
    if (Operation == opNone)
        Error = "You must specify operation: D, A, C, S or X.";
    if (!Html && DisplayDifferences != 0)
        Error = "Options 1234 and only available together with H";
    if (NotNulls == false && Operation != opAlter)
        Error = "Option N is only available with option A";
    if (Html && (Operation == opSingle || Operation == opCopy))
        Error = "Option H is only available with D, A or X";
    if (Update && Operation != opCopy)
        Error = "Option U is only avaliable with C";
}
void Args::createDBInfo(DatabaseInfo& db, char *string)
{
    db.Charset = "";
    db.Hostname = "";

    std::string s(string);
    std::string::size_type p = s.find("@");
    if (p == std::string::npos)
    {
        char *usr = getenv("ISC_USER");
        char *pwd = getenv("ISC_PASSWORD");
        if (!usr || !pwd)
        {
            Error = "Missing @ in path and ISC_USER and ISC_PASSWORD not set.";
            return;
        }
        db.Username = usr;
        db.Password = pwd;
    }
    else
    {
        std::string left = s.substr(0, p);
        s.erase(0, p+1);
        p = left.find(":");
        if (p == std::string::npos)
        {
            Error = "Missing : in username:password part";
            return;
        }

        db.Username = left.substr(0, p);
        db.Password = left.substr(p+1);
    }

    p = s.find(":");
    if (p != std::string::npos)
    {
#ifdef IBPP_WINDOWS
        if (p > 1)  // to avoid drive letter
        {
            db.Hostname = s.substr(0, p);
            s.erase(0, p+1);
        }
#else
        db.Hostname = s.substr(0, p);
        s.erase(0, p+1);
#endif
    }

    p = s.find("?");
    if (p != std::string::npos)
    {
        db.Charset = s.substr(p+1);
        s.erase(p);
    }
    db.Database = s;
}
#pragma package(smart_init)
