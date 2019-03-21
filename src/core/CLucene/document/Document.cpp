/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team

* Updated by https://github.com/farfella/.
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "Document.h"
#include "Field.h"
#include "CLucene/util/StringBuffer.h"
#include <assert.h>

CL_NS_USE(util)
CL_NS_DEF(document)


struct DocumentFieldEnumeration::Internal
{
    Document::FieldsType::iterator itr;
    Document::FieldsType::iterator end;
};
DocumentFieldEnumeration::DocumentFieldEnumeration(Document::FieldsType::iterator itr, Document::FieldsType::iterator end)
{
    this->_internal = new DocumentFieldEnumeration::Internal;
    this->_internal->itr = itr;
    this->_internal->end = end;
}

bool DocumentFieldEnumeration::hasMoreElements() const
{
    return _internal->itr != _internal->end;
}

Field* DocumentFieldEnumeration::nextElement()
{
    //Func - Return the next element in the enumeration
  //Pre  - true
  //Post - The next element is returned or NULL

    Field* result = NULL;
    if (_internal->itr != _internal->end)
    {
        result = *_internal->itr;
        _internal->itr++;
    }
    return result;
}

DocumentFieldEnumeration::~DocumentFieldEnumeration()
{
    //Func - Destructor
    //Pre  - true
    //Post - Instance has been destroyed
    delete _internal;
}
DocumentFieldEnumeration* Document::fields()
{
    return _CLNEW DocumentFieldEnumeration(_fields->begin(), _fields->end());
}

/** Constructs a new document with no fields-> */
Document::Document() :
    _fields(_CLNEW FieldsType(true))
{
    //Func - Constructor
    //Pre  - true
    //Post - Instance has been created
    boost = 1.0f;
}

Document::~Document()
{
    //Func - Destructor
    //Pre  - true
    //Post - Instance has been destroyed
    boost = 1.0f;
    _CLDELETE(_fields);
}

void Document::clear()
{
    _fields->clear();
}

void Document::add(Field& field)
{
    _fields->push_back(&field);
}

void Document::setBoost(const float_t boost)
{
    this->boost = boost;
}

float_t Document::getBoost() const
{
    return boost;
}


Field* Document::getField(const wchar_t* name) const
{
    CND_PRECONDITION(name != NULL, L"name is NULL");
    for (FieldsType::const_iterator itr = _fields->begin();
        itr != _fields->end(); itr++)
    {
        if (wcscmp((*itr)->name(), name) == 0)
            return *itr;
    }
    return NULL;
}
const wchar_t* Document::get(const wchar_t* field) const
{
    CND_PRECONDITION(field != NULL, L"field is NULL");
    Field* f = getField(field);
    if (f != NULL)
        return f->stringValue(); //this returns null it is a binary(reader)
    else
        return NULL;
}

const Document::FieldsType* Document::getFields() const
{
    return _fields;
}


std::wstring Document::toString() const
{
    std::wstring ret = L"Document<";
    for (FieldsType::const_iterator itr = _fields->begin();
        itr != _fields->end(); itr++)
    {
        std::wstring tmp = (*itr)->toString();
        if (ret.length() > 0)
            ret.append(L" ");
        ret.append(tmp);
    }

    ret.append(L">");
    return ret;
}



void Document::removeField(const wchar_t* name)
{
    CND_PRECONDITION(name != NULL, L"name is NULL");

    for (FieldsType::iterator itr = _fields->begin();
        itr != _fields->end(); itr++)
    {

        if (wcscmp((*itr)->name(), name) == 0)
        {
            _fields->remove(itr);
            return;
        }
    }
}

void Document::removeFields(const wchar_t* name)
{
    CND_PRECONDITION(name != NULL, L"name is NULL");
    bool flag = true;
    //TODO: make this more efficient
    while (flag)
    {
        for (FieldsType::iterator itr = _fields->begin();
            itr != _fields->end(); itr++)
        {
            if (wcscmp((*itr)->name(), name) == 0)
            {
                _fields->remove(itr);
                flag = false; //no modifications allowed on an iterator
                break;
            }
        }
        flag = !flag;
    }
}

wchar_t** Document::getValues(const wchar_t* name)
{
    int count = 0;
    for (FieldsType::iterator itr = _fields->begin();
        itr != _fields->end(); itr++)
    {
        if (wcscmp((*itr)->name(), name) == 0 && (*itr)->stringValue() != NULL)
            count++;
    }

    //todo: there must be a better way of doing this, we are doing two iterations of the fields
    wchar_t** ret = NULL;
    if (count > 0)
    {
        ret = _CL_NEWARRAY(wchar_t*, count + 1);
        int32_t i = 0;
        for (FieldsType::iterator itr = _fields->begin();
            itr != _fields->end(); itr++)
        {

            if (wcscmp((*itr)->name(), name) == 0 && (*itr)->stringValue() != NULL)
            {
                ret[i] = _wcsdup((*itr)->stringValue());
                i++;
            }
        }
        ret[count] = NULL;
    }
    return ret;
}
CL_NS_END
