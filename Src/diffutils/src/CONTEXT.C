/* Context-format output routines for GNU DIFF.

   Copyright (C) 1988-1989, 1991-1995, 1998, 2001-2002, 2004, 2006, 2009-2013,
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

#include "diff.h"

#if USE_GNU_REGEX(1)
static char const *find_function (char const * const *, lin);
#endif
static struct change *find_hunk (struct comparison *, struct change *);
static void mark_ignorable (struct comparison *, struct change *);
static void pr_context_hunk (struct comparison *, struct file_cursor *, struct change *);
static void pr_unidiff_hunk (struct comparison *, struct file_cursor *, struct change *);

/* Last place find_function started searching from.  */
static lin find_function_last_search;

/* The value find_function returned when it started searching there.  */
static lin find_function_last_match;

/* Print a label for a context diff, with a file name and date or a label.  */

static void
print_context_label (struct comparison *cmp,
                     char const *mark,
                     struct file_data *inf,
                     char const *name,
                     char const *label)
{
  if (label)
    fprintf (cmp->outfile, "%s %s\n", mark, label);
  else
    fprintf (cmp->outfile, "%s %s\t%s", mark, name, ctime (&inf->stat.st_mtime) );
}

/* Print a header for a context diff, with the file names and dates.  */

void
print_context_header (struct comparison *cmp, char const *const *names, bool unidiff)
{
  if (unidiff)
    {
      print_context_label (cmp, "---", &cmp->file[0], names[0], cmp->file_label[0]);
      print_context_label (cmp, "+++", &cmp->file[1], names[1], cmp->file_label[1]);
    }
  else
    {
      print_context_label (cmp, "***", &cmp->file[0], names[0], cmp->file_label[0]);
      print_context_label (cmp, "---", &cmp->file[1], names[1], cmp->file_label[1]);
    }
}

/* Print an edit script in context format.  */

void
print_context_script (struct comparison *cmp, struct file_cursor *cursors, struct change *script, bool unidiff)
{
  if (cmp->ignore_blank_lines || USE_GNU_REGEX(ignore_regexp.fastmap))
    mark_ignorable (cmp, script);
  else
    {
      struct change *e;
      for (e = script; e; e = e->link)
        e->ignore = false;
    }

  find_function_last_search = - cmp->file[0].prefix_lines;
  find_function_last_match = LIN_MAX;

  if (unidiff)
    print_script (cmp, cursors, script, find_hunk, pr_unidiff_hunk);
  else
    print_script (cmp, cursors, script, find_hunk, pr_context_hunk);
}

/* Print a pair of line numbers with a comma, translated for file FILE.
   If the second number is not greater, use the first in place of it.

   Args A and B are internal line numbers.
   We print the translated (real) line numbers.  */

static void
print_context_number_range (struct comparison *cmp, struct file_data const *file, lin a, lin b)
{
  printint trans_a, trans_b;
  translate_range (file, a, b, &trans_a, &trans_b);

  /* We can have B <= A in the case of a range of no lines.
     In this case, we should print the line number before the range,
     which is B.

     POSIX 1003.1-2001 requires two line numbers separated by a comma
     even if the line numbers are the same.  However, this does not
     match existing practice and is surely an error in the
     specification.  */

  if (trans_b <= trans_a)
    fprintf (cmp->outfile, "%" pI "d", trans_b);
  else
    fprintf (cmp->outfile, "%" pI "d,%" pI "d", trans_a, trans_b);
}

#if USE_GNU_REGEX(1)
/* Print FUNCTION in a context header.  */
static void
print_context_function (FILE *out, char const *function)
{
  int i, j;
  putc (' ', out);
  for (i = 0; c_isspace ((unsigned char) function[i]) && function[i] != '\n'; i++)
    continue;
  for (j = i; j < i + 40 && function[j] != '\n'; j++)
    continue;
  while (i < j && c_isspace ((unsigned char) function[j - 1]))
    j--;
  fwrite (function + i, sizeof (char), j - i, out);
}
#endif

/* Print a portion of an edit script in context format.
   HUNK is the beginning of the portion to be printed.
   The end is marked by a 'link' that has been nulled out.

   Prints out lines from both files, and precedes each
   line with the appropriate flag-character.  */

