/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* Updated by https://github.com/farfella/.
 Updated by https://github.com/farfella/.
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "_CharStream.h"
#include "_FastCharStream.h"
#include "CLucene/util/CLStreams.h"

CL_NS_DEF(queryParser)

FastCharStream::FastCharStream(CL_NS(util)::Reader* r, bool ownsReader) :
  buffer(NULL),
  _bufferSize(0),
  bufferLength(0),
  bufferPosition(0),
  tokenStart(0),
  bufferStart(0),
  input(r),
  _ownsReader(ownsReader)
{
}

FastCharStream::~FastCharStream()
{
	if (_ownsReader ){
		_CLLDELETE(input);
	}
	_CLDELETE_LCARRAY(buffer);
}

wchar_t FastCharStream::readChar() {
	if (bufferPosition >= bufferLength)
		refill();
	return buffer[bufferPosition++];
}

void FastCharStream::refill() {
	int32_t newPosition = bufferLength - tokenStart;

	if (tokenStart == 0) {			  // token won't fit in buffer
		if (buffer == NULL) {		  // first time: alloc buffer
			buffer = _CL_NEWARRAY(wchar_t, 2048);
			_bufferSize = 2048;
		} else if (bufferLength == _bufferSize) { // grow buffer
			_bufferSize *= 2;
			wchar_t* newBuffer = _CL_NEWARRAY(wchar_t, _bufferSize);
			wcsncpy(newBuffer, buffer, bufferLength);
			_CLDELETE_LCARRAY(buffer);
			buffer = newBuffer;
		}
	} else {					  // shift token to front
		wcsncpy(buffer, buffer+tokenStart,newPosition);
	}

	bufferLength = newPosition;			  // update state
	bufferPosition = newPosition;
	bufferStart += tokenStart;
	tokenStart = 0;

	const wchar_t* charBuf = NULL;
	int32_t charsRead =				  // fill space in buffer
		input->read(charBuf, newPosition, _bufferSize-newPosition);
	if (charsRead == -1){
		_CLTHROWA(CL_ERR_IO, "read past eof");
	}
	else {
		memcpy(buffer, charBuf, charsRead * sizeof(wchar_t)); // TODO: Can we use the reader buffer instead of copying to our own?
		bufferLength += charsRead;
	}
}

void FastCharStream::backup(const int32_t amount) {
	bufferPosition -= amount;
}

wchar_t* FastCharStream::GetImage() {
	size_t len = bufferPosition - tokenStart;
	wchar_t* ret = _CL_NEWARRAY(wchar_t, len + 1);
	wcsncpy(ret, buffer+tokenStart, len);
	ret[len] = 0; // NULL terminated string
	return ret;
}

wchar_t* FastCharStream::GetSuffix(const int32_t len) {
	wchar_t* value = _CL_NEWARRAY(wchar_t, len + 1);
	wcsncpy(value, buffer+(bufferPosition - len), len);
	value[len] = 0; // NULL terminated string
	return value;
}

void FastCharStream::Done() {
}

wchar_t FastCharStream::BeginToken() {
	tokenStart = bufferPosition;
	return readChar();
}

int32_t FastCharStream::getColumn() const {
	return bufferStart + bufferPosition;
}
int32_t FastCharStream::getLine() const {
	return 1;
}
int32_t FastCharStream::getEndColumn() const {
	return bufferStart + bufferPosition;
}
int32_t FastCharStream::getEndLine() const {
	return 1;
}
int32_t FastCharStream::getBeginColumn() const {
	return bufferStart + tokenStart;
}
int32_t FastCharStream::getBeginLine() const {
	return 1;
}

CL_NS_END
