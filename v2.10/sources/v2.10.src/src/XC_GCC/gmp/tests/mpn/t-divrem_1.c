/* Test mpn_divrem_1 and mpn_preinv_divrem_1.

Copyright 2003 Free Software Foundation, Inc.

This file is part of the GNU MP Library.

The GNU MP Library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 3 of the License, or (at your
option) any later version.

The GNU MP Library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with the GNU MP Library.  If not, see http://www.gnu.org/licenses/.  */

#include <stdio.h>
#include <stdlib.h>

#include "gmp.h"
#include "gmp-impl.h"
#include "tests.h"


void
check_data (void)
{
  static const struct {
    mp_limb_t  n[1];
    mp_size_t  nsize;
    mp_limb_t  d;
    mp_size_t  qxn;
    mp_limb_t  want_q[5];
    mp_limb_t  want_r;
  } data[] = {
    { { 0 }, 1, 1, 0,
      { 0 }, 0},

    { { 5 }, 1, 2, 0,
      { 2 }, 1},

#if GMP_NUMB_BITS == 32
    { { 0x3C }, 1, 0xF2, 1,
      { 0x3F789854, 0 }, 0x98 },
#endif

#if GMP_NUMB_BITS == 64
    { { 0x3C }, 1, 0xF2, 1,
      { CNST_LIMB(0x3F789854A0CB1B81), 0 }, 0x0E },

    /* This case exposed some wrong code generated by SGI cc on mips64 irix
       6.5 with -n32 -O2, in the fractional loop for normalized divisor
       using udiv_qrnnd_preinv.  A test "x>al" in one of the sub_ddmmss
       expansions came out wrong, leading to an incorrect quotient.  */
    { { CNST_LIMB(0x3C00000000000000) }, 1, CNST_LIMB(0xF200000000000000), 1,
      { CNST_LIMB(0x3F789854A0CB1B81), 0 }, CNST_LIMB(0x0E00000000000000) },
#endif
  };

  mp_limb_t  dinv, got_r, got_q[numberof(data[0].want_q)];
  mp_size_t  qsize;
  int        i, shift;

  for (i = 0; i < numberof (data); i++)
    {
      qsize = data[i].nsize + data[i].qxn;
      ASSERT_ALWAYS (qsize <= numberof (got_q));

      got_r = mpn_divrem_1 (got_q, data[i].qxn, data[i].n, data[i].nsize,
                            data[i].d);
      if (got_r != data[i].want_r
          || refmpn_cmp (got_q, data[i].want_q, qsize) != 0)
        {
          printf        ("mpn_divrem_1 wrong at data[%d]\n", i);
        bad:
          mpn_trace     ("  n", data[i].n, data[i].nsize);
          printf        ("  nsize=%ld\n", (long) data[i].nsize);
          mp_limb_trace ("  d", data[i].d);
          printf        ("  qxn=%ld\n", (long) data[i].qxn);
          mpn_trace     ("  want q", data[i].want_q, qsize);
          mpn_trace     ("  got  q", got_q, qsize);
          mp_limb_trace ("  want r", data[i].want_r);
          mp_limb_trace ("  got  r", got_r);
          abort ();
        }

      /* test if available */
#if USE_PREINV_DIVREM_1 || HAVE_NATIVE_mpn_preinv_divrem_1
      shift = refmpn_count_leading_zeros (data[i].d);
      dinv = refmpn_invert_limb (data[i].d << shift);
      got_r = mpn_preinv_divrem_1 (got_q, data[i].qxn,
                                   data[i].n, data[i].nsize,
                                   data[i].d, dinv, shift);
      if (got_r != data[i].want_r
          || refmpn_cmp (got_q, data[i].want_q, qsize) != 0)
        {
          printf        ("mpn_preinv divrem_1 wrong at data[%d]\n", i);
          printf        ("  shift=%d\n", shift);
          mp_limb_trace ("  dinv", dinv);
          goto bad;
        }
#endif
    }
}

int
main (void)
{
  tests_start ();
  mp_trace_base = -16;

  check_data ();

  tests_end ();
  exit (0);
}
