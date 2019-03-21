/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team

* Updated by https://github.com/farfella/.
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#pragma once

#include "CLucene/LuceneThreads.h"

CL_CLASS_DEF(store, LuceneLock)
CL_CLASS_DEF(store, NoLock)

CL_NS_DEF(store)
class LocksType;

class CLUCENE_EXPORT LockFactory : LUCENE_BASE
{
protected:
    std::wstring lockPrefix;
public:

    LockFactory();
    virtual ~LockFactory();

    void setLockPrefix(const wchar_t* lockPrefix);
    const wchar_t* getLockPrefix();

    virtual LuceneLock* makeLock(const wchar_t * lockName) = 0;
    virtual void clearLock(const wchar_t* lockName) = 0;
};

class CLUCENE_EXPORT SingleInstanceLockFactory : public LockFactory
{
private:
    LocksType * locks;
    DEFINE_MUTEX(locks_LOCK)
public:
    SingleInstanceLockFactory();
    ~SingleInstanceLockFactory();

    LuceneLock* makeLock(const wchar_t* lockName);
    void clearLock(const wchar_t* lockName);
};

class CLUCENE_EXPORT NoLockFactory : public LockFactory
{
public:
    static NoLockFactory* singleton;
    static NoLock* singletonLock;

    static NoLockFactory* getNoLockFactory();
    LuceneLock* makeLock(const wchar_t* lockName);
    void clearLock(const wchar_t* lockName);

    /** called when lucene_shutdown is called */
    static CLUCENE_LOCAL void _shutdown();
};

class CLUCENE_EXPORT FSLockFactory : public LockFactory
{
private:
    std::wstring lockDir;
    int filemode;
public:
    /** Constructs a FS Lock factory. The default file mode is user writable */
    FSLockFactory(const wchar_t* lockDir = NULL, int filemode = -1);
    ~FSLockFactory();

    void setLockDir(const wchar_t* lockDir);

    LuceneLock* makeLock(const wchar_t * lockName);
    void clearLock(const wchar_t* lockName);

    static const std::wstring getClassName();
    const std::wstring getObjectName();
};

CL_NS_END
