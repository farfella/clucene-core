/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* Updated by https://github.com/farfella/.
 Updated by https://github.com/farfella/.
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/

#include "_SharedHeader.h"


#ifdef _CL_HAVE_SYS_STAT_H
	#include <sys/stat.h>
#endif


const wchar_t* _LUCENE_BLANK_STRING= L"";
const char* _LUCENE_BLANK_ASTRING="";


#if defined(_ASCII)
  #if defined(_LUCENE_PRAGMA_WARNINGS)
	 #pragma message ("==================Using ascii mode!!!==================")
	#else
	 #warning "==================Using ascii mode!!!=================="
	#endif
#endif

wchar_t* lucene_wcsdup( const wchar_t* str )
{
    wchar_t * ret = NULL;
    if (NULL != str)
    {
        size_t len = wcslen(str);
        ret = (wchar_t*)malloc((len + 1) * sizeof(wchar_t));
        if (NULL != ret)
        {
            wcscpy_s(ret, len + 1, str);
        }
    }

	return ret;
}
