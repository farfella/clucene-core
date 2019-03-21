/*------------------------------------------------------------------------------
 * Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team

* Updated by https://github.com/farfella/.
 * 
 * Distributable under the terms of either the Apache License (Version 2.0) or 
 * the GNU Lesser General Public License, as specified in the COPYING file.
 ------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "CLucene/search/SearchHeader.h"
#include "CLucene/search/Similarity.h"
#include "CLucene/search/Explanation.h"
#include "CLucene/util/StringBuffer.h"

#include "SpanScorer.h"
#include "Spans.h"

CL_NS_DEF2(search, spans)

SpanScorer::SpanScorer( Spans * spans, Weight * weight, Similarity * similarity, uint8_t* norms ) :
Scorer( similarity ), firstTime( true ), more( true )
{
    this->spans = spans;
    this->norms = norms;
    this->weight = weight;
    this->value = weight->getValue();
    doc_ = -1;
}

SpanScorer::~SpanScorer()
{
    _CLLDELETE( spans );
}

bool SpanScorer::next()
{
    if( firstTime )
    {
        more = spans->next();
        firstTime = false;
    }
    return setFreqCurrentDoc();
}

bool SpanScorer::skipTo( int32_t target )
{
    if( firstTime ) 
    {
        more = spans->skipTo( target );
        firstTime = false;
    }

    if( ! more )
        return false;

    if( spans->doc() < target ) 
    { 
        // setFreqCurrentDoc() leaves spans.doc() ahead
        more = spans->skipTo( target );
    }

    return setFreqCurrentDoc();
}

bool SpanScorer::setFreqCurrentDoc()
{
    if( ! more )
        return false;

    doc_ = spans->doc();
    freq = 0.0f;
    while( more && doc_ == spans->doc() )
    {
        int32_t matchLength = spans->end() - spans->start();
        freq += getSimilarity()->sloppyFreq( matchLength );
        more = spans->next();
    }

    return more || ( freq != 0 );
}

int32_t SpanScorer::doc() const
{
    return doc_;
}

float_t SpanScorer::score()
{
    float_t raw = getSimilarity()->tf( freq ) * value; // raw score
    return raw * Similarity::decodeNorm( norms[ doc_ ]); // normalize
}

CL_NS(search)::Explanation * SpanScorer::explain( int32_t docIn )
{
    Explanation * tfExplanation = _CLNEW Explanation();

    skipTo( docIn );
    float_t phraseFreq = (doc() == docIn ) ? freq : 0.0f;
    tfExplanation->setValue( getSimilarity()->tf( phraseFreq ));

    std::wstring strBuf = L"tf(phraseFreq=";
    strBuf.append( float_to_wstring(phraseFreq, 2) );
    strBuf.append( L")" );
    tfExplanation->setDescription( strBuf.c_str() );

    return tfExplanation;
}

std::wstring SpanScorer::toString()
{
	std::wstring buf = L"SpanScorer(";

	std::wstring tmp = weight->toString();
	buf.append( tmp );
	buf.append( L")" );

	return buf;
}

CL_NS_END2
