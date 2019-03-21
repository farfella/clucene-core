/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team

* Updated by https://github.com/farfella/.
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"

#include "_SegmentInfos.h"
#include "_IndexFileNames.h"
#include "_SegmentHeader.h"
#include "MultiReader.h"
#include <assert.h>
#include <iostream>

#include "CLucene/store/Directory.h"
#include "CLucene/util/Misc.h"

CL_NS_USE(store)
CL_NS_USE(util)

CL_NS_DEF(index)

SegmentInfo::SegmentInfo(const wchar_t * _name, const int32_t _docCount, CL_NS(store)::Directory* _dir,
    bool _isCompoundFile, bool _hasSingleNormFile,
    int32_t _docStoreOffset, const wchar_t * _docStoreSegment, bool _docStoreIsCompoundFile)
    :
    docCount(_docCount),
    preLockless(false),
    delGen(SegmentInfo::NO),
    isCompoundFile(_isCompoundFile ? SegmentInfo::YES : SegmentInfo::NO),
    hasSingleNormFile(_hasSingleNormFile),
    _sizeInBytes(-1),
    docStoreOffset(_docStoreOffset),
    docStoreSegment(_docStoreSegment == NULL ? L"" : _docStoreSegment),
    docStoreIsCompoundFile(_docStoreIsCompoundFile)
{
    CND_PRECONDITION(docStoreOffset == -1 || !docStoreSegment.empty(), L"failed testing for (docStoreOffset == -1 || docStoreSegment != NULL)");

    this->name = _name;
    this->dir = _dir;
}

std::wstring SegmentInfo::segString(Directory* dir)
{
    std::wstring cfs;
    try
    {
        if (getUseCompoundFile())
            cfs = L"c";
        else
            cfs = L"C";
    }
    catch (CLuceneError& ioe)
    {
        if (ioe.number() != CL_ERR_IO) throw ioe;
        cfs = L"?";
    }

    std::wstring docStore;

    if (docStoreOffset != -1)
        docStore = std::wstring(L"->") + docStoreSegment;
    else
        docStore = L"";

    return std::wstring(name) + L":" +
        cfs +
        std::wstring(this->dir == dir ? L"" : L"x") + Misc::toString(docCount) + docStore;
}
SegmentInfo::SegmentInfo(CL_NS(store)::Directory* _dir, int32_t format, CL_NS(store)::IndexInput* input) :
    _sizeInBytes(-1)
{
    this->dir = _dir;

    {
        wchar_t aname[CL_MAX_PATH];
        input->readString(aname, CL_MAX_PATH);
        this->name = aname;
    }

    docCount = input->readInt();
    if (format <= SegmentInfos::FORMAT_LOCKLESS)
    {
        delGen = input->readLong();
        if (format <= SegmentInfos::FORMAT_SHARED_DOC_STORE)
        {
            docStoreOffset = input->readInt();
            if (docStoreOffset != -1)
            {
                wchar_t aname[CL_MAX_PATH];
                input->readString(aname, CL_MAX_PATH);
                docStoreSegment = aname;
                docStoreIsCompoundFile = (1 == input->readByte());
            }
            else
            {
                docStoreSegment = name;
                docStoreIsCompoundFile = false;
            }
        }
        else
        {
            docStoreOffset = -1;
            docStoreSegment = name;
            docStoreIsCompoundFile = false;
        }
        if (format <= SegmentInfos::FORMAT_SINGLE_NORM_FILE)
        {
            hasSingleNormFile = (1 == input->readByte());
        }
        else
        {
            hasSingleNormFile = false;
        }
        int32_t numNormGen = input->readInt();
        normGen.deleteValues();
        if (numNormGen == NO)
        {
            // normGen is already NULL, we'll just set normGenLen to 0
        }
        else
        {
            normGen.values = _CL_NEWARRAY(int64_t, numNormGen);
            normGen.length = numNormGen;
            for (int32_t j = 0; j < numNormGen; j++)
            {
                normGen.values[j] = input->readLong();
            }
        }
        isCompoundFile = input->readByte();
        preLockless = (isCompoundFile == CHECK_DIR);
    }
    else
    {
        delGen = CHECK_DIR;
        //normGen=NULL; normGenLen=0;
        isCompoundFile = CHECK_DIR;
        preLockless = true;
        hasSingleNormFile = false;
        docStoreOffset = -1;
        docStoreIsCompoundFile = false;
    }
}

void SegmentInfo::reset(const SegmentInfo* src)
{
    clearFiles();
    this->name = src->name;
    docCount = src->docCount;
    dir = src->dir;
    preLockless = src->preLockless;
    delGen = src->delGen;
    docStoreOffset = src->docStoreOffset;
    docStoreIsCompoundFile = src->docStoreIsCompoundFile;
    if (src->normGen.values == NULL)
    {
        this->normGen.deleteValues();
    }
    else
    {
        // optimized case to allocate new array only if current memory buffer is too small
        if (this->normGen.length < src->normGen.length)
        {
            normGen.resize(src->normGen.length);
        }
        else
        {
            this->normGen.length = src->normGen.length;
        }
        memcpy(this->normGen.values, src->normGen.values, sizeof(int64_t) * this->normGen.length);
    }
    isCompoundFile = src->isCompoundFile;
    hasSingleNormFile = src->hasSingleNormFile;
}

