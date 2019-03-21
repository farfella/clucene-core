/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team

* Updated by https://github.com/farfella/.
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "_FieldInfos.h"
#include "_TermVector.h"
#include "CLucene/util/StringBuffer.h"
#include "CLucene/util/Array.h"

CL_NS_USE(util)
CL_NS_DEF(index)

ValueArray<int32_t> SegmentTermPositionVector::EMPTY_TERM_POS;

SegmentTermVector::SegmentTermVector(const wchar_t* _field,
    ArrayBase<wchar_t*>* _terms, ArrayBase<int32_t>* _termFreqs)
{
    this->field = _wcsdup(_field); // TODO: Try and avoid this dup (using intern'ing perhaps?)
    this->terms = _terms;
    this->termFreqs = _termFreqs;
}

SegmentTermVector::~SegmentTermVector()
{
    _CLDELETE(terms);
    _CLDELETE(termFreqs);
}
TermPositionVector* SegmentTermVector::__asTermPositionVector()
{
    return NULL;
}

const std::wstring SegmentTermVector::getField()
{
    return field;
}

std::wstring SegmentTermVector::toString() const
{
    std::wstring sb = L"{" + field + L": ";

    int32_t i = 0;
    while (terms && terms->values[i] != NULL)
    {
        if (i > 0)
            sb.append(L", ");
        sb.append(terms->values[i]);
        sb.push_back('/');

        sb.append(std::to_wstring((*termFreqs)[i]));
    }
    sb.push_back('}');
    return sb;
}

int32_t SegmentTermVector::size()
{
    if (terms == NULL)
        return 0;

    return terms->length;
}

const CL_NS(util)::ArrayBase<const wchar_t*>* SegmentTermVector::getTerms()
{
    return (CL_NS(util)::ArrayBase<const wchar_t*>*)terms;
}

const ArrayBase<int32_t>* SegmentTermVector::getTermFrequencies()
{
    return termFreqs;
}

int32_t SegmentTermVector::binarySearch(const ArrayBase<wchar_t*>& a, const wchar_t* key) const
{
    int32_t low = 0;
    int32_t hi = a.length - 1;
    int32_t mid = 0;
    while (low <= hi)
    {
        mid = (low + hi) >> 1;

        int32_t c = wcscmp(a[mid], key);
        if (c == 0)
            return mid;
        else if (c > 0)
            hi = mid - 1;
        else // This gets the insertion point right on the last loop.
            low = ++mid;
    }
    return -mid - 1;
}

int32_t SegmentTermVector::indexOf(const wchar_t* termText)
{
    if (terms == NULL)
        return -1;
    int32_t res = binarySearch(*terms, termText);
    return res >= 0 ? res : -1;
}

ArrayBase<int32_t>* SegmentTermVector::indexesOf(const CL_NS(util)::ArrayBase<wchar_t*>& termNumbers, const int32_t start, const int32_t len)
{
    // TODO: there must be a more efficient way of doing this.
    //       At least, we could advance the lower bound of the terms array
    //       as we find valid indexes. Also, it might be possible to leverage
    //       this even more by starting in the middle of the termNumbers array
    //       and thus dividing the terms array maybe in half with each found index.
    ArrayBase<int32_t>* ret = _CLNEW ValueArray<int32_t>(len);
    for (int32_t i = 0; i < len; ++i)
    {
        ret->values[i] = indexOf(termNumbers[start + i]);
    }
    return ret;
}


SegmentTermPositionVector::SegmentTermPositionVector(const wchar_t* field,
    ArrayBase<wchar_t*>* terms, ArrayBase<int32_t>* termFreqs,
    ArrayBase< ArrayBase<int32_t>* >* _positions,
    ArrayBase< ArrayBase<TermVectorOffsetInfo*>* >* _offsets)
    : SegmentTermVector(field, terms, termFreqs),
    positions(_positions),
    offsets(_offsets)
{
}
SegmentTermPositionVector::~SegmentTermPositionVector()
{
    _CLLDELETE(offsets);
    _CLLDELETE(positions);
}

ArrayBase<int32_t>* SegmentTermPositionVector::indexesOf(const ArrayBase<wchar_t*>& termNumbers, const int32_t start, const int32_t len)
{
    return SegmentTermVector::indexesOf(termNumbers, start, len);
}

TermPositionVector* SegmentTermPositionVector::__asTermPositionVector()
{
    return this;
}

const ArrayBase<TermVectorOffsetInfo*>* SegmentTermPositionVector::getOffsets(const size_t index)
{
    if (offsets == NULL)
        return NULL;
    if (index < offsets->length)
        return offsets->values[index];
    else
        return TermVectorOffsetInfo_EMPTY_OFFSET_INFO;
}

const ArrayBase<int32_t>* SegmentTermPositionVector::getTermPositions(const size_t index)
{
    if (positions == NULL)
        return NULL;

    if (index < positions->length)
        return positions->values[index];
    else
        return &EMPTY_TERM_POS;
}

CL_NS_END

