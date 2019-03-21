/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* Updated by https://github.com/farfella/.
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "StringBuffer.h"
#include "Misc.h"
#include <assert.h>

std::wstring float_to_wstring(const float_t value, const size_t digits)
{
    //Func - Appends a float_t (after conversion to a character string)
    //Pre  - digits > 0. Indicates the minimum number of characters printed
    //Post - The converted float_t value has been appended to the string in buffer

      //using sprintf("%f" was not reliable on other plaforms... we use a custom float convertor
      //bvk: also, using sprintf and %f seems excessivelly slow
    assert(digits <= 8);

    //the maximum number of characters that int64 will hold is 23. so we need 23*2+2
    const size_t sz = 48;
    wchar_t buf[sz]; //the buffer to hold
    int64_t v = (int64_t) value; //the integer value of the float
    _i64tow_s(v, buf, sz, 10); //add the whole number

    size_t l = 99 - wcslen(buf); //how many digits we have to work with?
    size_t dig = l < (size_t) digits ? l : digits;
    if (dig > 0)
    {
        wcscat_s(buf, L"."); //add a decimal point

        int64_t remi = (int64_t) ((value - v)*pow((float_t) 10, (float_t) (dig + 1))); //take the remainder and make a whole number
        if (remi < 0) remi *= -1;
        int64_t remadj = remi / 10;
        if (remi - (remadj * 10) >= 5)
            remadj++; //adjust remainder

        // add as many zeros as necessary between the decimal point and the
        // significant part of the number. Fixes a bug when trying to print
        // numbers that have zeros right after the decimal point
        if (remadj)
        {
            size_t numZeros = dig - (size_t) log10((float_t) remadj) - 1;
            while (numZeros-- > 0 && numZeros < 10)
                wcscat_s(buf, L"0"); //add a zero before the decimal point
        }

        _i64tow(remadj, buf + wcslen(buf), 10); //add the remainder
    }

    return buf;
}

std::wstring boost_to_wstring(const float_t boost)
{
    std::wstring ret;
    if (boost != 1.0f)
    {
        ret.push_back(L'^');
        ret.append(float_to_wstring(boost, 1));
    }

    return ret;
}

std::wstring bool_to_wstring(const bool value)
{
    return value ? L"true" : L"false";
}

