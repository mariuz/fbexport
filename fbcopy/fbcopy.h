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
#ifndef FBCopyH
#define FBCopyH

#define FBCOPY_VERSION "1.70"
#include <set>
#include <vector>
#include <list>
#include <string>
#include "TableDependency.h"
#include "args.h"
#include "ibpp.h"

//---------------------------------------------------------------------------
class FBCopy
{
private:
    std::vector<std::string> triggers;
    IBPP::Database src, dest;
    IBPP::Statement stDepsFK, stDepsCheck;
    IBPP::Transaction trans1, trans2, transDep;
    TableDependency tree;
    Args *ar;

    void disableTriggers();
    void enableTriggers();
    bool connect(IBPP::Database& db1, DatabaseInfo d);
    bool copy(std::string select, std::string insert);
    void addDeps(std::list<std::string>& deps, const std::string& table, IBPP::Statement& st);
    void getDependencies(TableDependency* dep, std::string ntable);
    void setDependencies(std::list<std::string> tableList);
    void preOrder(TableDependency* a, IBPP::Statement& st1, IBPP::Statement& st2, IBPP::Transaction& tr1);
    void compareTable(std::string table, IBPP::Statement& st1, IBPP::Statement& st2, IBPP::Transaction& tr1);
    int  compareGenerators(IBPP::Transaction tr1, IBPP::Transaction tr2);
    void compareGeneratorValues(const std::string& gfrom, const std::string& gto);
    void copyGeneratorValues(const std::string& gfrom, const std::string& gto);
    void compare();

    void compareData(const std::string& table, const std::string& fields, const std::string& where);
    int cmpData(IBPP::Statement& st1, IBPP::Statement& st2, int col);
    void addRow(int& counter, int type, IBPP::Statement st, int index, IBPP::Statement *st2 = 0);

    enum CompareOrCopy { ccCopy, ccCompareData };
    void setupFromStdin(CompareOrCopy action = ccCopy);

    std::string join(const std::set<std::string>& s, const std::string& qualifier, const std::string& glue);
    std::string params(const std::string& fieldlist);
    bool copyData(IBPP::Statement& st1, IBPP::Statement& st2, int col);
    bool copyBlob(IBPP::Statement& st1, IBPP::Statement& st2, int col);
    std::string getDatatype(IBPP::Statement& st1, std::string table, std::string fieldname, bool not_nulls = true);

public:
    int Run(Args *a);
};
//---------------------------------------------------------------------------
#endif

