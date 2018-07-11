/////////////////////////////////////////////////////////////////////////////
//    License (GPLv3+):
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 3 of the License, or (at
//    your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include <diff.h>
#include <xalloc.h>

const char allocated_buffer_name[] = "<allocated_buffer>";

/* malloc a block of memory, with fatal error message if we can't do it. */

void *
xmalloc (size_t size)
{
  register void *value;

  if (size == 0)
    size = 1;

  value = malloc (size);

  if (!value)
    xalloc_die ();
  return value;
}

/* realloc a block of memory, with fatal error message if we can't do it. */

void *
xrealloc (void *old, size_t size)
{
  register void *value;

  if (size == 0)
    size = 1;

  value = realloc (old, size);

  if (!value)
    xalloc_die ();
  return value;
}
