/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* Updated by https://github.com/farfella/.
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "Misc.h"
#include <assert.h>
#include <iostream>
#include <map>
#include <Shlwapi.h>

#include <sys/timeb.h>


#include <cctype>
#include <limits.h>
#include "CLucene/util/dirent.h" //if we have dirent, then the native one will be used

//for zlib...
#include "zlib.h"
#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif
#define CL_Z_DEFAULT_CHUNK 1024



namespace lucene
{
    namespace util
    {
        size_t Misc::ahashCode(__in_z const char * str)
        {
            // Compute the hash code using a local variable to be reentrant.
            size_t hashCode = 0;
            while (*str != NULL)
                hashCode = hashCode * 31 + *str++;
            return hashCode;
        }

        int64_t Misc::filelength(__in int filehandle)
        {
            struct cl_stat_t info;
            if (fileHandleStat(filehandle, &info) == -1)
                return -1;
            return info.st_size;
        }

        void Misc::Sleep(__in DWORD dwMilliseconds)
        {
            ::Sleep(dwMilliseconds);
        }

        size_t Misc::whashCode(__in_z const wchar_t* str)
        {
            // Compute the hash code using a local variable to be reentrant.
            size_t hashCode = 0;
            while (*str != NULL)
                hashCode = hashCode * 31 + *str++;
            return hashCode;
        }

        size_t Misc::whashCode(__in_z const wchar_t* str, __in size_t len)
        {
            // Compute the hash code using a local variable to be reentrant.
            size_t hashCode = 0;
            for (size_t i = 0; i < len && str[i] != NULL; i++)
                hashCode = hashCode * 31 + *str++;
            return hashCode;
        }

        __checkReturn std::string Misc::toString(__in const wchar_t* s, __in int32_t len)
        {
            std::string ret;
            if (s != NULL && len != 0)
            {
                int sz = WideCharToMultiByte(CP_UTF8, 0, s, len, NULL, 0, NULL, FALSE);
                ret.resize(sz);
                sz = WideCharToMultiByte(CP_UTF8, 0, s, len, &ret[0], sz, NULL, FALSE);
            }

            return ret;
        }

        //static
        char* Misc::_wideToChar(const wchar_t* s)
        {
            size_t len = wcslen(s);
            char* msg = _CL_NEWARRAY(char, len + 1);
            _cpywideToChar(s, msg, len + 1);
            return msg;
        }

        wchar_t* Misc::_charToWide(const char* s)
        {
            size_t len = strlen(s);
            wchar_t* msg = _CL_NEWARRAY(wchar_t, len + 1);
            _cpycharToWide(s, msg, len + 1);
            return msg;
        }

        void Misc::_cpywideToChar(const wchar_t* s, char* d, size_t len)
        {
            size_t sLen = wcslen(s);
            for (uint32_t i = 0; i < len&&i < sLen + 1; i++)
                d[i] = LUCENE_OOR_CHAR(s[i]);
        }
        void Misc::_cpycharToWide(const char* s, wchar_t* d, size_t len)
        {
            size_t sLen = strlen(s);
            for (uint32_t i = 0; i < len&&i < sLen + 1; i++)
                d[i] = s[i];
        }

        //static
        uint64_t Misc::currentTimeMillis()
        {
            struct _timeb tstruct;
            _ftime_s(&tstruct);

            return (((uint64_t) tstruct.time) * 1000) + tstruct.millitm;
        }

