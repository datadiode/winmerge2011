/* Support routines for GNU DIFF.

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

#include <windows.h>
#include <assert.h>
#include "diff.h"
#include <xalloc.h>

/* Use when a system call returns non-zero status.
   NAME should normally be the file name.  */

void
perror_with_name (char const *name)
{
  perror (name);
}

/* Use when a system call returns non-zero status and that is fatal.  */

void
pfatal_with_name (char const *name)
{
  int e = errno;
  perror_with_name (name);
  die (EXIT_TROUBLE, e, "%s", name);
}

/* Print an error message containing MSGID, then exit.  */

void
fatal (char const *msgid)
{
  errno = 0;
  pfatal_with_name (msgid);
}

void
begin_output (void)
{
  assert(outfile);
}

static int iseolch (char ch)
{
  return ch == '\n' || ch == '\r';
}

/* Compare two lines (typically one from each input file)
   according to the command line options.
   For efficiency, this is invoked only when the lines do not match exactly
   but an option like -i might cause us to ignore the difference.
   Return nonzero if the lines differ.  */

bool
lines_differ (char const *s1, char const *s2)
{
  register char const *t1 = s1;
  register char const *t2 = s2;
  size_t column = 0;

  while (1)
    {
      register unsigned char c1 = *t1++;
      register unsigned char c2 = *t2++;

      /* Test for exact char equality first, since it's a common case.  */
      if (c1 != c2)
        {
          switch (ignore_white_space)
            {
            case IGNORE_ALL_SPACE:
              /* For -w, just skip past any white space.  */
              while (isspace (c1) && ! iseolch(c1)) c1 = *t1++;
              while (isspace (c2) && ! iseolch(c2)) c2 = *t2++;
              break;

            case IGNORE_SPACE_CHANGE:
              /* For -b, advance past any sequence of white space in
              line 1 and consider it just one space, or nothing at
              all if it is at the end of the line.  */
              if (isspace (c1))
                {
                  while (! iseolch(c1))
                    {
                      c1 = *t1++;
                      if (! isspace (c1))
                        {
                          --t1;
                          c1 = ' ';
                          break;
                        }
                    }
                }

              /* Likewise for line 2.  */
              if (isspace (c2))
                {
                  while (! iseolch(c2))
                    {
                      c2 = *t2++;
                      if (! isspace (c2))
                        {
                          --t2;
                          c2 = ' ';
                          break;
                        }
                    }
                }

              if (c1 != c2)
                {
                  /* If we went too far when doing the simple test
                     for equality, go back to the first non-white-space
                     character in both sides and try again.  */
                  if (c2 == ' ' && ! iseolch(c1)
                      && s1 + 1 < t1
                      && isspace ((unsigned char) t1[-2]))
                    {
                      --t1;
                      continue;
                    }
                  if (c1 == ' ' && ! iseolch(c2)
                      && s2 + 1 < t2
                      && isspace ((unsigned char) t2[-2]))
                    {
                      --t2;
                      continue;
                    }
                }

              break;

            case IGNORE_TRAILING_SPACE:
            case IGNORE_TAB_EXPANSION_AND_TRAILING_SPACE:
              /* Use while (...) so as to not acidentally break the switch */
              while (isspace (c1) && isspace (c2))
                {
                  unsigned char c;
                  if (! iseolch(c1))
                    {
                      char const *p = t1;
                      while (! iseolch(c = *p) && isspace (c))
                        ++p;
                      if (! iseolch(c))
                        break;
                    }
                  if (! iseolch(c2))
                    {
                      char const *p = t2;
                      while (! iseolch(c = *p) && isspace (c))
                        ++p;
                      if (! iseolch(c))
                        break;
                    }
                  /* Both lines have nothing but whitespace left.  */
                  return false;
                }
              if (ignore_white_space == IGNORE_TRAILING_SPACE)
                break;
              FALLTHROUGH;
            case IGNORE_TAB_EXPANSION:
              if ((c1 == ' ' && c2 == '\t')
                  || (c1 == '\t' && c2 == ' '))
                {
                  size_t column2 = column;
                  for (;; c1 = *t1++)
                    {
                      if (c1 == ' ')
                        column++;
                      else if (c1 == '\t')
                        column += tabsize - column % tabsize;
                      else
                        break;
                    }
                  for (;; c2 = *t2++)
                    {
                      if (c2 == ' ')
                        column2++;
                      else if (c2 == '\t')
                        column2 += tabsize - column2 % tabsize;
                      else
                        break;
                    }
                  if (column != column2)
                    return true;
                }
              break;

            case IGNORE_NO_WHITE_SPACE:
              break;
            }

          /* Lowercase all letters if -i is specified.  */

          if (ignore_case)
            {
              if (islower (c1) && (dbcs_codepage == 0 || CharPrevExA(dbcs_codepage, s1, t1, 0) == t1 - 1))
                c1 = (unsigned char)toupper (c1);
              if (islower (c2) && (dbcs_codepage == 0 || CharPrevExA(dbcs_codepage, s2, t2, 0) == t2 - 1))
                c2 = (unsigned char)toupper (c2);
            }

          if (ignore_eol_diff)
          {
            if (c1 == '\r')
              c1 = '\n';
            else if (c2 == '\r')
              c2 = '\n';
          }

          if (c1 != c2)
            break;
        }
      if (iseolch(c1))
        return false;

      column += c1 == '\t' ? tabsize - column % tabsize : 1;
    }

  return true;
}

