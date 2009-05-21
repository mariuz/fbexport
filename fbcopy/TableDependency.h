///////////////////////////////////////////////////////////////////////////////
//
//  Author      : Thiago Borges de Oliveira (thborges@gmail.com)
//  Purpose     : Implements a Tree structure for table dependencies
//
///////////////////////////////////////////////////////////////////////////////
/*

Copyright (c) 2005,2006 Milan Babuskov
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
///////////////////////////////////////////////////////////////////////////////

#ifndef TableDependencieH
#define TableDependencieH

#include <list>
#include <string>
//---------------------------------------------------------------------------------------
class TableDependency
{
private:
public:
    TableDependency();
    TableDependency(std::string name);
    ~TableDependency();

    std::string tableName;
    std::list<TableDependency *> dependencies;
    bool operator ==(const TableDependency a) const;
    bool operator > (const TableDependency a) const;
    bool operator < (const TableDependency a) const;
    bool existsInTree(TableDependency* a, const std::string ntable) const;
    void printTree(TableDependency* a, int nivel = 0) const;
};
//---------------------------------------------------------------------------------------
#endif