        //static
        const wchar_t* Misc::replace_all(const wchar_t * val, const wchar_t * srch, const wchar_t* repl)
        {
            int32_t cnt = 0;
            size_t repLen = wcslen(repl);
            size_t srchLen = wcslen(srch);
            size_t srcLen = wcslen(val);

            const wchar_t * pos = val;
            while ((pos = wcsstr(pos + 1, srch)) != NULL)
            {
                ++cnt;
            }

            size_t lenNew = (srcLen - (srchLen * cnt)) + (repLen * cnt);
            wchar_t* ret = _CL_NEWARRAY(wchar_t, lenNew + 1);
            ret[lenNew] = 0;

            if (cnt == 0)
            {
                wcscpy_s(ret, lenNew + 1, val);
                return ret;
            }

            wchar_t* cur = ret; //position of return buffer
            const wchar_t* lst = val; //position of value buffer
            pos = val; //searched position of value buffer
            while ((pos = wcsstr(pos + 1, srch)) != NULL)
            {
                wcsncpy(cur, lst, pos - lst); //copy till current
                cur += (pos - lst);
                lst = pos; //move val position

                wcscpy(cur, repl); //copy replace
                cur += repLen; //move return buffer position
                lst += srchLen; //move last value buffer position
            }
            wcscpy(cur, lst); //copy rest of buffer

            return ret;
        }

        //static
        __checkReturn bool Misc::dir_Exists(__in_z const wchar_t* path)
        {
            /*
            if (!path || !*path)
                return false;
            struct cl_stat_t buf;
            int32_t ret = fileStat(path, &buf);
            return (ret == 0);*/
            return PathFileExistsW(path) != 0;
        }

        int64_t Misc::file_Size(const char* path)
        {
            struct cl_stat_t buf;
            if (fileStat(path, &buf) == 0)
                return buf.st_size;
            else
                return -1;
        }

        int Misc::file_Unlink(const wchar_t * path, int32_t maxAttempts)
        {
            int32_t i;

            if (!path || !* path)
                return -1;

            if (maxAttempts == 0)
                maxAttempts = 1;

            while (maxAttempts != 0)
            {
                if (_wunlink(path) != 0)
                    return -1;

                i = 0;
                while (i < 100)
                {
                    if (!Misc::dir_Exists(path))
                        return 1;

                    if (++i > 50)      // if it still doesn't show up, then we do some sleeping for the last 50ms
                        Misc::Sleep(1);
                }

                if (maxAttempts > 0)
                    maxAttempts--;
            }

            return 0;
        }


        //static
        wchar_t * Misc::join(const wchar_t * a, const wchar_t * b, const wchar_t * c, const wchar_t * d, const wchar_t * e, const wchar_t * f)
        {
#define LEN(x) (x == NULL ? 0 : wcslen(x))
            const size_t totalLen =
                LEN(a) + LEN(b) + LEN(c) + LEN(d) + LEN(e) + LEN(f)
                + sizeof(wchar_t); /* Space for terminator. */

            wchar_t * buf = _CL_NEWARRAY(wchar_t, totalLen);
            if (NULL != buf)
            {
                buf[0] = 0;
                if (a != NULL) wcscat_s(buf, totalLen, a);
                if (b != NULL) wcscat_s(buf, totalLen, b);
                if (c != NULL) wcscat_s(buf, totalLen, c);
                if (d != NULL) wcscat_s(buf, totalLen, d);
                if (e != NULL) wcscat_s(buf, totalLen, e);
                if (f != NULL) wcscat_s(buf, totalLen, f);
            }
            return buf;
        }

        /*
        char* Misc::ajoin(const char* a, const char* b, const char* c, const char* d, const char* e, const char* f)
        {
#define aLEN(x) (x == NULL ? 0 : strlen(x))
            const size_t totalLen =
                aLEN(a) + aLEN(b) + aLEN(c) + aLEN(d) + aLEN(e) + aLEN(f)
                + sizeof(char); 

            char* buf = _CL_NEWARRAY(char, totalLen);
            if (NULL != buf)
            {
                buf[0] = 0;
                if (a != NULL) strcat_s(buf, totalLen, a);
                if (b != NULL) strcat_s(buf, totalLen, b);
                if (c != NULL) strcat_s(buf, totalLen, c);
                if (d != NULL) strcat_s(buf, totalLen, d);
                if (e != NULL) strcat_s(buf, totalLen, e);
                if (f != NULL) strcat_s(buf, totalLen, f);
            }

            return buf;
        }*/

