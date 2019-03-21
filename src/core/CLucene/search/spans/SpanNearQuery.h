/*------------------------------------------------------------------------------
 * Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team

* Updated by https://github.com/farfella/.
 *
 * Distributable under the terms of either the Apache License (Version 2.0) or
 * the GNU Lesser General Public License, as specified in the COPYING file.
 ------------------------------------------------------------------------------*/
#ifndef _lucene_search_spans_SpanNearQuery_
#define _lucene_search_spans_SpanNearQuery_

CL_CLASS_DEF(index, IndexReader);
#include "SpanQuery.h"

CL_NS_DEF2( search, spans )

/** Matches spans which are near one another.  One can specify <i>slop</i>, the
 * maximum number of intervening unmatched positions, as well as whether
 * matches are required to be in-order. */
class CLUCENE_EXPORT SpanNearQuery : public SpanQuery
{
private:
    SpanQuery **    clauses;
    size_t          clausesCount;
    bool            bDeleteClauses;

    int32_t         slop;
    bool            inOrder;

    std::wstring    field;

protected:
    SpanNearQuery( const SpanNearQuery& clone );

public:
    /** Construct a SpanNearQuery.  Matches spans matching a span from each
     * clause, with up to <code>slop</code> total unmatched positions between
     * them.  * When <code>inOrder</code> is true, the spans from each clause
     * must be * ordered as in <code>clauses</code>. */
    template<class ClauseIterator>
    SpanNearQuery( ClauseIterator first, ClauseIterator last, int32_t slop, bool inOrder, bool bDeleteClauses )
    {
        // CLucene specific: at least one clause must be here
        if( first ==  last )
            _CLTHROWA( CL_ERR_IllegalArgument, "Missing query clauses." );

        this->bDeleteClauses = bDeleteClauses;
        this->clausesCount = last - first;
        this->clauses = _CL_NEWARRAY( SpanQuery *, clausesCount );

        // copy clauses array into an array and check fields
        for( size_t i = 0; first != last; first++, i++ )
        {
            SpanQuery * clause = *first;
            if( i == 0 )
            {
                setField( clause->getField() );
            }
            else if( 0 != wcscmp( clause->getField(), field.c_str() ))
            {
                _CLTHROWA( CL_ERR_IllegalArgument, "Clauses must have same field." );
            }
            this->clauses[ i ] = clause;
        }

        this->slop = slop;
        this->inOrder = inOrder;
    }

    virtual ~SpanNearQuery();

    CL_NS(search)::Query * clone() const;

    static const std::wstring getClassName();
	const std::wstring getObjectName() const;

    /** Return the clauses whose spans are matched.
     * CLucene: pointer to the internal array
     */
    SpanQuery ** getClauses() const;
    size_t getClausesCount() const;

    /** Return the maximum number of intervening unmatched positions permitted.*/
    int32_t getSlop() const;

    /** Return true if matches are required to be in-order.*/
    bool isInOrder() const;

    const wchar_t * getField() const;

    /** Returns a collection of all terms matched by this query.
     * @deprecated use extractTerms instead
     * @see #extractTerms(Set)
     */
//    public Collection getTerms()

    void extractTerms( CL_NS(search)::TermSet * terms ) const;

    CL_NS(search)::Query * rewrite( CL_NS(index)::IndexReader * reader );

    using Query::toString;
    std::wstring toString( const wchar_t* field ) const;
    bool equals( Query* other ) const;
    size_t hashCode() const;

    Spans * getSpans( CL_NS(index)::IndexReader * reader );

protected:
    void setField( const wchar_t * field );
};

CL_NS_END2
#endif // _lucene_search_spans_SpanNearQuery_
