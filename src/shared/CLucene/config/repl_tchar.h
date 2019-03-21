/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* Updated by https://github.com/farfella/.
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#pragma once


#include <tchar.h>

//some wchar_t headers miss these...
#ifndef _tcstoi64
    #if defined(_UCS2)
        #define _tcstoi64 wcstoll //* convers a string to an 64bit bit integer
    #else
        #define _tcstoi64 strtoll
    #endif
#endif


namespace std
{
#ifndef tstring
  #ifdef _UNICODE
    typedef wstring tstring;
  #else
    typedef string tstring;
  #endif
#endif
};

#define STRCPY_AtoA(target,src,len) strncpy(target,src,len)
#define STRDUP_AtoA(x) _strdup(x)


#if _CL_HAVE_FUNCTION_WCSDUP
    #define STRDUP_WtoW	_wcsdup
#else
    #define STRDUP_WtoW	lucene_wcsdup
#endif

#define STRDUP_TtoT STRDUP_WtoW
#define STRDUP_WtoT STRDUP_WtoW
#define STRDUP_TtoW STRDUP_WtoW

#define STRDUP_AtoW(x) CL_NS(util)::Misc::_charToWide(x)
#define STRDUP_AtoT STRDUP_AtoW

#define STRDUP_WtoA(x) CL_NS(util)::Misc::_wideToChar(x)
#define STRDUP_TtoA STRDUP_WtoA

#define STRCPY_WtoW(target,src,len) wcsncpy(target,src,len)
#define STRCPY_TtoW STRCPY_WtoW
#define STRCPY_WtoT STRCPY_WtoW
//#define wcscpy STRCPY_WtoW

#define STRCPY_AtoW(target,src,len) CL_NS(util)::Misc::_cpycharToWide(src,target,len)
#define STRCPY_AtoT STRCPY_AtoW

#define STRCPY_WtoA(target,src,len) CL_NS(util)::Misc::_cpywideToChar(src,target,len)
#define STRCPY_TtoA STRCPY_WtoA
