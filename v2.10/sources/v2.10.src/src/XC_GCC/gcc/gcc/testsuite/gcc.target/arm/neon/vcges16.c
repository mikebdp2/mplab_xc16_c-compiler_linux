/* Test the `vcges16' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do assemble } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-save-temps -O0" } */
/* { dg-add-options arm_neon } */

#include "arm_neon.h"

void test_vcges16 (void)
{
  uint16x4_t out_uint16x4_t;
  int16x4_t arg0_int16x4_t;
  int16x4_t arg1_int16x4_t;

  out_uint16x4_t = vcge_s16 (arg0_int16x4_t, arg1_int16x4_t);
}

/* { dg-final { scan-assembler "vcge\.s16\[ 	\]+\[dD\]\[0-9\]+, \[dD\]\[0-9\]+, \[dD\]\[0-9\]+!?\(\[ 	\]+@.*\)?\n" } } */
/* { dg-final { cleanup-saved-temps } } */
