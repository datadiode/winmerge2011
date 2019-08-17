/* File I/O for GNU DIFF.

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

#include "diff.h"
#include <cmpbuf.h>
#include <xalloc.h>

/* Rotate an unsigned value to the left.  */
#define ROL(v, n) ((v) << (n) | (v) >> (sizeof (v) * CHAR_BIT - (n)))

/* Given a hash value and a new character, return a new hash value.  */
#define HASH(h, c) ((c) + ROL (h, 7))

/* The type of a hash value.  */
typedef unsigned hash_value;
verify (! TYPE_SIGNED (hash_value));

#include <common/unicoder.h> /* DetermineEncoding() */

size_t apply_prediffer (struct comparison *cmp, short side, char *buffer, size_t length);

/* Lines are put into equivalence classes of lines that match in lines_differ.
   Each equivalence class is represented by one of these structures,
   but only while the classes are being computed.
   Afterward, each class is represented by a number.  */
struct equivclass
{
  lin next;		/* Next item in this bucket.  */
  hash_value hash;	/* Hash of lines in this class.  */
  char const *line;	/* A line that fits this class.  */
  size_t length;	/* That line's length, not counting its newline.  */
};

/** @brief Get unicode signature from file_data. */
static void get_unicode_signature (struct file_data *current)
{
  /* Prevent bogus BOMs from crashing in-place transcoding upon Rescan(). */
  if (current->name == allocated_buffer_name &&
      current->buffered + sizeof (word) + 1 == current->bufsize)
    {
      current->sig = UTF8;
      current->bom = 0;
    }
  else
    {
      current->sig = DetermineEncoding ((unsigned char *) FILE_BUFFER (current), current->buffered, current->buffered, &current->bom);
    }
}

/* Read a block of data into a file buffer, checking for EOF and error.  */

unsigned
file_block_read (struct file_data *current, size_t size)
{
  unsigned block_size = (unsigned)MIN (size, STAT_BLOCKSIZE (current->stat));
  if (block_size)
    {
      int s = _read (current->desc,
                     FILE_BUFFER (current) + current->buffered, block_size);
      if (s == -1)
        pfatal_with_name (current->name);
      current->buffered += s;
      block_size = s;
    }
  return block_size;
}

/* Check for binary files and compare them for exact identity.  */

/* Return 1 if BUF contains a non text character.
   SIZE is the number of characters in BUF.  */

#define binary_file_p(buf, size) (memchr (buf, 0, size) != 0)

/* Get ready to read the current file.
   Return nonzero if SKIP_TEST is zero,
   and if it appears to be a binary file.  */

static bool
sip (struct file_data *current, bool skip_test)
{
  bool isbinary = false;
  /* If we have a nonexistent file at this stage, treat it as empty.  */
  if (current->name == allocated_buffer_name)
    /* nothing to do */
    ;
  else if (current->desc < 0)
    {
      /* Leave room for a sentinel.  */
      current->bufsize = sizeof (word);
      current->buffer = (word *) xmalloc (current->bufsize);
      current->buffered = 0;
    }
  else
    {
      current->bufsize = buffer_lcm (sizeof (word),
                                     STAT_BLOCKSIZE (current->stat),
                                     PTRDIFF_MAX - 2 * sizeof (word));
      current->buffer = (word *) xmalloc (current->bufsize);
      current->buffered = 0;
      file_block_read (current, current->bufsize);
    }

  get_unicode_signature (current);

  if (!skip_test && !current->sig)
    {
      /* Check first part of file to see if it's a binary file.  */
      isbinary = binary_file_p (current->buffer, current->buffered);
    }
  return isbinary;
}

/* Slurp the rest of the current file completely into memory.  */

static void
slurp (struct file_data *current)
{
  if (current->name == allocated_buffer_name)
    /* nothing to do */
    ;
  else if (current->desc < 0)
    /* The file is nonexistent.  */
    ;
  else if (current->buffered != 0)
    {
      size_t const alloc_extra
      = ((1 << current->sig) & ((1 << UCS2LE) | (1 << UCS2BE) | (1 << UCS4LE) | (1 << UCS4BE)))
        // some flavor of non octet encoded unicode?
        ? ~0U	// yes, allocate extra room for transcoding
        : 0U;	// no, allocate no extra room for transcoding

      for (;;)
        {
          if (current->buffered == current->bufsize)
            {
              if (S_ISREG (current->stat.st_mode))
                {
                  /* Get the size out of the stat block.
                     Allocate 50% extra room for a necessary transcoding to UTF-8.
                     Allocate enough room for appended newline and sentinel.
                     Allocate at least one block, to prevent overrunning the buffer
                     when comparing growing binary files. */
                  current->bufsize = MAX (current->bufsize,
                                          (size_t) current->stat.st_size + (alloc_extra & (size_t) current->stat.st_size / 2) + sizeof (word) + 1);
                }
              else
                {
                  current->bufsize = current->bufsize * 2;
                }
              current->buffer = (word *) xrealloc (current->buffer, current->bufsize);
            }
          if (file_block_read (current, current->bufsize - current->buffered) == 0)
            break;
        }
      /* Allocate 50% extra room for a necessary transcoding to UTF-8.
         Allocate enough room for appended newline and sentinel. */
      current->bufsize = current->buffered + (alloc_extra & current->buffered / 2) + sizeof (word) + 1;
      current->buffer = (word *) xrealloc (current->buffer, current->bufsize);
    }
}

