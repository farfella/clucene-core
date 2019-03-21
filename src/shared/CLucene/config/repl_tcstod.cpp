/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* Updated by https://github.com/farfella/.
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
 
#include "CLucene\_SharedHeader.h"
#include "CLucene/util/Misc.h"

#ifndef _ASCII
double lucene_tcstod(const wchar_t *value, wchar_t **end)
{
    int32_t len = wcslen(value)+1;
    char* avalue=_CL_NEWARRAY(char,len);
    char* aend=NULL;
    lucene::util::Misc::_cpywideToChar(value, avalue, len);
    
    double ret = strtod(avalue,&aend);
    *end=(wchar_t*)value+(aend-avalue);
    _CLDELETE_CaARRAY(avalue);

    return ret;
}
#endif
