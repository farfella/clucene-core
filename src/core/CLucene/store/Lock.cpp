/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team

* Updated by https://github.com/farfella/.
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "Lock.h"
#include "_Lock.h"
#include "CLucene/util/Misc.h"

#ifdef _CL_HAVE_IO_H
#include <io.h>
#endif
#ifdef _CL_HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef _CL_HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef _CL_HAVE_DIRECT_H
#include <direct.h>
#endif
#include <fcntl.h>


CL_NS_USE(util)
CL_NS_DEF(store)

LuceneLock::~LuceneLock()
{
}
bool LuceneLock::obtain(int64_t lockWaitTimeout)
{
    bool locked = obtain();
    if (lockWaitTimeout < 0 && lockWaitTimeout != LOCK_OBTAIN_WAIT_FOREVER)
    {
        _CLTHROWA(CL_ERR_IllegalArgument, "lockWaitTimeout should be LOCK_OBTAIN_WAIT_FOREVER or a non-negative number");
    }

    int64_t maxSleepCount = lockWaitTimeout / LOCK_POLL_INTERVAL;
    int64_t sleepCount = 0;

    while (!locked)
    {
        if (lockWaitTimeout != LOCK_OBTAIN_WAIT_FOREVER && sleepCount++ == maxSleepCount)
        {
            _CLTHROWA(CL_ERR_IO, "Lock obtain timed out");
        }
        Misc::Sleep(LOCK_POLL_INTERVAL);
        locked = obtain();
    }

    return locked;
}
std::wstring NoLock::toString()
{
    return L"NoLock";
}
bool NoLock::obtain() { return true; }
void NoLock::release() {}
bool NoLock::isLocked() { return false; }

const std::wstring NoLock::getClassName()
{
    return L"NoLock";
}
const std::wstring NoLock::getObjectName() const
{
    return getClassName();
}



SingleInstanceLock::SingleInstanceLock(LocksType* locks, _LUCENE_THREADMUTEX* locks_LOCK, const wchar_t* lockName)
{
    this->locks = locks;
#ifndef _CL_DISABLE_MULTITHREADING
    this->locks_LOCK = locks_LOCK;
#endif
    this->lockName = lockName;
}

SingleInstanceLock::~SingleInstanceLock()
{
}

const std::wstring SingleInstanceLock::getClassName()
{
    return L"SingleInstanceLock";
}

const std::wstring SingleInstanceLock::getObjectName() const
{
    return getClassName();
}

bool SingleInstanceLock::obtain()
{
    SCOPED_LOCK_MUTEX(*locks_LOCK);
    return locks->insert(lockName).second;
}

void SingleInstanceLock::release()
{
    SCOPED_LOCK_MUTEX(*locks_LOCK);
    LocksType::iterator itr = locks->find(lockName);
    if (itr != locks->end())
    {
        locks->remove(itr, true);
    }
}

bool SingleInstanceLock::isLocked()
{
    SCOPED_LOCK_MUTEX(*locks_LOCK);
    return locks->find(lockName) == locks->end();
}

std::wstring SingleInstanceLock::toString()
{
    return std::wstring(L"SingleInstanceLock:") + lockName;
}


FSLock::FSLock(const wchar_t* _lockDir, const wchar_t* name, int filemode)
{
    if (filemode <= 0)
        this->filemode = 0644; //must do this or file will be created Read only
    else
        this->filemode = filemode;

    this->lockFile = _CL_NEWARRAY(wchar_t, CL_MAX_PATH);
    this->lockDir = _wcsdup(_lockDir);

    std::wstring lockFile = _lockDir + std::wstring(PATH_DELIMITERW) + name;

    this->lockFile = _wcsdup(lockFile.c_str());
}

FSLock::~FSLock()
{
    _CLDELETE_ARRAY(lockFile);
    _CLDELETE_LCaARRAY(lockDir);
}

const std::wstring FSLock::getClassName()
{
    return L"FSLock";
}
const std::wstring FSLock::getObjectName() const
{
    return getClassName();
}

bool FSLock::obtain()
{
    if (!Misc::dir_Exists(lockDir))
    {
        if (_wmkdir(lockDir) == -1)
        {
            const size_t sz = 34 + wcslen(lockDir) + 1;
            wchar_t* err = _CL_NEWARRAY(wchar_t, sz); //34: len of "Couldn't create lock directory: "
            wcscpy_s(err, sz, L"Couldn't create lock directory: ");
            wcscat_s(err, sz, lockDir);
            _CLTHROWA_DEL(CL_ERR_IO, err);
        }
    }
    int32_t r = _wopen(lockFile, O_RDWR | O_CREAT | _O_RANDOM | O_EXCL, this->filemode);
    if (r < 0)
    {
        return false;
    }
    else
    {
        _close(r);
        return true;
    }
}

void FSLock::release()
{
    _wunlink(lockFile);
}

bool FSLock::isLocked()
{
    return Misc::dir_Exists(lockFile);
}

std::wstring FSLock::toString()
{
    return std::wstring(L"SimpleFSLock@") + lockFile;
}

CL_NS_END
