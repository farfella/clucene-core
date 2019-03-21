/*------------------------------------------------------------------------------
 * Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team

* Updated by https://github.com/farfella/.
 *
 * Distributable under the terms of either the Apache License (Version 2.0) or
 * the GNU Lesser General Public License, as specified in the COPYING file.
 ------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "CLucene/index/IndexReader.h"
#include "CLucene/search/Similarity.h"
#include "CLucene/search/Explanation.h"
#include "CLucene/search/Scorer.h"
#include "CLucene/util/StringBuffer.h"

#include "SpanWeight.h"
#include "SpanQuery.h"
#include "SpanScorer.h"

CL_NS_USE(util)
CL_NS_DEF2(search, spans)

SpanWeight::SpanWeight(SpanQuery * query, CL_NS(search)::Searcher * searcher)
{
    this->similarity = query->getSimilarity(searcher);
    this->query = query;

    terms = _CLNEW TermSet();
    query->extractTerms(terms);
    idf = similarity->idf(terms->begin(), terms->end(), searcher);
}

SpanWeight::~SpanWeight()
{
    for (TermSet::iterator iTerm = terms->begin(); iTerm != terms->end(); iTerm++)
        _CLLDECDELETE(*iTerm);

    _CLDELETE(terms);
}

CL_NS(search)::Query * SpanWeight::getQuery()
{
    return query;
}

float_t SpanWeight::getValue()
{
    return value;
}

float_t SpanWeight::sumOfSquaredWeights()
{
    queryWeight = idf * query->getBoost();         // compute query weight
    return queryWeight * queryWeight;             // square it
}

void SpanWeight::normalize(float_t norm)
{
    queryNorm = norm;
    queryWeight *= queryNorm;                     // normalize query weight
    value = queryWeight * idf;                    // idf for document
}

CL_NS(search)::Scorer * SpanWeight::scorer(CL_NS(index)::IndexReader* reader)
{
    return _CLNEW SpanScorer(query->getSpans(reader),
        this,
        similarity,
        reader->norms(query->getField()));
}

CL_NS(search)::Explanation * SpanWeight::explain(CL_NS(index)::IndexReader* reader, int32_t doc)
{
    ComplexExplanation * result = _CLNEW ComplexExplanation();
    std::wstring strBuf;

    const wchar_t * field = ((SpanQuery *) getQuery())->getField();
    std::wstring tQry = getQuery()->toString();
    std::wstring tQryF = getQuery()->toString(field);

    strBuf.append(L"weight(");
    strBuf.append(tQry);
    strBuf.append(L" in ");
    strBuf.append(std::to_wstring(doc));
    strBuf.append(L"), product of:");
    result->setDescription(strBuf.c_str());

    std::wstring docFreqs;
    for (TermSet::iterator itTerms = terms->begin(); itTerms != terms->end(); itTerms++)
    {
        CL_NS(index)::Term * term = (*itTerms);
        docFreqs.append(term->text());
        docFreqs.append(L"=");
        docFreqs.append(std::to_wstring(reader->docFreq(term)));
        if (itTerms != terms->end())
            docFreqs.append(L" ");
    }

    strBuf.clear();
    strBuf.append(L"idf(");
    strBuf.append(field);
    strBuf.append(L": ");
    strBuf.append(docFreqs);
    strBuf.append(L")");
    Explanation * idfExpl = _CLNEW Explanation(idf, strBuf.c_str());

    // explain query weight
    Explanation * queryExpl = _CLNEW Explanation();
    strBuf.clear();
    strBuf.append(L"queryWeight(");
    strBuf.append(tQry);
    strBuf.append(L"), product of:");
    queryExpl->setDescription(strBuf.c_str());

    if (getQuery()->getBoost() != 1.0f)
        queryExpl->addDetail(_CLNEW Explanation(getQuery()->getBoost(), L"boost"));

    queryExpl->addDetail(idfExpl);

    Explanation * queryNormExpl = _CLNEW Explanation(queryNorm, L"queryNorm");
    queryExpl->addDetail(queryNormExpl);
    queryExpl->setValue(getQuery()->getBoost() *
        idfExpl->getValue() *
        queryNormExpl->getValue());
    result->addDetail(queryExpl);

    // explain field weight
    ComplexExplanation * fieldExpl = _CLNEW ComplexExplanation();
    strBuf.clear();
    strBuf.append(L"fieldWeight(");
    strBuf.append(field);
    strBuf.append(L":");
    strBuf.append(tQryF);
    strBuf.append(L" in ");
    strBuf.append(std::to_wstring(doc));
    strBuf.append(L"), product of:");
    fieldExpl->setDescription(strBuf.c_str());

    Scorer * pScorer = scorer(reader);
    Explanation * tfExpl = pScorer->explain(doc);
    fieldExpl->addDetail(tfExpl);
    fieldExpl->addDetail(idfExpl->clone());

    Explanation * fieldNormExpl = _CLNEW Explanation();
    uint8_t * fieldNorms = reader->norms(field);
    float_t fieldNorm = fieldNorms != NULL ? Similarity::decodeNorm(fieldNorms[doc]) : 0.0f;
    fieldNormExpl->setValue(fieldNorm);
    strBuf.clear();
    strBuf.append(L"fieldNorm(field=");
    strBuf.append(field);
    strBuf.append(L", doc=");
    strBuf.append(std::to_wstring(doc));
    strBuf.append(L")");
    fieldNormExpl->setDescription(strBuf.c_str());
    fieldExpl->addDetail(fieldNormExpl);

    fieldExpl->setMatch(tfExpl->isMatch());
    fieldExpl->setValue(tfExpl->getValue() *
        idfExpl->getValue() *
        fieldNormExpl->getValue());

    _CLLDELETE(pScorer);


    if (queryExpl->getValue() == 1.0f)
    {
        _CLLDELETE(result);
        return fieldExpl;
    }
    else
    {
        result->addDetail(fieldExpl);
        result->setMatch(fieldExpl->getMatch());

        // combine them
        result->setValue(queryExpl->getValue() * fieldExpl->getValue());
        return result;
    }
}

CL_NS_END2
