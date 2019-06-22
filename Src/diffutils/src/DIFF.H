/* Shared definitions for GNU DIFF

   Copyright (C) 1988-1989, 1991-1995, 1998, 2001-2002, 2004, 2009-2013,
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

#define USE_GNU_REGEX($) 0

#include "../system32/system32.h"
#include "system.h"
#if USE_GNU_REGEX(1)
#include <regex.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include <io.h>

/* What kind of changes a hunk contains.  */
enum changes
{
  /* No changes: lines common to both files.  */
  UNCHANGED,

  /* Deletes only: lines taken from just the first file.  */
  OLD,

  /* Inserts only: lines taken from just the second file.  */
  NEW,

  /* Both deletes and inserts: a hunk containing both old and new lines.  */
  CHANGED
};

/* Variables for command line options */

enum output_style
{
  /* Default output style.  */
  OUTPUT_NORMAL,

  /* Output the differences with lines of context before and after (-c).  */
  OUTPUT_CONTEXT,

  /* Output the differences in a unified context diff format (-u).  */
  OUTPUT_UNIFIED,
};

/* True for output styles that are robust,
   i.e. can handle a file that ends in a non-newline.  */
#define ROBUST_OUTPUT_STYLE(S) true

/* The significance of white space during comparisons.  */
enum DIFF_white_space
{
  /* All white space is significant (the default).  */
  IGNORE_NO_WHITE_SPACE,

  /* Ignore changes due to tab expansion (-E).  */
  IGNORE_TAB_EXPANSION,

  /* Ignore changes in trailing horizontal white space (-Z).  */
  IGNORE_TRAILING_SPACE,

  /* IGNORE_TAB_EXPANSION and IGNORE_TRAILING_SPACE are a special case
     because they are independent and can be ORed together, yielding
     IGNORE_TAB_EXPANSION_AND_TRAILING_SPACE.  */
  IGNORE_TAB_EXPANSION_AND_TRAILING_SPACE,

  /* Ignore changes in horizontal white space (-b).  */
  IGNORE_SPACE_CHANGE,

  /* Ignore all horizontal white space (-w).  */
  IGNORE_ALL_SPACE
};

/* The result of comparison is an "edit script": a chain of 'struct change'.
   Each 'struct change' represents one place where some lines are deleted
   and some are inserted.

   LINE0 and LINE1 are the first affected lines in the two files (origin 0).
   DELETED is the number of lines deleted here from file 0.
   INSERTED is the number of lines inserted here in file 1.

   If DELETED is 0 then LINE0 is the number of the line before
   which the insertion was done; vice versa for INSERTED and LINE1.  */

struct change
{
  struct change *link;          /* Previous or next edit command  */
  lin line0;                    /* Line number of 1st deleted line.  */
  lin line1;                    /* Line number of 1st inserted line.  */
  lin deleted;                  /* # lines of file 0 changed here.  */
  lin inserted;                 /* # lines of file 1 changed here.  */
  bool ignore;                  /* Flag used in context.c.  */
  bool trivial;                 /* Flag if change is trivial (ignored blanks or filtered regexps) */
  lin match0;                   /* side0 matching line for line 1 */ /* WinMerge moved block code */
  lin match1;                   /* side1 matching line for line 0 */ /* WinMerge moved block code */
};

extern const char allocated_buffer_name[];

/* Structures that describe the input files.  */

/* Data on one input file being compared.  */

struct file_data {
    int             desc;   /* File descriptor  */
    char const      *name;  /* File name  */
    struct _stati64 stat;   /* File status from fstat()  */

    /* Buffer in which text of file is read.  */
    word *buffer;

    /* Allocated size of buffer, in bytes.  Always a multiple of
       sizeof *buffer.  */
    size_t bufsize;

    /* Number of valid bytes now in the buffer.  */
    size_t buffered;

    /* Array of pointers to lines in the file.  */
    char const **linbuf;

    /* linbuf_base <= buffered_lines <= valid_lines <= alloc_lines.
       linebuf[linbuf_base ... buffered_lines - 1] are possibly differing.
       linebuf[linbuf_base ... valid_lines - 1] contain valid data.
       linebuf[linbuf_base ... alloc_lines - 1] are allocated.  */
    lin linbuf_base, buffered_lines, valid_lines, alloc_lines;

    /* Pointer to end of prefix of this file to ignore when hashing.  */
    char const *prefix_end;

    /* Count of lines in the prefix.
       There are this many lines in the file before linbuf[0].  */
    lin prefix_lines;

    /* Pointer to start of suffix of this file to ignore when hashing.  */
    char const *suffix_begin;

    /* Vector, indexed by line number, containing an equivalence code for
       each line.  It is this vector that is actually compared with that
       of another file to generate differences.  */
    lin *equivs;

    /* Vector, like the previous one except that
       the elements for discarded lines have been squeezed out.  */
    lin *undiscarded;

    /* Vector mapping virtual line numbers (not counting discarded lines)
       to real ones (counting those lines).  Both are origin-0.  */
    lin *realindexes;

    /* Total number of nondiscarded lines.  */
    lin nondiscarded_lines;

    /* Vector, indexed by real origin-0 line number,
       containing 1 for a line that is an insertion or a deletion.
       The results of comparison are stored here.  */
    char *changed;

