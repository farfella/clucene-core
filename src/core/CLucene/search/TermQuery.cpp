/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* Updated by https://github.com/farfella/.
 Updated by https://github.com/farfella/.
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "TermQuery.h"

#include "SearchHeader.h"
#include "Scorer.h"
#include "CLucene/index/Term.h"
#include "Explanation.h"
#include "Similarity.h"
#include "Searchable.h"
#include "_TermScorer.h"
#include "CLucene/index/IndexReader.h"
#include "CLucene/util/StringBuffer.h"
#include "CLucene/index/Terms.h"

#include <assert.h>

CL_NS_USE(index)
CL_NS_DEF(search)



	class TermWeight: public Weight {
	private:
		Similarity* similarity; // ISH: was Searcher*, for no apparent reason
		float_t value;
		float_t idf;
		float_t queryNorm;
		float_t queryWeight;

		TermQuery* parentQuery;	// CLucene specific
		CL_NS(index)::Term* _term;

	public:
		TermWeight(Searcher* searcher, TermQuery* parentQuery, CL_NS(index)::Term* _term);
		virtual ~TermWeight();

		// return a *new* string describing this object
		wchar_t* toString();
		Query* getQuery() { return (Query*)parentQuery; }
		float_t getValue() { return value; }

		float_t sumOfSquaredWeights();
		void normalize(float_t queryNorm);
		Scorer* scorer(CL_NS(index)::IndexReader* reader);
		Explanation* explain(CL_NS(index)::IndexReader* reader, int32_t doc);
	};


	/** Constructs a query for the term <code>t</code>. */
	TermQuery::TermQuery(Term* t):
		term( _CL_POINTER(t) )
	{
	}
	TermQuery::TermQuery(const TermQuery& clone):
  		Query(clone){
		this->term=_CL_POINTER(clone.term);
	}
	TermQuery::~TermQuery(){
	    _CLLDECDELETE(term);
	}

	Query* TermQuery::clone() const{
		return _CLNEW TermQuery(*this);
	}

	const char* TermQuery::getClassName(){
		return "TermQuery";
	}
	const char* TermQuery::getObjectName() const{
		return getClassName();
	}
	size_t TermQuery::hashCode() const {
		return Similarity::floatToByte(getBoost()) ^ term->hashCode();
	}

	//added by search highlighter
	Term* TermQuery::getTerm(bool pointer) const
	{
		if ( pointer )
			return _CL_POINTER(term);
		else
			return term;
	}

	wchar_t* TermQuery::toString(const wchar_t* field) const{
		CL_NS(util)::StringBuffer buffer;
		if ( field==NULL || wcscmp(term->field(),field)!= 0 ) {
			buffer.append(term->field());
			buffer.append(L":");
		}
		buffer.append(term->text());
		if (getBoost() != 1.0f) {
			buffer.append(L"^");
			buffer.appendFloat( getBoost(),1 );
		}
		return buffer.toString();
	}

	bool TermQuery::equals(Query* other) const {
		if (!(other->instanceOf(TermQuery::getClassName())))
			return false;

		TermQuery* tq = (TermQuery*)other;
		return (this->getBoost() == tq->getBoost())
			&& this->term->equals(tq->term);
	}

   TermWeight::TermWeight(Searcher* _searcher, TermQuery* _parentQuery, Term* term):similarity(_searcher->getSimilarity()),
	   value(0), queryNorm(0),queryWeight(0), parentQuery(_parentQuery),_term(term)
   {
		   idf = similarity->idf(term, _searcher); // compute idf
   }

   TermWeight::~TermWeight(){
   }

   //
   wchar_t* TermWeight::toString() {
	   int32_t size=strlen(parentQuery->getObjectName()) + 10;
	   wchar_t* tmp = _CL_NEWARRAY(wchar_t, size);
	   _snwprintf(tmp,size, L"weight(%S)",parentQuery->getObjectName());
	   return tmp;
   }

	float_t TermWeight::sumOfSquaredWeights() {
		// legacy // idf = parentQuery->getSimilarity(searcher)->idf(_term, searcher); // compute idf
		queryWeight = idf * parentQuery->getBoost();             // compute query weight
		return queryWeight * queryWeight;           // square it
	}

	void TermWeight::normalize(float_t _queryNorm) {
		this->queryNorm = _queryNorm;
		queryWeight *= queryNorm;                   // normalize query weight
		value = queryWeight * idf;                  // idf for document
	}

	Scorer* TermWeight::scorer(IndexReader* reader) {
		TermDocs* termDocs = reader->termDocs(_term);

		if (termDocs == NULL)
			return NULL;

		return _CLNEW TermScorer(this, termDocs, similarity,
								reader->norms(_term->field()));
	}

	Explanation* TermWeight::explain(IndexReader* reader, int32_t doc){
		ComplexExplanation* result = _CLNEW ComplexExplanation();

		wchar_t buf[LUCENE_SEARCH_EXPLANATION_DESC_LEN];
        wchar_t* tmp;

        tmp = getQuery()->toString();
		_snwprintf(buf,LUCENE_SEARCH_EXPLANATION_DESC_LEN, L"weight(%s in %d), product of:",tmp,doc);
        _CLDELETE_LCARRAY(tmp);
		result->setDescription(buf);

		_snwprintf(buf,LUCENE_SEARCH_EXPLANATION_DESC_LEN, L"idf(docFreq=%d, numDocs=%d)", reader->docFreq(_term), reader->numDocs() );
		Explanation* idfExpl = _CLNEW Explanation(idf, buf);

		// explain query weight
		Explanation* queryExpl = _CLNEW Explanation();
        tmp = getQuery()->toString();
		_snwprintf(buf,LUCENE_SEARCH_EXPLANATION_DESC_LEN, L"queryWeight(%s), product of:", tmp);
        _CLDELETE_LCARRAY(tmp);
		queryExpl->setDescription(buf);

		Explanation* boostExpl = _CLNEW Explanation(parentQuery->getBoost(), L"boost");
		if (parentQuery->getBoost() != 1.0f)
			queryExpl->addDetail(boostExpl);
        else
            _CLDELETE(boostExpl);

		queryExpl->addDetail(idfExpl->clone());

		Explanation* queryNormExpl = _CLNEW Explanation(queryNorm, L"queryNorm");
		queryExpl->addDetail(queryNormExpl);

		queryExpl->setValue(parentQuery->getBoost()* // always 1.0 | TODO: original Java code is boostExpl.getValue()
							idfExpl->getValue() *
							queryNormExpl->getValue());
		result->addDetail(queryExpl);

		// explain field weight
		const wchar_t* field = _term->field();
		ComplexExplanation* fieldExpl = _CLNEW ComplexExplanation();

        tmp = _term->toString();
		_snwprintf(buf,LUCENE_SEARCH_EXPLANATION_DESC_LEN,
			L"fieldWeight(%s in %d), product of:",tmp,doc);
        _CLDELETE_LCARRAY(tmp);
		fieldExpl->setDescription(buf);

        Scorer* sc = scorer(reader);
		Explanation* tfExpl = sc->explain(doc);
        _CLLDELETE(sc);
		fieldExpl->addDetail(tfExpl);
		fieldExpl->addDetail(idfExpl);

		Explanation* fieldNormExpl = _CLNEW Explanation();
		uint8_t* fieldNorms = reader->norms(field);
		float_t fieldNorm =
			fieldNorms!=NULL ? Similarity::decodeNorm(fieldNorms[doc]) : 0.0f;
		fieldNormExpl->setValue(fieldNorm);

		_snwprintf(buf,LUCENE_SEARCH_EXPLANATION_DESC_LEN,
			L"fieldNorm(field=%s, doc=%d)",field,doc);
		fieldNormExpl->setDescription(buf);
		fieldExpl->addDetail(fieldNormExpl);

		fieldExpl->setMatch(tfExpl->isMatch());
		fieldExpl->setValue(tfExpl->getValue() *
							idfExpl->getValue() *
							fieldNormExpl->getValue());

        if (queryExpl->getValue() == 1.0f){
			_CLLDELETE(result);
            return fieldExpl;
        }

		// combine them
		result->setValue(queryExpl->getValue() * fieldExpl->getValue());

		result->addDetail(fieldExpl);
		result->setMatch(fieldExpl->getMatch());

		return result;
	}

	Weight* TermQuery::_createWeight(Searcher* _searcher) {
        return _CLNEW TermWeight(_searcher,this,term);
    }

    void TermQuery::extractTerms( TermSet * termset ) const
    {
        if( term && termset->end() == termset->find( term ))
            termset->insert( _CL_POINTER( term ));
    }


CL_NS_END

