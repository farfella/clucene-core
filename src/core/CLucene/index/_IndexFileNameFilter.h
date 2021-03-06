/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team

* Updated by https://github.com/farfella/.
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_index_IndexFileNameFilter_
#define _lucene_index_IndexFileNameFilter_

#include "CLucene/util/Equators.h"
#include "CLucene/util/VoidList.h"

CL_NS_DEF(index)
class FilenameFilter{
public:
	virtual bool accept(const wchar_t * dir, const wchar_t * name) const = 0;
	virtual ~FilenameFilter();
};

/**
 * Filename filter that accept filenames and extensions only created by Lucene.
 *
 * @author Daniel Naber / Bernhard Messer
 * @version $rcs = ' $Id: Exp $ ' ;
 */
class IndexFileNameFilter: public FilenameFilter {
  static IndexFileNameFilter* _singleton;
  static IndexFileNameFilter* singleton();
  CL_NS(util)::CLHashSet<const wchar_t*, CL_NS(util)::Compare::WChar> extensions;
  CL_NS(util)::CLHashSet<const wchar_t*, CL_NS(util)::Compare::WChar> extensionsInCFS;
public:
  IndexFileNameFilter();
  virtual ~IndexFileNameFilter();

  /* (non-Javadoc)
   * @see java.io.FilenameFilter#accept(java.io.File, java.lang.String)
   */
  bool accept(const wchar_t * dir, const wchar_t * name) const;

  /**
   * Returns true if this is a file that would be contained
   * in a CFS file.  This function should only be called on
   * files that pass the above "accept" (ie, are already
   * known to be a Lucene index file).
   */
  bool isCFSFile(const wchar_t * name) const;
  static const IndexFileNameFilter* getFilter();

  static void _shutdown();
};


CL_NS_END
#endif