SegmentInfo::~SegmentInfo()
{
    normGen.deleteValues();
}

void SegmentInfo::setNumFields(const int32_t numFields)
{
    if (normGen.values == NULL)
    {
        // normGen is null if we loaded a pre-2.1 segment
        // file, or, if this segments file hasn't had any
        // norms set against it yet:
        normGen.resize(numFields);

        if (preLockless)
        {
            // Do nothing: thus leaving normGen[k]==CHECK_DIR (==0), so that later we know
            // we have to check filesystem for norm files, because this is prelockless.

        }
        else
        {
            // This is a FORMAT_LOCKLESS segment, which means
            // there are no separate norms:
            for (int32_t i = 0; i < numFields; i++)
            {
                normGen.values[i] = NO;
            }
        }
    }
}
/** Returns total size in bytes of all of files used by
*  this segment. */
int64_t SegmentInfo::sizeInBytes()
{
    if (_sizeInBytes == -1)
    {
        const std::vector<std::wstring>& __files = files();
        size_t size = __files.size();
        _sizeInBytes = 0;
        for (size_t i = 0; i < size; i++)
        {
            const wchar_t * fileName = __files[i].c_str();
            // We don't count bytes used by a shared doc store
            // against this segment:
            if (docStoreOffset == -1 || !IndexFileNames::isDocStoreFile(fileName))
                _sizeInBytes += dir->fileLength(fileName);
        }
    }
    return _sizeInBytes;
}

void SegmentInfo::addIfExists(std::vector<std::wstring>& files, const std::wstring& fileName)
{
    if (dir->fileExists(fileName.c_str()))
        files.push_back(fileName);
}

const std::vector<std::wstring>& SegmentInfo::files()
{
    if (!_files.empty())
    {
        // Already cached:
        return _files;
    }

    bool useCompoundFile = getUseCompoundFile();

    if (useCompoundFile)
    {
        _files.push_back(name + std::wstring(L".") + IndexFileNames::COMPOUND_FILE_EXTENSION);
    }
    else
    {
        ConstValueArray<const wchar_t *>& exts = IndexFileNames::NON_STORE_INDEX_EXTENSIONS();
        for (size_t i = 0; i < exts.length; i++)
        {
            addIfExists(_files, name + std::wstring(L".") + exts[i]);
        }
    }

    if (docStoreOffset != -1)
    {
        // We are sharing doc stores (stored fields, term
        // vectors) with other segments
        assert(!docStoreSegment.empty());
        if (docStoreIsCompoundFile)
        {
            _files.push_back(docStoreSegment + std::wstring(L".") + IndexFileNames::COMPOUND_FILE_STORE_EXTENSION);
        }
        else
        {
            ConstValueArray<const wchar_t *>& exts = IndexFileNames::STORE_INDEX_EXTENSIONS();
            for (size_t i = 0; i < exts.length; i++)
                addIfExists(_files, docStoreSegment + std::wstring(L".") + exts[i]);
        }
    }
    else if (!useCompoundFile)
    {
        // We are not sharing, and, these files were not
        // included in the compound file
        ConstValueArray<const wchar_t *>& exts = IndexFileNames::STORE_INDEX_EXTENSIONS();
        for (size_t i = 0; i < exts.length; i++)
            addIfExists(_files, name + std::wstring(L".") + exts[i]);
    }

    std::wstring delFileName = IndexFileNames::fileNameFromGeneration(name.c_str(), (std::wstring(L".") + IndexFileNames::DELETES_EXTENSION).c_str(), delGen);
    if (!delFileName.empty() && (delGen >= YES || dir->fileExists(delFileName.c_str())))
    {
        _files.push_back(delFileName);
    }

    // Careful logic for norms files
    if (normGen.values != NULL)
    {
        for (size_t i = 0; i < normGen.length; i++)
        {
            int64_t gen = normGen[i];
            if (gen >= YES)
            {
                // Definitely a separate norm file, with generation:
                std::wstring gens = std::wstring(L".") + IndexFileNames::SEPARATE_NORMS_EXTENSION;
                gens += Misc::toString((int64_t) i);
                _files.push_back(IndexFileNames::fileNameFromGeneration(name.c_str(), gens.c_str(), gen));
            }
            else if (NO == gen)
            {
                // No separate norms but maybe plain norms
                // in the non compound file case:
                if (!hasSingleNormFile && !useCompoundFile)
                {
                    std::wstring fileName = name + L"." + IndexFileNames::PLAIN_NORMS_EXTENSION;
                    fileName += std::to_wstring(i);
                    if (dir->fileExists(fileName.c_str()))
                    {
                        _files.push_back(fileName);
                    }
                }
            }
            else if (CHECK_DIR == gen)
            {
                // Pre-2.1: we have to check file existence
                std::wstring fileName;
                if (useCompoundFile)
                {
                    fileName = name + L"." + IndexFileNames::SEPARATE_NORMS_EXTENSION;
                    fileName += Misc::toString((int64_t) i);
                }
                else if (!hasSingleNormFile)
                {
                    fileName = name + L"." + IndexFileNames::PLAIN_NORMS_EXTENSION;
                    fileName += Misc::toString((int64_t) i);
                }
                if (!fileName.empty() && dir->fileExists(fileName.c_str()))
                {
                    _files.push_back(fileName);
                }
            }
        }
    }
    else if (preLockless || (!hasSingleNormFile && !useCompoundFile))
    {
        // Pre-2.1: we have to scan the dir to find all
        // matching _X.sN/_X.fN files for our segment:
        std::wstring prefix;
        if (useCompoundFile)
            prefix = name + L"." + IndexFileNames::SEPARATE_NORMS_EXTENSION;
        else
            prefix = name + L"." + IndexFileNames::PLAIN_NORMS_EXTENSION;
        size_t prefixLength = prefix.length();
        std::vector<std::wstring> allFiles;
        if (dir->list(allFiles) == false)
        {
            std::wstring err = L"cannot read directory ";
            err += dir->toString();
            err += L": list() returned null";
            _CLTHROWT(CL_ERR_IO, err.c_str());
        }
        for (size_t i = 0; i < allFiles.size(); i++)
        {
            std::wstring& fileName = allFiles[i];
            if (fileName.length() > prefixLength && _istdigit(fileName[prefixLength]) && fileName.compare(0, prefix.length(), prefix) == 0)
            {
                _files.push_back(fileName);
            }
        }
    }
    return _files;
}



