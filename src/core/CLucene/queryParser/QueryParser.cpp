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
#include "QueryParserConstants.h"
#include "QueryParserTokenManager.h"
#include "QueryParser.h"

#include "CLucene/analysis/AnalysisHeader.h"

#include "CLucene/search/SearchHeader.h"

#include "CLucene/search/Query.h"
#include "CLucene/search/TermQuery.h"
#include "CLucene/search/BooleanQuery.h"
#include "CLucene/search/FuzzyQuery.h"
#include "CLucene/search/PhraseQuery.h"
#include "CLucene/search/WildcardQuery.h"
#include "CLucene/search/PrefixQuery.h"
#include "CLucene/search/RangeQuery.h"
#include "CLucene/search/MatchAllDocsQuery.h"
#include "CLucene/search/MultiPhraseQuery.h"
#include "CLucene/search/ConstantScoreQuery.h"

#include "CLucene/document/DateField.h"
#include "CLucene/document/DateTools.h"

#include "CLucene/index/Term.h"
#include "QueryToken.h"

#include "CLucene/util/CLStreams.h"
#include "CLucene/util/StringBuffer.h"

CL_NS_USE(util)
CL_NS_USE(index)
CL_NS_USE(analysis)
CL_NS_USE(search)

CL_NS_DEF(queryParser)

const wchar_t* QueryParserConstants::tokenImage[] = {
    L"<EOF>",
    L"<_NUM_CHAR>",
    L"<_ESCAPED_CHAR>",
    L"<_TERM_START_CHAR>",
    L"<_TERM_CHAR>",
    L"<_WHITESPACE>",
    L"<token of kind 6>",
    L"<AND>",
    L"<OR>",
    L"<NOT>",
    L"\"+\"",
    L"\"-\"",
    L"\"(\"",
    L"\")\"",
    L"\":\"",
    L"\"*\"",
    L"\"^\"",
    L"<QUOTED>",
    L"<TERM>",
    L"<FUZZY_SLOP>",
    L"<PREFIXTERM>",
    L"<WILDTERM>",
    L"\"[\"",
    L"\"{\"",
    L"<NUMBER>",
    L"\"TO\"",
    L"\"]\"",
    L"<RANGEIN_QUOTED>",
    L"<RANGEIN_GOOP>",
    L"\"TO\"",
    L"\"}\"",
    L"<RANGEEX_QUOTED>",
    L"<RANGEEX_GOOP>"
};

const int32_t QueryParser::jj_la1_0[] = {0x180,0x180,0xe00,0xe00,0x1f69f80,0x48000,0x10000,0x1f69000,0x1348000,0x80000,0x80000,0x10000,0x18000000,0x2000000,0x18000000,0x10000,0x80000000,0x20000000,0x80000000,0x10000,0x80000,0x10000,0x1f68000};
const int32_t QueryParser::jj_la1_1[] = {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1,0x0,0x1,0x0,0x0,0x0,0x0};

struct QueryParser::JJCalls {
public:
    int32_t gen;
    QueryToken* first;
    int32_t arg;
    JJCalls* next;

    JJCalls();
    ~JJCalls();
};

QueryParser::QueryParser(const wchar_t* f, Analyzer* a) : _operator(OR_OPERATOR),
  lowercaseExpandedTerms(true),useOldRangeQuery(false),allowLeadingWildcard(false),enablePositionIncrements(false),
  analyzer(a),field(NULL),phraseSlop(0),fuzzyMinSim(FuzzyQuery::defaultMinSimilarity),
  fuzzyPrefixLength(FuzzyQuery::defaultPrefixLength),/*locale(NULL),*/
  dateResolution(CL_NS(document)::DateTools::NO_RESOLUTION),fieldToDateResolution(NULL),
  token_source(NULL),token(NULL),jj_nt(NULL),_firstToken(NULL),jj_ntk(-1),jj_scanpos(NULL),jj_lastpos(NULL),jj_la(0),
  lookingAhead(false),jj_gen(0),jj_2_rtns(NULL),jj_rescan(false),jj_gc(0),jj_expentries(NULL),jj_expentry(NULL),
  jj_kind(-1),jj_endpos(0)
{
  StringReader* rdr = _CLNEW StringReader(L"");
  _init(_CLNEW FastCharStream(rdr, true));

  if ( f )
    field = wcsdup(f);
}

void QueryParser::_deleteTokens(){
  QueryToken* t = _firstToken;
  while (true){
    if (_firstToken == NULL) break;
    t = _firstToken->next;
    _CLLDELETE(_firstToken);
    _firstToken = t;
  }
}

QueryParser::~QueryParser(){
  _CLLDELETE(fieldToDateResolution);
  _CLLDELETE(token_source);

  _deleteTokens();

  _CLLDELETE(jj_expentries);
  _CLLDELETE(jj_expentry);
  _CLLDELETE(jj_2_rtns);

  _CLDELETE_CARRAY(field);
}

CL_NS(search)::Query* QueryParser::parse(const wchar_t* q, const wchar_t* f, CL_NS(analysis)::Analyzer* a){
  QueryParser* qp = _CLNEW QueryParser(f, a);
  CL_NS(search)::Query* qry = qp->parse(q);
  _CLDELETE(qp);
  return qry;
}

Query* QueryParser::parse(const wchar_t* _query)
{
  StringReader* rdr = _CLNEW StringReader(_query);
  ReInit(_CLNEW FastCharStream(rdr, true));
  try {
    // TopLevelQuery is a Query followed by the end-of-input (EOF)
    Query* res = TopLevelQuery(field);
    return (res!=NULL) ? res : _CLNEW BooleanQuery();
  }
  catch (CLuceneError& e) {
    // rethrow to include the original query:
    if (e.number()==CL_ERR_Parse || e.number()==CL_ERR_TokenMgr) {
      wchar_t* _twhat = e.twhat();
      const size_t errLen = wcslen(_twhat) + wcslen(_query) + 20;  // make sure we have enough room for our error message
      wchar_t *err = _CL_NEWARRAY(wchar_t,errLen);
      cl_stprintf(err, errLen, L"Cannot parse '%s': %s", _query,_twhat);
      _CLTHROWT_DEL(CL_ERR_Parse, err);
    } else if (e.number()==CL_ERR_TooManyClauses) {
      const size_t errLen = wcslen(_query) + 25; // make sure we have enough room for our error message
      wchar_t *err = _CL_NEWARRAY(wchar_t,errLen);
      cl_stprintf(err, errLen, L"Cannot parse '%s': too many boolean clauses", _query);
      _CLTHROWT_DEL(CL_ERR_Parse, err);
    } else
      throw e;
  }
}

Analyzer* QueryParser::getAnalyzer() const {
  return analyzer;
}

const wchar_t* QueryParser::getField() const {
  return field;
}