/* Split the file into lines, simultaneously computing the equivalence
   class for each line.  */

static void
find_and_hash_each_line (struct comparison *cmp, struct file_data *current)
{
  char const *p = current->prefix_end;
  lin i, *bucket;
  size_t length;

  /* Cache often-used quantities in local variables to help the compiler.  */
  char const **linbuf = current->linbuf;
  lin alloc_lines = current->alloc_lines;
  lin line = 0;
  lin linbuf_base = current->linbuf_base;
  lin *cureqs = (lin *) xmalloc (alloc_lines * sizeof *cureqs);
  struct equivclass *eqs = cmp->equivs;
  lin eqs_index = cmp->equivs_index;
  lin eqs_alloc = cmp->equivs_alloc;
  char const *suffix_begin = current->suffix_begin;
  char const *bufend = FILE_BUFFER (current) + current->buffered;
  unsigned const tabsize = cmp->tabsize;
  lin const context = cmp->context;
  bool const ig_case = cmp->ignore_case;
  enum DIFF_white_space const ig_white_space = cmp->ignore_white_space;
  bool const diff_length_compare_anyway =
    ig_white_space != IGNORE_NO_WHITE_SPACE;
  bool const same_length_diff_contents_compare_anyway =
    diff_length_compare_anyway | ig_case;
  char const eol_diff = cmp->ignore_eol_diff ? '\r' : 'r';

  /* prepare_text put a zero word at the end of the buffer,
     so we're not in danger of overrunning the end of the file */

  while (p < suffix_begin)
    {
      char const *ip = p;
      hash_value h = 0;
      unsigned char c;

      /* Compute the equivalence class (hash) for this line.  */

      /* loops advance pointer to eol (end of line)
         respecting UNIX (\r), MS-DOS/Windows (\r\n), and MAC (\r) eols */

      /* Hash this line until we find a newline.  */
      /* Note that \r must be hashed (if !ignore_eol_diff) */
      switch (ig_white_space)
        {
        case IGNORE_ALL_SPACE:
          while ((c = *p++) != '\n' && (c != '\r' || *p == '\n'))
            if (! isspace (c) || c == eol_diff)
              h = HASH (h, ig_case ? tolower (c) : c);
          break;

        case IGNORE_SPACE_CHANGE:
          while ((c = *p++) != '\n' && (c != '\r' || *p == '\n'))
            {
              if (isspace (c) && c != eol_diff)
                {
                  do
                    if ((c = *p++) == '\n')
                      goto hashing_done;
                  while (isspace (c));

                  h = HASH (h, ' ');
                }

              /* C is now the first non-space.  */
              h = HASH (h, ig_case ? tolower (c) : c);
            }
          break;

        case IGNORE_TAB_EXPANSION:
        case IGNORE_TAB_EXPANSION_AND_TRAILING_SPACE:
        case IGNORE_TRAILING_SPACE:
          {
            unsigned column = 0;
            while ((c = *p++) != '\n' && (c != '\r' || *p == '\n'))
              {
                if (ig_white_space & IGNORE_TRAILING_SPACE
                    && isspace (c) && c != eol_diff)
                  {
                    char const *p1 = p;
                    unsigned char c1;
                    do
                      if ((c1 = *p1++) == '\n' || (c1 == '\r' && *p1 != '\n'))
                        {
                          p = p1;
                          goto hashing_done;
                        }
                    while (isspace (c1) && c1 != eol_diff);
                  }

                unsigned repetitions = 1;

                if (ig_white_space & IGNORE_TAB_EXPANSION)
                  switch (c)
                  {
                    case '\b':
                      column -= 0 < column;
                      break;

                    case '\t':
                      c = ' ';
                      repetitions = tabsize - column % tabsize;
                      column = (column + repetitions < column
                                ? 0
                                : column + repetitions);
                      break;

                    case '\r':
                      column = 0;
                      break;

                    default:
                      column++;
                      break;
                    }

                if (ig_case)
                  c = tolower (c);

                do
                  h = HASH (h, c);
                while (--repetitions != 0);
              }
          }
          break;

        default:
          if (ig_case)
            while ((c = *p++) != '\n' && (c != '\r' || *p == '\n'))
              h = HASH (h, tolower (c));
          else
            while ((c = *p++) != '\n' && (c != '\r' || *p == '\n'))
              h = HASH (h, c);
          break;
        }

    hashing_done:;

      bucket = &cmp->buckets[h % cmp->nbuckets];
      length = p - ip - 1;

      if (p == bufend
          && current->missing_newline
          && ROBUST_OUTPUT_STYLE (output_style))
        {
          /* The last line is incomplete and we do not silently
             complete lines.  If the line cannot compare equal to any
             complete line, put it into buckets[-1] so that it can
             compare equal only to the other file's incomplete line
             (if one exists).  */
          bucket = &cmp->buckets[-1]; /* WinMerge: regardless of ig_white_space */
        }

      for (i = *bucket;  ;  i = eqs[i].next)
        if (!i)
          {
            /* Create a new equivalence class in this bucket.  */
            i = eqs_index++;
            if (i == eqs_alloc)
              {
                if (PTRDIFF_MAX / (2 * sizeof *eqs) <= eqs_alloc)
                  xalloc_die ();
                eqs_alloc *= 2;
                eqs = (struct equivclass *) xrealloc (eqs, eqs_alloc * sizeof *eqs);
              }
            eqs[i].next = *bucket;
            eqs[i].hash = h;
            eqs[i].line = ip;
            eqs[i].length = length;
            *bucket = i;
            break;
          }
        else if (eqs[i].hash == h)
          {
            char const *eqline = eqs[i].line;

            /* Reuse existing class if lines_differ reports the lines
               equal.  */
            if (eqs[i].length == length)
              {
                /* Reuse existing equivalence class if the lines are identical.
                   This detects the common case of exact identity
                   faster than lines_differ would.  */
                if (memcmp (eqline, ip, length) == 0)
                  break;
                if (!same_length_diff_contents_compare_anyway)
                  continue;
              }
            else if (!diff_length_compare_anyway)
              continue;

            if (! lines_differ (cmp, eqline, ip))
              break;
          }

      /* Maybe increase the size of the line table.  */
      if (line == alloc_lines)
        {
          /* Double (alloc_lines - linbuf_base) by adding to alloc_lines.  */
          if (PTRDIFF_MAX / 3 <= alloc_lines
              || PTRDIFF_MAX / sizeof *cureqs <= 2 * alloc_lines - linbuf_base
              || PTRDIFF_MAX / sizeof *linbuf <= alloc_lines - linbuf_base)
            xalloc_die ();
          alloc_lines = 2 * alloc_lines - linbuf_base;
          cureqs = (lin *) xrealloc (cureqs, alloc_lines * sizeof *cureqs);
          linbuf += linbuf_base;
          linbuf = (char const **) xrealloc ((void *) linbuf,
                                             (alloc_lines - linbuf_base) * sizeof *linbuf);
          linbuf -= linbuf_base;
        }
      linbuf[line] = ip;
      cureqs[line] = i;
      ++line;
    }

  current->buffered_lines = line;

  for (i = 0;  ;  i++)
    {
      /* Record the line start for lines in the suffix that we care about.
         Record one more line start than lines,
         so that we can compute the length of any buffered line.  */
      if (line == alloc_lines)
        {
          /* Double (alloc_lines - linbuf_base) by adding to alloc_lines.  */
          if (PTRDIFF_MAX / 3 <= alloc_lines
              || PTRDIFF_MAX / sizeof *cureqs <= 2 * alloc_lines - linbuf_base
              || PTRDIFF_MAX / sizeof *linbuf <= alloc_lines - linbuf_base)
            xalloc_die ();
          alloc_lines = 2 * alloc_lines - linbuf_base;
          linbuf += linbuf_base;
          linbuf = (char const **) xrealloc ((void *) linbuf,
                                             (alloc_lines - linbuf_base) * sizeof *linbuf);
          linbuf -= linbuf_base;
        }
      linbuf[line] = p;

      if (p == bufend)
        {
          /* If the last line is incomplete and we do not silently
             complete lines, don't count its appended newline.  */
          if (current->missing_newline && ROBUST_OUTPUT_STYLE (output_style))
            linbuf[line]--;
          break;
        }

      if (context <= i && cmp->no_diff_means_no_output)
        break;

      line++;

      while (*p++ != '\n' && (p[-1] != '\r' || p[0] == '\n'))
        continue;
    }

  /* Done with cache in local variables.  */
  current->linbuf = linbuf;
  current->valid_lines = line;
  current->alloc_lines = alloc_lines;
  current->equivs = cureqs;
  cmp->equivs = eqs;
  cmp->equivs_alloc = eqs_alloc;
  cmp->equivs_index = eqs_index;
}