#if 0
CL_NS_DEF(util)

	StringBuffer::StringBuffer(wchar_t* buf,size_t maxlen, const bool consumeBuffer){
		buffer = buf;
		bufferLength = maxlen;
		bufferOwner = !consumeBuffer;
	  len          = 0;
	}
  StringBuffer::StringBuffer(){
  //Func - Constructor. Allocates a buffer with the default length.
  //Pre  - true
  //Post - buffer of length bufferLength has been allocated

    //Initialize
    bufferLength = LUCENE_DEFAULT_TOKEN_BUFFER_SIZE;
    len          = 0;
    //Allocate a buffer of length bufferLength
    buffer       = _CL_NEWARRAY(wchar_t,bufferLength);
    bufferOwner  = true;
  }

  StringBuffer::StringBuffer(const size_t initSize){
  //Func - Constructor. Allocates a buffer of length initSize + 1
  //Pre  - initSize > 0
  //Post - A buffer has been allocated of length initSize + 1

	  //Initialize the bufferLength to initSize + 1 The +1 is for the terminator '\0'
    bufferLength = initSize + 1;
    len = 0;
    //Allocate a buffer of length bufferLength
    buffer = _CL_NEWARRAY(wchar_t,bufferLength);
    bufferOwner = true;
  }

  StringBuffer::StringBuffer(const wchar_t* value){
  //Func - Constructor.
  //       Creates an instance of Stringbuffer containing a copy of the string value
  //Pre  - value != NULL
  //Post - An instance of StringBuffer has been created containing the copy of the string value

      //Initialize the length of the string to be stored in buffer
	  len = (size_t) wcslen(value);

	  //Calculate the space occupied in buffer by a copy of value
      const size_t occupiedLength = len + 1;

	  // Minimum allocated buffer length is LUCENE_DEFAULT_TOKEN_BUFFER_SIZE.
      bufferLength = (occupiedLength >= LUCENE_DEFAULT_TOKEN_BUFFER_SIZE
	 	? occupiedLength : LUCENE_DEFAULT_TOKEN_BUFFER_SIZE);

	  //Allocate a buffer of length bufferLength
      buffer = _CL_NEWARRAY(wchar_t,bufferLength);
	  bufferOwner  = true;
      //Copy the string value into buffer
      wcsncpy_s(buffer, bufferLength, value, occupiedLength);
	  //Assert that the buffer has been terminated at the end of the string
      CND_PRECONDITION (buffer[len] == '\0', "Buffer was not correctly terminated");
  }

  StringBuffer::~StringBuffer() {
  // Func - Destructor
  // Pre  - true
  // Post - Instanc has been destroyed

	  if( bufferOwner ){
		  _CLDELETE_CARRAY(buffer);
	  }else
		  buffer = NULL;
  }

  void StringBuffer::clear(){
  //Func - Clears the Stringbuffer and resets it to it default empty state
  //Pre  - true
  //Post - pre(buffer) has been destroyed and a new one has been allocated

	  // TODO:	Should we really delete and recreate the buffer - perhaps just reseting len should suffice?
	  //		We should really look into at least providing both options

      //Destroy the current buffer if present
	    _CLDELETE_LCARRAY(buffer);

	  //Initialize
      len = 0;
      bufferLength = LUCENE_DEFAULT_TOKEN_BUFFER_SIZE;
      //Allocate a buffer of length bufferLength
      buffer = _CL_NEWARRAY(wchar_t,bufferLength);
  }

  void StringBuffer::appendChar(const wchar_t character) {
  //Func - Appends a single character
  //Pre  - true
  //Post - The character has been appended to the string in the buffer

	  //Check if the current buffer length is sufficient to have the string value appended
      if (len + 1 > bufferLength){
		   //Have the size of the current string buffer increased because it is too small
          growBuffer(len + 1);
      }
	  //Put character at position len which is the end of the string in the buffer
	  //Note that this action might overwrite the terminator of the string '\0', which
	  //is kind of tricky
      buffer[len] = character;
	  //Increase the len by to represent the correct length of the string in the buffer
      len++;
  }

  void StringBuffer::append(const wchar_t* value) {
  //Func - Appends a copy of the string value
  //Pre  - value != NULL
  //Post - value has been copied and appended to the string in buffer

      append(value, wcslen(value));
  }
  void StringBuffer::append(const wchar_t* value, size_t appendedLength) {
  //Func - Appends a copy of the string value
  //Pre  - value != NULL
  //       appendedLength contains the length of the string value which is to be appended
  //Post - value has been copied and appended to the string in buffer

      //Check if the current buffer length is sufficient to have the string value appended
      if (len + appendedLength + 1 > bufferLength){
          //Have the size of the current string buffer increased because it is too small
          growBuffer(len + appendedLength + 1);
      }

      //Copy the string value into the buffer at postion len
      wcsncpy(buffer + len, value, appendedLength);

      //Add the length of the copied string to len to reflect the new length of the string in
      //the buffer (Note: len is not the bufferlength!)
      len += appendedLength;
  }

  void StringBuffer::appendInt(const int64_t value, const int32_t _Radix) {
  //Func - Appends an integer (after conversion to a character string)
  //Pre  - true
  //Post - The converted integer value has been appended to the string in buffer

      //instantiate a buffer of 30 charactes for the conversion of the integer
      const size_t sz = 30;
      wchar_t buf[sz];
      //Convert the integer value to a string buf using _Radix
      _i64tow_s(value, buf, sz, _Radix);
	  //Have the converted integer now stored in buf appended to the string in buffer
      append(buf);
  }

  void StringBuffer::appendFloat(const float_t value, const size_t digits){
  //Func - Appends a float_t (after conversion to a character string)
  //Pre  - digits > 0. Indicates the minimum number of characters printed
  //Post - The converted float_t value has been appended to the string in buffer

    //using sprintf("%f" was not reliable on other plaforms... we use a custom float convertor
    //bvk: also, using sprintf and %f seems excessivelly slow
    assert(digits <= 8);

	  //the maximum number of characters that int64 will hold is 23. so we need 23*2+2
    const size_t sz = 48;
	 wchar_t buf[sz]; //the buffer to hold
	 int64_t v = (int64_t)value; //the integer value of the float
	 _i64tow_s(v,buf, sz, 10); //add the whole number

	 size_t l = 99-wcslen(buf); //how many digits we have to work with?
	 size_t dig = l< (size_t)digits ? l : digits;
	 if ( dig > 0 ){
		wcscat_s(buf, L"."); //add a decimal point

		int64_t remi=(int64_t)((value-v)*pow((float_t)10,(float_t)(dig+1))); //take the remainder and make a whole number
		if ( remi<0 ) remi*=-1;
		int64_t remadj=remi/10;
		if ( remi-(remadj*10) >=5 )
			remadj++; //adjust remainder

		// add as many zeros as necessary between the decimal point and the
		// significant part of the number. Fixes a bug when trying to print
		// numbers that have zeros right after the decimal point
		if (remadj) {
			size_t numZeros = dig - (size_t)log10((float_t)remadj) - 1;
		    while(numZeros-- > 0 && numZeros < 10)
				wcscat_s(buf, L"0"); //add a zero before the decimal point
		}

		_i64tow(remadj,buf+wcslen(buf),10); //add the remainder
	 }

	 append(buf);
  }

  void StringBuffer::appendBoost(const float_t boost)
  {
	  if (boost != 1.0f)
      {
		  appendChar(L'^'); appendFloat(boost,1);
	  }
  }

  void StringBuffer::appendBool(const bool value)
  {
      append( value ? L"true" : L"false");
  }

  void StringBuffer::prepend(const wchar_t* value)
  {
  //Func - Puts a copy of the string value infront of the current string in the StringBuffer
  //Pre  - value != NULL
  //Post - The string in pre(buffer) has been shifted n positions where n equals the length of value.
  //       The string value was then copied to the beginning of stringbuffer

      prepend(value, wcslen(value));
  }

  void StringBuffer::prepend(const wchar_t* value, const size_t prependedLength) {
  //Func - Puts a copy of the string value in front of the string in the StringBuffer
  //Pre  - value != NULL
  //       prependedLength contains the length of the string value which is to be prepended
  //Post - A copy of the string value is has been in front of the string in buffer
   //todo: something is wrong with this code, i'm sure... it only grows (and therefore moves if the buffer is to small)
	  //Check if the current buffer length is sufficient to have the string value prepended
	  if (prependedLength + len + 1 > bufferLength){
		  //Have the size of the current string buffer increased because it is too small
		  //Because prependedLength is passed as the second argument to growBuffer,
          //growBuffer will have left the first prependedLength characters empty
          //when it recopied buffer during reallocation.
          growBuffer(prependedLength + len + 1, prependedLength);
	  }

      //Copy the string value into the buffer at postion 0
      wcsncpy_s(buffer, bufferLength, value, prependedLength);
      //Add the length of the copied string to len to reflect the new length of the string in
      //the buffer (Note: len is not the bufferlength!)
      len += prependedLength;
  }

  size_t StringBuffer::length() const{
  //Func - Returns the length of the string in the StringBuffer
  //Pre  - true
  //Post - The length len of the string in the buffer has been returned

      return len;
  }
  wchar_t* StringBuffer::toString(){
  //Func - Returns a copy of the current string in the StringBuffer sized equal to the length of the string
  //       in the StringBuffer.
  //Pre  - true
  //Post - The copied string has been returned

	  //Instantiate a buffer equal to the length len + 1
      wchar_t* ret = _CL_NEWARRAY(wchar_t,len + 1);
	  if (ret){
		  //Copy the string in buffer
	      wcsncpy_s(ret, len+1, buffer, len);
		  //terminate the string
          ret[len] = '\0';
	  }
      //return the the copy
      return ret;
  }
  wchar_t* StringBuffer::getBuffer() {
  //Func - '\0' terminates the buffer and returns its pointer
  //Pre  - true
  //Post - buffer has been '\0' terminated and returned

      // Check if the current buffer is '\0' terminated
	  if (len == bufferLength){
          //Make space for terminator, if necessary.
          growBuffer(len + 1);
	  }
    //'\0' buffer so it can be returned properly
    buffer[len] = '\0';

    return buffer;
  }

  wchar_t* StringBuffer::giveBuffer() {
    wchar_t* ret = getBuffer();
    buffer = NULL;
    len = 0;
    bufferLength = 0;
    bufferOwner = false;
    return ret;
  }

  void StringBuffer::reserve(const size_t size){
  	if ( bufferLength >= size )
  		return;
	bufferLength = size;

	//Allocate a new buffer of length bufferLength
    wchar_t* tmp = _CL_NEWARRAY(wchar_t,bufferLength);
    wcsncpy_s(tmp, bufferLength, buffer, len);
    tmp[len] = '\0';

	//destroy the old buffer
	if (buffer)
    {
		_CLDELETE_CARRAY(buffer);
	}
	//Assign the new buffer tmp to buffer
    buffer = tmp;
  }

  void StringBuffer::growBuffer(const size_t minLength, const size_t skippingNInitialChars) {
  //Func - Has the buffer grown to a minimum length of minLength or bigger and shifts the
  //       current string in buffer by skippingNInitialChars forward
  //Pre  - After growth, must have at least enough room for contents + terminator so
  //       minLength >= skippingNInitialChars + len + 1
  //       skippingNInitialChars >= 0
  //Post - The buffer has been grown to a minimum length of minLength or bigger and
  //       if skippingNInitialChars > 0, the contents of the buffer has beeen shifted
  //       forward by skippingNInitialChars positions as the buffer is reallocated,
  //       leaving the first skippingNInitialChars uninitialized (presumably to be
  //       filled immediately thereafter by the caller).

    CND_PRECONDITION (minLength >= skippingNInitialChars + len + 1,"skippingNInitialChars is not large enough");

    //More aggressive growth strategy to offset smaller default buffer size:
    if ( !bufferOwner ){
      assert(bufferLength>=minLength);
      return;
    }

    bufferLength *= 2;
    //Check that bufferLength is bigger than minLength
    if (bufferLength < minLength){
      //Have bufferLength become minLength because it still was too small
      bufferLength = minLength;
    }

    //Allocate a new buffer of length bufferLength
    wchar_t* tmp = _CL_NEWARRAY(wchar_t,bufferLength);
    memset(tmp, 0, sizeof(wchar_t) * skippingNInitialChars);

    //The old buffer might not have been null-terminated, so we wcsncpy
    //only len bytes, not len+1 bytes (the latter might read one char off the
    //end of the old buffer), then apply the terminator to the new buffer.
    wcsncpy(tmp + skippingNInitialChars, buffer, len);
    tmp[skippingNInitialChars + len] = '\0';

    //destroy the old buffer
    _CLDELETE_LCARRAY(buffer);

	//Assign the new buffer tmp to buffer
    buffer = tmp;
  }

  void StringBuffer::sewchar_tAt(size_t pos, const wchar_t chr) {
    CND_PRECONDITION (pos < len, "pos is not in string");
    buffer[pos] = chr;
  }

  wchar_t StringBuffer::charAt(size_t pos) {
    CND_PRECONDITION (pos < len, "pos is not in string");
    return buffer[pos];
  }

  void StringBuffer::insert(const size_t pos, wchar_t chr) {
    CND_PRECONDITION (pos <= len, "pos is larger than string len");
    growBuffer(len + 1, 0);
    memmove(&buffer[pos + 1], &buffer[pos], sizeof(wchar_t) * (len - pos));
    buffer[pos] = chr;
    len++;
  }

  void StringBuffer::insert(const size_t pos, const wchar_t* chrs, size_t length) {
    CND_PRECONDITION (pos <= len, "pos is larger than string len");

    if (length == -1) {
      length = wcslen(chrs);
    }

    if (length > 0) {
      growBuffer(len + length, 0);
      memmove(&buffer[pos + length], &buffer[pos], sizeof(wchar_t) * (len - pos));
      memcpy(&buffer[pos], chrs, sizeof(wchar_t) * (length));
      len += length;
    }
  }

  void StringBuffer::deleteCharAt(size_t pos) {
    CND_PRECONDITION (pos < len, "pos is larger than string len");

    memmove(&buffer[pos], &buffer[pos + 1], sizeof(wchar_t) * (len - pos));
    len--;
    buffer[len] = L'\0';
  }

  void StringBuffer::deleteChars(size_t start, size_t end) {
    CND_PRECONDITION (start <= end && end <= len, "start/end is not in string");

    if (start < end) {
      memmove(&buffer[start], &buffer[end], sizeof(wchar_t) * (len - end));
      buffer[len - (end - start)] = L'\0';
      len -= end - start;
    }
  }

  void StringBuffer::toLower() {
    _tcslwr(buffer);
  }

  bool StringBuffer::substringEquals(size_t start, size_t end, const wchar_t* str, size_t length) const {
    if (length == -1) {
      length = wcslen(str);
    }

    if (end - start != length) {
      return false;
    }

    for (size_t c = start; c < end; c++) {
      if (buffer[c] != str[c - start]) {
        return false;
      }
    }

    return true;
  }

CL_NS_END
#endif