float_t QueryParser::getFuzzyMinSim() const {
  return fuzzyMinSim;
}

void QueryParser::setFuzzyMinSim(const float_t _fuzzyMinSim) {
  fuzzyMinSim = _fuzzyMinSim;
}

int32_t QueryParser::getFuzzyPrefixLength() const {
  return fuzzyPrefixLength;
}

void QueryParser::setFuzzyPrefixLength(const int32_t _fuzzyPrefixLength) {
  fuzzyPrefixLength = _fuzzyPrefixLength;
}

void QueryParser::setPhraseSlop(const int32_t _phraseSlop) {
  phraseSlop = _phraseSlop;
}
int32_t QueryParser::getPhraseSlop() const {
  return phraseSlop;
}
void QueryParser::setAllowLeadingWildcard(const bool _allowLeadingWildcard) {
  allowLeadingWildcard = _allowLeadingWildcard;
}
bool QueryParser::getAllowLeadingWildcard() const {
  return allowLeadingWildcard;
}
void QueryParser::setEnablePositionIncrements(const bool _enable) {
  enablePositionIncrements = _enable;
}
bool QueryParser::getEnablePositionIncrements() const {
  return enablePositionIncrements;
}
void QueryParser::setDefaultOperator(Operator _op) {
  _operator = _op;
}
QueryParser::Operator QueryParser::getDefaultOperator() const {
  return _operator;
}
void QueryParser::setLowercaseExpandedTerms(const bool _lowercaseExpandedTerms) {
  lowercaseExpandedTerms = _lowercaseExpandedTerms;
}
bool QueryParser::getLowercaseExpandedTerms() const {
  return lowercaseExpandedTerms;
}
void QueryParser::setUseOldRangeQuery(const bool _useOldRangeQuery) {
  useOldRangeQuery = _useOldRangeQuery;
}
bool QueryParser::getUseOldRangeQuery() const {
  return useOldRangeQuery;
}
void QueryParser::setDateResolution(const CL_NS(document)::DateTools::Resolution _dateResolution) {
  dateResolution = _dateResolution;
}
void QueryParser::setDateResolution(const wchar_t* fieldName, const CL_NS(document)::DateTools::Resolution _dateResolution) {
  if (fieldName == NULL)
    _CLTHROWA(CL_ERR_IllegalArgument, "Field cannot be null.");

  if (fieldToDateResolution == NULL) {
    // lazily initialize HashMap
    fieldToDateResolution = _CLNEW FieldToDateResolutionType();
  }

  fieldToDateResolution->put(fieldName, _dateResolution);
}
CL_NS(document)::DateTools::Resolution QueryParser::getDateResolution(const wchar_t* fieldName) const {
  if (fieldName == NULL)
    _CLTHROWA(CL_ERR_IllegalArgument,"Field cannot be null.");

  if (fieldToDateResolution == NULL) {
    // no field specific date resolutions set; return default date resolution instead
    return dateResolution;
  }

  CL_NS(document)::DateTools::Resolution resolution = fieldToDateResolution->get(fieldName);
  if (resolution == CL_NS(document)::DateTools::NO_RESOLUTION) {
    // no date resolutions set for the given field; return default date resolution instead
    resolution = dateResolution;
  }

  return resolution;
}

void QueryParser::addClause(std::vector<BooleanClause*>& clauses, int32_t conj, int32_t mods, Query* q){
  bool required, prohibited;

  // If this term is introduced by AND, make the preceding term required,
  // unless it's already prohibited
  const uint32_t nPreviousClauses = clauses.size();
  if (nPreviousClauses > 0 && conj == CONJ_AND) {
    BooleanClause* c = clauses[nPreviousClauses-1];
    if (!c->isProhibited())
      c->setOccur(BooleanClause::MUST);
  }

  if (nPreviousClauses > 0 && _operator == AND_OPERATOR && conj == CONJ_OR) {
    // If this term is introduced by OR, make the preceding term optional,
    // unless it's prohibited (that means we leave -a OR b but +a OR b-->a OR b)
    // notice if the input is a OR b, first term is parsed as required; without
    // this modification a OR b would parsed as +a OR b
    BooleanClause* c = clauses[nPreviousClauses-1];
    if (!c->isProhibited())
      c->setOccur(BooleanClause::SHOULD);
  }

  // We might have been passed a null query; the term might have been
  // filtered away by the analyzer.
  if (q == NULL)
    return;

  if (_operator == OR_OPERATOR) {
    // We set REQUIRED if we're introduced by AND or +; PROHIBITED if
    // introduced by NOT or -; make sure not to set both.
    prohibited = (mods == MOD_NOT);
    required = (mods == MOD_REQ);
    if (conj == CONJ_AND && !prohibited) {
      required = true;
    }
  } else {
    // We set PROHIBITED if we're introduced by NOT or -; We set REQUIRED
    // if not PROHIBITED and not introduced by OR
    prohibited = (mods == MOD_NOT);
    required   = (!prohibited && conj != CONJ_OR);
  }
  if (required && !prohibited)
    clauses.push_back(_CLNEW BooleanClause(q,true, BooleanClause::MUST));
  else if (!required && !prohibited)
    clauses.push_back(_CLNEW BooleanClause(q,true, BooleanClause::SHOULD));
  else if (!required && prohibited)
    clauses.push_back(_CLNEW BooleanClause(q,true, BooleanClause::MUST_NOT));
  else {
    _CLTHROWA(CL_ERR_Runtime, "Clause cannot be both required and prohibited");
  }
}

