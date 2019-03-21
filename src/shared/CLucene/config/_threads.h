/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* Updated by https://github.com/farfella/.
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#pragma once

#include <windows.h>
#include <process.h>

namespace lucene
{
    namespace util
    {

        struct CLuceneThreadIdCompare
        {
            enum
            {	// parameters for hash table
                bucket_size = 4,	// 0 < bucket_size
                min_buckets = 8
            };	// min_buckets = 2 ^^ N, 0 < N

            constexpr bool operator()(const uint64_t & Left, const uint64_t & Right) const
            {	// apply operator< to operands
                return Left < Right;
            }
        };

    }
}