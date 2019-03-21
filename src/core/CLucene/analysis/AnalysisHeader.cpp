/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team

* Updated by https://github.com/farfella/.
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "AnalysisHeader.h"
#include "CLucene/util/StringBuffer.h"
#include "CLucene/util/_ThreadLocal.h"
#include <assert.h>

CL_NS_USE(util)
CL_NS_DEF(analysis)

struct Analyzer::Internal
{
    CL_NS(util)::ThreadLocal<TokenStream*,
        CL_NS(util)::Deletor::Object<TokenStream> >* tokenStreams;
};
Analyzer::Analyzer()
{
    _internal = new Internal;
    _internal->tokenStreams = _CLNEW CL_NS(util)::ThreadLocal<TokenStream*,
        CL_NS(util)::Deletor::Object<TokenStream> >;
}
Analyzer::~Analyzer()
{
    _CLLDELETE(_internal->tokenStreams);
    delete _internal;
}
TokenStream* Analyzer::getPreviousTokenStream()
{
    return _internal->tokenStreams->get();
}
void Analyzer::setPreviousTokenStream(TokenStream* obj)
{
    _internal->tokenStreams->set(obj);
}
TokenStream* Analyzer::reusableTokenStream(const wchar_t* fieldName, CL_NS(util)::Reader* reader)
{
    return tokenStream(fieldName, reader);
}

///Compares the Token for their order
class OrderCompare :LUCENE_BASE, public CL_NS(util)::Compare::_base //<Token*>
{
public:
    bool operator()(Token* t1, Token* t2) const
{
if (t1->startOffset() > t2->startOffset())
    return false;
if (t1->startOffset() < t2->startOffset())
    return true;
return true;
}
};

Token::Token() :
    _startOffset(0),
    _endOffset(0),
    _type(getDefaultType()),
    positionIncrement(1),
    payload(NULL)
{
    _termTextLen = 0;
#ifndef LUCENE_TOKEN_WORD_LENGTH
    _buffer = NULL;
    bufferTextLen = 0;
#else
    _buffer[0] = 0; //make sure null terminated
    bufferTextLen = LUCENE_TOKEN_WORD_LENGTH + 1;
#endif
}

Token::~Token()
{
#ifndef LUCENE_TOKEN_WORD_LENGTH
    free(_buffer);
#endif
    _CLLDELETE(payload);
}

Token::Token(const wchar_t* text, const int32_t start, const int32_t end, const wchar_t* typ) :
    _startOffset(start),
    _endOffset(end),
    _type((typ == NULL ? getDefaultType() : typ)),
    positionIncrement(1),
    payload(NULL)
{
    _termTextLen = 0;
#ifndef LUCENE_TOKEN_WORD_LENGTH
    _buffer = NULL;
    bufferTextLen = 0;
#else
    _buffer[0] = 0; //make sure null terminated
    bufferTextLen = LUCENE_TOKEN_WORD_LENGTH + 1;
#endif
    setText(text, -1);
}

size_t Token::bufferLength()
{
    return bufferTextLen;
}


int32_t Token::startOffset() const
{
    return _startOffset;
}

void Token::setStartOffset(const int32_t val)
{
    _startOffset = val;
}
int32_t Token::endOffset() const
{
    return _endOffset;
}
const wchar_t* Token::getDefaultType()
{
    return L"word";
}

void Token::setEndOffset(const int32_t val)
{
    _endOffset = val;
}
const wchar_t* Token::type() const { return _type; }

void Token::setType(const wchar_t* val)
{
    _type = val;
}

void Token::set(const wchar_t* text, const int32_t start, const int32_t end, const wchar_t* typ)
{
    _startOffset = start;
    _endOffset = end;
    _type = (typ == NULL ? getDefaultType() : typ);
    positionIncrement = 1;
    setText(text);
}

void Token::setText(const wchar_t* text, int32_t l)
{
    if (l < 0) l = wcslen(text);
#ifndef LUCENE_TOKEN_WORD_LENGTH
    if (bufferTextLen < l + 1)
        growBuffer(l + 1);
#else
    if (_termTextLen > LUCENE_TOKEN_WORD_LENGTH)
    {
        //in the case where this occurs, we will leave the endOffset as it is
        //since the actual word still occupies that space.
        l = LUCENE_TOKEN_WORD_LENGTH;
    }
#endif
    wcsncpy(_buffer, text, l);
    _termTextLen = l;
    _buffer[_termTextLen] = 0; //make sure null terminated
}

