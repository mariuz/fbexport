///////////////////////////////////////////////////////////////////////////////
//
//  File        : ParseArgs.h
//  Author      : Milan Babuskov (mbabuskov@yahoo.com)
//  Purpose     : Arguments class definition
//                Parses command line arguments and assigns values to Argument
//                class attributes. The idea is to take all command-line
//                arguments into single class for easy management
//
//  Note        : Uses IBPP library (also released under MPL) to access Firebird
//                Check http://ibpp.sourceforge.net for more info
//
//
//  Used to parse command-line parameters (arguments) into useful variables
//  Can be used to parse any command line and output to program
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
//  Contributor(s): ______________________________________.
//
///////////////////////////////////////////////////////////////////////////////
//---------------------------------------------------------------------------
#ifndef ParseArgsH
#define ParseArgsH

#include <string>
#ifndef _RWSTD_NO_NAMESPACE
  using namespace std;
#endif

typedef enum { xopInsertFull, xopInsertUpdate, xopInsert, xopSelect, xopExec,
    xopListUsers, xopNone } XOperation;
typedef enum { xefDefault, xefCSV, xefInserts, xefHTML  } XExportFormat;
//----------------------------------------------------------------------------
class Arguments
{
private:
public:
    string Username;
    string Role;
    string Password;
    string Database;
    string Host;
    string SQL;
    string Filename;
    string Charset;
    string VerbatimCopyTable;
    string DateFormat;
    string TimeFormat;
    string Separator;
    int CheckPoint;
    int IgnoreErrors;
    bool Rollback;
    bool TrimChars;
    bool NoAutoUndo;
    bool CommitOnCheckpoint;
    XOperation Operation;
    XExportFormat ExportFormat;

    string Error;       // if not "OK" - error text

    int Count;          // argc
    Arguments();                        // GUI uses this
    Arguments(int argc, char **argv);   // CLI uses this
};
//----------------------------------------------------------------------------
#endif