Query* QueryParser::getFieldQuery(const wchar_t* _field, wchar_t* queryText) {
  // Use the analyzer to get all the tokens, and then build a TermQuery,
  // PhraseQuery, or nothing based on the term count

  StringReader reader(queryText);
  TokenStream* source = analyzer->tokenStream(_field, &reader);

  CLVector<CL_NS(analysis)::Token*, Deletor::Object<CL_NS(analysis)::Token> > v;
  CL_NS(analysis)::Token* t = NULL;
  int32_t positionCount = 0;
  bool severalTokensAtSamePosition = false;

  while (true) {
    t = _CLNEW Token();
    try {
      Token* _t = source->next(t);
      if (_t == NULL) _CLDELETE(t);
    }_CLCATCH_ERR(CL_ERR_IO, _CLLDELETE(source);_CLLDELETE(t);_CLDELETE_LCARRAY(queryText);,{
      t = NULL;
    });
    if (t == NULL)
      break;
    v.push_back(t);
    if (t->getPositionIncrement() != 0)
      positionCount += t->getPositionIncrement();
    else
      severalTokensAtSamePosition = true;
  }
  try {
    source->close();
  }
  _CLCATCH_ERR_CLEANUP(CL_ERR_IO, {_CLLDELETE(source);_CLLDELETE(t);_CLDELETE_LCARRAY(queryText);} ); /* cleanup */
  _CLLDELETE(source);

  if (v.size() == 0)
    return NULL;
  else if (v.size() == 1) {
    Term* tm = _CLNEW Term(_field, v.at(0)->termBuffer());
    Query* ret = _CLNEW TermQuery( tm );
    _CLDECDELETE(tm);
    return ret;
  } else {
    if (severalTokensAtSamePosition) {
      if (positionCount == 1) {
        // no phrase query:
        BooleanQuery* q = _CLNEW BooleanQuery(true);
        for(size_t i=0; i<v.size(); i++ ){
          Term* tm = _CLNEW Term(_field, v.at(i)->termBuffer());
          q->add(_CLNEW TermQuery(tm), true, BooleanClause::SHOULD);
          _CLDECDELETE(tm);
        }
        return q;
      }else {
		    MultiPhraseQuery* mpq = _CLNEW MultiPhraseQuery();
		    mpq->setSlop(phraseSlop);
		    CLArrayList<Term*> multiTerms;
		    int32_t position = -1;
		    for (size_t i = 0; i < v.size(); i++) {
			    t = v.at(i);
			    if (t->getPositionIncrement() > 0 && multiTerms.size() > 0) {
            ValueArray<Term*> termsArray(multiTerms.size());
            multiTerms.toArray(termsArray.values);
				    if (enablePositionIncrements) {
					    mpq->add(&termsArray,position);
				    } else {
					    mpq->add(&termsArray);
				    }
				    multiTerms.clear();
			    }
			    position += t->getPositionIncrement();
			    multiTerms.push_back(_CLNEW Term(field, t->termBuffer()));
		    }
        ValueArray<Term*> termsArray(multiTerms.size());
        multiTerms.toArray(termsArray.values);
		    if (enablePositionIncrements) {
			    mpq->add(&termsArray,position);
		    } else {
			    mpq->add(&termsArray);
		    }
		    return mpq;
      }
    }else {
      PhraseQuery* pq = _CLNEW PhraseQuery();
      pq->setSlop(phraseSlop);
      int32_t position = -1;

      for (size_t i = 0; i < v.size(); i++) {
        t = v.at(i);
        Term* tm = _CLNEW Term(_field, t->termBuffer());
        if (enablePositionIncrements) {
          position += t->getPositionIncrement();
          pq->add(tm,position);
        } else {
          pq->add(tm);
        }
        _CLDECDELETE(tm);
      }
      return pq;
    }
  }
}

Query* QueryParser::getFieldQuery(const wchar_t* _field, wchar_t* queryText, const int32_t slop) {
  Query* query = getFieldQuery(_field, queryText);

  if (query) {
	  if ( query->instanceOf(PhraseQuery::getClassName()) ) {
		  static_cast<PhraseQuery*>(query)->setSlop(slop);
	  } else if ( query->instanceOf(MultiPhraseQuery::getClassName()) ) {
		  static_cast<MultiPhraseQuery*>(query)->setSlop(slop);
	  }
  }
  return query;
}

Query* QueryParser::getRangeQuery(const wchar_t* _field, wchar_t* part1, wchar_t* part2, const bool inclusive)
{
  if (lowercaseExpandedTerms) {
    _tcslwr(part1);
    _tcslwr(part2);
  }

  wchar_t* _part1 = part1, *_part2 = part2; // just in case anything go wrong...
  try {
      /*DateFormat df = DateFormat.getDateInstance(DateFormat.SHORT, locale); // SHORT means completely numeric
      df.setLenient(true);
      Date d1 = df.parse(part1);
      Date d2 = df.parse(part2);
      */
      const int64_t d1 = CL_NS(document)::DateTools::stringToTime(part1);
      int64_t d2 = CL_NS(document)::DateTools::stringToTime(part2);
      if (inclusive) {
          // The user can only specify the date, not the time, so make sure
          // the time is set to the latest possible time of that date to really
          // include all documents:
          d2 = CL_NS(document)::DateTools::timeMakeInclusive(d2);
      }

      CL_NS(document)::DateTools::Resolution resolution = getDateResolution(_field);
      if (resolution == CL_NS(document)::DateTools::NO_RESOLUTION) {
          // no default or field specific date resolution has been set,
          // use deprecated DateField to maintain compatibilty with
          // pre-1.9 Lucene versions.
          _part1 = CL_NS(document)::DateField::timeToString(d1);
          _part2 = CL_NS(document)::DateField::timeToString(d2);
      } else {
          _part1 = CL_NS(document)::DateTools::timeToString(d1, resolution);
          _part2 = CL_NS(document)::DateTools::timeToString(d2, resolution);
      }
  }
  catch (...) { }

  if(useOldRangeQuery)
  {
      Term* t1 = _CLNEW Term(_field,part1);
      Term* t2 = _CLNEW Term(_field,part2);
      Query* ret = _CLNEW RangeQuery(t1, t2, inclusive);
      _CLDECDELETE(t1);
      _CLDECDELETE(t2);

      // Make sure to delete the date strings we allocated only if we indeed allocated them
      if (part1 != _part1) _CLDELETE_LCARRAY(_part1);
      if (part2 != _part2) _CLDELETE_LCARRAY(_part2);
      
      return ret;
  }
  else
  {
      Query* q = _CLNEW ConstantScoreRangeQuery(_field,part1,part2,inclusive,inclusive);
      
      // Make sure to delete the date strings we allocated only if we indeed allocated them
      if (part1 != _part1) _CLDELETE_LCARRAY(_part1);
      if (part2 != _part2) _CLDELETE_LCARRAY(_part2);
      
      return q;
  }
}

Query* QueryParser::getBooleanQuery(std::vector<CL_NS(search)::BooleanClause*>& clauses, bool disableCoord)
{
  if (clauses.size()==0) {
    return NULL; // all clause words were filtered away by the analyzer.
  }
  BooleanQuery* query = _CLNEW BooleanQuery(disableCoord);

  for (size_t i = 0; i < clauses.size(); i++) {
    query->add(clauses[i]);
  }
  return query;
}

Query* QueryParser::getWildcardQuery(const wchar_t* _field, wchar_t* termStr)
{
  if (wcscmp( L"*", _field) == 0) {
    if (wcscmp(L"*", termStr) == 0)
		return _CLNEW MatchAllDocsQuery();
  }
  if (!allowLeadingWildcard && (termStr[0]==L'*' || termStr[0]==L'?')){
    _CLDELETE_LCARRAY(termStr);
    _CLTHROWT(CL_ERR_Parse, L"'*' or '?' not allowed as first character in WildcardQuery");
  }
  if (lowercaseExpandedTerms) {
    _tcslwr(termStr);
  }

  Term* t = _CLNEW Term(_field, termStr);
  Query* q = _CLNEW WildcardQuery(t);
  _CLDECDELETE(t);

  return q;
}