/* Convert any non octet encoded unicode text to UTF-8.
   Prepare the end of the text. Make sure it's initialized.
   Make sure text ends in a newline,
   but remember that we had to add one.
   Strip trailing CRs, if that was requested.  */

static char *
prepare_text (struct comparison *cmp, struct file_data *current, short side)
{
  size_t buffered = current->buffered;
  char *const p = FILE_BUFFER (current);
  char *r = p; // receives the return value
  char *q, *t;
  UNICODESET const sig = current->sig;
  char *const u = p + current->bom;

  if (sig == UCS4LE)
    {
      size_t buffered_words = buffered / 2;
      unsigned long *q = (unsigned long *)p + buffered_words / 2;
      buffered += buffered_words;
      r = p + buffered;
      while (--q >= (unsigned long *)u) // exclude the BOM
        {
          unsigned long u = *q;
          if (u >= 0x80000000)
            {
              *--r = '?';
            }
          else if (u >= 0x4000000)
            {
              *--r = 0x80 + (u & 0x3F);
              *--r = 0x80 + ((u >> 6) & 0x3F);
              *--r = 0x80 + ((u >> 12) & 0x3F);
              *--r = 0x80 + ((u >> 18) & 0x3F);
              *--r = 0x80 + ((u >> 24) & 0x3F);
              *--r = 0xFC + (u >> 30);
            }
          else if (u >= 0x200000)
            {
              *--r = 0x80 + (u & 0x3F);
              *--r = 0x80 + ((u >> 6) & 0x3F);
              *--r = 0x80 + ((u >> 12) & 0x3F);
              *--r = 0x80 + ((u >> 18) & 0x3F);
              *--r = 0xF8 + (u >> 24);
            }
          else if (u >= 0x10000)
            {
              *--r = 0x80 + (u & 0x3F);
              *--r = 0x80 + ((u >> 6) & 0x3F);
              *--r = 0x80 + ((u >> 12) & 0x3F);
              *--r = 0xF0 + (char)(u >> 18);
            }
          else if (u >= 0x800)
            {
              *--r = 0x80 + (u & 0x3F);
              *--r = 0x80 + ((u >> 6) & 0x3F);
              *--r = 0xE0 + (char)(u >> 12);
            }
          else if (u >= 0x80)
            {
              *--r = 0x80 + (u & 0x3F);
              *--r = 0xC0 + (char)(u >> 6);
            }
          else
            {
              *--r = (char)u;
            }
        }
    }
  else if (sig == UCS4BE)
    {
      size_t buffered_words = buffered / 2;
      unsigned long *q = (unsigned long *)p + buffered_words / 2;
      buffered += buffered_words;
      r = p + buffered;
      while (--q >= (unsigned long *)u) // exclude the BOM
        {
          unsigned long u =
            ((*q & 0x000000FF) << 24) |
            ((*q & 0x0000FF00) << 8) |
            ((*q & 0x00FF0000) >> 8) |
            ((*q & 0xFF000000) >> 24); // fix byte order
          if (u >= 0x80000000)
            {
              *--r = '?';
            }
          else if (u >= 0x4000000)
            {
              *--r = 0x80 + (u & 0x3F);
              *--r = 0x80 + ((u >> 6) & 0x3F);
              *--r = 0x80 + ((u >> 12) & 0x3F);
              *--r = 0x80 + ((u >> 18) & 0x3F);
              *--r = 0x80 + ((u >> 24) & 0x3F);
              *--r = 0xFC + (u >> 30);
            }
          else if (u >= 0x200000)
            {
              *--r = 0x80 + (u & 0x3F);
              *--r = 0x80 + ((u >> 6) & 0x3F);
              *--r = 0x80 + ((u >> 12) & 0x3F);
              *--r = 0x80 + ((u >> 18) & 0x3F);
              *--r = 0xF8 + (u >> 24);
            }
          else if (u >= 0x10000)
            {
              *--r = 0x80 + (u & 0x3F);
              *--r = 0x80 + ((u >> 6) & 0x3F);
              *--r = 0x80 + ((u >> 12) & 0x3F);
              *--r = 0xF0 + (char)(u >> 18);
            }
          else if (u >= 0x800)
            {
              *--r = 0x80 + (u & 0x3F);
              *--r = 0x80 + ((u >> 6) & 0x3F);
              *--r = 0xE0 + (char)(u >> 12);
            }
          else if (u >= 0x80)
            {
              *--r = 0x80 + (u & 0x3F);
              *--r = 0xC0 + (char)(u >> 6);
            }
          else
            {
              *--r = (char)u;
            }
        }
    }
  else if (sig == UCS2LE)
    {
      size_t buffered_words = buffered / 2;
      unsigned short *q = (unsigned short *)p + buffered_words;
      buffered += buffered_words;
      r = p + buffered;
      while (--q >= (unsigned short *)u) // exclude the BOM
        {
          unsigned short u = *q;
          if (u >= 0x800)
            {
              *--r = 0x80 + (u & 0x3F);
              *--r = 0x80 + ((u >> 6) & 0x3F);
              *--r = 0xE0 + (u >> 12);
            }
          else if (u >= 0x80)
            {
              *--r = 0x80 + (u & 0x3F);
              *--r = 0xC0 + (u >> 6);
            }
          else
            {
              *--r = (char)u;
            }
        }
    }
  else if (sig == UCS2BE)
    {
      size_t buffered_words = buffered / 2;
      unsigned short *q = (unsigned short *)p + buffered_words;
      buffered += buffered_words;
      r = p + buffered;
      while (--q >= (unsigned short *)u) // exclude the BOM
        {
          unsigned short u = (*q << 8) | (*q >> 8); // fix byte order
          if (u >= 0x800)
            {
              *--r = 0x80 + (u & 0x3F);
              *--r = 0x80 + ((u >> 6) & 0x3F);
              *--r = 0xE0 + (u >> 12);
            }
          else if (u >= 0x80)
            {
              *--r = 0x80 + (u & 0x3F);
              *--r = 0xC0 + (u >> 6);
            }
          else
            {
              *--r = (char)u;
            }
        }
    }
  else if (sig == UTF8)
    {
      r = u; // skip the BOM
    }

  if (buffered == 0 || p[buffered - 1] == '\n' || p[buffered - 1] == '\r')
    current->missing_newline = false;
  else
    {
      p[buffered++] = '\n';
      current->missing_newline = true;
      --current->count_lfs; // compensate for extra newline
    }

  if (side != -1)
    buffered = apply_prediffer (cmp, side, p, buffered);

  current->buffered = buffered;

  /* Count line endings and map them to '\n' if ignore_eol_diff is set. */
  t = q = p + buffered;
  while (q > r)
    {
      switch (*--t = *--q)
        {
        case '\r':
          ++current->count_crs;
          if (cmp->ignore_eol_diff)
            *t = '\n';
          break;
        case '\n':
          if (q > r && q[-1] == '\r')
            {
              ++current->count_crlfs;
              --current->count_crs; // compensate for bogus increment
              if (cmp->ignore_eol_diff)
                ++t;
            }
          else
            {
              ++current->count_lfs;
            }
          break;
        case '\0':
          ++current->count_zeros;
          break;
        }
    }

  /* Don't use uninitialized storage when planting or using sentinels.  */
  *(word *)(p + buffered) = 0;
  return t;
}

