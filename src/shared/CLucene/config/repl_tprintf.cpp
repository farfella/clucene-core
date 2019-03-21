/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* Updated by https://github.com/farfella/.
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene\_SharedHeader.h"
#include "CLucene/util/StringBuffer.h"

#include <limits.h> //MB_LEN_MAX

namespace lucene {
    namespace util {

        //print a variable argument to a stream
        //currently special number formatting is not supported. it is very minimalistic
        void lucene_vfnwprintf(StringBuffer* buffer, size_t /*count*/, const wchar_t * format, va_list& valist) {
            const wchar_t *iter = format;
            StringBuffer* tmp = NULL;
            if (buffer == NULL)
                tmp = _CLNEW StringBuffer;
            else
                tmp = buffer;

            while (*iter)
            {
                while (*iter && *iter != '%')
                {
                    tmp->appendChar(*iter++);
                }
                if (*iter == '%')
                {
                    if (iter[1] == '%')
                    {
                        //just print a %
                        tmp->appendChar('%');
                        iter += 2;
                        continue;
                    }

                    iter++;
                    switch (*iter)
                    {
                    case 's':
                    {
                        //todo: this is faulty. it doesn't heed count

                        //print a string or null
                        const wchar_t *wstr = va_arg(valist, wchar_t *);
                        if (!wstr)
                            wstr = L"(null)";

                        tmp->append(wstr);
                        iter++;
                        break;
                    }

                    case 'c':
                        tmp->appendChar((wchar_t)va_arg(valist, int));
                        iter++;
                        break;

                    default:
                    {
                        //todo: this is faulty. it doesn't heed count

                        if (*iter == 'p')
                            tmp->appendInt((int32_t)va_arg(valist, long));
                        else
                        {
                            if (*iter == 'a' || *iter == 'A' ||
                                *iter == 'e' || *iter == 'E' ||
                                *iter == 'f' || *iter == 'F' ||
                                *iter == 'g' || *iter == 'G')
                                tmp->appendFloat((float_t)va_arg(valist, double), 8);
                            else if (*iter == 'd' || *iter == 'i') {
                                tmp->appendInt((int32_t)va_arg(valist, int));
                            }
                            else if (*iter == 'l') {
                                const size_t sz = 100;
                                wchar_t b[sz];
                                _i64tow_s((int64_t)va_arg(valist, int64_t), b, sz, 10);
                                tmp->append(b);
                            }
                        }
                        iter++;
                        break;
                    }
                    }
                }
            }

            if (buffer == NULL)
            {
                //we are supposed to be writing to the console

                wchar_t * pointer = tmp->getBuffer();
                char ob[MB_LEN_MAX];
                size_t v;
                size_t len = tmp->length();
                for (size_t i = 0; i < len; i++)
                {
                    int converted;
                    v = wctomb_s(&converted, ob, MB_LEN_MAX, *pointer);
                    if (v > 0)
                    {
                        ob[v] = '\0';
                        fputs(ob, stdout);
                    }
                    pointer++;
                }

                _CLDELETE(tmp);
            }
        }


        //print a list of arguments to a string
        int CLUCENE_SHARED_EXPORT lucene_snwprintf(wchar_t* strbuf, size_t count, const wchar_t * format, ...) {
            va_list ap;
            va_start(ap, format);
            StringBuffer buffer;
            lucene_vfnwprintf(&buffer, count, format, ap);
            va_end(ap);

            size_t ret = cl_min(count, (size_t)(buffer.length() + 1));
            wcsncpy_s(strbuf, count, buffer.getBuffer(), ret);
            return ret;
        }

        //print a list of arguments to the stdout
        void lucene_wprintf(const wchar_t * format, ...) {
            va_list ap;
            va_start(ap, format);
            lucene_vfnwprintf(NULL, LUCENE_INT32_MAX_SHOULDBE, format, ap);
            va_end(ap);
        }

        //print a variable argument to a string
        int lucene_vsnwprintf(wchar_t * strbuf, size_t count, const wchar_t * format, va_list& ap) {
            StringBuffer buffer;
            lucene_vfnwprintf(&buffer, count, format, ap);
            int ret = cl_min((size_t)count, buffer.length() + 1);
            wcsncpy_s(strbuf, count, buffer.getBuffer(), ret);
            return ret;
        }
    }
}