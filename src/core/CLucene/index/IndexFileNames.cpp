/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team

* Updated by https://github.com/farfella/.
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"

#include "_IndexFileNames.h"
#include "_SegmentInfos.h"
#include "CLucene/util/Misc.h"


CL_NS_DEF(index)

	const wchar_t* IndexFileNames::SEGMENTS = L"segments";
	const wchar_t* IndexFileNames::SEGMENTS_GEN = L"segments.gen";
	const wchar_t* IndexFileNames::DELETABLE = L"deletable";
	const wchar_t* IndexFileNames::NORMS_EXTENSION = L"nrm";
	const wchar_t* IndexFileNames::FREQ_EXTENSION = L"frq";
	const wchar_t* IndexFileNames::PROX_EXTENSION = L"prx";
	const wchar_t* IndexFileNames::TERMS_EXTENSION = L"tis";
	const wchar_t* IndexFileNames::TERMS_INDEX_EXTENSION = L"tii";
	const wchar_t* IndexFileNames::FIELDS_INDEX_EXTENSION = L"fdx";
	const wchar_t* IndexFileNames::FIELDS_EXTENSION = L"fdt";
	const wchar_t* IndexFileNames::VECTORS_FIELDS_EXTENSION = L"tvf";
	const wchar_t* IndexFileNames::VECTORS_DOCUMENTS_EXTENSION = L"tvd";
	const wchar_t* IndexFileNames::VECTORS_INDEX_EXTENSION = L"tvx";
	const wchar_t* IndexFileNames::COMPOUND_FILE_EXTENSION = L"cfs";
	const wchar_t* IndexFileNames::COMPOUND_FILE_STORE_EXTENSION = L"cfx";
	const wchar_t* IndexFileNames::DELETES_EXTENSION = L"del";
	const wchar_t* IndexFileNames::FIELD_INFOS_EXTENSION = L"fnm";
	const wchar_t* IndexFileNames::PLAIN_NORMS_EXTENSION = L"f";
	const wchar_t* IndexFileNames::SEPARATE_NORMS_EXTENSION = L"s";
	const wchar_t* IndexFileNames::GEN_EXTENSION = L"gen";
  
	const wchar_t* IndexFileNames_INDEX_EXTENSIONS_s[] =
		{
			IndexFileNames::COMPOUND_FILE_EXTENSION,
			IndexFileNames::FIELD_INFOS_EXTENSION,
			IndexFileNames::FIELDS_INDEX_EXTENSION,
			IndexFileNames::FIELDS_EXTENSION,
			IndexFileNames::TERMS_INDEX_EXTENSION,
			IndexFileNames::TERMS_EXTENSION,
			IndexFileNames::FREQ_EXTENSION,
			IndexFileNames::PROX_EXTENSION,
			IndexFileNames::DELETES_EXTENSION,
			IndexFileNames::VECTORS_INDEX_EXTENSION,
			IndexFileNames::VECTORS_DOCUMENTS_EXTENSION,
			IndexFileNames::VECTORS_FIELDS_EXTENSION,
			IndexFileNames::GEN_EXTENSION,
			IndexFileNames::NORMS_EXTENSION,
			IndexFileNames::COMPOUND_FILE_STORE_EXTENSION
		};
  
	CL_NS(util)::ConstValueArray<const wchar_t*> IndexFileNames::_INDEX_EXTENSIONS;
  CL_NS(util)::ConstValueArray<const wchar_t*>& IndexFileNames::INDEX_EXTENSIONS(){
    if ( _INDEX_EXTENSIONS.length == 0 ){
      _INDEX_EXTENSIONS.values = IndexFileNames_INDEX_EXTENSIONS_s;
      _INDEX_EXTENSIONS.length = 15;
    }
    return _INDEX_EXTENSIONS;
  }

	const wchar_t* IndexFileNames_INDEX_EXTENSIONS_IN_COMPOUND_FILE_s[] = {
		IndexFileNames::FIELD_INFOS_EXTENSION,
		IndexFileNames::FIELDS_INDEX_EXTENSION,
		IndexFileNames::FIELDS_EXTENSION,
		IndexFileNames::TERMS_INDEX_EXTENSION,
		IndexFileNames::TERMS_EXTENSION,
		IndexFileNames::FREQ_EXTENSION,
		IndexFileNames::PROX_EXTENSION,
		IndexFileNames::VECTORS_INDEX_EXTENSION,
		IndexFileNames::VECTORS_DOCUMENTS_EXTENSION,
		IndexFileNames::VECTORS_FIELDS_EXTENSION,
		IndexFileNames::NORMS_EXTENSION
	};
	CL_NS(util)::ConstValueArray<const wchar_t*> IndexFileNames::_INDEX_EXTENSIONS_IN_COMPOUND_FILE;
  CL_NS(util)::ConstValueArray<const wchar_t*>& IndexFileNames::INDEX_EXTENSIONS_IN_COMPOUND_FILE(){
    if ( _INDEX_EXTENSIONS_IN_COMPOUND_FILE.length == 0 ){
      _INDEX_EXTENSIONS_IN_COMPOUND_FILE.values = IndexFileNames_INDEX_EXTENSIONS_IN_COMPOUND_FILE_s;
      _INDEX_EXTENSIONS_IN_COMPOUND_FILE.length = 11;
    }
    return _INDEX_EXTENSIONS_IN_COMPOUND_FILE;
  }

	const wchar_t* IndexFileNames_STORE_INDEX_EXTENSIONS_s[] = {
		IndexFileNames::VECTORS_INDEX_EXTENSION,
		IndexFileNames::VECTORS_FIELDS_EXTENSION,
		IndexFileNames::VECTORS_DOCUMENTS_EXTENSION,
		IndexFileNames::FIELDS_INDEX_EXTENSION,
		IndexFileNames::FIELDS_EXTENSION
	};
	CL_NS(util)::ConstValueArray<const wchar_t*> IndexFileNames::_STORE_INDEX_EXTENSIONS;
  CL_NS(util)::ConstValueArray<const wchar_t*>& IndexFileNames::STORE_INDEX_EXTENSIONS(){
    if ( _STORE_INDEX_EXTENSIONS.length == 0 ){
      _STORE_INDEX_EXTENSIONS.values = IndexFileNames_STORE_INDEX_EXTENSIONS_s;
      _STORE_INDEX_EXTENSIONS.length = 5;
    }
    return _STORE_INDEX_EXTENSIONS;
  }
	
	const wchar_t* IndexFileNames_NON_STORE_INDEX_EXTENSIONS_s[] = {
		IndexFileNames::FIELD_INFOS_EXTENSION,
		IndexFileNames::FREQ_EXTENSION,
		IndexFileNames::PROX_EXTENSION,
		IndexFileNames::TERMS_EXTENSION,
		IndexFileNames::TERMS_INDEX_EXTENSION,
		IndexFileNames::NORMS_EXTENSION
	};
	CL_NS(util)::ConstValueArray<const wchar_t*> IndexFileNames::_NON_STORE_INDEX_EXTENSIONS;
  CL_NS(util)::ConstValueArray<const wchar_t*>& IndexFileNames::NON_STORE_INDEX_EXTENSIONS(){
    if ( _NON_STORE_INDEX_EXTENSIONS.length == 0 ){
      _NON_STORE_INDEX_EXTENSIONS.values = IndexFileNames_NON_STORE_INDEX_EXTENSIONS_s;
      _NON_STORE_INDEX_EXTENSIONS.length = 6;
    }
    return _NON_STORE_INDEX_EXTENSIONS;
  }

	const wchar_t* IndexFileNames_COMPOUND_EXTENSIONS_s[] = {
		IndexFileNames::FIELD_INFOS_EXTENSION,
		IndexFileNames::FREQ_EXTENSION,
		IndexFileNames::PROX_EXTENSION,
		IndexFileNames::FIELDS_INDEX_EXTENSION,
		IndexFileNames::FIELDS_EXTENSION,
		IndexFileNames::TERMS_INDEX_EXTENSION,
		IndexFileNames::TERMS_EXTENSION
	};
	CL_NS(util)::ConstValueArray<const wchar_t*> IndexFileNames::_COMPOUND_EXTENSIONS;
  CL_NS(util)::ConstValueArray<const wchar_t*>& IndexFileNames::COMPOUND_EXTENSIONS(){
    if ( _COMPOUND_EXTENSIONS.length == 0 ){
      _COMPOUND_EXTENSIONS.values = IndexFileNames_COMPOUND_EXTENSIONS_s;
      _COMPOUND_EXTENSIONS.length = 7;
    }
    return _COMPOUND_EXTENSIONS;
  }

	const wchar_t* IndexFileNames_VECTOR_EXTENSIONS_s[] = {
		IndexFileNames::VECTORS_INDEX_EXTENSION,
		IndexFileNames::VECTORS_DOCUMENTS_EXTENSION,
		IndexFileNames::VECTORS_FIELDS_EXTENSION
	};
	CL_NS(util)::ConstValueArray<const wchar_t*> IndexFileNames::_VECTOR_EXTENSIONS;
  CL_NS(util)::ConstValueArray<const wchar_t*>& IndexFileNames::VECTOR_EXTENSIONS(){
    if ( _VECTOR_EXTENSIONS.length == 0 ){
      _VECTOR_EXTENSIONS.values = IndexFileNames_VECTOR_EXTENSIONS_s;
      _VECTOR_EXTENSIONS.length = 3;
    }
    return _VECTOR_EXTENSIONS;
  }

	std::wstring IndexFileNames::fileNameFromGeneration( const wchar_t* base, const wchar_t* extension, int64_t gen ) {
		if ( gen == SegmentInfo::NO ) {
			return L"";
		} else if ( gen == SegmentInfo::WITHOUT_GEN ) {
			return std::wstring(base) + extension;
		} else {
      wchar_t buf[(sizeof(unsigned long) << 3) + 1];
      CL_NS(util)::Misc::longToBase( gen, 36, buf );
      return std::wstring(base) + L"_" + buf + extension;
		}
	}
	
	bool IndexFileNames::isDocStoreFile( const wchar_t* fileName ) {
		
		const wchar_t * p = wcschr( fileName, L'.' );
		
		if ( p != NULL && wcscmp( p+1, COMPOUND_FILE_STORE_EXTENSION ) == 0 ) {
			return true;
		}
		for ( int32_t i = 0; i < STORE_INDEX_EXTENSIONS().length; i++ ) {
			if ( p != NULL && wcscmp( p+1, STORE_INDEX_EXTENSIONS()[i] ) == 0 ) {
				return true;
			}
		}
		return false;
	}

CL_NS_END