/* We have found N lines in a buffer of size S; guess the
   proportionate number of lines that will be found in a buffer of
   size T.  However, do not guess a number of lines so large that the
   resulting line table might cause overflow in size calculations.  */
static lin
guess_lines (lin n, size_t s, size_t t)
{
  size_t guessed_bytes_per_line = n < 10 ? 32 : s / (n - 1);
  size_t guessed_lines = MAX (1, t / guessed_bytes_per_line);
  return (lin)(MIN (guessed_lines, LIN_MAX / (2 * sizeof (char *) + 1) - 5) + 5);
}

/* Given a vector of two file_data objects, find the identical
   prefixes and suffixes of each object.  */

static void
find_identical_ends (struct comparison *cmp)
{
  word *w0, *w1;
  char *p0, *p1, *buffer0, *buffer1;
  char const *end0, *beg0;
  char const **linbuf0, **linbuf1;
  lin i, lines;
  size_t n0, n1;
  lin alloc_lines0, alloc_lines1;
  bool prefix_needed;
  lin buffered_prefix, prefix_count, prefix_mask;
  lin middle_guess, suffix_guess;
  lin const context = cmp->context;

  if (cmp->file[0].desc != cmp->file[1].desc)
    {
      slurp (&cmp->file[0]);
      buffer0 = prepare_text (cmp, &cmp->file[0], 0);
      slurp (&cmp->file[1]);
      buffer1 = prepare_text (cmp, &cmp->file[1], 1);
    }
  else
    {
      slurp (&cmp->file[0]);
      buffer0 = buffer1 = prepare_text (cmp, &cmp->file[0], -1);
      cmp->file[1].buffer = cmp->file[0].buffer;
      cmp->file[1].bufsize = cmp->file[0].bufsize;
      cmp->file[1].buffered = cmp->file[0].buffered;
      cmp->file[1].missing_newline = cmp->file[0].missing_newline;
    }

  /* Find identical prefix.  */

  p0 = buffer0;
  p1 = buffer1;

  n0 = cmp->file[0].buffered - (buffer0 - FILE_BUFFER (&cmp->file[0]));
  n1 = cmp->file[1].buffered - (buffer1 - FILE_BUFFER (&cmp->file[1]));

  if (p0 == p1)
    /* The buffers are the same; sentinels won't work.  */
    p0 = p1 += n1;
  else
    {
      /* Insert end sentinels, in this case characters that are guaranteed
         to make the equality test false, and thus terminate the loop.  */

      if (n0 < n1)
        p0[n0] = ~p1[n0];
      else
        p1[n1] = ~p0[n1];

      /* Loop until first mismatch, or to the sentinel characters.  */

      /* Do bytewise comparison until p0 is aligned on word boundary.  */
      while (((size_t)p0 & (sizeof(word) - 1)) && *p0 == *p1)
        p0++, p1++;

      /* Compare a word at a time for speed.  */
      w0 = (word *) p0;
      w1 = (word *) p1;
      while (*w0 == *w1)
        w0++, w1++;

      /* Do the last few bytes of comparison a byte at a time.  */
      p0 = (char *) w0;
      p1 = (char *) w1;
      while (*p0 == *p1)
        p0++, p1++;

      /* Don't mistakenly count missing newline as part of prefix.  */
      if (ROBUST_OUTPUT_STYLE (output_style)
          && ((buffer0 + n0 - cmp->file[0].missing_newline < p0)
              !=
              (buffer1 + n1 - cmp->file[1].missing_newline < p1)))
        p0--, p1--;
    }

  /* Now P0 and P1 point at the first nonmatching characters.  */

  /* Skip back to last line-beginning in the prefix,
     and then discard up to HORIZON_LINES lines from the prefix.  */
  i = cmp->horizon_lines;
  /* This loop can be done in one line, but isn't not easy to read, so unrolled into simple statements */
  while (p0 != buffer0)
    {
      /* we know p0[-1] == p1[-1], but maybe p0[0] != p1[0] */
      int linestart = 0;
      if (p0[-1] == '\n')
        linestart = 1;
      /* only count \r if not followed by a \n on either side */
      if (p0[-1] == '\r' && p0[0] != '\n' && p1[0] != '\n')
        linestart = 1;
      if (linestart && ! (i--))
        break;
      --p0, --p1;
    }

  /* Record the prefix.  */
  cmp->file[0].prefix_end = p0;
  cmp->file[1].prefix_end = p1;

  /* Find identical suffix.  */

  /* P0 and P1 point beyond the last chars not yet compared.  */
  p0 = buffer0 + n0;
  p1 = buffer1 + n1;

  if (! ROBUST_OUTPUT_STYLE (output_style)
      || cmp->file[0].missing_newline == cmp->file[1].missing_newline)
    {
      end0 = p0;  /* Addr of last char in file 0.  */

      /* Get value of P0 at which we should stop scanning backward:
         this is when either P0 or P1 points just past the last char
         of the identical prefix.  */
      beg0 = cmp->file[0].prefix_end + (n0 < n1 ? 0 : n0 - n1);

      /* Scan back until chars don't match or we reach that point.  */
      while (p0 != beg0)
        if (*--p0 != *--p1)
          {
            /* Point at the first char of the matching suffix.  */
            ++p0, ++p1;
            beg0 = p0;
            break;
          }

      /* Are we at a line-beginning in both files?  If not, add the rest of
         this line to the main body.  Discard up to HORIZON_LINES lines from
         the identical suffix.  Also, discard one extra line,
         because shift_boundaries may need it.  */
      i = cmp->horizon_lines + ! ((buffer0 == p0 || p0[-1] == '\n' || (p0[-1] == '\r' && p0[0] != '\n'))
                                  &&
                                  (buffer1 == p1 || p1[-1] == '\n' || (p1[-1] == '\r' && p1[0] != '\n')));
      while (i-- && p0 != end0)
        while (*p0++ != '\n' && (p0[-1] != '\r' || p0[0] == '\n'))
          continue;

      p1 += p0 - beg0;
    }

  /* Record the suffix.  */
  cmp->file[0].suffix_begin = p0;
  cmp->file[1].suffix_begin = p1;

  /* Calculate number of lines of prefix to save.

     prefix_count == 0 means save the whole prefix;
     we need this for options like -D that output the whole file,
     or for enormous contexts (to avoid worrying about arithmetic overflow).
     We also need it for options like -F that output some preceding line;
     at least we will need to find the last few lines,
     but since we don't know how many, it's easiest to find them all.

     Otherwise, prefix_count != 0.  Save just prefix_count lines at start
     of the line buffer; they'll be moved to the proper location later.
     Handle 1 more line than the context says (because we count 1 too many),
     rounded up to the next power of 2 to speed index computation.  */

  if (cmp->no_diff_means_no_output && ! USE_GNU_REGEX(function_regexp.fastmap)
      && context < LIN_MAX / 4 && context < (int) n0)
    {
      middle_guess = guess_lines (0, 0, p0 - cmp->file[0].prefix_end);
      suffix_guess = guess_lines (0, 0, buffer0 + n0 - p0);
      for (prefix_count = 1;  prefix_count <= context;  prefix_count *= 2)
        continue;
      alloc_lines0 = (prefix_count + middle_guess
                      + MIN (context, suffix_guess));
    }
  else
    {
      prefix_count = 0;
      alloc_lines0 = guess_lines (0, 0, n0);
    }

  prefix_mask = prefix_count - 1;
  lines = 0;
  linbuf0 = (char const **) xmalloc (alloc_lines0 * sizeof *linbuf0);
  prefix_needed = ! (cmp->no_diff_means_no_output
                     && cmp->file[0].prefix_end == p0
                     && cmp->file[1].prefix_end == p1);
  p0 = buffer0;

  /* If the prefix is needed, find the prefix lines.  */
  if (prefix_needed)
    {
      end0 = cmp->file[0].prefix_end;
      while (p0 != end0)
        {
          lin l = lines++ & prefix_mask;
          if (l == alloc_lines0)
            {
              if (PTRDIFF_MAX / (2 * sizeof *linbuf0) <= alloc_lines0)
                xalloc_die ();
              alloc_lines0 *= 2;
              linbuf0 = (char const **) xrealloc ((void *) linbuf0, alloc_lines0 * sizeof *linbuf0);
            }
          linbuf0[l] = p0;
          /* Perry/WinMerge (2004-01-05) altered original diffutils loop "while (*p0++ != '\n') ;" for other EOLs */
          while (1)
            {
              char ch = *p0++;
              /* stop at any eol, \n or \r or \r\n */
              if (ch == '\n') break;
              if (ch == '\r' && (p0 == end0 || *p0 != '\n')) break;
            }
        }
    }
  buffered_prefix = prefix_count && context < lines ? context : lines;

  /* Allocate line buffer 1.  */

  middle_guess = guess_lines (lines, p0 - buffer0, p1 - cmp->file[1].prefix_end);
  suffix_guess = guess_lines (lines, p0 - buffer0, buffer1 + n1 - p1);
  alloc_lines1 = buffered_prefix + middle_guess + MIN (context, suffix_guess);
  if (alloc_lines1 < buffered_prefix
      || PTRDIFF_MAX / sizeof *linbuf1 <= alloc_lines1)
    xalloc_die ();
  linbuf1 = (char const **) xmalloc (alloc_lines1 * sizeof (*linbuf1));

  if (buffered_prefix != lines)
    {
      /* Rotate prefix lines to proper location.  */
      for (i = 0;  i < buffered_prefix;  i++)
        linbuf1[i] = linbuf0[(lines - context + i) & prefix_mask];
      for (i = 0;  i < buffered_prefix;  i++)
        linbuf0[i] = linbuf1[i];
    }

  /* Initialize line buffer 1 from line buffer 0.  */
  for (i = 0; i < buffered_prefix; i++)
    linbuf1[i] = linbuf0[i] - buffer0 + buffer1;

  /* Record the line buffer, adjusted so that
     linbuf[0] points at the first differing line.  */
  cmp->file[0].linbuf = linbuf0 + buffered_prefix;
  cmp->file[1].linbuf = linbuf1 + buffered_prefix;
  cmp->file[0].linbuf_base = cmp->file[1].linbuf_base = - buffered_prefix;
  cmp->file[0].alloc_lines = alloc_lines0 - buffered_prefix;
  cmp->file[1].alloc_lines = alloc_lines1 - buffered_prefix;
  cmp->file[0].prefix_lines = cmp->file[1].prefix_lines = lines;
}