/* Find the consecutive changes at the start of the script START.
   Return the last link before the first gap.  */

struct change * _GL_ATTRIBUTE_CONST
find_change (struct change *start)
{
  return start;
}

struct change * _GL_ATTRIBUTE_CONST
find_reverse_change (struct change *start)
{
  return start;
}

/* Divide SCRIPT into pieces by calling HUNKFUN and
   print each piece with PRINTFUN.
   Both functions take one arg, an edit script.

   HUNKFUN is called with the tail of the script
   and returns the last link that belongs together with the start
   of the tail.

   PRINTFUN takes a subscript which belongs together (with a null
   link at the end) and prints it.  */

void
print_script (struct change *script,
              struct change * (*hunkfun) (struct change *),
              void (*printfun) (struct change *))
{
  struct change *next = script;

  while (next)
    {
      struct change *this, *end;

      /* Find a set of changes that belong together.  */
      this = next;
      end = (*hunkfun) (next);

      /* Disconnect them from the rest of the changes,
         making them a hunk, and remember the rest for next iteration.  */
      next = end->link;
      end->link = 0;
#ifdef DEBUG
      debug_script (this);
#endif

      /* Print this hunk.  */
      (*printfun) (this);

      /* Reconnect the script so it will all be freed properly.  */
      end->link = next;
    }
}

/* Print the text of a single line LINE,
   flagging it with the characters in LINE_FLAG (which say whether
   the line is inserted, deleted, changed, etc.).  LINE_FLAG must not
   end in a blank, unless it is a single blank.  */

void
print_1_line (char const *line_flag, char const *const *line)
{
  char const *base = line[0], *limit = line[1]; /* Help the compiler.  */
  FILE *out = outfile; /* Help the compiler some more.  */
  char const *flag_format = 0;

  /* If -T was specified, use a Tab between the line-flag and the text.
     Otherwise use a Space (as Unix diff does).
     Print neither space nor tab if line-flags are empty.
     But omit trailing blanks if requested.  */

  if (line_flag && *line_flag)
    {
      char const *flag_format_1 = flag_format = initial_tab ? "%s\t" : "%s ";
      char const *line_flag_1 = line_flag;

      if (suppress_blank_empty && **line == '\n')
        {
          flag_format_1 = "%s";

          /* This hack to omit trailing blanks takes advantage of the
             fact that the only way that LINE_FLAG can end in a blank
             is when LINE_FLAG consists of a single blank.  */
          line_flag_1 += *line_flag_1 == ' ';
        }

      fprintf (out, flag_format_1, line_flag_1);
    }

  output_1_line (base, limit, flag_format, line_flag);

  if ((!line_flag || line_flag[0]) && limit[-1] != '\n' && limit[-1] != '\r')
    {
      fprintf (out, "\n\\ %s\n", "No newline at end of file");
    }
}

