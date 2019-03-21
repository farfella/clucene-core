/*------------------------------------------------------------------------------
 * Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team

* Updated by https://github.com/farfella/.
 *
 * Distributable under the terms of either the Apache License (Version 2.0) or
 * the GNU Lesser General Public License, as specified in the COPYING file.
 ------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "CLucene/search/Similarity.h"
#include "CLucene/util/StringBuffer.h"

#include "SpanNearQuery.h"
#include "_EmptySpans.h"
#include "_NearSpansOrdered.h"
#include "_NearSpansUnordered.h"

CL_NS_DEF2(search, spans)

SpanNearQuery::SpanNearQuery(const SpanNearQuery& clone) :
    SpanQuery(clone)
{
    this->clauses = _CL_NEWARRAY(SpanQuery *, clone.clausesCount);
    for (size_t i = 0; i < clone.clausesCount; i++)
        this->clauses[i] = (SpanQuery *) clone.clauses[i]->clone();

    this->clausesCount = clone.clausesCount;
    this->bDeleteClauses = true;

    this->slop = clone.slop;
    this->inOrder = clone.inOrder;

    setField(clone.field.c_str());
}

SpanNearQuery::~SpanNearQuery()
{
    if (bDeleteClauses)
    {
        for (size_t i = 0; i < clausesCount; i++)
            _CLLDELETE(clauses[i]);
    }

    clausesCount = 0;
    _CLDELETE_LARRAY(clauses);

}

CL_NS(search)::Query * SpanNearQuery::clone() const
{
    return _CLNEW SpanNearQuery(*this);
}

const std::wstring SpanNearQuery::getClassName()
{
    return L"SpanNearQuery";
}

const std::wstring SpanNearQuery::getObjectName() const
{
    return getClassName();
}

SpanQuery ** SpanNearQuery::getClauses() const
{
    return clauses;
}

size_t SpanNearQuery::getClausesCount() const
{
    return clausesCount;
}

void SpanNearQuery::setField(const wchar_t * field_)
{
    this->field = field_;
}

const wchar_t * SpanNearQuery::getField() const
{
    return field.c_str();
}

int32_t SpanNearQuery::getSlop() const
{
    return slop;
}

bool SpanNearQuery::isInOrder() const
{
    return inOrder;
}

void SpanNearQuery::extractTerms(CL_NS(search)::TermSet * terms) const
{
    for (size_t i = 0; i < clausesCount; i++)
        clauses[i]->extractTerms(terms);
}

CL_NS(search)::Query * SpanNearQuery::rewrite(CL_NS(index)::IndexReader * reader)
{
    SpanNearQuery * clone = NULL;

    for (size_t i = 0; i < clausesCount; i++)
    {
        SpanQuery * c = clauses[i];
        SpanQuery * query = (SpanQuery *) c->rewrite(reader);
        if (query != c)
        {                     // clause rewrote: must clone
            if (clone == NULL)
                clone = (SpanNearQuery *) this->clone();

            _CLLDELETE(clone->clauses[i]);
            clone->clauses[i] = query;
        }
    }
    if (clone)
        return clone;                        // some clauses rewrote
    else
        return this;                         // no clauses rewrote
}

std::wstring SpanNearQuery::toString(const wchar_t* field) const
{
    std::wstring buffer = L"spanNear([";
    for (size_t i = 0; i < clausesCount; i++)
    {
        if (i != 0)
            buffer.append(L", ");

        std::wstring tszClause = clauses[i]->toString(field);
        buffer.append(tszClause);
    }

    buffer.append(L"], ");
    buffer.append(std::to_wstring(slop));
    buffer.append(L", ");
    buffer.append(bool_to_wstring(inOrder));
    buffer.append(L")");
    buffer.append(boost_to_wstring(getBoost()));

    return buffer;
}

bool SpanNearQuery::equals(Query* other) const
{
    if (this == other) return true;
    if (other == NULL || !(other->instanceOf(SpanNearQuery::getClassName())))
        return false;

    SpanNearQuery * that = (SpanNearQuery *) other;
    if (inOrder != that->inOrder
        || slop != that->slop
        || getBoost() != that->getBoost()
        || 0 != wcscmp(field.c_str(), that->field.c_str()))     // CLucene: java version does not compare field names
    {
        return false;
    }

    if (clausesCount != that->clausesCount)
        return false;
    for (size_t i = 0; i < clausesCount; i++)
    {
        if (!clauses[i]->equals(that->clauses[i]))
            return false;
    }

    return true;
}

size_t SpanNearQuery::hashCode() const
{
    size_t result = 1;
    for (size_t i = 0; i < clausesCount; i++)
        result = 31 * result + clauses[i]->hashCode();

    // Mix bits before folding in things like boost, since it could cancel the
    // last element of clauses.  This particular mix also serves to
    // differentiate SpanNearQuery hash codes from others.
    result ^= (result << 14) | (result >> 19);  // reversible
    result += Similarity::floatToByte(getBoost());
    result += slop;
    result ^= (inOrder ? 0x99AFD3BD : 0);

    return result;
}

Spans * SpanNearQuery::getSpans(CL_NS(index)::IndexReader * reader)
{
    if (clausesCount == 0)
        return _CLNEW EmptySpans();               // CLucene: 0-clause case - different to java version, because java creates SpanOrQuery to call its function to create empty spans

    if (clausesCount == 1)                       // optimize 1-clause case
        return clauses[0]->getSpans(reader);

    return inOrder
        ? (Spans *) _CLNEW NearSpansOrdered(this, reader)
        : (Spans *) _CLNEW NearSpansUnordered(this, reader);
}

CL_NS_END2