/* If 1 < k, then (2**k - prime_offset[k]) is the largest prime less
   than 2**k.  This table is derived from Chris K. Caldwell's list
   <http://www.utm.edu/research/primes/lists/2small/>.  */

static unsigned char const prime_offset[] =
{
  0, 0, 1, 1, 3, 1, 3, 1, 5, 3, 3, 9, 3, 1, 3, 19, 15, 1, 5, 1, 3, 9, 3,
  15, 3, 39, 5, 39, 57, 3, 35, 1, 5, 9, 41, 31, 5, 25, 45, 7, 87, 21,
  11, 57, 17, 55, 21, 115, 59, 81, 27, 129, 47, 111, 33, 55, 5, 13, 27,
  55, 93, 1, 57, 25
};

/* Verify that this host's size_t is not too wide for the above table.  */

verify (sizeof (size_t) * CHAR_BIT <= sizeof prime_offset);

/* Given a vector of two file_data objects, read the file associated
   with each one, and build the table of equivalence classes.
   Return nonzero if either file appears to be a binary file.
   If PRETEND_BINARY is nonzero, pretend they are binary regardless.  */
/* WinMerge: Add int * bin_file param for getting actual binary file
   If bin_file is given, then check both files for binary files,
   otherwise check second file only if first wasn't binary */
