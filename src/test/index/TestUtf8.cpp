/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team

* Updated by https://github.com/farfella/.
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "test.h"
#include "CLucene/util/dirent.h"
#include "CLucene/util/CLStreams.h"

CL_NS_USE(util)

#ifdef _UCS2
  void _Index(CuTest *tc, IndexWriter* ndx, const wchar_t * file){
    wchar_t path[CL_MAX_PATH];
    wchar_t tlang[20];

	  wcscpy(path,clucene_data_location);
	  wcscat(path,L"/utf8text");
	  CuAssert(tc,_T("Utf8 directory does not exist"),Misc::dir_Exists(path));
	  wcscat(path,L"/");
	  wcscat(path,file);
	  wcscat(path,L"_utf8.txt");
	  CuAssert(tc,_T("Language file does not exist"),Misc::dir_Exists(path));
	
	  wcscpy(tlang,file);

    Document doc;
	  doc.add(* _CLNEW Field(_T("language"),tlang,Field::STORE_YES | Field::INDEX_UNTOKENIZED));

	  doc.add(* _CLNEW Field(_T("contents"),_CLNEW FileReader(path, L"UTF-8",1024), Field::INDEX_TOKENIZED));

	  ndx->addDocument(&doc);
  }
  void _Search(CuTest *tc, IndexSearcher* srch, Analyzer* analyzer, const char* file, const char* query){
	  wchar_t tlang[20];
	  STRCPY_AtoT(tlang,file,CL_MAX_PATH);
  	
	  wchar_t tquery[80];
	  lucene_utf8towcs(tquery,query,80);

  	Query* q = QueryParser::parse(tquery,_T("contents"), analyzer);
  	Hits* h = srch->search(q);
  	CLUCENE_ASSERT( h->length() == 1 );
  	
    Document& doc = h->doc(0);
  	CLUCENE_ASSERT( wcscmp(doc.get(_T("language")), tlang)==0 );

    _CLDELETE(q);
    _CLDELETE(h);
  }

  void testUTF8(CuTest *tc) {
    RAMDirectory ram;
	  StandardAnalyzer a;
	  IndexWriter ndx(&ram,&a,true);
	  _Index(tc, &ndx,L"arabic");
	  _Index(tc, &ndx,L"chinese");
	  _Index(tc, &ndx,L"czech");
	  _Index(tc, &ndx,L"english");
	  _Index(tc, &ndx,L"french");
	  _Index(tc, &ndx,L"german");
	  _Index(tc, &ndx,L"greek");
	  _Index(tc, &ndx,L"hebrew");
	  _Index(tc, &ndx,L"japanese");
	  _Index(tc, &ndx,L"korean");
	  _Index(tc, &ndx,L"polish");
	  _Index(tc, &ndx,L"russian");
    ndx.close();

    IndexSearcher srch(&ram);
	  _Search(tc,&srch,&a,"arabic", "\xef\xbb\x9e\xef\xbb\xb4\xef\xbb\xa4\xef\xbb\xb3\xef\xba\xad"); //????? - arabic
	  _Search(tc,&srch,&a,"chinese", "\xe5\x95\xa4\xe9\x85\x92"); //?? - chinese
	  _Search(tc,&srch,&a,"czech", "Bud\xc4\x9bjovick\xc3\xbd" ); //Budejovick� - czech
  	
	  _Search(tc,&srch,&a,"english", "google"); //English - google
	  _Search(tc,&srch,&a,"french", "r\xc3\xa9put\xc3\xa9"); //r�put� - french
	  _Search(tc,&srch,&a,"german", "k\xc3\xb6nnen"); //k�nnen - german
	  _Search(tc,&srch,&a,"greek", "\xcf\x83\xcf\x84\xce\xb5\xce\xaf\xce\xbb\xcf\x84\xce\xb5"); //ste??te - greek
	  _Search(tc,&srch,&a,"hebrew", "\xd7\x91\xd7\x90\xd7\xa8\xd7\xa6\xd7\x95\xd7\xaa" ); //?????? - hebrew
	  _Search(tc,&srch,&a,"japanese", "\xe8\xa6\x8b\xe5\xad\xa6" ); //?? - japanese
	  _Search(tc,&srch,&a,"korean", "\xea\xb8\x88" ); //? - korean
	  _Search(tc,&srch,&a,"polish", "sp\xc3\xb3\xc5\x82ka"); ;//sp�lka - polish
	  _Search(tc,&srch,&a,"russian", "\xd0\x92\xd0\xb5\xd0\xbb\xd0\xb8\xd0\xba\xd0\xb8\xd0\xb5\x20"); //??????? - russian

    srch.close();
  }

  void readBuffered(CuTest* tc, Reader& utf8, Reader& unicode, int readLen){
	const wchar_t* buf1;
	const wchar_t* buf2;
	int32_t s;
	size_t p, p1, p2;
	p = p1 = p2 = 0;
	while(true){
		s = utf8.read(buf1, 1, readLen);
		if ( s == -1 )
			break;
		p1+=s;

		s = unicode.read(buf2, 1, readLen);
		if (s == -1) 
			break;
		p2+=s;

		CLUCENE_ASSERT(p1==p2); //make sure both readers read the same amount. todo: i guess this is not strictly required...
		for ( int32_t i=0;i<s;i++ ){
			if ( buf1[i] != buf2[i] ){
				wchar_t buf[200];
				_snwprintf(buf,200,_T("testReader (read) match failed at position %d"),p+i);
				CuAssert(tc,buf,false);
			}
		}
		p+=s;
	}
  }

  void doReadChars(CuTest* tc, CL_NS(util)::Reader& utf8, CL_NS(util)::Reader& unicode){
	int p=0;
    int c1,c2;
	while ( true ){
		c1 = utf8.read();
		c2 = unicode.read();

		if ( c1 < 0 || c2 < 0 )
			break;
		if ( c1 != c2 ){
			wchar_t buf[200];
			_snwprintf(buf,200,_T("testReader (readChar) match failed at position %d"),p);
			CuAssert(tc,buf,false);
		}
		p++;
	}
	c2 = unicode.read(); //read unicode once more
	CLUCENE_ASSERT(c1==c2);
  }


 void testReader(CuTest *tc) {
	wchar_t utf8text[1024];
	wcscpy(utf8text, clucene_data_location);
	wcscat(utf8text, L"/utf8text/french_utf8.txt");
	CuAssert(tc,_T("french_utf8.txt does not exist"),Misc::dir_Exists(utf8text));

	wchar_t unicodetext[1024];
	wcscpy(unicodetext, clucene_data_location);
	wcscat(unicodetext, L"/french_unicode.bin");
	CuAssert(tc,_T("french_unicode.bin does not exist"),Misc::dir_Exists(unicodetext));

	FileReader utf8(utf8text,L"UTF-8");
	FileReader unicode(unicodetext, L"UCS-2LE");

	utf8.setMinBufSize(10000);
	unicode.setMinBufSize(10000);

	doReadChars(tc,utf8,unicode);
	//now try reading with read, instead of readChar

	utf8.reset(0);unicode.reset(0);
	readBuffered(tc,utf8,unicode,3); //test with small buffer
	
	utf8.reset(0);unicode.reset(0);
	readBuffered(tc,utf8,unicode,10); //test with word sized buffer
	
	utf8.reset(0);unicode.reset(0);
	readBuffered(tc,utf8,unicode,1024); //test with large buffer
 }
#endif
  
void testNotImplemented(CuTest *tc){
	CuNotImpl(tc,_T("UTF8 tests not implemented"));
}

CuSuite *testutf8(void)
{
	CuSuite *suite = CuSuiteNew(_T("CLucene UTF8 Test"));

	//todo: temporarily disabled until a solution is found
#ifdef _UCS2
    SUITE_ADD_TEST(suite, testReader);
    SUITE_ADD_TEST(suite, testUTF8);
#else
    SUITE_ADD_TEST(suite, testNotImplemented);
    SUITE_ADD_TEST(suite, testNotImplemented);
#endif
    return suite; 
}
// EOF

