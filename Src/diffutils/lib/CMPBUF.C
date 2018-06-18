/* Buffer primitives for comparison operations.

   Copyright (C) 1993, 1995, 1998, 2001-2002, 2006, 2009-2013, 2015-2017 Free
   Software Foundation, Inc.

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

#include "system.h"
#include "cmpbuf.h"

/* Least common multiple of two buffer sizes A and B.  However, if
   either A or B is zero, or if the multiple is greater than LCM_MAX,
   return a reasonable buffer size.  */

size_t
buffer_lcm (size_t a, size_t b, size_t lcm_max)
{
  size_t lcm, m, n, q, r;

  /* Yield reasonable values if buffer sizes are zero.  */
  if (!a)
    return b ? b : 8 * 1024;
  if (!b)
    return a;

  /* n = gcd (a, b) */
  for (m = a, n = b;  (r = m % n) != 0;  m = n, n = r)
    continue;

  /* Yield a if there is an overflow.  */
  q = a / n;
  lcm = q * b;
  return lcm <= lcm_max && lcm / b == q ? lcm : a;
}
