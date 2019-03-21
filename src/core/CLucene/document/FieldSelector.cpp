/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team

* Updated by https://github.com/farfella/.
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"

#include "FieldSelector.h"

CL_NS_USE(util)
CL_NS_DEF(document)

FieldSelector::~FieldSelector(){
}

LoadFirstFieldSelector::~LoadFirstFieldSelector(){
}

FieldSelector::FieldSelectorResult LoadFirstFieldSelector::accept(const wchar_t* /*fieldName*/) const{
	return LOAD_AND_BREAK;
}

MapFieldSelector::~MapFieldSelector(){
    _CLDELETE(fieldSelections);
}

MapFieldSelector::MapFieldSelector() :
  fieldSelections(_CLNEW FieldSelectionsType(true,false))
{
}

MapFieldSelector::MapFieldSelector(std::vector<const wchar_t*>& fields) :
  fieldSelections(_CLNEW FieldSelectionsType(true,false))
{
  std::vector<const wchar_t*>::iterator itr = fields.begin();
  while ( itr != fields.end() ){
    add(*itr);
    itr++;
  }
}

MapFieldSelector::MapFieldSelector(ArrayBase<const wchar_t*>& fields):
  fieldSelections(_CLNEW FieldSelectionsType(true,false))
{
  for ( size_t i=0;i<fields.length;i++ ){
    add(fields[i]);
  }
}

FieldSelector::FieldSelectorResult MapFieldSelector::accept(const wchar_t* field) const{
  FieldSelectionsType::iterator itr = fieldSelections->find((wchar_t*)field);
  if ( itr != fieldSelections->end() ){
      return itr->second;
  }
  return FieldSelector::NO_LOAD;
}

void MapFieldSelector::add(const wchar_t* field, FieldSelector::FieldSelectorResult action){
  fieldSelections->insert(fieldSelections->end(),std::pair<wchar_t*,FieldSelectorResult>(
    _wcsdup(field), action));
}


CL_NS_END