static void
pr_context_hunk (struct comparison *cmp, struct file_cursor *cursors, struct change *hunk)
{
  lin first0, last0, first1, last1, i;
  char const *prefix;
#if USE_GNU_REGEX(1)
  char const *function;
#endif
  FILE *out;

  /* Determine range of line numbers involved in each file.  */

  enum changes changes = analyze_hunk (cmp, hunk, &first0, &last0, &first1, &last1);
  if (! changes)
    return;

  /* Include a context's width before and after.  */

  i = - cmp->file[0].prefix_lines;
  first0 = MAX (first0 - cmp->context, i);
  first1 = MAX (first1 - cmp->context, i);
  if (last0 < cmp->file[0].valid_lines - cmp->context)
    last0 += cmp->context;
  else
    last0 = cmp->file[0].valid_lines - 1;
  if (last1 < cmp->file[1].valid_lines - cmp->context)
    last1 += cmp->context;
  else
    last1 = cmp->file[1].valid_lines - 1;

#if USE_GNU_REGEX(1)
  /* If desired, find the preceding function definition line in file 0.  */
  function = NULL;
  if (function_regexp.fastmap)
    function = find_function (cmp->file[0].linbuf, first0);
#endif

  begin_output (cmp);
  out = cmp->outfile;

  fputs ("***************", out);

#if USE_GNU_REGEX(1)
  if (function)
    print_context_function (out, function);
#endif

  putc ('\n', out);
  fputs ("*** ", out);
  print_context_number_range (cmp, &cmp->file[0], first0, last0);
  fputs (" ****", out);
  putc ('\n', out);

  if (changes & OLD)
    {
      struct change *next = hunk;

      for (i = first0; i <= last0; i++)
        {
          /* Skip past changes that apply (in file 0)
             only to lines before line I.  */

          while (next && next->line0 + next->deleted <= i)
            next = next->link;

          /* Compute the marking for line I.  */

          prefix = " ";
          if (next && next->line0 <= i)
            {
              /* The change NEXT covers this line.
                 If lines were inserted here in file 1, this is "changed".
                 Otherwise it is "deleted".  */
              prefix = (next->inserted > 0 ? "!" : "-");
            }
          print_1_line (cmp, &cursors[0], prefix, cmp->file[0].prefix_lines + i, iseolch(*cmp->file[0].linbuf[i]));
        }
    }

  fputs ("--- ", out);
  print_context_number_range (cmp, &cmp->file[1], first1, last1);
  fputs (" ----", out);
  putc ('\n', out);

  if (changes & NEW)
    {
      struct change *next = hunk;

      for (i = first1; i <= last1; i++)
        {
          /* Skip past changes that apply (in file 1)
             only to lines before line I.  */

          while (next && next->line1 + next->inserted <= i)
            next = next->link;

          /* Compute the marking for line I.  */

          prefix = " ";
          if (next && next->line1 <= i)
            {
              /* The change NEXT covers this line.
                 If lines were deleted here in file 0, this is "changed".
                 Otherwise it is "inserted".  */
              prefix = (next->deleted > 0 ? "!" : "+");
            }
          print_1_line (cmp, &cursors[1], prefix, cmp->file[1].prefix_lines + i, iseolch(*cmp->file[1].linbuf[i]));
        }
    }
}

/* Print a pair of line numbers with a comma, translated for file FILE.
   If the second number is smaller, use the first in place of it.
   If the numbers are equal, print just one number.

   Args A and B are internal line numbers.
   We print the translated (real) line numbers.  */

static void
print_unidiff_number_range (struct comparison *cmp, struct file_data const *file, lin a, lin b)
{
  printint trans_a, trans_b;
  translate_range (file, a, b, &trans_a, &trans_b);

  /* We can have B < A in the case of a range of no lines.
     In this case, we print the line number before the range,
     which is B.  It would be more logical to print A, but
     'patch' expects B in order to detect diffs against empty files.  */
  if (trans_b <= trans_a)
    fprintf (cmp->outfile, trans_b < trans_a ? "%" pI "d,0" : "%" pI "d", trans_b);
  else
    fprintf (cmp->outfile, "%" pI "d,%" pI "d", trans_a, trans_b - trans_a + 1);
}

/* Print a portion of an edit script in unidiff format.
   HUNK is the beginning of the portion to be printed.
   The end is marked by a 'link' that has been nulled out.

   Prints out lines from both files, and precedes each
   line with the appropriate flag-character.  */