/* A version of fwrite which converts any embedded \r or \n or \r\n to \n,
   for use with mixed eol mode input being written to a text mode stream.  */
static void
fwrite_textify (char const *p, char const *q, FILE *stream)
{
  while (p < q)
    {
      char c = *p++;
      if (c == '\r') /* carriage return? */
        {
          if (p < q && *p == '\n')
            continue; /* yes, but it's followed by \n, so ignore it */
          c = '\n'; /* replace with \n */
        }
      /* (any bare \n characters are ok, stream will convert them) */
      putc(c, stream);
    }
}

/* Output a line from BASE up to LIMIT.
   With -t, expand white space characters to spaces, and if FLAG_FORMAT
   is nonzero, output it with argument LINE_FLAG after every
   internal carriage return, so that tab stops continue to line up.  */

void
output_1_line (char const *base, char const *limit, char const *flag_format,
               char const *line_flag)
{
  char * pos = NULL;
  if (!expand_tabs)
    fwrite_textify (base, limit, outfile);
  else
    {
      register FILE *out = outfile;
      register unsigned char c;
      register char const *t = base;
      register size_t column = 0;
      size_t tab_size = tabsize;

      while (t < limit)
        {
          switch ((c = *t++))
            {
            case '\t':
              {
                size_t spaces = tab_size - column % tab_size;
                column += spaces;
                do
                  putc (' ', out);
                while (--spaces);
              }
              break;

            case '\r':
              putc (c, out);
              if (flag_format && t < limit && *t != '\n')
                fprintf (out, flag_format, line_flag);
              column = 0;
              break;

            case '\b':
              if (column == 0)
                continue;
              column--;
              putc (c, out);
              break;

            default:
              column += isprint (c) != 0;
              putc (c, out);
              break;
            }
        }
    }
}


char const change_letter[] = { 0, 'd', 'a', 'c' };

/* Translate an internal line number (an index into diff's table of lines)
   into an actual line number in the input file.
   The internal line number is I.  FILE points to the data on the file.

   Internal line numbers count from 0 starting after the prefix.
   Actual line numbers count from 1 within the entire file.  */

lin _GL_ATTRIBUTE_PURE
translate_line_number (struct file_data const *file, lin i)
{
  return i + file->prefix_lines + 1;
}

/* Translate a line number range.  This is always done for printing,
   so for convenience translate to printint rather than lin, so that the
   caller can use printf with "%"pI"d" without casting.  */

void
translate_range (struct file_data const *file,
                 lin a, lin b,
                 printint *aptr, printint *bptr)
{
  *aptr = translate_line_number (file, a - 1) + 1;
  *bptr = translate_line_number (file, b + 1) - 1;
}

/* Print a pair of line numbers with SEPCHAR, translated for file FILE.
   If the two numbers are identical, print just one number.

   Args A and B are internal line numbers.
   We print the translated (real) line numbers.  */

void
print_number_range (char sepchar, struct file_data *file, lin a, lin b)
{
  printint trans_a, trans_b;
  translate_range (file, a, b, &trans_a, &trans_b);

  /* Note: we can have B < A in the case of a range of no lines.
     In this case, we should print the line number before the range,
     which is B.  */
  if (trans_b > trans_a)
    fprintf (outfile, "%"pI"d%c%"pI"d", trans_a, sepchar, trans_b);
  else
    fprintf (outfile, "%"pI"d", trans_b);
}

/* Look at a hunk of edit script and report the range of lines in each file
   that it applies to.  HUNK is the start of the hunk, which is a chain
   of 'struct change'.  The first and last line numbers of file 0 are stored in
   *FIRST0 and *LAST0, and likewise for file 1 in *FIRST1 and *LAST1.
   Note that these are internal line numbers that count from 0.

   If no lines from file 0 are deleted, then FIRST0 is LAST0+1.

   Return UNCHANGED if only ignorable lines are inserted or deleted,
   OLD if lines of file 0 are deleted,
   NEW if lines of file 1 are inserted,
   and CHANGED if both kinds of changes are found. */