        //static
        __checkReturn bool Misc::priv_isDotDir(__in const wchar_t * name)
        {
            return wcscmp(name, L".") == 0 || wcscmp(name, L"..") == 0;
        }

        //internal static function shared for clucene
        wstring Misc::segmentname(const wchar_t * segment, const wchar_t * ext, const int32_t x)
        {
            //Func -  Returns an allocated buffer in which it creates a filename by
            //       concatenating segment with ext and x
            //Pre    ext != NULL and holds the extension
            //       x contains a number
            //Post - A buffer has been instantiated an when x = -1 buffer contains the concatenation of
            //       segment and ext otherwise buffer contains the contentation of segment, ext and x

            CND_PRECONDITION(ext != NULL, L"ext is NULL");


            if (x != -1)
            {
                wchar_t buf[30];
                _snwprintf_s(buf, 10, L"%d", x);
                return std::wstring(segment) + ext + buf;
            }
            else
            {
                return std::wstring(segment) + ext;
            }
        }

        void Misc::segmentname(wchar_t * buffer, int32_t bufferLen, const wchar_t * Segment, const wchar_t * ext, const int32_t x)
        {
            //Func - Static Method
            //       Creates a filename in buffer by concatenating Segment with ext and x
            //Pre  - buffer  != NULL
            //       Segment != NULL and holds the name of the segment
            //       ext     != NULL and holds the extension
            //       x contains a number
            //Post - When x = -1 buffer contains the concatenation of Segment and ext otherwise
            //       buffer contains the contentation of Segment, ext and x

            CND_PRECONDITION(buffer != NULL, L"buffer is NULL");
            CND_PRECONDITION(Segment != NULL, L"Segment is NULL");
            CND_PRECONDITION(ext != NULL, L"ext is NULL");

            if (x == -1)
                _snwprintf_s(buffer, bufferLen, bufferLen, L"%s%s", Segment, ext);
            else
                _snwprintf_s(buffer, bufferLen, bufferLen, L"%s%s%d", Segment, ext, x);
        }


        //static
        int32_t Misc::stringDifference(const wchar_t* s1, const int32_t len1, const wchar_t* s2, const int32_t len2)
        {
            int32_t len = len1 < len2 ? len1 : len2;
            for (int32_t i = 0; i < len; i++)
                if (s1[i] != s2[i])
                    return i;
            return len;
        }

        wchar_t * Misc::stringTrim(wchar_t * text)
        {
            size_t j, i;
            size_t len = wcslen(text);

            for (i = 0; i < len; i++)
            { // find the first non-space character and store it as i
                if (!_istspace(text[i]))
                    break;
            }

            for (j = len - 1; j > i; --j)
            { // find the last non-space character and store it as j
                if (!_istspace(text[j]))
                {
                    break;
                }
            }

            if (i == 0 && j == len - 1) // prevent unnecessary copy
                return text;

            if (i == 0)
            {
                text[j + 1] = 0;
            }
            else
            {
                j++;
                wcsncpy(text, text + i, j - i);
                text[j - i] = 0;
            }

            return text;
        }

        wchar_t * Misc::wordTrim(wchar_t* text)
        {
            size_t j, i;
            const size_t len = wcslen(text);

            for (i = 0; i < len; i++)
            { // find the first non-space character and store it as i
                if (!_istspace(text[i]))
                    break;
            }

            for (j = i; j < len; j++)
            { // find the last non-space character and store it as j
                if (_istspace(text[j]))
                {
                    break;
                }
            }

            if (i == 0 && j == len)
                return text;

            if (i == j) // empty string
                return NULL;

            if (i == 0)
            {
                text[j] = 0;
                return text;
            }
            else
            {
                wcsncpy(text, text + i, j - i);
                text[j - i] = 0;
            }

            return text;
        }

