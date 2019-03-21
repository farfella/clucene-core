/*------------------------------------------------------------------------------
* Copyright (C) 2003-2010 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/

#include "test.h"
#include "CLucene/util/StringBuffer.h"
#include <stdexcept>

CL_NS_USE(util)

void testStringBufferConstruct(CuTest *tc) {
  std::wstring sb;
  CuAssertEquals(tc, 0, sb.length());

  std::wstring sb1;
  sb1.reserve(10);
  CuAssertEquals(tc, 0, sb1.length());

  std::wstring sb2(_T("test"));
  CuAssertEquals(tc, 4, sb2.length());
  CuAssertStrEquals(tc, _T(""), _T("test"), sb2.c_str());
}

void testStringBufferCharAt(CuTest *tc) {
  std::wstring sb(_T("test abcd"));

  CuAssertTrue(tc, sb[0] == _T('t'), _T("unexpected character"));
  CuAssertTrue(tc, sb[1] == _T('e'), _T("unexpected character"));
  CuAssertTrue(tc, sb[2] == _T('s'), _T("unexpected character"));
  CuAssertTrue(tc, sb[3] == _T('t'), _T("unexpected character"));
  CuAssertTrue(tc, sb[4] == _T(' '), _T("unexpected character"));
  CuAssertTrue(tc, sb[5] == _T('a'), _T("unexpected character"));
  CuAssertTrue(tc, sb[6] == _T('b'), _T("unexpected character"));
  CuAssertTrue(tc, sb[7] == _T('c'), _T("unexpected character"));
  CuAssertTrue(tc, sb[8] == _T('d'), _T("unexpected character"));

  sb[4] = L'-';

  CuAssertTrue(tc, sb[0] == _T('t'), _T("unexpected character"));
  CuAssertTrue(tc, sb[1] == _T('e'), _T("unexpected character"));
  CuAssertTrue(tc, sb[2] == _T('s'), _T("unexpected character"));
  CuAssertTrue(tc, sb[3] == _T('t'), _T("unexpected character"));
  CuAssertTrue(tc, sb[4] == _T('-'), _T("unexpected character"));
  CuAssertTrue(tc, sb[5] == _T('a'), _T("unexpected character"));
  CuAssertTrue(tc, sb[6] == _T('b'), _T("unexpected character"));
  CuAssertTrue(tc, sb[7] == _T('c'), _T("unexpected character"));
  CuAssertTrue(tc, sb[8] == _T('d'), _T("unexpected character"));
}

void testStringBufferClear(CuTest *tc) {
  std::wstring sb(_T("test abcd wxyz"));
  sb.clear();
  CuAssertEquals(tc, 0, sb.length());
  CuAssertStrEquals(tc, _T(""), _T("\0"), sb.c_str());
}

void testStringBufferInsert(CuTest *tc) {
  std::wstring sb(_T("test abcd"));

  sb.insert(5, 1, _T('X'));
  CuAssertStrEquals(tc, _T(""), _T("test Xabcd"), sb.c_str());

  sb.insert(7, _T("YY"));
  CuAssertStrEquals(tc, _T(""), _T("test XaYYbcd"), sb.c_str());

  sb.insert(7, _T(""));
  CuAssertStrEquals(tc, _T(""), _T("test XaYYbcd"), sb.c_str());

  sb.insert(12, _T("ZZZ"));
  CuAssertStrEquals(tc, _T(""), _T("test XaYYbcdZZZ"), sb.c_str());

  sb.insert(15, 1, _T('_'));
  CuAssertStrEquals(tc, _T(""), _T("test XaYYbcdZZZ_"), sb.c_str());

  sb.insert(0, 1, _T('_'));
  CuAssertStrEquals(tc, _T(""), _T("_test XaYYbcdZZZ_"), sb.c_str());

  sb.insert(0, _T("123"));
  CuAssertStrEquals(tc, _T(""), _T("123_test XaYYbcdZZZ_"), sb.c_str());
}

void testStringBufferDelete(CuTest *tc) {
  std::wstring sb(_T("test abcd"));

  //sb.deleteCharAt(4);
  sb.erase(4, 1);
  CuAssertStrEquals(tc, _T(""), _T("testabcd"), sb.c_str());

  //sb.deleteChars(4, 7);
  sb.erase(4, 7 - 4);
  CuAssertStrEquals(tc, _T(""), _T("testd"), sb.c_str());

  sb.erase(3, 1);
  //sb.deleteChars(3, 3);
  CuAssertStrEquals(tc, _T(""), _T("testd"), sb.c_str());
}

void testSubstringEquals(CuTest *tc) {
  std::wstring sb(_T("test abcd"));
  
  CuAssertTrue(tc, sb.substr(3, 6-3) == _T("t a"));
  CuAssertTrue(tc, sb.substr(3, 6-3) == _T("t a"));
  CuAssertTrue(tc, sb.substr(3, 6-3) != _T("t-a"));
}

CuSuite *testStringBuffer(void) {
    CuSuite *suite = CuSuiteNew(_T("CLucene StringBuffer Test"));

    SUITE_ADD_TEST(suite, testStringBufferConstruct);
    SUITE_ADD_TEST(suite, testStringBufferCharAt);
    SUITE_ADD_TEST(suite, testStringBufferClear);
    SUITE_ADD_TEST(suite, testStringBufferInsert);
    SUITE_ADD_TEST(suite, testStringBufferDelete);
    SUITE_ADD_TEST(suite, testSubstringEquals);

    return suite;
}
