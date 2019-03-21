/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team

* Updated by https://github.com/farfella/.
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"

#include "Directory.h"
#include "LockFactory.h"
#include "CLucene/util/Misc.h"

CL_NS_DEF(store)


Directory::Directory(){
  this->lockFactory = NULL;
}
Directory::~Directory(){
	if (lockFactory != NULL)
		_CLDELETE(lockFactory);
}

LuceneLock* Directory::makeLock(const wchar_t* name) {
	return lockFactory->makeLock( name );
}

void Directory::setLockFactory( LockFactory* lockFactory ) {
	this->lockFactory = lockFactory;
	lockFactory->setLockPrefix( getLockID().c_str() );
}

LockFactory* Directory::getLockFactory() {
	return lockFactory;
}

std::wstring Directory::getLockID() {
	return toString();
}

void Directory::clearLock(const wchar_t* name) {
	if ( lockFactory != NULL ) {
		lockFactory->clearLock( name );
	}
}

bool Directory::deleteFile(const wchar_t * name, const bool throwError){
	bool ret = doDeleteFile(name);
	if ( !ret && throwError ){
      wchar_t buffer[200];
      _snwprintf(buffer,200,L"couldn't delete %s",name);
      _CLTHROWT(CL_ERR_IO, buffer );
    }
    return ret;
}
IndexInput* Directory::openInput(const wchar_t * name, int32_t bufferSize){
	IndexInput* ret;
	CLuceneError err;
	if ( ! openInput(name, ret, err, bufferSize) )
		throw err;
	return ret;
}

wchar_t ** Directory::list() const{
	std::vector<std::wstring> names;

	list(&names);

	size_t size = names.size();
    wchar_t ** ret = _CL_NEWARRAY(wchar_t*,size+1);
    for ( size_t i=0;i<size;i++ )
      ret[i] = _wcsdup(names[i].c_str());
    ret[size]=NULL;
    return ret;
}
bool Directory::list(std::vector<std::wstring>& names) const{
  return list(&names);
}
CL_NS_END
