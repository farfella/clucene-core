/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* Updated by https://github.com/farfella/.
 Updated by https://github.com/farfella/.
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "QueryFilter.h"
#include "IndexSearcher.h"
#include "CLucene/index/IndexReader.h"
#include "CLucene/util/BitSet.h"
#include "SearchHeader.h"
#include "Query.h"

CL_NS_DEF(search)
CL_NS_USE(util)
CL_NS_USE(index)


class QFHitCollector: public HitCollector{
	CL_NS(util)::BitSet* bits;
public:
	QFHitCollector(CL_NS(util)::BitSet* bits){
		this->bits = bits;
	}
	void collect(const int32_t doc, const float_t /*score*/){
		bits->set(doc);  // set bit for hit
	}
};


QueryFilter::QueryFilter( const Query* query )
{
	this->query = query->clone();
    bDeleteQuery = true;
}

QueryFilter::QueryFilter( Query* query, bool bDeleteQuery )
{
    this->query = query;
    this->bDeleteQuery = bDeleteQuery;
}

QueryFilter::~QueryFilter()
{
    if( bDeleteQuery )
	    _CLDELETE( query );
}


QueryFilter::QueryFilter( const QueryFilter& copy )
{
	this->query = copy.query->clone();
    bDeleteQuery = true;
}


Filter* QueryFilter::clone() const {
	return _CLNEW QueryFilter(*this );
}


wchar_t* QueryFilter::toString()
{
	wchar_t* qt = query->toString();
	size_t len = wcslen(qt) + 14;
	wchar_t* ret = _CL_NEWARRAY( wchar_t, len );
	ret[0] = 0;
	_snwprintf( ret, len, L"QueryFilter(%s)", qt );
	_CLDELETE_CARRAY(qt);
	return ret;
}


/** Returns a BitSet with true for documents which should be permitted in
search results, and false for those that should not. */
BitSet* QueryFilter::bits( IndexReader* reader )
{
    BitSet* bits = _CLNEW BitSet(reader->maxDoc());

	IndexSearcher s(reader);
	QFHitCollector hc(bits);
	s._search(query, NULL, &hc);
    return bits;
}


CL_NS_END
