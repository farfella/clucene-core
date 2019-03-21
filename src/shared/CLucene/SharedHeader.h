/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* Updated by https://github.com/farfella/.
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#pragma once

/**
* This header contains definitions and macros for helping create cross-platform code.
* It is primarily for use by the clucene-core library, but is split off so that
* other applications such as the demo, test, benchmarks, etc can benefit from the
* cross platform code. Cross platform code is not distributed with the clucene-core
* and is not available through the shared library.
*/

#include "_clucene-config.h"

#define _LUCENE_PRAGMA_WARNINGS //tell lucene to display warnings using pragmas instead of #warning


////////////////////////////////////////////////////////
//platform includes that MUST be included for the public headers to work...
////////////////////////////////////////////////////////
#include <cstddef> //need this for wchar_t, size_t, NULL

#include <math.h> //required for float_t
#include <string> //need to include this really early...

////////////////////////////////////////////////////////
//namespace helper
////////////////////////////////////////////////////////
#define CL_NS_DEF(sub) namespace lucene{ namespace sub{
#define CL_NS_DEF2(sub,sub2) namespace lucene{ namespace sub{ namespace sub2 {

#define CL_NS_END }}
#define CL_NS_END2 }}}

#define CL_NS_USE(sub) using namespace lucene::sub;
#define CL_NS_USE2(sub,sub2) using namespace lucene::sub::sub2;

#define CL_NS(sub) lucene::sub
#define CL_NS2(sub,sub2) lucene::sub::sub2
	    
#define CL_STRUCT_DEF(sub,clazz) namespace lucene { namespace sub{ struct clazz; } }
#define CL_CLASS_DEF(sub,clazz) namespace lucene { namespace sub{ class clazz; } }
#define CL_CLASS_DEF2(sub,sub2, clazz) namespace lucene { namespace sub{ namespace sub2{ class clazz; } } }

#define CL_TEMPATE_DEF(sub, clazz, typedefs) namespace lucene { namespace sub{ template<typedefs> class clazz; }}
#define CL_TYPE_DEF(sub, clazz, def) namespace lucene { namespace sub{ typedef def clazz; }}

#if defined(LUCENE_NO_STDC_NAMESPACE)
   //todo: haven't actually tested this on a non-stdc compliant compiler
   #define CL_NS_STD(func) ::func
#else
   #define CL_NS_STD(func) std::func
#endif
//
////////////////////////////////////////////////////////

////////////////////////////////////////////////////////
// EXPORTS definition
////////////////////////////////////////////////////////
#if defined(_WIN32) || defined(_WIN64)
	#define CLUCENE_EXPORT_DECL __declspec(dllexport)
  #define CLUCENE_IMPORT_DECL __declspec(dllimport)
  #define CLUCENE_LOCAL_DECL
#endif

//define for the libraries
#define CLUCENE_SHARED_EXPORT //don't export if we are building a static library

#define CLUCENE_EXPORT

#if defined(clucene_contribs_lib_EXPORTS)
	#define CLUCENE_CONTRIBS_EXPORT CLUCENE_EXPORT_DECL
	#define CLUCENE_LOCAL CLUCENE_LOCAL_DECL
#elif defined(MAKE_CLUCENE_CONTRIBS_LIB)
	#define CLUCENE_CONTRIBS_EXPORT
#else
    #define CLUCENE_CONTRIBS_EXPORT CLUCENE_IMPORT_DECL
#endif
#ifndef CLUCENE_LOCAL
	#define CLUCENE_LOCAL
#endif

//inline definitions
 #define CLUCENE_SHARED_INLINE_EXPORT
 #define CLUCENE_INLINE_EXPORT
 #define CLUCENE_CONTRIBS_INLINE_EXPORT

////////////////////////////////////////////////////////


//todo: put this logic in cmake
#if defined(_MSC_VER)
	#if _MSC_FULL_VER >= 140050320
	    #define _CL_DEPRECATE_TEXT(_Text) __declspec(deprecated(_Text))
	#elif _MSC_VER >= 1300
	    #define _CL_DEPRECATE_TEXT(_Text) __declspec(deprecated)
	#else
	    #define _CL_DEPRECATE_TEXT(_Text)
	#endif
#endif
#define _CL_DEPRECATED(_NewItem) _CL_DEPRECATE_TEXT("This function or variable has been superceded by newer library or operating system functionality. Consider using " #_NewItem " instead. See online help for details.")
//
////////////////////////////////////////////////////////


////////////////////////////////////////////////////////
//Class interfaces
////////////////////////////////////////////////////////
#include "CLucene/debug/lucenebase.h"
////////////////////////////////////////////////////////

//memory handling macros/functions
#include "CLucene/debug/mem.h"
