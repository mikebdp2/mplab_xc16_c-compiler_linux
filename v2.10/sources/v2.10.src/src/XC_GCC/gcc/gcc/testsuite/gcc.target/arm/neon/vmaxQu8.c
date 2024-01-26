/* Test the `vmaxQu8' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do assemble } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-save-temps -O0" } */
/* { dg-add-options arm_neon } */

#include "arm_neon.h"

void test_vmaxQu8 (void)
{
  uint8x16_t out_uint8x16_t;
  uint8x16_t arg0_uint8x16_t;
  uint8x16_t arg1_uint8x16_t;

  out_uint8x16_t = vmaxq_u8 (arg0_uint8x16_t, arg1_uint8x16_t);
}

/* { dg-final { scan-assembler "vmax\.u8\[ 	\]+\[qQ\]\[0-9\]+, \[qQ\]\[0-9\]+, \[qQ\]\[0-9\]+!?\(\[ 	\]+@.*\)?\n" } } */
/* { dg-final { cleanup-saved-temps } } */