enum changes
analyze_hunk (struct change *hunk,
              lin *first0, lin *last0,
              lin *first1, lin *last1)
{
  struct change *next;
  lin l0, l1;
  lin show_from, show_to;
  lin i;
  bool trivial = ignore_blank_lines || USE_GNU_REGEX(ignore_regexp.fastmap);
  size_t trivial_length = ignore_blank_lines - 1;
  /* If 0, ignore zero-length lines;
     if SIZE_MAX, do not ignore lines just because of their length.  */

  bool skip_white_space =
    ignore_blank_lines && IGNORE_TRAILING_SPACE <= ignore_white_space;
  bool skip_leading_white_space =
    skip_white_space && IGNORE_SPACE_CHANGE <= ignore_white_space;

  char const * const *linbuf0 = files[0].linbuf;  /* Help the compiler.  */
  char const * const *linbuf1 = files[1].linbuf;

  show_from = show_to = 0;

  *first0 = hunk->line0;
  *first1 = hunk->line1;

  next = hunk;
  do
    {
      l0 = next->line0 + next->deleted - 1;
      l1 = next->line1 + next->inserted - 1;
      show_from += next->deleted;
      show_to += next->inserted;

      for (i = next->line0; i <= l0 && trivial; i++)
          {
            char const *line = linbuf0[i];
          char const *lastbyte = linbuf0[i + 1] - 1;
          char const *newline = lastbyte + ! iseolch(*lastbyte);
          size_t len = newline - line;
          char const *p = line;
          if (skip_white_space)
            for (; ! iseolch(*p); p++)
              if (! isspace ((unsigned char) *p))
                {
                  if (! skip_leading_white_space)
                    p = line;
                  break;
                }
          if (newline - p != trivial_length
              && (! USE_GNU_REGEX(ignore_regexp.fastmap)
                  || USE_GNU_REGEX(re_search (&ignore_regexp, line, len, 0, len, 0) < 0)))
              trivial = 0;
          }

      for (i = next->line1; i <= l1 && trivial; i++)
          {
            char const *line = linbuf1[i];
          char const *lastbyte = linbuf1[i + 1] - 1;
          char const *newline = lastbyte + ! iseolch(*lastbyte);
          size_t len = newline - line;
          char const *p = line;
          if (skip_white_space)
            for (; ! iseolch(*p); p++)
              if (! isspace ((unsigned char) *p))
                {
                  if (! skip_leading_white_space)
                    p = line;
                  break;
                }
          if (newline - p != trivial_length
              && (! USE_GNU_REGEX(ignore_regexp.fastmap)
                  || USE_GNU_REGEX(re_search (&ignore_regexp, line, len, 0, len, 0) < 0)))
              trivial = 0;
          }
    }
  while ((next = next->link) != 0);

  *last0 = l0;
  *last1 = l1;

  /* If all inserted or deleted lines are ignorable,
     tell the caller to ignore this hunk.  */

  /* WinMerge editor needs to know if there were trivial changes though,
     so stash that off in the trivial field */
  hunk->trivial = trivial;
  if (trivial)
    return UNCHANGED;

  return (enum changes)((show_from ? OLD : UNCHANGED) | (show_to ? NEW : UNCHANGED));
}

/* Concatenate three strings, returning a newly malloc'd string.  */

char *
concat (char const *s1, char const *s2, char const *s3)
{
  char *new = (char *)xmalloc (strlen (s1) + strlen (s2) + strlen (s3) + 1);
  sprintf (new, "%s%s%s", s1, s2, s3);
  return new;
}

/* Yield a new block of SIZE bytes, initialized to zero.  */

void *
zalloc (size_t size)
{
  void *p = xmalloc (size);
  memset (p, 0, size);
  return p;
}

void
debug_script (struct change *sp)
{
  fflush (stdout);

  for (; sp; sp = sp->link)
    {
      printint line0 = sp->line0;
      printint line1 = sp->line1;
      printint deleted = sp->deleted;
      printint inserted = sp->inserted;
      fprintf (stderr, "%3"pI"d %3"pI"d delete %"pI"d insert %"pI"d\n",
               line0, line1, deleted, inserted);
    }

  fflush (stderr);
}
