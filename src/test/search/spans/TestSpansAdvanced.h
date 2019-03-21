/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team

* Updated by https://github.com/farfella/.
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_search_spans_TestSpansAdvanced
#define _lucene_search_spans_TestSpansAdvanced

#include "test.h"

/*******************************************************************************
 * Tests the span query bug in Lucene. It demonstrates that SpanTermQuerys don't
 * work correctly in a BooleanQuery.
 *
 * @author Reece Wilton
 */
class TestSpansAdvanced
{
protected:
    CL_NS(search)::IndexSearcher *  searcher;
    CL_NS(store)::RAMDirectory *    directory;

    static const wchar_t *            field_id;
    static const wchar_t *            field_text;

public:
    CuTest *                        tc;

public:
    TestSpansAdvanced( CuTest* tc );
    virtual ~TestSpansAdvanced();

    void setUp();
    void testBooleanQueryWithSpanQueries();

protected:
    virtual void addDocuments( IndexWriter * writer );
    void addDocument( IndexWriter * writer, const wchar_t * id, const wchar_t * text );

    /**
     * Tests two span queries.
     */
    void doTestBooleanQueryWithSpanQueries( const float_t expectedScore );

    /**
     * Checks to see if the hits are what we expected.
     *
     * @param query the query to execute
     * @param description the description of the search
     * @param expectedIds the expected document ids of the hits
     * @param expectedScores the expected scores of the hits
     *
     * @throws IOException
     */
    void assertHits( Query * query, const wchar_t * description, const wchar_t ** expectedIds, float_t * expectedScores, size_t expectedCount );
};
#endif

