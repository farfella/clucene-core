/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* Updated by https://github.com/farfella/.
 Updated by https://github.com/farfella/.
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "CLucene/index/Term.h"
#include "CLucene/index/Terms.h"
#include "CLucene/index/IndexReader.h"
#include "CLucene/util/BitSet.h"
#include "RangeFilter.h"

CL_NS_DEF(search)
CL_NS_USE(index)
CL_NS_USE(util)
CL_NS_USE(document)


RangeFilter::RangeFilter( const wchar_t* _fieldName, const wchar_t* _lowerTerm, const wchar_t* _upperTerm,
                         bool _includeLower, bool _includeUpper )
                         : fieldName(NULL), lowerTerm(NULL), upperTerm(NULL)
                         , includeLower(_includeLower), includeUpper(_includeUpper)
{
    if (NULL == _lowerTerm && NULL == _upperTerm) {
        _CLTHROWT(CL_ERR_IllegalArgument, L"At least one value must be non-null");
    }
    if (_includeLower && NULL == _lowerTerm) {
        _CLTHROWT(CL_ERR_IllegalArgument, L"The lower bound must be non-null to be inclusive");
    }
    if (_includeUpper && NULL == _upperTerm) {
        _CLTHROWT(CL_ERR_IllegalArgument, L"The upper bound must be non-null to be inclusive");
    }

	this->fieldName = _wcsdup(_fieldName);
	if ( _lowerTerm != NULL )
		this->lowerTerm = _wcsdup(_lowerTerm);
	if ( _upperTerm != NULL )
		this->upperTerm = _wcsdup(_upperTerm);
}

RangeFilter::RangeFilter( const RangeFilter& copy ) : 
	fieldName( _wcsdup(copy.fieldName) ),
	lowerTerm( _wcsdup(copy.lowerTerm) ), 
	upperTerm( _wcsdup(copy.upperTerm) ),
	includeLower( copy.includeLower ),
	includeUpper( copy.includeUpper )
{
}

RangeFilter::~RangeFilter()
{
    _CLDELETE_LCARRAY( fieldName );
	_CLDELETE_LCARRAY( lowerTerm );
	_CLDELETE_LCARRAY( upperTerm );
}

RangeFilter* RangeFilter::Less( const wchar_t* _fieldName, const wchar_t* _upperTerm ) {
	return _CLNEW RangeFilter( _fieldName, NULL, _upperTerm, false, true );
}

RangeFilter* RangeFilter::More( const wchar_t* _fieldName, const wchar_t* _lowerTerm ) {
	return _CLNEW RangeFilter( _fieldName, _lowerTerm, NULL, true, false );
}

BitSet* RangeFilter::bits( IndexReader* reader )
{
	BitSet* bts = _CLNEW BitSet( reader->maxDoc() );
	Term* term = NULL;
	
	Term* t = _CLNEW Term( fieldName, (lowerTerm ? lowerTerm : L""), false );
	TermEnum* enumerator = reader->terms( t );	// get enumeration of all terms after lowerValue
	_CLDECDELETE( t );
	
	if( enumerator->term(false) == NULL ) {
		_CLLDELETE( enumerator );
		return bts;
	}
	
	bool checkLower = false;
	if( !includeLower ) // make adjustments to set to exclusive
		checkLower = true;
	
	TermDocs* termDocs = reader->termDocs();
	
  #define CLEANUP \
    _CLLDECDELETE( term ); \
    termDocs->close(); \
    _CLLDELETE( termDocs ); \
    enumerator->close(); \
    _CLLDELETE( enumerator )
    
	try
	{
		do
		{
			term = enumerator->term();
			
			if( term == NULL || wcscmp(term->field(), fieldName) )
				break;
			
			if( !checkLower || lowerTerm == NULL || wcscmp(term->text(), lowerTerm) > 0 )
			{
				checkLower = false;
				if( upperTerm != NULL )
				{
					int compare = wcscmp( upperTerm, term->text() );
					
					/* if beyond the upper term, or is exclusive and
					 * this is equal to the upper term, break out */
					if( (compare < 0) || (!includeUpper && compare == 0) )
						break;
				}
				
				termDocs->seek( enumerator->term(false) );
				while( termDocs->next() ) {
					bts->set( termDocs->doc() );
				}
			}
			
			_CLDECDELETE( term );
		}
		while( enumerator->next() );
	}catch(CLuceneError& err){
    _CLDELETE(bts);
    CLEANUP;
    throw err;
  }
  CLEANUP;
	
	return bts;
}

wchar_t* RangeFilter::toString()
{
	size_t len = (fieldName ? wcslen(fieldName) : 0) + (lowerTerm ? wcslen(lowerTerm) : 0) + (upperTerm ? wcslen(upperTerm) : 0) + 8;
	wchar_t* ret = _CL_NEWARRAY( wchar_t, len );
	ret[0] = 0;
	_snwprintf( ret, len, L"%s: [%s-%s]", fieldName, (lowerTerm?lowerTerm: L""), (upperTerm?upperTerm: L"") );
	
	return ret;
}

Filter* RangeFilter::clone() const {
	return _CLNEW RangeFilter(*this );
}

CL_NS_END
