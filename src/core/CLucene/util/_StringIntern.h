/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* Updated by https://github.com/farfella/.
 Updated by https://github.com/farfella/.
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_util_StringIntern_H
#define _lucene_util_StringIntern_H

namespace lucene {
    namespace util {

        /** Functions for intern'ing strings. This
         * is a process of pooling strings thus using less memory,
         * and furthermore allows intern'd strings to be directly
         * compared:
         * string1==string2, rather than wcscmp(string1,string2)
         */
        class CLStringIntern
        {
        public:

            /**
            * Internalise the specified string.
            * \return Returns a pointer to the internalised string
            */
            static const char* internA(const char* str, const int8_t count = 1, const bool use_provided = false);

            /**
            * Uninternalise the specified string. Decreases
            * the reference count and frees the string if
            * reference count is zero
            * \returns true if string was destroyed, otherwise false
            */
            static bool uninternA(const char* str, const int8_t count = 1);

            /**
            * Internalise the specified string.
            * \return Returns a pointer to the internalised string
            */
            static const wchar_t* intern(const wchar_t* str);

            /**
            * Uninternalise the specified string. Decreases
            * the reference count and frees the string if
            * reference count is zero
            * \returns true if string was destroyed, otherwise false
            */
            static bool unintern(const wchar_t* str);

            /** Cleanup static data */
            static CLUCENE_LOCAL void _shutdown();
        };

    }
}
#endif