Query* QueryParser::getPrefixQuery(const wchar_t* _field, wchar_t* _termStr)
{
  if (!allowLeadingWildcard && _termStr[0] == L'*'){
    _CLDELETE_LCARRAY(_termStr);
    _CLTHROWT(CL_ERR_Parse, L"'*' not allowed as first character in PrefixQuery");
  }
  if (lowercaseExpandedTerms) {
    _tcslwr(_termStr);
  }
  Term* t = _CLNEW Term(_field, _termStr);
  Query *q = _CLNEW PrefixQuery(t);
  _CLDECDELETE(t);
  return q;
}

Query* QueryParser::getFuzzyQuery(const wchar_t* _field, wchar_t* termStr, const float_t minSimilarity)
{
  if (lowercaseExpandedTerms) {
    _tcslwr(termStr);
  }

  Term* t = _CLNEW Term(_field, termStr);
  Query *q = _CLNEW FuzzyQuery(t, minSimilarity, fuzzyPrefixLength);
  _CLDECDELETE(t);
  return q;
}

wchar_t* QueryParser::discardEscapeChar(wchar_t* input, wchar_t* output) {
  // Create char array to hold unescaped char sequence
  const size_t inputLen = wcslen(input);
  bool outputOwned=false;
  if (output == NULL){
    // TODO: Perhaps we can re-use an inner buffer instead of creating new char arrays here and in several other places
    output = _CL_NEWARRAY(wchar_t, inputLen + 1);
    outputOwned=true;
  }

  // The length of the output can be less than the input
  // due to discarded escape chars. This variable holds
  // the actual length of the output
  int32_t length = 0;

  // We remember whether the last processed character was
  // an escape character
  bool laswchar_tWasEscapeChar = false;

  // The multiplier the current unicode digit must be multiplied with.
  // E. g. the first digit must be multiplied with 16^3, the second with 16^2...
  uint32_t codePointMultiplier = 0;

  // Used to calculate the codepoint of the escaped unicode character
  int32_t codePoint = 0;

  for (size_t i = 0; i < inputLen; i++) {
    wchar_t curChar = input[i];
    if (codePointMultiplier > 0) {
		try {
			codePoint += hexToInt(curChar) * codePointMultiplier;
		} catch (CLuceneError& e) {
			if (outputOwned)_CLDELETE_LCARRAY(output);
			throw e;
		}
      codePointMultiplier = codePointMultiplier >> 4;
      if (codePointMultiplier == 0) {
        output[length++] = (wchar_t)codePoint;
        codePoint = 0;
      }
    } else if (laswchar_tWasEscapeChar) {
      if (curChar == L'u') {
        // found an escaped unicode character
        codePointMultiplier = 16 * 16 * 16;
      } else {
        // this character was escaped
        output[length] = curChar;
        length++;
      }
      laswchar_tWasEscapeChar = false;
    } else {
      if (curChar == L'\\') {
        laswchar_tWasEscapeChar = true;
      } else {
        output[length] = curChar;
        length++;
      }
    }
  }

  if (codePointMultiplier > 0) {
    if (outputOwned)_CLDELETE_LCARRAY(output);
    _CLTHROWT(CL_ERR_Parse, L"Truncated unicode escape sequence.");
  }

  if (laswchar_tWasEscapeChar) {
    if (outputOwned)_CLDELETE_LCARRAY(output);
    _CLTHROWT(CL_ERR_Parse, L"Term can not end with escape character.");
  }

  output[length]=0;
  return output;
}

//static
int32_t QueryParser::hexToInt(wchar_t c) {
  if (L'0' <= c && c <= L'9') {
    return c - L'0';
  } else if (L'a' <= c && c <= L'f'){
    return c - L'a' + 10;
  } else if (L'A' <= c && c <= L'F') {
    return c - L'A' + 10;
  } else {
    wchar_t err[50];
    cl_stprintf(err,50, L"Non-hex character in unicode escape sequence: %c", c);
    _CLTHROWT(CL_ERR_Parse,err);
  }
}

//static
wchar_t* QueryParser::escape(const wchar_t* s) {
  size_t len = wcslen(s);
  // Create a StringBuffer object a bit longer from the length of the query (to prevent some reallocations),
  // and declare we are the owners of the buffer (to save on a copy)
  // TODO: 1. Test to see what is the optimal initial length
  //     2. Allow re-using the provided string buffer (argument s) instead of creating another one?
  StringBuffer sb(len+5);
  for (size_t i = 0; i < len; i++) {
    const wchar_t c = s[i];
    // These characters are part of the query syntax and must be escaped
    if (c == '\\' || c == '+' || c == '-' || c == '!' || c == '(' || c == ')' || c == ':'
      || c == '^' || c == '[' || c == ']' || c == '"' || c == '{' || c == '}' || c == '~'
      || c == '*' || c == '?' || c == '|' || c == '&') {
        sb.appendChar(L'\\');
    }
    sb.appendChar(c);
  }
  return sb.giveBuffer();
}

int32_t QueryParser::Conjunction() {
  int32_t ret = CONJ_NONE;
  switch ((jj_ntk==-1)?f_jj_ntk():jj_ntk)
  {
  case AND:
  case OR:
    switch ((jj_ntk==-1)?f_jj_ntk():jj_ntk)
    {
    case AND:
      jj_consume_token(AND);
      ret = CONJ_AND;
      break;
    case OR:
      jj_consume_token(OR);
      ret = CONJ_OR;
      break;
    default:
      jj_la1[0] = jj_gen;
      jj_consume_token(-1);
      _CLTHROWT(CL_ERR_Parse, L"");
    }
    break;
  default:
    jj_la1[1] = jj_gen;
  }
  return ret;
}

int32_t QueryParser::Modifiers() {
  int32_t ret = MOD_NONE;
  switch ((jj_ntk==-1)?f_jj_ntk():jj_ntk)
  {
  case NOT:
  case PLUS:
  case MINUS:
    switch ((jj_ntk==-1)?f_jj_ntk():jj_ntk)
    {
    case PLUS:
      jj_consume_token(PLUS);
      ret = MOD_REQ;
      break;
    case MINUS:
      jj_consume_token(MINUS);
      ret = MOD_NOT;
      break;
    case NOT:
      jj_consume_token(NOT);
      ret = MOD_NOT;
      break;
    default:
      jj_la1[2] = jj_gen;
      jj_consume_token(-1);
      _CLTHROWT(CL_ERR_Parse, L"");
    }
    break;
  default:
    jj_la1[3] = jj_gen;
  }
  return ret;
}

