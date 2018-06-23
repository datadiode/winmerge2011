/* Normal-format output routines for GNU DIFF.

   Copyright (C) 1988-1989, 1993, 1995, 1998, 2001, 2006, 2009-2013, 2015-2017
   Free Software Foundation, Inc.

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

static void print_normal_hunk (struct comparison *, struct file_cursor *, struct change *);

/* Print the edit-script SCRIPT as a normal diff.
   INF points to an array of descriptions of the two files.  */

void
print_normal_script (struct comparison *cmp, struct file_cursor *cursors, struct change *script)
{
  print_script (cmp, cursors, script, find_change, print_normal_hunk);
}

/* Print a hunk of a normal diff.
   This is a contiguous portion of a complete edit script,
   describing changes in consecutive lines.  */

static void
print_normal_hunk (struct comparison *cmp, struct file_cursor *cursors, struct change *hunk)
{
  lin first0, last0, first1, last1;
  register lin i;

  /* Determine range of line numbers involved in each file.  */
  enum changes changes = analyze_hunk (cmp, hunk, &first0, &last0, &first1, &last1);
  if (!changes)
    return;

  begin_output (cmp);

  /* Print out the line number header for this hunk */
  print_number_range (',', &cmp->file[0], first0, last0, cmp->outfile);
  fputc (change_letter[changes], cmp->outfile);
  print_number_range (',', &cmp->file[1], first1, last1, cmp->outfile);
  fputc ('\n', cmp->outfile);

  /* Print the lines that the first file has.  */
  if (changes & OLD)
    {
      for (i = first0; i <= last0; i++)
        {
          print_1_line (cmp, &cursors[0], "<", cmp->file[0].prefix_lines + i, iseolch(*cmp->file[0].linbuf[i]));
        }
    }

  if (changes == CHANGED)
    fputs ("---\n", cmp->outfile);

  /* Print the lines that the second file has.  */
  if (changes & NEW)
    {
      for (i = first1; i <= last1; i++)
        {
          print_1_line (cmp, &cursors[1], ">", cmp->file[1].prefix_lines + i, iseolch(*cmp->file[1].linbuf[i]));
        }
    }
}
