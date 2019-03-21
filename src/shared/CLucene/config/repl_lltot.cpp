/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* Updated by https://github.com/farfella/.
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene\_SharedHeader.h"

/*
wchar_t * lucene_i64tot(
    int64_t value, 
    wchar_t * str, 
    int radix)     
{
    uint64_t val;
    int negative;
    wchar_t buffer[65];
    wchar_t* pos;
    int digit;

	if (value < 0 && radix == 10) {
		negative = 1;
		val = -value;
	} else {
		negative = 0;
		val = value;
	} 

	pos = &buffer[64];
	*pos = '\0';

	do {
		digit = (int)(val % radix);
		val = val / radix;
		if (digit < 10) {
			*--pos = '0' + digit;
		} else {
			*--pos = 'a' + digit - 10;
		} 
	} while (val != 0L);

	if (negative) {
		*--pos = '-';
	} 

    wcsncpy(str, pos, &buffer[64] - pos + 1); //needed for unicode to work
    return str;
}

*/