void Token::setTermLength(int32_t len)
{
    if (bufferTextLen < len)
        this->growBuffer(len);
    this->_termTextLen = len;
}
wchar_t* Token::resizeTermBuffer(size_t size)
{
    if (bufferTextLen < size)
        growBuffer(size);
    return this->_buffer;
}
void Token::growBuffer(size_t size)
{
    if (bufferTextLen >= size)
        return;
#ifndef LUCENE_TOKEN_WORD_LENGTH
    if (_buffer == NULL)
    {
        _buffer = (wchar_t*) malloc(size * sizeof(wchar_t));
        *_buffer = 0;
    }
    else
    {
        //use realloc. growBuffer is public, therefore could be called
        //without a subsequent call to overwriting the memory
        _buffer = (wchar_t*) realloc(_buffer, size * sizeof(wchar_t));
    }
    bufferTextLen = size;
#else
    _CLTHROWA(CL_ERR_TokenMgr, "Couldn't grow Token buffer");
#endif
}

void Token::setPositionIncrement(int32_t posIncr)
{
    if (posIncr < 0)
    {
        _CLTHROWA(CL_ERR_IllegalArgument, "positionIncrement must be >= 0");
    }
    positionIncrement = posIncr;
}

int32_t Token::getPositionIncrement() const { return positionIncrement; }

const wchar_t* Token::termText() const
{
    return (const wchar_t*) _buffer;
}
wchar_t* Token::termBuffer() const
{
    return  _buffer;
}
size_t Token::termTextLength()
{
    if (_termTextLen == -1) //it was invalidated by growBuffer
        _termTextLen = wcslen(_buffer);
    return _termTextLen;
}
size_t Token::termLength()
{
    if (_termTextLen == -1) //it was invalidated by growBuffer
        _termTextLen = wcslen(_buffer);
    return _termTextLen;
}
void Token::resetTermTextLen()
{
    _termTextLen = -1;
}

std::wstring Token::toString() const
{
    std::wstring sb = L"(";
    if (_buffer)
        sb.append(_buffer);
    else
        sb.append(L"null");

    sb.append(L",");
    sb.append(std::to_wstring(_startOffset));
    sb.append(L",");
    sb.append(std::to_wstring(_endOffset));

    if (!wcscmp(_type, L"word") == 0)
    {
        sb.append(L",type=");
        sb.append(_type);
    }

    if (positionIncrement != 1)
    {
        sb.append(L",posIncr=");
        sb.append(std::to_wstring(positionIncrement));
    }

    sb.append(L")");

    return sb;
}

CL_NS(index)::Payload* Token::getPayload() { return this->payload; }
void Token::setPayload(CL_NS(index)::Payload* payload)
{
    _CLLDELETE(this->payload);
    this->payload = payload;
}
void Token::clear()
{
    _CLDELETE(payload);
    // Leave _buffer to allow re-use
    _termTextLen = 0;
    positionIncrement = 1;
    // startOffset = endOffset = 0;
    // type = DEFAULT_TYPE;
}

void TokenStream::reset()
{
}

Token* TokenStream::next()
{
    Token* t = _CLNEW Token; //deprecated
    if (!next(t))
        _CLDELETE(t);
    return t;
}
TokenStream::~TokenStream()
{
}

TokenFilter::TokenFilter(TokenStream* in, bool deleteTS) :
    input(in),
    deleteTokenStream(deleteTS)
{
}
TokenFilter::~TokenFilter()
{
    if (deleteTokenStream && input != NULL) { input->close(); _CLLDELETE(input); }
    //close(); -- ISH 04/11/09
}

// Close the input TokenStream.
void TokenFilter::close()
{
    if (input != NULL)
    {
        input->close();
        //if ( deleteTokenStream ) _CLDELETE( input ); -- ISH 04/11/09
    }
    //input = NULL; -- ISH 04/11/09
}



Tokenizer::Tokenizer() : input(NULL)
{
}

Tokenizer::Tokenizer(CL_NS(util)::Reader* _input) :
    input(_input)
{
}

void Tokenizer::close()
{
    if (input != NULL)
    {
        // ? delete input;
        input = NULL;
    }
}

void Tokenizer::reset(CL_NS(util)::Reader* _input)
{
    // ? delete input;
    this->input = _input;
}

Tokenizer::~Tokenizer()
{
    close();
}


int32_t Analyzer::getPositionIncrementGap(const wchar_t* /*fieldName*/)
{
    return 0;
}

CL_NS_END
