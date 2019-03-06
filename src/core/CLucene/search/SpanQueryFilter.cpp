/*------------------------------------------------------------------------------
 * Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* Updated by https://github.com/farfella/.
 Updated by https://github.com/farfella/.
 * 
 * Distributable under the terms of either the Apache License (Version 2.0) or 
 * the GNU Lesser General Public License, as specified in the COPYING file.
 ------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "SpanQueryFilter.h"
#include "CLucene/index/IndexReader.h"
#include "CLucene/search/spans/Spans.h"


CL_NS_DEF(search)

SpanQueryFilter::SpanQueryFilter() : query( NULL )
{}

SpanQueryFilter::SpanQueryFilter( const CL_NS2(search,spans)::SpanQuery * query )
{
    this->query = (CL_NS2(search,spans)::SpanQuery *) query->clone();
    bDeleteQuery = true;
}

SpanQueryFilter::SpanQueryFilter( CL_NS2(search,spans)::SpanQuery * query, bool bDeleteQuery )
{
    this->query = query;
    this->bDeleteQuery = bDeleteQuery; 
}

SpanQueryFilter::SpanQueryFilter( const SpanQueryFilter& copy )
{
	this->query = (CL_NS2(search,spans)::SpanQuery *) copy.query->clone();
    bDeleteQuery = true;
}

SpanQueryFilter::~SpanQueryFilter()
{
    if( bDeleteQuery )
        _CLDELETE( query );
}

Filter* SpanQueryFilter::clone() const 
{
	return _CLNEW SpanQueryFilter( *this );
}

CL_NS(util)::BitSet * SpanQueryFilter::bits( CL_NS(index)::IndexReader * reader )
{
    SpanFilterResult *    result = bitSpans( reader );
    CL_NS(util)::BitSet * bits   = result->getBits();
    _CLLDELETE( result );
    return bits;
}

SpanFilterResult * SpanQueryFilter::bitSpans( CL_NS(index)::IndexReader * reader )
{
    CL_NS(util)::BitSet *                                   bits        = _CLNEW CL_NS(util)::BitSet( reader->maxDoc() );
    CL_NS2(search,spans)::Spans *                           spans       = query->getSpans( reader );
    CL_NS(util)::CLList<SpanFilterResult::PositionInfo*> *  tmp         = new CL_NS(util)::CLList<SpanFilterResult::PositionInfo*>();
    int32_t                                                 currentDoc  = -1;
    SpanFilterResult::PositionInfo *                        currentInfo = NULL;

    while( spans->next() )
    {
        int32_t doc = spans->doc();
        bits->set( doc );
        if( currentDoc != doc )
        {
            currentInfo = _CLNEW SpanFilterResult::PositionInfo( doc );
            tmp->push_back( currentInfo );
            currentDoc = doc;
        }
        currentInfo->addPosition( spans->start(), spans->end() );
    }
    return _CLNEW SpanFilterResult( bits, tmp );
}

wchar_t* SpanQueryFilter::toString()
{
	wchar_t* qt = query->toString();
	size_t len = wcslen( qt ) + 21;
	wchar_t* ret = _CL_NEWARRAY( wchar_t, len );
	ret[0] = 0;
	_snwprintf( ret, len, L"QueryWrapperFilter(%s)", qt );
	_CLDELETE_CARRAY( qt );
	return ret;
}

CL_NS_END