        size_t Misc::longToBase(int64_t value, int32_t base, wchar_t* retval)
        {
            static wchar_t digits[] = L"0123456789abcdefghijklmnopqrstuvwxyz";
            wchar_t buf[(sizeof(unsigned long) << 3) + 1];
            wchar_t *ptr, *end;

            ptr = end = buf + ARRAYSIZE(buf) - 1;

            *ptr = '\0';
            do
            {
                *--ptr = digits[value % base];
                value /= base;
            } while (ptr > buf && value);

            memcpy(retval, ptr, (end - ptr)*sizeof(wchar_t));
            retval[end - ptr] = 0;

            return end - ptr;
        }

        int64_t Misc::base36ToLong(const wchar_t * value)
        {
            wchar_t * ptr = (wchar_t*) value;
            int64_t lval = 0;

            while (*ptr != L'\0')
            {
                lval = isdigit(*ptr) ? (36 * lval) + (*ptr - L'0') : (36 * lval) + (*ptr - L'a' + 10);
                ptr++;
            }

            return lval;
        }

        bool Misc::listFiles(const wchar_t* directory, std::vector<std::wstring>& files, bool fullPath)
        {
            //clear old files
            DIR* dir = opendir(directory);
            if (dir == NULL) return false;
            struct dirent* fl = readdir(dir);
            struct cl_stat_t buf;
            std::wstring path;
            while (fl != NULL)
            {
                path = directory + std::wstring(L"/") + fl->d_name;
                int32_t ret = _wstati64(path.c_str(), &buf);
                if (ret == 0 && !(buf.st_mode & S_IFDIR))
                {
                    if ((wcscmp(fl->d_name, L".")) && (wcscmp(fl->d_name, L"..")))
                    {
                        if (fullPath)
                        {
                            files.push_back(path);
                        }
                        else
                        {
                            files.push_back(fl->d_name);
                        }
                    }
                }
                fl = readdir(dir);
            }
            closedir(dir);
            return true;
        }


        std::wstring Misc::toString(bool value)
        {
            return value ? L"true" : L"false";
        }

        std::wstring Misc::toString(_LUCENE_THREADID_TYPE value)
        {
            static int32_t nextindex = 0;
            static std::map<_LUCENE_THREADID_TYPE, int32_t> ids;
            if (ids.find(value) == ids.end())
            {
                ids[value] = nextindex++;
            }

            return toString(ids[value]);
        }

        std::wstring Misc::toWString(_LUCENE_THREADID_TYPE value)
        {
            static int32_t nextindex = 0;
            static std::map<_LUCENE_THREADID_TYPE, int32_t> ids;
            if (ids.find(value) == ids.end())
            {
                ids[value] = nextindex++;
            }

            return std::to_wstring(ids[value]);
        }

        std::wstring Misc::toString(const int32_t value)
        {
            const size_t sz = 20;
            wchar_t tbuf[sz];
            _itow_s(value, tbuf, sz, 10);
            
            return tbuf;
        }

        std::wstring Misc::toString(const int64_t value)
        {
            const size_t sz = 20;
            wchar_t wbuf[sz];
            _i64tow_s(value, wbuf, sz, 10);
            
            return wbuf;
        }

        std::wstring Misc::toString(const float_t value)
        {
            wchar_t buf[20];
            _snwprintf_s(buf, 20, L"%0.2f", (double) value);
            return buf;
        }

        void Misc::zerr(int ret, string& err)
        {
            switch (ret)
            {
            case Z_ERRNO:
                err = "error occurred while reading or writing from the zlib streams";
                break;
            case Z_STREAM_ERROR:
                err = "invalid compression level";
                break;
            case Z_DATA_ERROR:
                err = "invalid or incomplete deflate data";
                break;
            case Z_MEM_ERROR:
                err = "out of memory";
                break;
            case Z_VERSION_ERROR:
                err = "zlib version mismatch";
            }
        }



