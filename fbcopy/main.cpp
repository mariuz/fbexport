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
#ifdef IBPP_WINDOWS
#include <windows.h>
#endif

#ifdef IBPP_LINUX
#define __cdecl /**/
#include "stdarg.h"
#endif

#include "ibpp.h"

#ifdef HAS_HDRSTOP
#pragma hdrstop
#endif

#include <stdio.h>

#include "args.h"
#include "fbcopy.h"
int __cdecl main(int argc, char* argv[])
{
    if (! IBPP::CheckVersion(IBPP::Version))
    {
        printf("\nThis program got linked to an incompatible version of the library.\nCan't execute safely.\n");
        return 2;
    }

    // parse command-line args into Argument class
    Args ar(argc, argv);
    FBCopy F;
    return F.Run(&ar);  // Init() is used since ctor can't return values
}
