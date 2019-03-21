/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* Updated by https://github.com/farfella/.
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#pragma once

#define LUCENE_INT32_MAX_SHOULDBE 0x7FFFFFFFL

#include "_clucene-config.h"
#include "SharedHeader.h"

//don't show deprecated warnings while building internally...
#undef _CL_DEPRECATE_TEXT
#define _CL_DEPRECATE_TEXT(_Text)

#define LUCENE_INT64_MAX_SHOULDBE _ILONGLONG(0x7FFFFFFFFFFFFFFF)
#define LUCENE_INT64_MIN_SHOULDBE (-LUCENE_INT64_MAX_SHOULDBE - _ILONGLONG(1) )

//required globally (internally only)
#include <stdio.h>
#include <stdlib.h>

//we always need this stuff....
#include "CLucene/debug/_condition.h"
#include "CLucene/LuceneThreads.h"

#include "CLucene/config/repl_wchar.h"
#include "CLucene/config/repl_wctype.h" //replacements for functions

#define cl_min(a,b) ((a)>(b) ? (b) : (a))
#define cl_min3(a,b,c) ((a)<(b) ? ((a)<(c) ? (a) : (c)) : ((b)<(c) ? (b) : (c)))
#define cl_max(a,b) ((a)>(b) ? (a): (b))
#define cl_max3(a,b,c) ((a)>(b) ? ((a)>(c) ? (a) : (c)) : ((b)>(c) ? (b) : (c)))


#define cl_sprintf sprintf_s
#define cl_stprintf swprintf_s
#define cl_strcpy(Dst,Src,DstLen) strcpy_s(Dst,DstLen,Src)



///a blank string...
CLUCENE_SHARED_EXPORT extern const wchar_t* _LUCENE_BLANK_STRING;
#define LUCENE_BLANK_STRING _LUCENE_BLANK_STRING
CLUCENE_SHARED_EXPORT extern const char* _LUCENE_BLANK_ASTRING;
#define LUCENE_BLANK_ASTRING _LUCENE_BLANK_ASTRING

#if defined(_WIN32) || defined(_WIN64)
    //#define PATH_DELIMITERA "\\"
    #define PATH_DELIMITERW L"\\"
#else
    #define PATH_DELIMITERA "/"
#endif

//if a wide character is being converted to a ascii character and it
//cannot fit, this character is used instead.
#define LUCENE_OOR_CHAR(c) ((char)(((unsigned short)c)&0xFF))


