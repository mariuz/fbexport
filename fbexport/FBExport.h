///////////////////////////////////////////////////////////////////////////////
//
//  File        : fbexport.h
//  Author      : Milan Babuskov (mbabuskov@yahoo.com)
//  Purpose     : Declaration of FBExport class
//
//  Note        : Uses IBPP library (also released under MPL) to access Firebird
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
#ifndef FBExportH
#define FBExportH

#define FBEXPORT_FILE_VERSION 180
#define FBEXPORT_VERSION "1.80"
#include "ParseArgs.h"
#include "ibpp.h"

#include <exception>
#include <map>
#include <set>
#include <string>

//---------------------------------------------------------------------------
class DataFormatException: public std::exception
{
private:
    std::string messageM;
public:
    DataFormatException(const std::string& msg) 
    : messageM(msg) {};
    virtual ~DataFormatException() throw() {};
    virtual const char* what() const throw()
    {
        return messageM.c_str();
    }
};
//---------------------------------------------------------------------------
class FBExport
{
private:

    typedef map<int,set<int> > ParamMap;

    int Dialect;
    bool WereErrors;
    Arguments *ar;
    string CurrentData;

    ParamMap parmap;
    int fieldcount;

    unsigned char SDT2uc(IBPP::SDT st);
    void StringToParam(string src, IBPP::Statement& st, int i, IBPP::SDT ft);
    bool CreateString(IBPP::Statement& st, int col, string &value);
    string CreateHumanString(IBPP::Statement& st, int col);
    string GetHumanDate(int year, int month, int day);
    string GetHumanTime(int hour, int minute, int second);
    string GetHumanTimestamp(IBPP::Timestamp ts);
    void MakeInsertSQL(IBPP::Statement& st1, FILE*fp);
    void BuildParamMap();
    void StringToNumParams(string src, IBPP::Statement& st, int i, IBPP::SDT ft);

    int Export(IBPP::Statement& st, FILE *fp);
    int ExportHuman(IBPP::Statement& st, FILE *fp);
    int Import(IBPP::Statement& st, FILE *fp);
    int ExecuteSqlScript(IBPP::Statement& st, IBPP::Transaction &tr, FILE *fp);

    void WriteBlob(FILE *fp, IBPP::Statement& st, int col);
    int ReadBlob(FILE *fp, IBPP::Statement& st, int col, bool needed);

    // output abstraction layer, for cmdline it calls printf(), and for GUI it fills the textbox
    void Printf(const char *format, ...);
public:
    void *GUI;              // placeholder, GUI uses it, CLI version ignores it
    int Init(Arguments *a);
};
//---------------------------------------------------------------------------
#endif

