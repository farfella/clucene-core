/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* Updated by https://github.com/farfella/.
 Updated by https://github.com/farfella/.
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
 
#include "CLucene\_SharedHeader.h"

wchar_t* lucene_tcslwr( wchar_t* str )
{
    wchar_t* ret = str;
    for ( ; *str; str++) *str = _totlower(*str);
    return ret;
}
