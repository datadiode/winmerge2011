/** 
 * @file FileLocation.h
 *
 * @brief Declaration for FileLocation class.
 *
 */
// ID line follows -- this is updated by SVN
// $Id$

#ifndef FileLocation_included
#define FileLocation_included

#include "FileTextEncoding.h"

/**
 * @brief A structure containing file's path and encoding information.
 */
struct FileLocation
{
	String filepath; /**< Full path for the file. */
	/**
	 * @name Textual label/descriptor
	 * These descriptors overwrite dir/filename usually shown in headerbar
	 * and can be given from command-line. For example version control
	 * system can set these to "WinMerge v2.1.2.0" and "WinMerge 2.1.4.0"
	 * which is more pleasant and informative than temporary paths.
	 */
	/*@{*/ 
	String description;
	/*@}*/
	FileTextEncoding encoding; /**< Encoding info for the file. */
};

#endif // FileLocation_included
