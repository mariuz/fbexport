///////////////////////////////////////////////////////////////////////////////
//
//  Author      : Milan Babuskov (mbabuskov@yahoo.com)
//  Purpose     : Arguments class definition
//                Parses command line arguments and assigns values to Args
//                class attributes. The idea is to take all command-line
//                arguments into single class for easy management
//
//  Used to parse command-line parameters (arguments) into useful variables
//  Can be used to parse any command line and output to program
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
#ifndef ArgsH
#define ArgsH

#include <string>
#ifndef _RWSTD_NO_NAMESPACE
  using namespace std;
#endif

typedef enum { opNone, opDefine, opAlter, opCopy, opSingle, opCompare
    } tOperation;

struct DatabaseInfo
{
    string Username;
    string Password;
    string Database;
    string Hostname;
    string Charset;
};

class Args
{
private:
    void createDBInfo(DatabaseInfo& db, char *string);
public:
    enum WhatToShow {
        ShowCommon    = 0x01,
        ShowMissing   = 0x02,
        ShowExtra     = 0x04,
        ShowDifferent = 0x08
    };

    DatabaseInfo Src;
    DatabaseInfo Dest;

    bool FireTriggers;
    bool KeepGoing;
    bool SingleTransaction;
    bool NotNulls;
    bool Html;
    bool Verbose;
    bool Update;
    bool Limited;
    int DisplayDifferences;
    tOperation Operation;

    string Error;       // if not "OK" - error text
    Args(int argc, char **argv);    // CLI uses this
};
//---------------------------------------------------------------------------
#endif
