/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* Updated by https://github.com/farfella/.
 Updated by https://github.com/farfella/.
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/

#ifndef _lucene_search_RangeFilter_
#define _lucene_search_RangeFilter_

#include "Filter.h"

CL_CLASS_DEF(index,Term)

CL_NS_DEF(search)

/**
 * A Filter that restricts search results to a range of values in a given
 * field.
 *
 * <p>
 * This code borrows heavily from {@link RangeQuery}, but is implemented as a Filter
 *
 * </p>
 */
class CLUCENE_EXPORT RangeFilter: public Filter 
{
private:
	wchar_t* fieldName;
	wchar_t* lowerTerm;
	wchar_t* upperTerm;
	bool   includeLower;
	bool   includeUpper;

public:
    /**
     * @param fieldName The field this range applies to
     * @param lowerTerm The lower bound on this range
     * @param upperTerm The upper bound on this range
     * @param includeLower Does this range include the lower bound?
     * @param includeUpper Does this range include the upper bound?
     */
	RangeFilter( const wchar_t* fieldName, const wchar_t* lowerTerm, const wchar_t* upperTerm,
        bool includeLower, bool includeUpper );
    virtual ~RangeFilter();
	
    /**
    * Constructs a filter for field <code>fieldName</code> matching
    * less than or equal to <code>upperTerm</code>.
    */
	static RangeFilter* Less( const wchar_t* fieldName, const wchar_t* upperTerm );
	
    /**
    * Constructs a filter for field <code>fieldName</code> matching
    * more than or equal to <code>lowerTerm</code>.
    */
	static RangeFilter* More( const wchar_t* fieldName, const wchar_t* lowerTerm );
	
    /**
    * Returns a BitSet with true for documents which should be
    * permitted in search results, and false for those that should
    * not.
    */
	CL_NS(util)::BitSet* bits( CL_NS(index)::IndexReader* reader );
	
	Filter* clone() const;
	
	wchar_t* toString();

protected:
	RangeFilter( const RangeFilter& copy );
};

CL_NS_END
#endif
