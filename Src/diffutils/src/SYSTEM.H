/* System dependent declarations.

   Copyright (C) 1988-1989, 1992-1995, 1998, 2001-2002, 2004, 2006, 2009-2013,
   2015-2017 Free Software Foundation, Inc.

   This file is part of GNU DIFF.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <config.h>

/* Use this to suppress gcc's "...may be used before initialized" warnings. */
#ifdef lint
# define IF_LINT(Code) Code
#else
# define IF_LINT(Code) /* empty */
#endif

/* Define '__attribute__' and 'volatile' first
   so that they're used consistently in all system includes.  */
#if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 6) || __STRICT_ANSI__
# define __attribute__(x)
#endif

#include <verify.h>

#include <sys/types.h>

#include <sys/stat.h>
#include "stat-macros.h"

#ifndef STAT_BLOCKSIZE
# if HAVE_STRUCT_STAT_ST_BLKSIZE
#  define STAT_BLOCKSIZE(s) ((s).st_blksize)
# else
#  define STAT_BLOCKSIZE(s) (8 * 1024)
# endif
#endif

#include <fcntl.h>
#include <time.h>

#include <stdlib.h>
#define EXIT_TROUBLE 2

#include <limits.h>
#include <locale.h>
#include <stddef.h>

// Avoid noise from redefinition of EASTL's {U}INT{8|16|32|64}_{MIN|MAX}
#pragma warning(disable: 4005)
#include <intsafe.h>
#pragma warning(default: 4005)
#define PTRDIFF_MAX PTRDIFF_T_MAX

#include <string.h>
#if ! HAVE_STRCASECOLL
# if HAVE_STRICOLL || defined stricoll
#  define strcasecoll(a, b) stricoll (a, b)
# else
#  define strcasecoll(a, b) strcasecmp (a, b) /* best we can do */
# endif
#endif
#if ! (HAVE_STRCASECMP || defined strcasecmp)
int strcasecmp (char const *, char const *);
#endif


/* ISDIGIT differs from isdigit, as follows:
   - Its arg may be any int or unsigned int; it need not be an unsigned char.
   - It's guaranteed to evaluate its argument exactly once.
   - It's typically faster.
   POSIX 1003.1-2001 says that only '0' through '9' are digits.
   Prefer ISDIGIT to isdigit unless it's important to use the locale's
   definition of 'digit' even when the host does not conform to POSIX.  */
#define ISDIGIT(c) ((unsigned int) (c) - '0' <= 9)

#include <errno.h>

#undef MIN
#undef MAX
#define MIN(a, b) ((a) <= (b) ? (a) : (b))
#define MAX(a, b) ((a) >= (b) ? (a) : (b))

#include <intprops.h>
/* Type used for fast comparison of several bytes at a time.
   This used to be uintmax_t, but changing it to size_t
   made plain 'cmp' 90% faster (GCC 4.8.1, x86).  */

typedef unsigned UNALIGNED word;

/* The signed integer type of a line number.  Since files are read
   into main memory, ptrdiff_t should be wide enough.  */

typedef int lin;
#define LIN_MAX INT_MAX

/* The signed integer type for printing line numbers, and its printf
   length modifier.  This is not simply ptrdiff_t, to cater to older
   and/or nonstandard C libraries where "l" works but "ll" and "t" do
   not, or where 'long' is too narrow and "ll" works but "t" does not.  */

#if LIN_MAX <= LONG_MAX
typedef long int printint;
# define pI "l"
#elif LIN_MAX <= LLONG_MAX
typedef long long int printint;
# define pI "ll"
#else
typedef ptrdiff_t printint;
# define pI "t"
#endif

verify (TYPE_SIGNED (lin));
verify (TYPE_SIGNED (printint));
verify (LIN_MAX == TYPE_MAXIMUM (lin));
verify (LIN_MAX <= TYPE_MAXIMUM (printint));

/* Limit so that 2 * CONTEXT + 1 does not overflow.  */

#define CONTEXT_MAX ((LIN_MAX - 1) / 2)


/* Do struct stat *S, *T describe the same file?  Answer -1 if unknown.  */
#ifndef same_file
# define same_file(s, t) \
    ((((s)->st_ino == (t)->st_ino) && ((s)->st_dev == (t)->st_dev)) \
     || same_special_file (s, t))
#endif

/* Do struct stat *S, *T have the same file attributes?

   POSIX says that two files are identical if st_ino and st_dev are
   the same, but many file systems incorrectly assign the same (device,
   inode) pair to two distinct files, including:

   - GNU/Linux NFS servers that export all local file systems as a
     single NFS file system, if a local device number (st_dev) exceeds
     255, or if a local inode number (st_ino) exceeds 16777215.

   - Network Appliance NFS servers in snapshot directories; see
     Network Appliance bug #195.

   - ClearCase MVFS; see bug id ATRia04618.

   Check whether two files that purport to be the same have the same
   attributes, to work around instances of this common bug.  Do not
   inspect all attributes, only attributes useful in checking for this
   bug.

   It's possible for two distinct files on a buggy file system to have
   the same attributes, but it's not worth slowing down all
   implementations (or complicating the configuration) to cater to
   these rare cases in buggy implementations.  */

#ifndef same_file_attributes
# define same_file_attributes(s, t) \
   ((s)->st_mode == (t)->st_mode \
    && (s)->st_nlink == (t)->st_nlink \
    && (s)->st_uid == (t)->st_uid \
    && (s)->st_gid == (t)->st_gid \
    && (s)->st_size == (t)->st_size \
    && (s)->st_mtime == (t)->st_mtime \
    && (s)->st_ctime == (t)->st_ctime)
#endif

#define STREQ(a, b) (strcmp (a, b) == 0)

#ifndef FALLTHROUGH
# if __GNUC__ < 7
#  define FALLTHROUGH ((void) 0)
# else
#  define FALLTHROUGH __attribute__ ((__fallthrough__))
# endif
#endif
