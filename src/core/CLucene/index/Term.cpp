/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team

* Updated by https://github.com/farfella/.
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "Term.h"
#include "CLucene/util/_StringIntern.h"
#include "CLucene/util/Misc.h"

CL_NS_USE(util)
CL_NS_DEF(index)

Term::Term(){
	//Intern fld and assign it to field
	_field = LUCENE_BLANK_STRING;
	internF = false;
	cachedHashCode = 0;
	textLen = 0;		
		
	//Duplicate txt and assign it to text
	#ifdef LUCENE_TERM_TEXT_LENGTH
		_text[0]=0;
	#else
		_text = _wcsdup(LUCENE_BLANK_STRING);
		textLenBuf = 0;
	#endif
	textLen = 0;
}

Term::Term(const wchar_t* fld, const wchar_t* txt, bool internField){
//Func - Constructor.
//       Constructs a Term with the given field and text. Field and text are not copied
//       Field and text are deleted in destructor only if intern is false. 
//Pre  - fld != NULL and contains the name of the field
//       txt != NULL and contains the value of the field
//       internF is true or false and indicates if term Field is interned or not
//       internT is true or false and indicates if term Text is interned or not
//       canDelete defaults to true but can be false and indicates to the IGarbageCollector that the Term can be deleted when finalized
//Post - An instance of Term has been created.Field and txt have not been copied but assigned
	
	_field = LUCENE_BLANK_STRING;
	internF = false;
	textLen = 0;
	#ifdef LUCENE_TERM_TEXT_LENGTH
		_text[0]=0;
	#else
		_text = _wcsdup(LUCENE_BLANK_STRING);
		textLenBuf = 0;
	#endif

    set(fld,txt,internField);
}


Term::Term(const Term* fieldTerm, const wchar_t* txt){
	_field = LUCENE_BLANK_STRING;
	internF = false;
	textLen = 0;
	#ifdef LUCENE_TERM_TEXT_LENGTH
		_text[0]=0;
	#else
		_text = _wcsdup(LUCENE_BLANK_STRING);
		textLenBuf = 0;
	#endif

    set(fieldTerm,txt);
}

Term::Term(const wchar_t* fld, const wchar_t* txt){
	_field = LUCENE_BLANK_STRING;
	internF = false;
	textLen = 0;
	#ifdef LUCENE_TERM_TEXT_LENGTH
		_text[0]=0;
	#else
		_text = _wcsdup(LUCENE_BLANK_STRING);
		textLenBuf = 0;
	#endif

    set(fld,txt);
}

Term::~Term(){
//Func - Destructor.
//Pre  - true
//Post - The instance has been destroyed. field and text have been deleted if pre(intrn) is false

	//Unintern field
	if ( internF )
		CLStringIntern::unintern(_field);
	_field = NULL;

#ifndef LUCENE_TERM_TEXT_LENGTH
	//Deletetext if it is the owner
	_CLDELETE_CARRAY( _text );
#endif
}

const wchar_t* Term::field() const {
//Func - Returns the field of this term, an interned string. The field indicates
//       the part of a document which this term came from. 
//Pre  - true
//Post - field has been returned

    return _field;
}

const wchar_t* Term::text() const {
//Func - Returns the text of this term.  In the case of words, this is simply the
//       text of the word.  In the case of dates and other types, this is an
//       encoding of the object as a string.
//Pre  - true
//Post - text has been returned

    return _text;
}


void Term::set(const Term* term, const wchar_t* txt){
	set(term->field(),txt,false);
}

void Term::set(const wchar_t* fld, const wchar_t* txt,const bool internField){
//Func - Resets the field and text of a Term.
//Pre  - fld != NULL and contains the name of the field
//       txt != NULL and contains the value of the field
//       internF is true or false
//       internT is true or false
//Post - field and text of Term have been reset

    CND_PRECONDITION(fld != NULL, L"fld contains NULL");
    CND_PRECONDITION(txt != NULL, L"txt contains NULL");

	//save field for unintern later
	const wchar_t* oldField = _field;
	//bool oldInternF = internF; //Not used
	cachedHashCode = 0;

    textLen = wcslen(txt);

	//Delete text if it is the owner
#ifdef LUCENE_TERM_TEXT_LENGTH
	if ( textLen > LUCENE_TERM_TEXT_LENGTH )
	   textLen = LUCENE_TERM_TEXT_LENGTH;
	wcsncpy(_text,txt,textLen+1);
    _text[textLen]=0;
#else

	//if the term text buffer is bigger than what we have
	if ( _text && textLen > textLenBuf){
		_CLDELETE_ARRAY( _text );
		textLenBuf = 0;
	}

	if ( _text==NULL ){
		//duplicate the text
		_text  = _wcsdup(txt);
		textLenBuf = textLen;
	}else{
		//re-use the buffer
		wcscpy(_text, txt);
	}

#endif

    //Set Term Field
	if ( internField )
		_field = CLStringIntern::intern(fld);
	else
		_field = fld;

	//unintern old field after interning new one, 
	if ( internF )
		CLStringIntern::unintern(oldField);
	internF = internField;
		    
    CND_PRECONDITION(wcscmp(fld, _field)==0,L"field not equal");
}

/** Compares two terms, returning true iff they have the same
  field and text. */
bool Term::equals(const Term* other) const{
	if (other == this)
		return true;
  if (other == NULL)
      return false;

   if ( cachedHashCode != 0 && other->cachedHashCode != 0 && other->cachedHashCode != cachedHashCode )
		return false;

  if ( _field==other->_field ){
     //this can be quicker than using compareTo, because checks
     //field length first
	  if ( textLen == other->textLen ){
		  return (wcscmp(_text,other->_text)==0);
	  }else
        return false;
  }else
     return false;
}

size_t Term::hashCode(){
	if ( cachedHashCode == 0 )
		cachedHashCode = Misc::thashCode(_field) + Misc::thashCode(_text,textLen);
	return cachedHashCode;
}

size_t Term::textLength() const { return textLen; }

int32_t Term::compareTo(const Term* other) const {
//Func - Compares two terms, to see if this term belongs before,is equal to or after
//       after the argument term.
//Pre  - other is a reference to another term
//Post - A negative integer is returned if this term belongs before the argument, 
//       zero is returned if this term is equal to the argument, and a positive integer 
//       if this term belongs after the argument.

	//Check ret to see if text needs to be compared
	if ( _field == other->_field ){ // fields are interned
		//Compare text with text of other and return the result
		return wcscmp(_text,other->_text);
	}else{
		int32_t ret = wcscmp(_field,other->_field);
                if ( ret == 0 ){
                    return wcscmp(_text,other->_text);
                }else{
                    return ret;
                }
        }
}

int32_t Term::hashedCompareTo(Term* other) {
    size_t hc1 = this->hashCode();
    size_t hc2 = other->hashCode();
    
    if ( hc1 == hc2 )
        return compareTo(other);
    else if ( hc1 > hc2 )
        return -1;
    else
        return 1;
}

std::wstring Term::toString() const{
//Func - Forms the contents of Field and term in some kind of tuple notation
//       <field:text>
//Pre  - true
//Post - a string formatted as <field:text> is returned if pre(field) is NULL and
//       text is NULL the returned string will be formatted as <:>

	// Note: Should this representation ever change, make sure to update Query and Filter classes
	//       that may be using this format without calling toString (to save on memory allocations)
	//       For example: PrefixFilter::toString()

	return CL_NS(util)::Misc::join( _field, L":", _text);
}

CL_NS_END