bool SegmentInfo::hasDeletions() const
{
    // Cases:
    //
    //   delGen == NO: this means this segment was written
    //     by the LOCKLESS code and for certain does not have
    //     deletions yet
    //
    //   delGen == CHECK_DIR: this means this segment was written by
    //     pre-LOCKLESS code which means we must check
    //     directory to see if .del file exists
    //
    //   delGen >= YES: this means this segment was written by
    //     the LOCKLESS code and for certain has
    //     deletions
    //
    if (delGen == NO)
    {
        return false;
    }
    else if (delGen >= YES)
    {
        return true;
    }
    else
    {
        return dir->fileExists(getDelFileName().c_str());
    }
}

void SegmentInfo::advanceDelGen()
{
    // delGen 0 is reserved for pre-LOCKLESS format
    if (delGen == NO)
    {
        delGen = YES;
    }
    else
    {
        delGen++;
    }
    clearFiles();
}

void SegmentInfo::clearDelGen()
{
    delGen = NO;
    clearFiles();
}

SegmentInfo* SegmentInfo::clone()
{
    SegmentInfo* si = _CLNEW SegmentInfo(name.c_str(), docCount, dir);
    si->isCompoundFile = isCompoundFile;
    si->delGen = delGen;
    si->preLockless = preLockless;
    si->hasSingleNormFile = hasSingleNormFile;
    if (this->normGen.values != NULL)
    {
        si->normGen.resize(this->normGen.length);
        memcpy(si->normGen.values, this->normGen.values, sizeof(int64_t) * this->normGen.length);
    }
    si->docStoreOffset = docStoreOffset;
    si->docStoreSegment = docStoreSegment;
    si->docStoreIsCompoundFile = docStoreIsCompoundFile;

    return si;
}

std::wstring SegmentInfo::getDelFileName() const
{
    if (delGen == NO)
    {
        // In this case we know there is no deletion filename
        // against this segment
        return NULL;
    }
    else
    {
        // If delGen is CHECK_DIR, it's the pre-lockless-commit file format
        return IndexFileNames::fileNameFromGeneration(name.c_str(), (std::wstring(L".") + IndexFileNames::DELETES_EXTENSION).c_str(), delGen);
    }
}