        /* Compress from file source to file dest until EOF on source.
           def() returns Z_OK on success, Z_MEM_ERROR if memory could not be
           allocated for processing, Z_STREAM_ERROR if an invalid compression
           level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
           version of the library linked do not match, or Z_ERRNO if there is
           an error reading or writing the files. */
        bool Misc::deflate(const uint8_t* in, size_t inlen, std::ostream& dest, string& err, int CHUNK, int level)
        {
            int ret, flush;
            unsigned have;
            z_stream strm;
            if (level == -1) level = Z_BEST_COMPRESSION;
            if (CHUNK == -1) CHUNK = CL_Z_DEFAULT_CHUNK;
            uint8_t* out = (uint8_t*) malloc(CHUNK);

            /* allocate deflate state */
            strm.zalloc = Z_NULL;
            strm.zfree = Z_NULL;
            strm.opaque = Z_NULL;
            ret = deflateInit(&strm, level);
            if (ret != Z_OK)
            {
                free(out);
                zerr(ret, err);
                return false;
            }

            /* compress until end of file */
            do
            {
                strm.avail_in = inlen;
                strm.next_in = (uint8_t*) in;
                flush = Z_FINISH;

                /* run deflate() on input until output buffer not full, finish
                   compression if all of source has been read in */
                do
                {
                    strm.avail_out = CHUNK;
                    strm.next_out = out;
                    ret = ::deflate(&strm, flush);  /* no bad return value */
                    assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
                    have = CHUNK - strm.avail_out;
                    dest.write((char*) out, have);
                    if (dest.fail())
                    {
                        (void) deflateEnd(&strm);
                        free(out);
                        zerr(Z_ERRNO, err);
                        return false;
                    }
                } while (strm.avail_out == 0);
                assert(strm.avail_in == 0);   /* all input will be used */

                /* done when last data in file processed */
            } while (flush != Z_FINISH);
            assert(ret == Z_STREAM_END);    /* stream will be complete */

            /* clean up and return */
            (void) deflateEnd(&strm);
            free(out);
            return true;
        }

        /* Decompress from file source to file dest until stream ends or EOF.
           inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
           allocated for processing, Z_DATA_ERROR if the deflate data is
           invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
           the version of the library linked do not match, or Z_ERRNO if there
           is an error reading or writing the files. */
        bool Misc::inflate(const uint8_t* in, size_t inlen, std::ostream& dest, string& err, int CHUNK)
        {
            int ret;
            unsigned have;
            z_stream strm;
            if (CHUNK == -1) CHUNK = CL_Z_DEFAULT_CHUNK;
            uint8_t* out = (uint8_t*) malloc(CHUNK);

            /* allocate inflate state */
            strm.zalloc = Z_NULL;
            strm.zfree = Z_NULL;
            strm.opaque = Z_NULL;
            strm.avail_in = 0;
            strm.next_in = Z_NULL;
            ret = inflateInit(&strm);
            if (ret != Z_OK)
            {
                free(out);
                zerr(ret, err);
                return false;
            }

            /* decompress until deflate stream ends or end of file */
            do
            {
                strm.avail_in = inlen;
                if (strm.avail_in == 0)
                    break;
                strm.next_in = (uint8_t*) in;

                /* run inflate() on input until output buffer not full */
                do
                {
                    strm.avail_out = CHUNK;
                    strm.next_out = out;
                    ret = ::inflate(&strm, Z_NO_FLUSH);
                    assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
                    switch (ret)
                    {
                    case Z_NEED_DICT:
                        ret = Z_DATA_ERROR;   /* and fall through */
                    case Z_DATA_ERROR:
                    case Z_MEM_ERROR:
                        (void) inflateEnd(&strm);
                        free(out);
                        zerr(ret, err);
                        return false;
                    }
                    have = CHUNK - strm.avail_out;
                    dest.write((char*) out, have);
                    if (dest.fail())
                    {
                        (void) inflateEnd(&strm);
                        free(out);
                        zerr(Z_ERRNO, err);
                        return false;
                    }
                } while (strm.avail_out == 0);

                /* done when inflate() says it's done */
            } while (ret != Z_STREAM_END);

            /* clean up and return */
            (void) inflateEnd(&strm);
            free(out);
            if (ret == Z_STREAM_END)
                return true;
            zerr(Z_DATA_ERROR, err);
            return false;
        }

    }
}