bool
read_files (struct comparison *cmp, int *bin_file)
{
  int i;
  bool skip_test = false;
  bool appears_binary = false;

  if (bin_file)
    *bin_file = 0;

  if (sip (&cmp->file[0], skip_test))
    {
      appears_binary = true;
      if (bin_file)
        *bin_file |= 0x1; // set first bit for first file
      else
        skip_test = true; // no need to test the second file
    }

  if (cmp->file[0].desc == cmp->file[1].desc)
    {
      cmp->file[1].buffer = cmp->file[0].buffer;
      cmp->file[1].bufsize = cmp->file[0].bufsize;
      cmp->file[1].buffered = cmp->file[0].buffered;
      cmp->file[1].sig = cmp->file[0].sig;
      cmp->file[1].bom = cmp->file[0].bom;
    }
  else if (sip (&cmp->file[1], skip_test))
    {
      appears_binary = true;
      if (bin_file)
        *bin_file |= 0x2; // set second bit for second file
    }

  find_identical_ends (cmp);

  /* Don't slurp rest of file when comparing file to itself. */
  if (cmp->file[0].desc == cmp->file[1].desc)
    {
      cmp->file[1].count_crs = cmp->file[0].count_crs;
      cmp->file[1].count_lfs = cmp->file[0].count_lfs;
      cmp->file[1].count_crlfs = cmp->file[0].count_crlfs;
      cmp->file[1].count_zeros = cmp->file[0].count_zeros;
      return false;
    }

  if (appears_binary || (bin_file && *bin_file > 0))
    return true;

  cmp->equivs_alloc = cmp->file[0].alloc_lines + cmp->file[1].alloc_lines + 1;
  if (PTRDIFF_MAX / sizeof *cmp->equivs <= cmp->equivs_alloc)
    xalloc_die ();
  cmp->equivs = (struct equivclass *) xmalloc (cmp->equivs_alloc * sizeof *cmp->equivs);
  /* Equivalence class 0 is permanently safe for lines that were not
     hashed.  Real equivalence classes start at 1.  */
  cmp->equivs_index = 1;

  /* Allocate (one plus) a prime number of hash buckets.  Use a prime
     number between 1/3 and 2/3 of the value of equiv_allocs,
     approximately.  */
  for (i = 9; (1 << i) < cmp->equivs_alloc / 3; i++)
    continue;
  cmp->nbuckets = (1 << i) - prime_offset[i];
  if (PTRDIFF_MAX / sizeof *cmp->buckets <= cmp->nbuckets)
    xalloc_die ();
  cmp->buckets = (lin *) zalloc ((cmp->nbuckets + 1) * sizeof *cmp->buckets) + 1;

  for (i = 0; i < 2; i++)
    find_and_hash_each_line (cmp, &cmp->file[i]);

  cmp->file[0].equiv_max = cmp->file[1].equiv_max = cmp->equivs_index;

  free (cmp->equivs);
  cmp->equivs = NULL;
  free (cmp->buckets - 1);
  cmp->buckets = NULL;

  return false;
}