Query* QueryParser::TopLevelQuery(wchar_t* _field) {
  Query* q = NULL;;
  try {
    q = fQuery(_field);
	jj_consume_token(0);
  } catch (CLuceneError& e) {
	_CLLDELETE(q);
    throw e;
  }
  return q;
}

Query* QueryParser::fQuery(wchar_t* _field) {
  CLVector<CL_NS(search)::BooleanClause*, Deletor::Object<CL_NS(search)::BooleanClause> > clauses;
  Query *q, *firstQuery=NULL;
  int32_t conj, mods;
  mods = Modifiers();
  q = fClause(_field);
  addClause(clauses, CONJ_NONE, mods, q);
  if (mods == MOD_NONE)
    firstQuery=q;
  while (true) {
    switch ((jj_ntk==-1)?f_jj_ntk():jj_ntk)
    {
    case AND:
    case OR:
    case NOT:
    case PLUS:
    case MINUS:
    case LPAREN:
    case STAR:
    case QUOTED:
    case TERM:
    case PREFIXTERM:
    case WILDTERM:
    case RANGEIN_START:
    case RANGEEX_START:
    case NUMBER:
      break;
    default:
      jj_la1[4] = jj_gen;
      goto label_1_brk;
    }

    conj = Conjunction();
    mods = Modifiers();
    q = fClause(_field);
    addClause(clauses, conj, mods, q);
  }

label_1_brk:
  if (clauses.size() == 1 && firstQuery != NULL){
    clauses[0]->deleteQuery = false;
    return firstQuery;
  }else{
    clauses.setDoDelete(false);
    return getBooleanQuery(clauses);
  }
}

Query* QueryParser::fClause(wchar_t* _field) {
  Query* q=NULL;
  QueryToken *fieldToken=NULL, *boost=NULL;
  wchar_t* tmpField=NULL;
  if (jj_2_1(2)) {
    switch ((jj_ntk==-1)?f_jj_ntk():jj_ntk)
    {
    case TERM:
      {
        fieldToken = jj_consume_token(TERM);
        jj_consume_token(COLON);
        // make sure to delete _field only if it's not contained already by the QP
        tmpField=discardEscapeChar(fieldToken->image);
        break;
      }
    case STAR:
      jj_consume_token(STAR);
      jj_consume_token(COLON);
      tmpField=_CL_NEWARRAY(wchar_t,2);
      tmpField[0]=L'*';
	  tmpField[1]=0;
      break;
    default:
      jj_la1[5] = jj_gen;
      jj_consume_token(-1);
      _CLTHROWT(CL_ERR_Parse,L"");
    }
  }
  
  switch ((jj_ntk==-1)?f_jj_ntk():jj_ntk)
  {
  case STAR:
  case QUOTED:
  case TERM:
  case PREFIXTERM:
  case WILDTERM:
  case RANGEIN_START:
  case RANGEEX_START:
  case NUMBER:
    {
      q = fTerm( tmpField==NULL ? _field : tmpField );
      break;
    }
  case LPAREN:
    {
      jj_consume_token(LPAREN);
      q = fQuery( tmpField==NULL ? _field : tmpField );
      jj_consume_token(RPAREN);
      if (((jj_ntk==-1)?f_jj_ntk():jj_ntk) == CARAT)
      {
        jj_consume_token(CARAT);
        boost = jj_consume_token(NUMBER);
      }
      else
        jj_la1[6] = jj_gen;
      break;
    }
  default:
    {
      jj_la1[7] = jj_gen;
      jj_consume_token(-1);
      _CLDELETE_LCARRAY(tmpField);
      _CLTHROWT(CL_ERR_Parse, L"");
    }
  }
  _CLDELETE_LCARRAY(tmpField);
  if (q && boost != NULL) {
    float_t f = 1.0;
    try {
      f = wcstod(boost->image, NULL);
      q->setBoost(f);
    } catch (...) { /* ignore errors */ }
  }
  return q;
}