    /* 1 if file ends in a line with no final newline.  */
    bool missing_newline;

    /* 1 more than the maximum equivalence value used for this or its
       sibling file.  */
    lin equiv_max;

    /* text stats for WinMerge */
    lin count_crlfs, count_crs, count_lfs, count_zeros;
};

/* The file buffer, considered as an array of bytes rather than
   as an array of words.  */
#define FILE_BUFFER(f) ((char *) (f)->buffer)

/* Data on two input files being compared.  */

struct comparison
  {
    /* Describe the two files currently being compared.  */
    struct file_data file[2];

    /* Hash-table: array of buckets, each being a chain of equivalence classes.  */
    lin *buckets;

    /* Number of buckets in the hash table array. */
    int nbuckets;

    /* Array in which the equivalence classes are allocated.
       The bucket-chains go through the elements in this array.
       The number of an equivalence class is its index in this array.  */
    struct equivclass *equivs;

    /* Index of first free element in the array 'equivs'.  */
    lin equivs_index;

    /* Number of elements allocated in the array 'equivs'.  */
    lin equivs_alloc;

    /* Stdio stream to output diffs to.  */
    FILE *outfile;

    /* File labels for '-c' output headers (--label).  */
    char const *file_label[2];

#if USE_GNU_REGEX(1)
    /* Regexp to identify function-header lines (-F).  */
    struct re_pattern_buffer function_regexp;

    /* Ignore changes that affect only lines matching this regexp (-I).  */
    struct re_pattern_buffer ignore_regexp;
#endif

    enum output_style output_style;

    /* The significance of white space during comparisons.  */
    enum DIFF_white_space ignore_white_space;

    /* DBCS code page, or 0 if source is ASCII or an extension thereof.  */
    int dbcs_codepage;

    /* Allow for a fallback to Diffutils 2.5 cost control behavior.  */
    lin cost_limit;

    /* Number of lines of context to show in each set of diffs.
       This is zero when context is not to be shown.  */
    lin context;

    /* Number of lines to keep in identical prefix and suffix.  */
    lin horizon_lines;

    /* Number of columns between tab stops.  */
    unsigned tabsize;

    /* Ignore differences in case of letters (-i).  */
    bool ignore_case;

    /* Ignore end of line differences (at least between UNIX & DOS */
    bool ignore_eol_diff;

    /* Ignore changes that affect only blank lines (-B).  */
    bool ignore_blank_lines;

    /* Use a tab in the output, rather than a space, before the text of an
       input line, so as to keep the proper alignment in the input line
       without changing the characters in it (-T).  */
    bool initial_tab;

    /* Do not output an initial space or tab before the text of an empty line.  */
    bool suppress_blank_empty;

    /* Nonzero if output cannot be generated for identical files.  */
    bool no_diff_means_no_output;

    /* Use heuristics for better speed with large files with a small
       density of changes.  */
    bool speed_large_files;

    /* Don't discard lines.  This makes things slower (sometimes much
       slower) but will find a guaranteed minimal set of changes.  */
    bool minimal;

    /* LibXDiff only: Whether to apply indent heuristic.  */
    bool indent_heuristic;
  };

struct file_cursor
{
    int desc;
    lin line;
    int index;
    int ahead;
    bool ignore_lf;
    char chunk[256];
};

inline bool iseolch (char ch) { return ch == '\n' || ch == '\r'; }

/* Declare various functions.  */

/* analyze.c */
/* WinMerge: add last two params */
extern struct change *diff_2_files (struct comparison *, int *, bool, int*);
extern void cleanup_file_buffers (struct comparison *);

/* context.c */
extern void print_context_header (struct comparison *, char const * const *, bool);
extern void print_context_script (struct comparison *, struct file_cursor *, struct change *, bool);

/* io.c */
extern unsigned file_block_read (struct file_data *, size_t);
/* WinMerge: add last pointer param */
extern bool read_files (struct comparison *, int *);

/* normal.c */
extern void print_normal_script (struct comparison *, struct file_cursor *, struct change *);

/* util.c */
extern char const change_letter[4];
extern char *concat (char const *, char const *, char const *);
extern bool lines_differ (struct comparison const *, char const *, char const *) _GL_ATTRIBUTE_PURE;
extern lin translate_line_number (struct file_data const *, lin);
extern struct change *find_change (struct comparison *, struct change *);
extern void *zalloc (size_t);
extern enum changes analyze_hunk (struct comparison *, struct change *, lin *, lin *, lin *, lin *);
extern void begin_output (struct comparison *);
extern void debug_script (struct change *);
extern void fatal (char const *) __attribute__((noreturn));
extern void perror_with_name (char const *);
extern void pfatal_with_name (char const *) __attribute__((noreturn));
extern void print_1_line (struct comparison *, struct file_cursor *, char const *, lin, bool);
extern void print_message_queue (void);
extern void print_number_range (char, struct file_data *, lin, lin, FILE *);
extern void print_script (struct comparison *, struct file_cursor *, struct change *,
                          struct change * (*) (struct comparison *, struct change *),
                          void (*) (struct comparison *, struct file_cursor *, struct change *));
extern void translate_range (struct file_data const *, lin, lin,
                             printint *, printint *);