bool SegmentInfo::hasSeparateNorms(const int32_t fieldNumber) const
{
    if ((normGen.values == NULL && preLockless) || (normGen.values != NULL && normGen[fieldNumber] == CHECK_DIR))
    {
        // Must fallback to directory file exists check:
        return dir->fileExists((name + std::wstring(L".s") + Misc::toString(fieldNumber)).c_str());
    }
    else if (normGen.values == NULL || normGen[fieldNumber] == NO)
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool SegmentInfo::hasSeparateNorms() const
{
    if (normGen.values == NULL)
    {
        if (!preLockless)
        {
            // This means we were created w/ LOCKLESS code and no
            // norms are written yet:
            return false;
        }
        else
        {
            // This means this segment was saved with pre-LOCKLESS
            // code.  So we must fallback to the original
            // directory list check:
            std::vector<std::wstring> result;
            if (!dir->list(result))
            {
                _CLTHROWT(CL_ERR_IO, (std::wstring(L"cannot read directory: ") + dir->toString() + std::wstring(L" list() returned NULL")).c_str());
            }

            std::wstring pattern = name + std::wstring(L".s");
            for (std::vector<std::wstring>::iterator itr = result.begin();
                itr != result.end(); itr++)
            {
                if (wcsncmp(itr->c_str(), pattern.c_str(), pattern.length()) == 0 &&
                    isdigit((*itr)[pattern.length()]))
                {
                    return true;
                }
            }
            return false;
        }
    }
    else
    {
        // This means this segment was saved with LOCKLESS
        // code so we first check whether any normGen's are >= 1
        // (meaning they definitely have separate norms):
        for (size_t i = 0; i < normGen.length; i++)
        {
            if (normGen[i] >= YES)
            {
                return true;
            }
        }
        // Next we look for any == 0.  These cases were
        // pre-LOCKLESS and must be checked in directory:
        for (size_t j = 0; j < normGen.length; j++)
        {
            if (normGen[j] == CHECK_DIR)
            {
                if (hasSeparateNorms(j))
                {
                    return true;
                }
            }
        }
    }

    return false;
}

void SegmentInfo::advanceNormGen(const int32_t fieldIndex)
{
    if (normGen[fieldIndex] == NO)
    {
        normGen.values[fieldIndex] = YES;
    }
    else
    {
        normGen.values[fieldIndex]++;
    }
    clearFiles();
}

std::wstring SegmentInfo::getNormFileName(const int32_t number) const
{
    wchar_t prefix[10];

    int64_t gen;
    if (normGen.values == NULL)
    {
        gen = CHECK_DIR;
    }
    else
    {
        gen = normGen[number];
    }

    if (hasSeparateNorms(number))
    {
        // case 1: separate norm
        swprintf_s(prefix, 10, L".s%d", number);
        return IndexFileNames::fileNameFromGeneration(name.c_str(), prefix, gen);
    }

    if (hasSingleNormFile)
    {
        // case 2: lockless (or nrm file exists) - single file for all norms
        swprintf_s(prefix, 10, L".%s", IndexFileNames::NORMS_EXTENSION);
        return IndexFileNames::fileNameFromGeneration(name.c_str(), prefix, WITHOUT_GEN);
    }

    // case 3: norm file for each field
    swprintf_s(prefix, 10, L".f%d", number);
    return IndexFileNames::fileNameFromGeneration(name.c_str(), prefix, WITHOUT_GEN);
}

void SegmentInfo::setUseCompoundFile(const bool isCompoundFile)
{
    if (isCompoundFile)
    {
        this->isCompoundFile = YES;
    }
    else
    {
        this->isCompoundFile = NO;
    }
    clearFiles();
}

bool SegmentInfo::getUseCompoundFile() const
{
    if (isCompoundFile == NO)
    {
        return false;
    }
    else if (isCompoundFile == YES)
    {
        return true;
    }
    else
    {
        return dir->fileExists(((std::wstring) name + L"." + IndexFileNames::COMPOUND_FILE_EXTENSION).c_str());
    }
}

int32_t SegmentInfo::getDocStoreOffset() const { return docStoreOffset; }

bool SegmentInfo::getDocStoreIsCompoundFile() const { return docStoreIsCompoundFile; }

void SegmentInfo::setDocStoreIsCompoundFile(const bool v)
{
    docStoreIsCompoundFile = v;
    clearFiles();
}

const std::wstring& SegmentInfo::getDocStoreSegment() const
{
    return docStoreSegment;
}

void SegmentInfo::setDocStoreOffset(const int32_t offset)
{
    docStoreOffset = offset;
    clearFiles();
}

void SegmentInfo::write(CL_NS(store)::IndexOutput* output)
{
    output->writeString(name);
    output->writeInt(docCount);
    output->writeLong(delGen);
    output->writeInt(docStoreOffset);
    if (docStoreOffset != -1)
    {
        output->writeString(docStoreSegment);
        output->writeByte(static_cast<uint8_t>(docStoreIsCompoundFile ? 1 : 0));
    }

    output->writeByte(static_cast<uint8_t>(hasSingleNormFile ? 1 : 0));
    if (normGen.values == NULL)
    {
        output->writeInt(NO);
    }
    else
    {
        output->writeInt(normGen.length);
        for (size_t j = 0; j < normGen.length; j++)
        {
            output->writeLong(normGen[j]);
        }
    }
    output->writeByte(isCompoundFile);
}

void SegmentInfo::clearFiles()
{
    _files.clear();
    _sizeInBytes = -1;
}

/** We consider another SegmentInfo instance equal if it
*  has the same dir and same name. */
bool SegmentInfo::equals(const SegmentInfo* obj)
{
    return (obj->dir == this->dir && obj->name.compare(this->name) == 0);
}





std::ostream* SegmentInfos::infoStream = NULL;

/** If non-null, information about retries when loading
* the segments file will be printed to this.
*/
void SegmentInfos::setInfoStream(std::ostream* infoStream)
{
    SegmentInfos::infoStream = infoStream;
}

/**
* @see #setInfoStream
*/
std::ostream* SegmentInfos::getInfoStream()
{
    return infoStream;
}

SegmentInfos::SegmentInfos(bool deleteMembers, int32_t reserveCount) :
    generation(0), lastGeneration(0), infos(deleteMembers)
{
    //Func - Constructor
    //Pre  - deleteMembers indicates if the instance to be created must delete
    //       all SegmentInfo instances it manages when the instance is destroyed or not
    //       true -> must delete, false may not delete
    //Post - An instance of SegmentInfos has been created.

        //initialize counter to 0
    counter = 0;
    version = Misc::currentTimeMillis();
    if (reserveCount > 1)
        infos.reserve(reserveCount);
}

SegmentInfos::~SegmentInfos()
{
    //Func - Destructor
    //Pre  - true
    //Post - The instance has been destroyed. Depending on the constructor used
    //       the SegmentInfo instances that this instance managed have been deleted or not.

        //Clear the list of SegmentInfo instances - make sure everything is deleted
    infos.clear();
}

SegmentInfo* SegmentInfos::info(int32_t i) const
{
    //Func - Returns a reference to the i-th SegmentInfo in the list.
    //Pre  - i >= 0
    //Post - A reference to the i-th SegmentInfo instance has been returned

    CND_PRECONDITION(i >= 0 && i < infos.size(), L"i is out of bounds");

    //Get the i-th SegmentInfo instance
    SegmentInfo *ret = infos[i];

    //Condition check to see if the i-th SegmentInfo has been retrieved
    CND_CONDITION(ret != NULL, L"No SegmentInfo instance found");

    return ret;
}

int64_t SegmentInfos::getCurrentSegmentGeneration(std::vector<std::wstring>& files)
{
    if (files.size() == 0)
    {
        return -1;
    }

    int64_t max = -1;

    std::vector<std::wstring>::iterator itr = files.begin();
    const wchar_t* file;
    size_t seglen = wcslen(IndexFileNames::SEGMENTS);
    while (itr != files.end())
    {
        file = itr->c_str();
        if (wcsncmp(file, IndexFileNames::SEGMENTS, seglen) == 0 && wcscmp(file, IndexFileNames::SEGMENTS_GEN) != 0)
        {
            int64_t gen = generationFromSegmentsFileName(file);
            if (gen > max)
            {
                max = gen;
            }
        }

        itr++;
    }

    return max;
}

int64_t SegmentInfos::getCurrentSegmentGeneration(const CL_NS(store)::Directory* directory)
{
    std::vector<std::wstring> files;
    if (!directory->list(&files))
    {
        _CLTHROWT(CL_ERR_IO, (std::wstring(L"cannot read directory ") + directory->toString() + std::wstring(L": list() returned NULL")).c_str());
    }
    int64_t gen = getCurrentSegmentGeneration(files);
    return gen;
}

std::wstring SegmentInfos::getCurrentSegmentFileName(std::vector<std::wstring>& files)
{
    return IndexFileNames::fileNameFromGeneration(IndexFileNames::SEGMENTS, L"", getCurrentSegmentGeneration(files));
}

std::wstring SegmentInfos::getCurrentSegmentFileName(CL_NS(store)::Directory* directory)
{
    return IndexFileNames::fileNameFromGeneration(IndexFileNames::SEGMENTS, L"", getCurrentSegmentGeneration(directory));
}

std::wstring SegmentInfos::getCurrentSegmentFileName()
{
    return IndexFileNames::fileNameFromGeneration(IndexFileNames::SEGMENTS, L"", lastGeneration);
}

int64_t SegmentInfos::generationFromSegmentsFileName(const wchar_t* fileName)
{
    if (wcscmp(fileName, IndexFileNames::SEGMENTS) == 0)
    {
        return 0;
    }
    else if (wcsncmp(fileName, IndexFileNames::SEGMENTS, wcslen(IndexFileNames::SEGMENTS)) == 0)
    {
        return CL_NS(util)::Misc::base36ToLong(fileName + wcslen(IndexFileNames::SEGMENTS) + 1);
    }
    else
    {
        wchar_t err[CL_MAX_PATH + 35];
        _snwprintf(err, CL_MAX_PATH + 35, L"fileName \"%s\" is not a segments file", fileName);
        _CLTHROWA(CL_ERR_IllegalArgument, err);
        return 0;
    }
}

std::wstring SegmentInfos::getNextSegmentFileName()
{
    int64_t nextGeneration;

    if (generation == -1)
    {
        nextGeneration = 1;
    }
    else
    {
        nextGeneration = generation + 1;
    }

    return IndexFileNames::fileNameFromGeneration(IndexFileNames::SEGMENTS, L"", nextGeneration);
}

void SegmentInfos::clearto(size_t from, size_t end)
{
    size_t range = end - from;
    if ((infos.size() - from) >= range)
    { // Make sure we actually need to remove
        segmentInfosType::iterator itr, bitr = infos.begin() + from, eitr = infos.end();
        size_t count = 0;
        for (itr = bitr; itr != eitr && count < range; ++itr, count++)
        {
            _CLLDELETE((*itr));
        }
        infos.erase(bitr, bitr + count);
    }
}
void SegmentInfos::add(SegmentInfo* info, int32_t pos)
{
    if (pos == -1)
    {
        infos.push_back(info);
    }
    else
    {
        if (pos < 0 || pos >= (int32_t) infos.size() + 1) _CLTHROWA(CL_ERR_IllegalArgument, "pos is out of range");
        infos.insert(infos.begin() + pos, info);
    }
}
int32_t SegmentInfos::size() const
{
    return infos.size();
}
SegmentInfo* SegmentInfos::elementAt(int32_t pos)
{
    return infos.at(pos);
}
void SegmentInfos::setElementAt(SegmentInfo* si, int32_t pos)
{
    infos.set(pos, si);
}
void SegmentInfos::clear() { infos.clear(); }


void SegmentInfos::insert(SegmentInfos* _infos, bool takeMemory)
{
    infos.insert(infos.end(), _infos->infos.begin(), _infos->infos.end());
    if (takeMemory)
    {
        while (_infos->infos.size() > 0)
            _infos->infos.remove(_infos->infos.begin(), true);
    }
}
void SegmentInfos::insert(SegmentInfo* info)
{
    infos.push_back(info);
}
int32_t SegmentInfos::indexOf(const SegmentInfo* info) const
{
    segmentInfosType::const_iterator itr = infos.begin();
    int32_t c = -1;
    while (itr != infos.end())
    {
        c++;
        if (*itr == info)
        {
            return c;
        }
        itr++;
    }
    return -1;
}
void SegmentInfos::range(size_t from, size_t to, SegmentInfos& ret) const
{
    segmentInfosType::const_iterator itr = infos.begin();
    itr += from;
    for (size_t i = from; i < to && itr != infos.end(); i++)
    {
        ret.infos.push_back(*itr);

        itr++;
    }
}
void SegmentInfos::remove(size_t index, bool dontDelete)
{
    infos.remove(index, dontDelete);
}

void SegmentInfos::read(Directory* directory, const wchar_t* segmentFileName)
{
    bool success = false;

    // Clear any previous segments:
    clear();

    IndexInput* input = directory->openInput(segmentFileName);
    CND_CONDITION(input != NULL, L"input == NULL");

    generation = generationFromSegmentsFileName(segmentFileName);
    lastGeneration = generation;

    try
    {
        int32_t format = input->readInt();
        if (format < 0)
        {     // file contains explicit format info
// check that it is a format we can understand
            if (format < CURRENT_FORMAT)
            {
                char err[30];
                cl_sprintf(err, 30, "Unknown format version: %d", format);
                _CLTHROWA(CL_ERR_CorruptIndex, err);
            }
            version = input->readLong(); // read version
            counter = input->readInt(); // read counter
        }
        else
        {     // file is in old format without explicit format info
            counter = format;
        }

        for (int32_t i = input->readInt(); i > 0; i--)
        { // read segmentInfos
            infos.push_back(_CLNEW SegmentInfo(directory, format, input));
        }

        if (format >= 0)
        {    // in old format the version number may be at the end of the file
            if (input->getFilePointer() >= input->length())
                version = CL_NS(util)::Misc::currentTimeMillis(); // old file format without version number
            else
                version = input->readLong(); // read version
        }
        success = true;
    } _CLFINALLY({
        input->close();
        _CLDELETE(input);
        if (!success)
{
            // Clear any segment infos we had loaded so we
            // have a clean slate on retry:
            clear();
        }
        });
}

void SegmentInfos::read(Directory* directory)
{
    generation = lastGeneration = -1;

    FindSegmentsRead find(directory, this);

    find.run();
}


void SegmentInfos::write(Directory* directory)
{
    //Func - Writes a new segments file based upon the SegmentInfo instances it manages
    //Pre  - directory is a valid reference to a Directory
    //Post - The new segment has been written to disk

    std::wstring segmentFileName = getNextSegmentFileName();

    // Always advance the generation on write:
    if (generation == -1)
    {
        generation = 1;
    }
    else
    {
        generation++;
    }

    IndexOutput* output = directory->createOutput(segmentFileName.c_str());

    bool success = false;

    try
    {
        output->writeInt(CURRENT_FORMAT); // write FORMAT
        output->writeLong(++version); // every write changes
                                     // the index
        output->writeInt(counter); // write counter
        output->writeInt(size()); // write infos
        for (int32_t i = 0; i < size(); i++)
        {
            info(i)->write(output);
        }
    }_CLFINALLY(
        try
    {
        output->close();
        _CLDELETE(output);
        success = true;
    } _CLFINALLY(
        if (!success)
        {
            // Try not to leave a truncated segments_N file in
            // the index:
            directory->deleteFile(segmentFileName.c_str());
        }
    )
        )

        try
    {
        output = directory->createOutput(IndexFileNames::SEGMENTS_GEN);
        try
        {
            output->writeInt(FORMAT_LOCKLESS);
            output->writeLong(generation);
            output->writeLong(generation);
        } _CLFINALLY(
            output->close();
        _CLDELETE(output);
        )
    }
    catch (CLuceneError& e)
    {
        if (e.number() != CL_ERR_IO) throw e;
        // It's OK if we fail to write this file since it's
        // used only as one of the retry fallbacks.
    }

    lastGeneration = generation;
}

SegmentInfos* SegmentInfos::clone() const
{
    SegmentInfos* sis = _CLNEW SegmentInfos(true, infos.size());
    for (size_t i = 0; i < infos.size(); i++)
    {
        sis->setElementAt(infos[i]->clone(), i);
    }
    return sis;
}

int64_t SegmentInfos::getVersion() const { return version; }
int64_t SegmentInfos::getGeneration() const { return generation; }
int64_t SegmentInfos::getLastGeneration() const { return lastGeneration; }

int64_t SegmentInfos::readCurrentVersion(Directory* directory)
{
    FindSegmentsVersion find(directory);
    return find.run();
}

//void SegmentInfos::setDefaultGenFileRetryCount(const int32_t count) { defaultGenFileRetryCount = count; }
int32_t SegmentInfos::getDefaultGenFileRetryCount() { return defaultGenFileRetryCount; }

//void SegmentInfos::setDefaultGenFileRetryPauseMsec(const int32_t msec) { defaultGenFileRetryPauseMsec = msec; }
int32_t SegmentInfos::getDefaultGenFileRetryPauseMsec() { return defaultGenFileRetryPauseMsec; }

//void SegmentInfos::setDefaultGenLookaheadCount(const int32_t count) { defaultGenLookaheadCount = count;}
int32_t SegmentInfos::getDefaultGenLookahedCount() { return defaultGenLookaheadCount; }

void SegmentInfos::_FindSegmentsFile::doRun()
{
    std::wstring segmentFileName;
    int64_t lastGen = -1;
    int64_t gen = 0;
    int32_t genLookaheadCount = 0;
    bool retry = false;
    CLuceneError exc; //saved exception

    int32_t method = 0;

    // Loop until we succeed in calling doBody() without
    // hitting an IOException.  An IOException most likely
    // means a commit was in process and has finished, in
    // the time it took us to load the now-old infos files
    // (and segments files).  It's also possible it's a
    // true error (corrupt index).  To distinguish these,
    // on each retry we must see "forward progress" on
    // which generation we are trying to load.  If we
    // don't, then the original error is real and we throw
    // it.

    // We have three methods for determining the current
    // generation.  We try the first two in parallel, and
    // fall back to the third when necessary.

    while (true)
    {

        if (0 == method)
        {
            // Method 1: list the directory and use the highest
            // segments_N file.  This method works well as long
            // as there is no stale caching on the directory
            // contents (NOTE: NFS clients often have such stale
            // caching):
            std::vector<std::wstring> files;

            int64_t genA = -1;

            if (directory != NULL)
            {
                if (directory->list(&files))
                {
                    genA = getCurrentSegmentGeneration(files);
                    files.clear();
                }
            }


            if (infoStream)
            {
                (*infoStream) << "[SIS]: directory listing genA=" << genA << "\n";
            }

            // Method 2: open segments.gen and read its
            // contents.  Then we take the larger of the two
            // gen's.  This way, if either approach is hitting
            // a stale cache (NFS) we have a better chance of
            // getting the right generation.
            int64_t genB = -1;
            if (directory != NULL)
            {
                CLuceneError e;
                for (int32_t i = 0; i < defaultGenFileRetryCount; i++)
                {
                    IndexInput* genInput = NULL;
                    if (!directory->openInput(IndexFileNames::SEGMENTS_GEN, genInput, e))
                    {
                        if (e.number() == CL_ERR_IO)
                        {
                            if (infoStream)
                            {
                                (*infoStream) << "[SIS]: segments.gen open: IOException " << e.what() << "\n";
                            }
                            break;
                        }
                        else
                        {
                            genInput->close();
                            _CLLDELETE(genInput);
                            throw e;
                        }
                    }

                    if (genInput != NULL)
                    {
                        try
                        {
                            int32_t version = genInput->readInt();
                            if (version == FORMAT_LOCKLESS)
                            {
                                int64_t gen0 = genInput->readLong();
                                int64_t gen1 = genInput->readLong();
                                //CL_TRACE("fallback check: %d; %d", gen0, gen1);
                                if (gen0 == gen1)
                                {
                                    // The file is consistent.
                                    genB = gen0;
                                    genInput->close();
                                    _CLDELETE(genInput);
                                    break;
                                }
                            }
                        }
                        catch (CLuceneError &err2)
                        {
                            if (err2.number() != CL_ERR_IO)
                            {
                                genInput->close();
                                _CLLDELETE(genInput);
                                throw err2; // retry only for IOException
                            }
                        } _CLFINALLY({
                            genInput->close();
                            _CLDELETE(genInput);
                            });
                    }

                    Misc::Sleep(defaultGenFileRetryPauseMsec);
                    /*
                    //todo: Wrap the LUCENE_SLEEP call above with the following try/catch block if
                    //	  InterruptedException is implemented
                    try {
                    } catch (CLuceneError &e) {
                    //if (err2.number != CL_ERR_Interrupted) // retry only for InterruptedException
                    // todo: see if CL_ERR_Interrupted needs to be added...
                    throw e;
                    }*/

                }
            }

            //CL_TRACE("%s check: genB=%d", IndexFileNames::SEGMENTS_GEN, genB);

            // Pick the larger of the two gen's:
            if (genA > genB)
                gen = genA;
            else
                gen = genB;

            if (gen == -1)
            {
                // Neither approach found a generation
                _CLTHROWT(CL_ERR_IO, (std::wstring(L"No segments* file found in ") + directory->toString()).c_str());
            }
        }

        // Third method (fallback if first & second methods
        // are not reliable): since both directory cache and
        // file contents cache seem to be stale, just
        // advance the generation.
        if (1 == method || (0 == method && lastGen == gen && retry))
        {

            method = 1;

            if (genLookaheadCount < defaultGenLookaheadCount)
            {
                gen++;
                genLookaheadCount++;
                //CL_TRACE("look ahead increment gen to %d", gen);
            }
        }

        if (lastGen == gen)
        {

            // This means we're about to try the same
            // segments_N last tried.  This is allowed,
            // exactly once, because writer could have been in
            // the process of writing segments_N last time.

            if (retry)
            {
                // OK, we've tried the same segments_N file
                // twice in a row, so this must be a real
                // error.  We throw the original exception we
                // got.
                throw exc;
            }
            else
            {
                retry = true;
            }

        }
        else
        {
            // Segment file has advanced since our last loop, so
            // reset retry:
            retry = false;
        }

        lastGen = gen;

        segmentFileName = IndexFileNames::fileNameFromGeneration(IndexFileNames::SEGMENTS, L"", gen);

        CLuceneError saved_error;
        if (tryDoBody(segmentFileName.c_str(), saved_error))
        {
            return;
        }

        // Save the original root cause:
        if (exc.number() == 0)
        {
            CND_CONDITION(saved_error.number() > 0, L"Unsupported error code");
            exc.set(saved_error.number(), saved_error.what());
        }

        //CL_TRACE("primary Exception on '" + segmentFileName + "': " + err + "'; will retry: retry=" + retry + "; gen = " + gen);

        if (!retry && gen > 1)
        {

            // This is our first time trying this segments
            // file (because retry is false), and, there is
            // possibly a segments_(N-1) (because gen > 1).
            // So, check if the segments_(N-1) exists and
            // try it if so:
            std::wstring prevSegmentFileName = IndexFileNames::fileNameFromGeneration(IndexFileNames::SEGMENTS, L"", gen - 1);

            bool prevExists = false;
            if (directory != NULL)
                prevExists = directory->fileExists(prevSegmentFileName.c_str());
            else
                prevExists = Misc::dir_Exists((std::wstring(fileDirectory) + prevSegmentFileName).c_str());

            if (prevExists)
            {
                //CL_TRACE("fallback to prior segment file '%s'", prevSegmentFileName);
                CLuceneError saved_error;
                if (tryDoBody(prevSegmentFileName.c_str(), saved_error))
                {
                    return;
                }
                //CL_TRACE("secondary Exception on '" + prevSegmentFileName + "': " + err2 + "'; will retry");
            }
        }
    }
}
SegmentInfos::FindSegmentsRead::FindSegmentsRead(CL_NS(store)::Directory* dir, SegmentInfos* _this) :
    SegmentInfos::FindSegmentsFile<bool>(dir)
{
    this->_this = _this;
}
bool SegmentInfos::FindSegmentsRead::doBody(const wchar_t * segmentFileName)
{
    //Have SegmentInfos read the segments file in directory
    _this->read(directory, segmentFileName);
    return true;
}

SegmentInfos::FindSegmentsVersion::FindSegmentsVersion(CL_NS(store)::Directory* dir) :
    SegmentInfos::FindSegmentsFile<int64_t>(dir)
{
}

int64_t SegmentInfos::FindSegmentsVersion::doBody(const wchar_t * segmentFileName)
{

    IndexInput* input = directory->openInput(segmentFileName);

    int32_t format = 0;
    int64_t version = 0;
    try
    {
        format = input->readInt();
        if (format < 0)
        {
            if (format < CURRENT_FORMAT)
            {
                char err[30];
                cl_sprintf(err, 30, "Unknown format version: %d", format);
                _CLTHROWA(CL_ERR_CorruptIndex, err);
            }
            version = input->readLong(); // read version
        }
    }
    _CLFINALLY(input->close(); _CLDELETE(input); );

    if (format < 0)
        return version;

    // We cannot be sure about the format of the file.
    // Therefore we have to read the whole file and cannot simply seek to the version entry.
    SegmentInfos* sis = _CLNEW SegmentInfos();
    sis->read(directory, segmentFileName);
    version = sis->getVersion();
    _CLDELETE(sis);

    return version;

}

CL_NS_END
