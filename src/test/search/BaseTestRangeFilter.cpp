/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* Updated by https://github.com/farfella/.
 Updated by https://github.com/farfella/.
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "test.h"
#include "BaseTestRangeFilter.h"

#include <limits>

static const size_t getMaxIntLength()
{
    wchar_t buf[40];
    _itow_s(std::numeric_limits<int>::max(), buf, 10);
    return wcslen(buf);
}

BaseTestRangeFilter::BaseTestRangeFilter(CuTest* _tc)
        : index(new RAMDirectory())
        , maxR(std::numeric_limits<int>::min()), minR(std::numeric_limits<int>::max())
        , minId(0), maxId(10000), intLength(getMaxIntLength())
        , tc(_tc)
{
    srand ( 101 ); // use a set seed to test is deterministic
    build();
}

BaseTestRangeFilter::~BaseTestRangeFilter()
{
    index->close();
    _CLLDECDELETE(index);
}
    
std::wstring BaseTestRangeFilter::pad(int32_t n)
{
    std::wstring b;
    if (n < 0)
    {
        b = L"-";
        n = std::numeric_limits<int32_t>::max() + n + 1;
    }
    else
        b = L"0";

    wchar_t buf[40];
    _itow_s(n, buf, 10);
    for (size_t i = wcslen(buf); i <= intLength; i++)
    {
        b += L"0";
    }
    b += buf;
    
    return b;
}
    
void BaseTestRangeFilter::build()
{
    try
    {    
        /* build an index */
        SimpleAnalyzer a;
        IndexWriter* writer = _CLNEW IndexWriter(index, &a, T);

        for (int32_t d = minId; d <= maxId; d++) {
            Document doc;
            std::wstring paddedD = pad(d);
            doc.add(* _CLNEW Field(L"id",paddedD.c_str(), Field::STORE_YES | Field::INDEX_UNTOKENIZED));
            int r= rand();
            if (maxR < r) {
                maxR = r;
            }
            if (r < minR) {
                minR = r;
            }
            std::wstring paddedR = pad(r);
            doc.add( * _CLNEW Field(L"rand",paddedR.c_str(), Field::STORE_YES | Field::INDEX_UNTOKENIZED));
            doc.add( * _CLNEW Field(L"body", L"body", Field::STORE_YES | Field::INDEX_UNTOKENIZED));
            writer->addDocument(&doc);
        }
        
        writer->optimize();
        writer->close();
        _CLLDELETE(writer);
    } catch (CLuceneError&) {
        _CLTHROWA(CL_ERR_Runtime, "can't build index");
    }
}

void BaseTestRangeFilter::testPad() 
{
    const int32_t tests[] = {
        -9999999, -99560, -100, -3, -1, 0, 3, 9, 10, 1000, 999999999
    };
    const size_t testsLen = 11;

    for (size_t i = 0; i < testsLen - 1; i++) {
        const int32_t a = tests[i];
        const int32_t b = tests[i+1];
        const wchar_t* aa = pad(a).c_str();
        const wchar_t* bb = pad(b).c_str();

        StringBuffer label(50);
        label << a << L':' << aa << L" vs " << b << L':' << bb;

        std::wstring tmp = L"length of ";
        tmp += label.getBuffer();
        assertEqualsMsg(tmp.c_str(), wcslen(aa), wcslen(bb));

        tmp = L"compare less than ";
        tmp += label.getBuffer();
        assertTrueMsg(tmp.c_str(), wcscmp(aa, bb) < 0);
    }
}

// EOF
