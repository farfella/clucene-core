/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team

* Updated by https://github.com/farfella/.
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/

#include "CLucene/_ApiHeader.h"
#include "LockFactory.h"
#include "_Lock.h"
#include "CLucene/util/Misc.h"

#ifdef _CL_HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef _CL_HAVE_UNISTD_H
#include <unistd.h>
#endif

CL_NS_USE(util)
CL_NS_DEF(store)


LockFactory::LockFactory()
{
}

LockFactory::~LockFactory()
{
}

void LockFactory::setLockPrefix(const wchar_t* lockPrefix)
{
    if (lockPrefix != NULL)
        this->lockPrefix = lockPrefix;
    else
        this->lockPrefix.clear();
}

const wchar_t* LockFactory::getLockPrefix()
{
    return lockPrefix.c_str();
}

SingleInstanceLockFactory::SingleInstanceLockFactory()
{
    locks = _CLNEW LocksType();
}

SingleInstanceLockFactory::~SingleInstanceLockFactory()
{
    _CLDELETE(locks);
}

LuceneLock* SingleInstanceLockFactory::makeLock(const wchar_t* lockName)
{
#ifdef _CL_DISABLE_MULTITHREADING
    return _CLNEW SingleInstanceLock(locks, NULL, lockName);
#else
    return _CLNEW SingleInstanceLock(locks, &locks_LOCK, lockName);
#endif
}

void SingleInstanceLockFactory::clearLock(const wchar_t* lockName)
{
    SCOPED_LOCK_MUTEX(locks_LOCK);
    LocksType::iterator itr = locks->find(lockName);
    if (itr != locks->end())
    {
        locks->remove(itr);
    }
}


NoLockFactory* NoLockFactory::singleton = NULL;
NoLock* NoLockFactory::singletonLock = NULL;

void NoLockFactory::_shutdown()
{
    _CLDELETE(NoLockFactory::singleton);
    _CLDELETE(NoLockFactory::singletonLock);
}

NoLockFactory* NoLockFactory::getNoLockFactory()
{
    if (singleton == NULL)
    {
        singleton = _CLNEW NoLockFactory();
    }
    return singleton;
}

LuceneLock* NoLockFactory::makeLock(const wchar_t* /*lockName*/)
{
    if (singletonLock == NULL)
    {
        singletonLock = _CLNEW NoLock();
    }
    return singletonLock;
}

void NoLockFactory::clearLock(const wchar_t* /*lockName*/)
{
}


FSLockFactory::FSLockFactory(const wchar_t* lockDir, int filemode)
{
    setLockDir(lockDir);
    if (filemode > 0)
        this->filemode = filemode;
    else
        this->filemode = 0644;
}

FSLockFactory::~FSLockFactory()
{
}

void FSLockFactory::setLockDir(const wchar_t* lockDir)
{
    this->lockDir = lockDir;
}

LuceneLock* FSLockFactory::makeLock(const wchar_t* lockName)
{
    wchar_t name[CL_MAX_DIR];

    if (!lockPrefix.empty())
    {
        swprintf_s(name, L"%s-%s", lockPrefix.c_str(), lockName);
    }
    else
    {
        wcscpy_s(name, lockName);
    }

    return _CLNEW FSLock(lockDir.c_str(), name, this->filemode);
}

void FSLockFactory::clearLock(const wchar_t* lockName)
{
    if (Misc::dir_Exists(lockDir.c_str()))
    {
        wchar_t name[CL_MAX_DIR];
        wchar_t path[CL_MAX_DIR];
        struct cl_stat_t buf;

        if (!lockPrefix.empty())
        {
            wcscpy_s(name, lockPrefix.c_str());
            wcscat_s(name, L"-");
            wcscat_s(name, lockName);
        }
        else
        {
            wcscpy_s(name, lockName);
        }

        _snwprintf(path, CL_MAX_DIR, L"%s/%s", lockDir.c_str(), name);

        int32_t ret = _wstati64(path, &buf);
        if (ret == 0 && !(buf.st_mode & S_IFDIR) && _wunlink(path) == -1)
        {
            _CLTHROWA(CL_ERR_IO, "Couldn't delete file"); // TODO: make richer error
        }
    }
}


CL_NS_END