Query* QueryParser::fTerm(const wchar_t* _field) {
  QueryToken *term, *boost=NULL, *fuzzySlop=NULL, *goop1, *goop2;
  bool prefix = false;
  bool wildcard = false;
  bool fuzzy = false;
  //bool rangein = false;
  Query* q = NULL;
  switch ((jj_ntk==-1)?f_jj_ntk():jj_ntk)
  {
  case STAR:
  case TERM:
  case PREFIXTERM:
  case WILDTERM:
  case NUMBER:
    {
      switch ((jj_ntk==-1)?f_jj_ntk():jj_ntk)
      {
      case TERM:
        term = jj_consume_token(TERM);
        break;
      case STAR:
        term = jj_consume_token(STAR);
        wildcard=true;
        break;
      case PREFIXTERM:
        term = jj_consume_token(PREFIXTERM);
        prefix=true;
        break;
      case WILDTERM:
        term = jj_consume_token(WILDTERM);
        wildcard=true;
        break;
      case NUMBER:
        term = jj_consume_token(NUMBER);
        break;
      default:
        jj_la1[8] = jj_gen;
        jj_consume_token(-1);
        _CLTHROWT(CL_ERR_Parse, L"");
      }

      if (((jj_ntk==-1)?f_jj_ntk():jj_ntk) == FUZZY_SLOP)
      {
        fuzzySlop = jj_consume_token(FUZZY_SLOP);
        fuzzy=true;
      }
      else
        jj_la1[9] = jj_gen;

      if (((jj_ntk==-1)?f_jj_ntk():jj_ntk) == CARAT)
      {
        jj_consume_token(CARAT);
        boost = jj_consume_token(NUMBER);
        if (((jj_ntk==-1)?f_jj_ntk():jj_ntk) == FUZZY_SLOP)
        {
          fuzzySlop = jj_consume_token(FUZZY_SLOP);
          fuzzy=true;
        }
        else
          jj_la1[10] = jj_gen;
      }
      else
        jj_la1[11] = jj_gen;

      wchar_t* termImage=NULL;
      if (wildcard) {
        termImage=discardEscapeChar(term->image);
        q = getWildcardQuery(_field, termImage);
      } else if (prefix) {
        termImage = _wcsdup(term->image);
        size_t tiLen = wcslen(termImage);
        termImage[tiLen-1]=0;
        q = getPrefixQuery(_field,discardEscapeChar(termImage, termImage));
      } else if (fuzzy) {
        float_t fms = fuzzyMinSim;
        try {
          if (fuzzySlop->image[1] != 0)
            fms = wcstod(fuzzySlop->image + 1, NULL);
        } catch (...) { /* ignore exceptions */ }
        if(fms < 0.0f || fms > 1.0f){
          _CLTHROWT(CL_ERR_Parse, L"Minimum similarity for a FuzzyQuery has to be between 0.0f and 1.0f !");
        }
        termImage=discardEscapeChar(term->image);
        q = getFuzzyQuery(_field, termImage,fms);
      } else {
        termImage=discardEscapeChar(term->image);
        q = getFieldQuery(_field, termImage);
      }
      _CLDELETE_LCARRAY(termImage);
      break;
    }
  case RANGEIN_START:
    {
      jj_consume_token(RANGEIN_START);
      switch ((jj_ntk==-1)?f_jj_ntk():jj_ntk)
      {
      case RANGEIN_GOOP:
        goop1 = jj_consume_token(RANGEIN_GOOP);
        break;
      case RANGEIN_QUOTED:
        goop1 = jj_consume_token(RANGEIN_QUOTED);
        break;
      default:
        jj_la1[12] = jj_gen;
        jj_consume_token(-1);
        _CLTHROWT(CL_ERR_Parse, L"");
      }
      if (((jj_ntk==-1)?f_jj_ntk():jj_ntk) == RANGEIN_TO)
        jj_consume_token(RANGEIN_TO);
      else
        jj_la1[13] = jj_gen;

      switch ((jj_ntk==-1)?f_jj_ntk():jj_ntk)
      {
      case RANGEIN_GOOP:
        goop2 = jj_consume_token(RANGEIN_GOOP);
        break;
      case RANGEIN_QUOTED:
        goop2 = jj_consume_token(RANGEIN_QUOTED);
        break;
      default:
        jj_la1[14] = jj_gen;
        jj_consume_token(-1);
        _CLTHROWT(CL_ERR_Parse, L"");
      }
      jj_consume_token(RANGEIN_END);
      if (((jj_ntk==-1)?f_jj_ntk():jj_ntk) == CARAT)
      {
        jj_consume_token(CARAT);
        boost = jj_consume_token(NUMBER);
      }
      else
        jj_la1[15] = jj_gen;

	  // TODO: Allow analysis::Term to accept ownership on a wchar_t* and save on extra dup's
      if (goop1->kind == RANGEIN_QUOTED) {
        wcscpy(goop1->image, goop1->image+1);
		goop1->image[wcslen(goop1->image)-1]='\0';
      }
      if (goop2->kind == RANGEIN_QUOTED) {
        wcscpy(goop2->image, goop2->image+1);
		goop2->image[wcslen(goop2->image)-1]='\0';
      }
      wchar_t* t1 = discardEscapeChar(goop1->image);
      wchar_t* t2 = discardEscapeChar(goop2->image);
      q = getRangeQuery(_field, t1, t2, true);
      _CLDELETE_LCARRAY(t1);
      _CLDELETE_LCARRAY(t2);
      break;
    }
  case RANGEEX_START:
    {
      jj_consume_token(RANGEEX_START);
      switch ((jj_ntk==-1)?f_jj_ntk():jj_ntk)
      {
      case RANGEEX_GOOP:
        goop1 = jj_consume_token(RANGEEX_GOOP);
        break;
      case RANGEEX_QUOTED:
        goop1 = jj_consume_token(RANGEEX_QUOTED);
        break;
      default:
        jj_la1[16] = jj_gen;
        jj_consume_token(-1);
        _CLTHROWT(CL_ERR_Parse, L"");
      }

      if (((jj_ntk==-1)?f_jj_ntk():jj_ntk) == RANGEEX_TO)
        jj_consume_token(RANGEEX_TO);
      else
        jj_la1[17] = jj_gen;

      switch ((jj_ntk==-1)?f_jj_ntk():jj_ntk)
      {
      case RANGEEX_GOOP:
        goop2 = jj_consume_token(RANGEEX_GOOP);
        break;
      case RANGEEX_QUOTED:
        goop2 = jj_consume_token(RANGEEX_QUOTED);
        break;
      default:
        jj_la1[18] = jj_gen;
        jj_consume_token(-1);
        _CLTHROWT(CL_ERR_Parse, L"");
      }
      jj_consume_token(RANGEEX_END);
      if (((jj_ntk==-1)?f_jj_ntk():jj_ntk) == CARAT)
      {
        jj_consume_token(CARAT);
        boost = jj_consume_token(NUMBER);
      }
      else
        jj_la1[19] = jj_gen;

      if (goop1->kind == RANGEEX_QUOTED)
        goop1->image = goop1->image + 1;

      if (goop2->kind == RANGEEX_QUOTED)
        goop2->image = goop2->image + 1;

      wchar_t* t1 = discardEscapeChar(goop1->image);
      wchar_t* t2 = discardEscapeChar(goop2->image);
      q = getRangeQuery(_field, t1, t2, false);
      _CLDELETE_LCARRAY(t1);
      _CLDELETE_LCARRAY(t2);
      break;
    }
  case QUOTED:
    {
      term = jj_consume_token(QUOTED);
      if (((jj_ntk==-1)?f_jj_ntk():jj_ntk) == FUZZY_SLOP)
        fuzzySlop = jj_consume_token(FUZZY_SLOP);
      else
        jj_la1[20] = jj_gen;

      if (((jj_ntk==-1)?f_jj_ntk():jj_ntk) == CARAT)
      {
        jj_consume_token(CARAT);
        boost = jj_consume_token(NUMBER);
      }
      else
        jj_la1[21] = jj_gen;

      int32_t s = phraseSlop;

      if (fuzzySlop != NULL) {
        try {
          s = _wtoi(fuzzySlop->image + 1);
        }
        catch (...) { /* ignore exceptions */ }
      }
	  // TODO: Make sure this hack, save an extra dup, is legal and not harmful
	  const size_t st = wcslen(term->image);
	  term->image[st-1]='\0';
      wchar_t* tmp = discardEscapeChar(term->image+1);
      q = getFieldQuery(_field, tmp, s);
      _CLDELETE_LCARRAY(tmp);
      break;
    }
  default:
    {
      jj_la1[22] = jj_gen;
      jj_consume_token(-1);
      _CLTHROWT(CL_ERR_Parse, L"");
    }
  }
  if (boost != NULL) {
    float_t f = 1.0;
    try {
      f = wcstod(boost->image, NULL);
    }
    catch (...) {
      /* Should this be handled somehow? (defaults to "no boost", if
      * boost number is invalid)
      */
    }

    // avoid boosting null queries, such as those caused by stop words
    if (q != NULL) {
      q->setBoost(f);
    }
  }
  return q;
}

bool QueryParser::jj_2_1(const int32_t xla) {
  jj_la = xla; jj_lastpos = jj_scanpos = token;
  try { return !jj_3_1(); }
  catch(CLuceneError& e) {
    // TODO: Improve this handling
    if (e.number()==CL_ERR_Parse && wcscmp(e.twhat(), L"LookaheadSuccess")==0)
      return true;
    else
      throw e;
  }
  _CLFINALLY( jj_save(0, xla); );
}