static void
pr_unidiff_hunk (struct comparison *cmp, struct file_cursor *cursors, struct change *hunk)
{
  lin first0, last0, first1, last1;
  lin i, j, k;
  struct change *next;
#if USE_GNU_REGEX(1)
  char const *function;
#endif
  FILE *out;

  /* Determine range of line numbers involved in each file.  */

  if (! analyze_hunk (cmp, hunk, &first0, &last0, &first1, &last1) )
    return;

  /* Include a context's width before and after.  */

  i = - cmp->file[0].prefix_lines;
  first0 = MAX (first0 - cmp->context, i);
  first1 = MAX (first1 - cmp->context, i);
  if (last0 < cmp->file[0].valid_lines - cmp->context)
    last0 += cmp->context;
  else
    last0 = cmp->file[0].valid_lines - 1;
  if (last1 < cmp->file[1].valid_lines - cmp->context)
    last1 += cmp->context;
  else
    last1 = cmp->file[1].valid_lines - 1;

#if USE_GNU_REGEX(1)
  /* If desired, find the preceding function definition line in file 0.  */
  function = NULL;
  if (function_regexp.fastmap)
    function = find_function (cmp->file[0].linbuf, first0);
#endif

  begin_output (cmp);
  out = cmp->outfile;

  fputs ("@@ -", out);
  print_unidiff_number_range (cmp, &cmp->file[0], first0, last0);
  fputs (" +", out);
  print_unidiff_number_range (cmp, &cmp->file[1], first1, last1);
  fputs (" @@", out);

#if USE_GNU_REGEX(1)
  if (function)
    print_context_function (out, function);
#endif

  putc ('\n', out);

  next = hunk;
  i = first0;
  j = first1;

  while (i <= last0 || j <= last1)
    {

      /* If the line isn't a difference, output the context from file 0. */

      if (!next || i < next->line0)
        {
          char const *line = cmp->file[0].linbuf[i];
          if (! (cmp->suppress_blank_empty && iseolch (*line)) )
            putc (cmp->initial_tab ? '\t' : ' ', out);
          print_1_line (cmp, &cursors[0], NULL, cmp->file[0].prefix_lines + i++, false);
          j++;
        }
      else
        {
          /* For each difference, first output the deleted part. */

          k = next->deleted;
          while (k--)
            {
              char const *line = cmp->file[0].linbuf[i];
              putc ('-', out);
              if (cmp->initial_tab && ! (cmp->suppress_blank_empty && iseolch (*line)) )
                putc ('\t', out);
              print_1_line (cmp, &cursors[0], NULL, cmp->file[0].prefix_lines + i++, false);
            }

          /* Then output the inserted part. */

          k = next->inserted;

          while (k--)
            {
              char const *line = cmp->file[1].linbuf[j];
              putc ('+', out);
              if (cmp->initial_tab && ! (cmp->suppress_blank_empty && iseolch (*line)) )
                putc ('\t', out);
              print_1_line (cmp, &cursors[1], NULL, cmp->file[1].prefix_lines + j++, false);
            }

          /* We're done with this hunk, so on to the next! */

          next = next->link;
        }
    }
}

/* Scan a (forward-ordered) edit script for the first place that more than
   2*CONTEXT unchanged lines appear, and return a pointer
   to the 'struct change' for the last change before those lines.  */

static struct change * _GL_ATTRIBUTE_PURE
find_hunk (struct comparison *cmp, struct change *start)
{
  struct change *prev;
  lin top0, top1;
  lin thresh;

  /* Threshold distance is CONTEXT if the second change is ignorable,
     2 * CONTEXT + 1 otherwise.  Integer overflow can't happen, due
     to CONTEXT_LIM.  */
  lin ignorable_threshold = cmp->context;
  lin non_ignorable_threshold = 2 * cmp->context + 1;

  do
    {
      /* Compute number of first line in each file beyond this changed.  */
      top0 = start->line0 + start->deleted;
      top1 = start->line1 + start->inserted;
      prev = start;
      start = start->link;
      thresh = (start && start->ignore
                ? ignorable_threshold
                : non_ignorable_threshold);
      /* It is not supposed to matter which file we check in the end-test.
      If it would matter, crash.  */
      if (start && start->line0 - top0 != start->line1 - top1)
        abort ();
    } while (start
         /* Keep going if less than THRESH lines
         elapse before the affected line.  */
         && start->line0 - top0 < thresh);

  return prev;
}

/* Set the 'ignore' flag properly in each change in SCRIPT.
   It should be 1 if all the lines inserted or deleted in that change
   are ignorable lines.  */

static void
mark_ignorable (struct comparison *cmp, struct change *script)
{
  while (script)
    {
      struct change *next = script->link;
      lin first0, last0, first1, last1;

      /* Turn this change into a hunk: detach it from the others.  */
      script->link = NULL;

      /* Determine whether this change is ignorable.  */
      script->ignore = ! analyze_hunk (cmp, script,
                                       &first0, &last0, &first1, &last1);

      /* Reconnect the chain as before.  */
      script->link = next;

      /* Advance to the following change.  */
      script = next;
    }
}

#if USE_GNU_REGEX(1)
/* Find the last function-header line in LINBUF prior to line number LINENUM.
   This is a line containing a match for the regexp in 'function_regexp'.
   Return the address of the text, or NULL if no function-header is found.  */

static char const *
find_function (char const * const *linbuf, lin linenum)
{
  lin i = linenum;
  lin last = find_function_last_search;
  find_function_last_search = i;

  while (last <= --i)
    {
      /* See if this line is what we want.  */
      char const *line = linbuf[i];
      size_t linelen = linbuf[i + 1] - line - 1;

      /* FIXME: re_search's size args should be size_t, not int.  */
      int len = MIN (linelen, INT_MAX);

      if (0 <= re_search (&function_regexp, line, len, 0, len, NULL))
        {
          find_function_last_match = i;
          return line;
        }
    }
  /* If we search back to where we started searching the previous time,
     find the line we found last time.  */
  if (find_function_last_match != LIN_MAX)
    return linbuf[find_function_last_match];

  return NULL;
}
#endif
