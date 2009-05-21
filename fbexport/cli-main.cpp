///////////////////////////////////////////////////////////////////////////////
//
//  File        : fbxmain.cpp
//  Version     : 1.35
//  Modified    : 27. Dec 2003.
//  Author      : Milan Babuskov (mbabuskov@yahoo.com)
//  Purpose     : This file contains main() function for command-line version
//                and some functions specific to cmdline version
//
//  Note        : Uses version 2 of IBPP library (also released under MPL)
//                Check http://ibpp.sourceforge.net for more info
//
///////////////////////////////////////////////////////////////////////////////
//
//	The contents of this file are subject to the Mozilla Public License
//	Version 1.0 (the "License"); you may not use this file except in
//	compliance with the License. You may obtain a copy of the License at
//	http://www.mozilla.org/MPL/
//
//	Software distributed under the License is distributed on an "AS IS"
//	basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
//	License for the specific language governing rights and limitations
//	under the License.
//
//	The Original Code is "FBExport 1.35" and all its associated documentation.
//
//	The Initial Developer of the Original Code is Milan Babuskov.
//
//	Contributor(s): ______________________________________.
//
///////////////////////////////////////////////////////////////////////////////
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

#include "ParseArgs.h"
#include "FBExport.h"
//---------------------------------------------------------------------------------------
int __cdecl main(int argc, char* argv[])
{
	if (! IBPP::CheckVersion(IBPP::Version))
	{
		printf("\nThis program got linked to an incompatible version of the library.\nCan't execute safely.\n");
		return 2;
	}

    // parse command-line args into Argument class
    Arguments ar(argc, argv);
	FBExport F;
	return F.Init(&ar);	// Init() is used since ctor can't return values
}
//---------------------------------------------------------------------------------------
void FBExport::Printf(const char *format, ...)
{
   va_list argptr;
   va_start(argptr, format);
   vfprintf(stderr, format, argptr);
   va_end(argptr);
}
//---------------------------------------------------------------------------------------
