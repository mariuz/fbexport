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

#include "TableDependency.h"
#include <stdio.h>

using namespace std;
//---------------------------------------------------------------------------------------
TableDependency::TableDependency()
{
}
//---------------------------------------------------------------------------------------
TableDependency::TableDependency(std::string name)
{
    tableName = name;
}
//---------------------------------------------------------------------------------------
TableDependency::~TableDependency()
{
    for (std::list<TableDependency *>::iterator it = dependencies.begin(); it != dependencies.end(); ++it)
        delete (*it);
}
//---------------------------------------------------------------------------------------
bool TableDependency::operator == (const TableDependency a) const
{
    return (tableName == a.tableName);
}
//---------------------------------------------------------------------------------------
bool TableDependency:: operator > (const TableDependency a) const
{
    return (a.tableName < tableName);
}
//---------------------------------------------------------------------------------------
bool TableDependency:: operator < (const TableDependency a) const
{
    return (a.tableName > tableName);
}
//---------------------------------------------------------------------------------------
bool TableDependency::existsInTree(TableDependency* a, const string ntable) const
{
    bool resultado = false;

    if (a->tableName == ntable)
        return true;
    else
    {
        for (std::list<TableDependency *>::iterator tmp = a->dependencies.begin(); tmp != a->dependencies.end(); ++tmp)
        {
            resultado = existsInTree(*tmp, ntable);
            if (resultado)
                return true;
        }
    }
    return resultado;
}
//---------------------------------------------------------------------------------------
void TableDependency::printTree(TableDependency* a, int nivel) const
{
    for(int aux = nivel; aux > 0; aux--)
        printf(" ");
    printf("%s\n", a->tableName.c_str());
    for (std::list<TableDependency *>::iterator tmp = a->dependencies.begin(); tmp != a->dependencies.end(); ++tmp)
        printTree(*tmp, nivel + 2);
}
//---------------------------------------------------------------------------------------