bool QueryParser::jj_3R_2() {
  if (jj_scan_token(TERM)) return true;
  if (jj_scan_token(COLON)) return true;
  return false;
}

bool QueryParser::jj_3_1() {
  QueryToken* xsp = jj_scanpos;
  if (jj_3R_2()) {
    jj_scanpos = xsp;
    if (jj_3R_3()) return true;
  }
  return false;
}

bool QueryParser::jj_3R_3() {
  if (jj_scan_token(STAR)) return true;
  if (jj_scan_token(COLON)) return true;
  return false;
}

QueryParser::QueryParser(CharStream* stream):_operator(OR_OPERATOR),
  lowercaseExpandedTerms(true),useOldRangeQuery(false),allowLeadingWildcard(false),enablePositionIncrements(false),
  analyzer(NULL),field(NULL),phraseSlop(0),fuzzyMinSim(FuzzyQuery::defaultMinSimilarity),
  fuzzyPrefixLength(FuzzyQuery::defaultPrefixLength),/*locale(NULL),*/
  dateResolution(CL_NS(document)::DateTools::NO_RESOLUTION),fieldToDateResolution(NULL),
  token_source(NULL),token(NULL),jj_nt(NULL),_firstToken(NULL),jj_ntk(-1),jj_scanpos(NULL),jj_lastpos(NULL),jj_la(0),
  lookingAhead(false),jj_gen(0),jj_2_rtns(NULL),jj_rescan(false),jj_gc(0),jj_expentries(NULL),jj_expentry(NULL),
  jj_kind(-1),jj_endpos(0)
{
  _init(stream);
}

void QueryParser::_init(CharStream* stream){
  if (token_source == NULL)
    token_source = _CLNEW QueryParserTokenManager(stream);
  _firstToken = token = _CLNEW QueryToken();
  jj_ntk = -1;
  jj_gen = 0;
  for (int32_t i = 0; i < 23; i++) jj_la1[i] = -1;
  jj_2_rtns = new JJCalls();
}

QueryToken* QueryParser::jj_consume_token(const int32_t kind)
{
  QueryToken* oldToken = token;
  if (token->next != NULL)
    token = token->next;
  else
    token = token->next = token_source->getNextToken();
  jj_ntk = -1;
  if (token->kind == kind) {
    jj_gen++;
    if (++jj_gc > 100) {
      jj_gc = 0;
      JJCalls* c = jj_2_rtns;
      while (c != NULL) {
        if (c->gen < jj_gen) c->first = NULL;
        c = c->next;
      }
    }
    return token;
  }
  token = oldToken;
  jj_kind = kind;
  generateParseException();
  return NULL;
}

bool QueryParser::jj_scan_token(const int32_t kind) {
  if (jj_scanpos == jj_lastpos) {
    jj_la--;
    if (jj_scanpos->next == NULL) {
      jj_lastpos = jj_scanpos = jj_scanpos->next = token_source->getNextToken();
    } else {
      jj_lastpos = jj_scanpos = jj_scanpos->next;
    }
  } else {
    jj_scanpos = jj_scanpos->next;
  }
  if (jj_rescan) {
    int32_t i = 0; QueryToken* tok = token;
    while (tok != NULL && tok != jj_scanpos) { i++; tok = tok->next; }
    if (tok != NULL) jj_add_error_token(kind, i);
  }
  if (jj_scanpos->kind != kind) return true;
  if (jj_la == 0 && jj_scanpos == jj_lastpos) _CLTHROWT(CL_ERR_Parse, L"LookaheadSuccess");
  return false;
}

void QueryParser::ReInit(CharStream* stream) {
  token_source->ReInit(stream);
  delete jj_2_rtns;
  _deleteTokens();
  _init(NULL);
}

QueryParser::QueryParser(QueryParserTokenManager* tm):_operator(OR_OPERATOR),
  lowercaseExpandedTerms(true),useOldRangeQuery(false),allowLeadingWildcard(false),enablePositionIncrements(false),
  analyzer(NULL),field(NULL),phraseSlop(0),fuzzyMinSim(FuzzyQuery::defaultMinSimilarity),
  fuzzyPrefixLength(FuzzyQuery::defaultPrefixLength),/*locale(NULL),*/
  dateResolution(CL_NS(document)::DateTools::NO_RESOLUTION),fieldToDateResolution(NULL),
  token_source(NULL),token(NULL),jj_nt(NULL),_firstToken(NULL),jj_ntk(-1),jj_scanpos(NULL),jj_lastpos(NULL),jj_la(0),
  lookingAhead(false),jj_gen(0),jj_2_rtns(NULL),jj_rescan(false),jj_gc(0),jj_expentries(NULL),jj_expentry(NULL),
  jj_kind(-1),jj_endpos(0)
{
  ReInit(tm);
}

void QueryParser::ReInit(QueryParserTokenManager* tm){
  _CLLDELETE(token_source);
  token_source = tm;
  _deleteTokens();
  _firstToken = token = _CLNEW QueryToken();
  jj_ntk = -1;
  jj_gen = 0;
  for (int32_t i = 0; i < 23; i++) jj_la1[i] = -1;
  delete jj_2_rtns;
  jj_2_rtns = new JJCalls();
}

QueryToken* QueryParser::getNextToken() {
  if (token->next != NULL) token = token->next;
  else token = token->next = token_source->getNextToken();
  jj_ntk = -1;
  jj_gen++;
  return token;
}

QueryToken* QueryParser::getToken(int32_t index) {
  QueryToken* t = lookingAhead ? jj_scanpos : token;
  for (int32_t i = 0; i < index; i++) {
    if (t->next != NULL) t = t->next;
    else t = t->next = token_source->getNextToken();
  }
  return t;
}

int32_t QueryParser::f_jj_ntk() {
  if ((jj_nt=token->next) == NULL){
    token->next=token_source->getNextToken();
    jj_ntk = token->next->kind;
    return jj_ntk;
  }
  else{
    jj_ntk = jj_nt->kind;
    return jj_ntk;
  }
}

QueryParser::JJCalls::JJCalls():gen(0),first(NULL),arg(0),next(NULL)
{
}
QueryParser::JJCalls::~JJCalls(){
	_CLLDELETE(first);
	delete next;
}

