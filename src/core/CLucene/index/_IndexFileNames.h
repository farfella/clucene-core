
#pragma once

#include "CLucene/clucene-config.h"

#include "CLucene/util/Array.h"

CL_NS_DEF(index)

class CLUCENE_EXPORT IndexFileNames {
	static CL_NS(util)::ConstValueArray<const wchar_t*> _INDEX_EXTENSIONS;
	static CL_NS(util)::ConstValueArray<const wchar_t*> _INDEX_EXTENSIONS_IN_COMPOUND_FILE;
	static CL_NS(util)::ConstValueArray<const wchar_t*> _STORE_INDEX_EXTENSIONS;
	static CL_NS(util)::ConstValueArray<const wchar_t*> _NON_STORE_INDEX_EXTENSIONS;
	static CL_NS(util)::ConstValueArray<const wchar_t*> _COMPOUND_EXTENSIONS;
	static CL_NS(util)::ConstValueArray<const wchar_t*> _VECTOR_EXTENSIONS;
public:
	static const wchar_t* SEGMENTS;
	static const wchar_t* SEGMENTS_GEN;
	static const wchar_t* DELETABLE;
	static const wchar_t* NORMS_EXTENSION;
	static const wchar_t* FREQ_EXTENSION;
	static const wchar_t* PROX_EXTENSION;
	static const wchar_t* TERMS_EXTENSION;
	static const wchar_t* TERMS_INDEX_EXTENSION;
	static const wchar_t* FIELDS_INDEX_EXTENSION;
	static const wchar_t* FIELDS_EXTENSION;
	static const wchar_t* VECTORS_FIELDS_EXTENSION;
	static const wchar_t* VECTORS_DOCUMENTS_EXTENSION;
	static const wchar_t* VECTORS_INDEX_EXTENSION;
	static const wchar_t * COMPOUND_FILE_EXTENSION;
	static const wchar_t* COMPOUND_FILE_STORE_EXTENSION;
	static const wchar_t* DELETES_EXTENSION;
	static const wchar_t* FIELD_INFOS_EXTENSION;
	static const wchar_t* PLAIN_NORMS_EXTENSION;
	static const wchar_t* SEPARATE_NORMS_EXTENSION;
	static const wchar_t* GEN_EXTENSION;
	
	LUCENE_STATIC_CONSTANT(int32_t,COMPOUND_EXTENSIONS_LENGTH=7);
	LUCENE_STATIC_CONSTANT(int32_t,VECTOR_EXTENSIONS_LENGTH=3);
	LUCENE_STATIC_CONSTANT(int32_t,STORE_INDEX_EXTENSIONS_LENGTH=5);
	
	static CL_NS(util)::ConstValueArray<const wchar_t*>& INDEX_EXTENSIONS();
	static CL_NS(util)::ConstValueArray<const wchar_t*>& INDEX_EXTENSIONS_IN_COMPOUND_FILE();
	static CL_NS(util)::ConstValueArray<const wchar_t*>& STORE_INDEX_EXTENSIONS();
	static CL_NS(util)::ConstValueArray<const wchar_t *>& NON_STORE_INDEX_EXTENSIONS();
	static CL_NS(util)::ConstValueArray<const wchar_t*>& COMPOUND_EXTENSIONS();
	static CL_NS(util)::ConstValueArray<const wchar_t*>& VECTOR_EXTENSIONS();
	
    static std::wstring fileNameFromGeneration( const wchar_t* base, const wchar_t* extension, int64_t gen );
	static bool isDocStoreFile( const wchar_t* fileName );
	
};

CL_NS_END