void QueryParser::jj_add_error_token(const int32_t kind, int32_t pos) {
  if (pos >= 100) return;
  if (pos == jj_endpos + 1) {
    jj_lasttokens[jj_endpos++] = kind;
  } else if (jj_endpos != 0) {
    _CLLDELETE(jj_expentry);
    jj_expentry = _CLNEW ValueArray<int32_t>(jj_endpos);
    for (int32_t i = 0; i < jj_endpos; i++) {
      jj_expentry->values[i] = jj_lasttokens[i];
    }
    bool exists = false;
    if (!jj_expentries) jj_expentries = _CLNEW CL_NS(util)::CLVector<CL_NS(util)::ValueArray<int32_t>*, CL_NS(util)::Deletor::Object< CL_NS(util)::ValueArray<int32_t> > >();
    for (size_t i=0;i<jj_expentries->size();i++){
      const ValueArray<int32_t>* oldentry = jj_expentries->at(i);
      if (oldentry->length == jj_expentry->length) {
        exists = true;
        for (size_t i = 0; i < jj_expentry->length; i++) {
          if (oldentry->values[i] != jj_expentry->values[i]) {
            exists = false;
            break;
          }
        }
        if (exists) break;
      }
    }
    if (!exists) {jj_expentries->push_back(jj_expentry); jj_expentry=NULL;}
    if (pos != 0) jj_lasttokens[(jj_endpos = pos) - 1] = kind;
  }
}

void QueryParser::generateParseException() {
  // lazily create the vectors required for this operation
  if (!jj_expentries)
    jj_expentries = _CLNEW CL_NS(util)::CLVector<CL_NS(util)::ValueArray<int32_t>*, CL_NS(util)::Deletor::Object< CL_NS(util)::ValueArray<int32_t> > >();
  else
    jj_expentries->clear();
  bool la1tokens[33];
  for (int32_t i = 0; i < 33; i++) {
    la1tokens[i] = false;
  }
  if (jj_kind >= 0) {
    la1tokens[jj_kind] = true;
    jj_kind = -1;
  }
  for (int32_t i = 0; i < 23; i++) {
    if (jj_la1[i] == jj_gen) {
      for (int32_t j = 0; j < 32; j++) {
        if ((jj_la1_0[i] & (1<<j)) != 0) {
          la1tokens[j] = true;
        }
        if ((jj_la1_1[i] & (1<<j)) != 0) {
          la1tokens[32+j] = true;
        }
      }
    }
  }

  _CLLDELETE(jj_expentry);
  for (int32_t i = 0; i < 33; i++) {
    if (la1tokens[i]) {
      jj_expentry = _CLNEW ValueArray<int32_t>(1);
      jj_expentry->values[0] = i;
      jj_expentries->push_back(jj_expentry);
      jj_expentry=NULL;
    }
  }
  jj_endpos = 0;
  jj_rescan_token();
  jj_add_error_token(0, 0);

  wchar_t* err = getParseExceptionMessage(token, jj_expentries, tokenImage);
  _CLTHROWT_DEL(CL_ERR_Parse, err);
}

void QueryParser::jj_rescan_token() {
  jj_rescan = true;
  JJCalls* p = jj_2_rtns;
  do {
    if (p->gen > jj_gen) {
      jj_la = p->arg; jj_lastpos = jj_scanpos = p->first;
      jj_3_1();
    }
    p = p->next;
  } while (p != NULL);
  jj_rescan = false;
}

void QueryParser::jj_save(const int32_t /*index*/, int32_t xla) {
  JJCalls* p = jj_2_rtns;
  while (p->gen > jj_gen) {
    if (p->next == NULL) { p = p->next = new JJCalls(); break; }
    p = p->next;
  }
  p->gen = jj_gen + xla - jj_la;
  p->first = token;
  p->arg = xla;
}

wchar_t* QueryParserConstants::addEscapes(wchar_t* str) {
  const size_t len = wcslen(str);
  StringBuffer retval(len * 2);
  wchar_t ch;
  for (size_t i = 0; i < len; i++) {
    switch (str[i])
    {
    case 0 :
      continue;
    case L'\b':
      retval.append(L"\\b");
      continue;
    case L'\t':
      retval.append(L"\\t");
      continue;
    case L'\n':
      retval.append(L"\\n");
      continue;
    case L'\f':
      retval.append(L"\\f");
      continue;
    case L'\r':
      retval.append(L"\\r");
      continue;
    case L'"':
      retval.append(L"\\\"");
      continue;
    case L'\'':
      retval.append(L"\\'");
      continue;
    case L'\\':
      retval.append(L"\\\\");
      continue;
    default:
      if ((ch = str[i]) < 0x20 || ch > 0x7e) {
        wchar_t buf[4];
        _snwprintf(buf, 4, L"%012X", static_cast<unsigned int>(ch));
        retval.append(L"\\u");
        retval.append(buf);
      } else {
        retval.appendChar(ch);
      }
      continue;
    }
  }
  return retval.giveBuffer();
}

wchar_t* QueryParser::getParseExceptionMessage(QueryToken* currentToken,
                       CL_NS(util)::CLVector< CL_NS(util)::ValueArray<int32_t>*, CL_NS(util)::Deletor::Object< CL_NS(util)::ValueArray<int32_t> > >* expectedTokenSequences,
                       const wchar_t* tokenImage[])
{
  // TODO: Check to see what's a realistic initial value for the buffers here
  // TODO: Make eol configurable (will be useful for PrintStream implementation as well later)?
  const wchar_t* eol = L"\n";

  StringBuffer expected(CL_MAX_PATH);
  size_t maxSize = 0;
  for (size_t i = 0; i < expectedTokenSequences->size(); i++) {
    if (maxSize < expectedTokenSequences->at(i)->length) {
      maxSize = expectedTokenSequences->at(i)->length;
    }
    for (size_t j = 0; j < expectedTokenSequences->at(i)->length; j++) {
      expected.append(tokenImage[expectedTokenSequences->at(i)->values[j]]);
      expected.appendChar(L' ');
    }
    if (expectedTokenSequences->at(i)->values[expectedTokenSequences->at(i)->length - 1] != 0) {
      expected.append(L"...");
    }
    expected.append(eol);
    expected.append(L"    ");
  }

  StringBuffer retval(CL_MAX_PATH);
  retval.append(L"Encountered \"");
  QueryToken* tok = currentToken->next;
  for (size_t i = 0; i < maxSize; i++) {
    if (i != 0) retval.appendChar(' ');
    if (tok->kind == 0) {
      retval.append(tokenImage[0]);
      break;
    }
    if (tok->image){
      wchar_t* buf = addEscapes(tok->image);
      retval.append(buf);
      _CLDELETE_CARRAY(buf);
    }
    tok = tok->next;
  }
  retval.append(L"\" at line ");
  retval.appendInt(currentToken->next->beginLine);
  retval.append(L", column ");
  retval.appendInt(currentToken->next->beginColumn);
  retval.appendChar(L'.');
  retval.append(eol);
  if (expectedTokenSequences->size() == 1) {
    retval.append(L"Was expecting:");
    retval.append(eol);
    retval.append(L"    ");
  } else {
    retval.append(L"Was expecting one of:");
    retval.append(eol);
    retval.append(L"    ");
  }
  retval.append(expected.getBuffer());

  return retval.giveBuffer();
}

CL_NS_END
