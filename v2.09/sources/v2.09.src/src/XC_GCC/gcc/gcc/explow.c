/* Subroutines for manipulating rtx's in semantically interesting ways.
   Copyright (C) 1987, 1991, 1994, 1995, 1996, 1997, 1998,
   1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008
   Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */


#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "toplev.h"
#include "rtl.h"
#include "tree.h"
#include "tm_p.h"
#include "flags.h"
#include "except.h"
#include "function.h"
#include "expr.h"
#include "optabs.h"
#include "hard-reg-set.h"
#include "insn-config.h"
#include "ggc.h"
#include "recog.h"
#include "langhooks.h"
#include "target.h"
#include "output.h"

static rtx break_out_memory_refs (rtx);
static void emit_stack_probe (rtx);


/* Truncate and perhaps sign-extend C as appropriate for MODE.  */

HOST_WIDE_INT
trunc_int_for_mode (HOST_WIDE_INT c, enum machine_mode mode)
{
  int width = GET_MODE_BITSIZE (mode);

  /* You want to truncate to a _what_?  */
  gcc_assert (SCALAR_INT_MODE_P (mode));

  /* Canonicalize BImode to 0 and STORE_FLAG_VALUE.  */
  if (mode == BImode)
    return c & 1 ? STORE_FLAG_VALUE : 0;

  /* Sign-extend for the requested mode.  */

  if (width < HOST_BITS_PER_WIDE_INT)
    {
      HOST_WIDE_INT sign = 1;
      sign <<= width - 1;
      c &= (sign << 1) - 1;
      c ^= sign;
      c -= sign;
    }

  return c;
}

/* Return an rtx for the sum of X and the integer C.  */

rtx
plus_constant (rtx x, HOST_WIDE_INT c)
{
  RTX_CODE code;
  rtx y;
  enum machine_mode mode;
  rtx tem;
  int all_constant = 0;

  if (c == 0)
    return x;

 restart:

  code = GET_CODE (x);
  mode = GET_MODE (x);
  y = x;

  switch (code)
    {
    case CONST_INT:
      return GEN_INT (INTVAL (x) + c);

    case CONST_DOUBLE:
      {
	unsigned HOST_WIDE_INT l1 = CONST_DOUBLE_LOW (x);
	HOST_WIDE_INT h1 = CONST_DOUBLE_HIGH (x);
	unsigned HOST_WIDE_INT l2 = c;
	HOST_WIDE_INT h2 = c < 0 ? ~0 : 0;
	unsigned HOST_WIDE_INT lv;
	HOST_WIDE_INT hv;

	add_double (l1, h1, l2, h2, &lv, &hv);

	return immed_double_const (lv, hv, VOIDmode);
      }

    case MEM:
      /* If this is a reference to the constant pool, try replacing it with
	 a reference to a new constant.  If the resulting address isn't
	 valid, don't return it because we have no way to validize it.  */
      if (GET_CODE (XEXP (x, 0)) == SYMBOL_REF
	  && CONSTANT_POOL_ADDRESS_P (XEXP (x, 0)))
	{
	  tem
	    = force_const_mem (GET_MODE (x),
			       plus_constant (get_pool_constant (XEXP (x, 0)),
					      c));
	  if (memory_address_p (GET_MODE (tem), XEXP (tem, 0)))
	    return tem;
	}
      break;

    case CONST:
      /* If adding to something entirely constant, set a flag
	 so that we can add a CONST around the result.  */
      x = XEXP (x, 0);
      all_constant = 1;
      goto restart;

    case SYMBOL_REF:
    case LABEL_REF:
      all_constant = 1;
      break;

    case PLUS:
      /* The interesting case is adding the integer to a sum.
	 Look for constant term in the sum and combine
	 with C.  For an integer constant term, we make a combined
	 integer.  For a constant term that is not an explicit integer,
	 we cannot really combine, but group them together anyway.

	 Restart or use a recursive call in case the remaining operand is
	 something that we handle specially, such as a SYMBOL_REF.

	 We may not immediately return from the recursive call here, lest
	 all_constant gets lost.  */

      if (CONST_INT_P (XEXP (x, 1)))
	{
	  c += INTVAL (XEXP (x, 1));

	  if (GET_MODE (x) != VOIDmode)
	    c = trunc_int_for_mode (c, GET_MODE (x));

	  x = XEXP (x, 0);
	  goto restart;
	}
      else if (CONSTANT_P (XEXP (x, 1)))
	{
	  x = gen_rtx_PLUS (mode, XEXP (x, 0), plus_constant (XEXP (x, 1), c));
	  c = 0;
	}
      else if (find_constant_term_loc (&y))
	{
	  /* We need to be careful since X may be shared and we can't
	     modify it in place.  */
	  rtx copy = copy_rtx (x);
	  rtx *const_loc = find_constant_term_loc (&copy);

	  *const_loc = plus_constant (*const_loc, c);
	  x = copy;
	  c = 0;
	}
      break;

    default:
      break;
    }

  if (c != 0)
    x = gen_rtx_PLUS (mode, x, GEN_INT (c));

  if (GET_CODE (x) == SYMBOL_REF || GET_CODE (x) == LABEL_REF)
    return x;
  else if (all_constant)
    return gen_rtx_CONST (mode, x);
  else
    return x;
}

/* If X is a sum, return a new sum like X but lacking any constant terms.
   Add all the removed constant terms into *CONSTPTR.
   X itself is not altered.  The result != X if and only if
   it is not isomorphic to X.  */

rtx
eliminate_constant_term (rtx x, rtx *constptr)
{
  rtx x0, x1;
  rtx tem;

  if (GET_CODE (x) != PLUS)
    return x;

  /* First handle constants appearing at this level explicitly.  */
  if (CONST_INT_P (XEXP (x, 1))
      && 0 != (tem = simplify_binary_operation (PLUS, GET_MODE (x), *constptr,
						XEXP (x, 1)))
      && CONST_INT_P (tem))
    {
      *constptr = tem;
      return eliminate_constant_term (XEXP (x, 0), constptr);
    }

  tem = const0_rtx;
  x0 = eliminate_constant_term (XEXP (x, 0), &tem);
  x1 = eliminate_constant_term (XEXP (x, 1), &tem);
  if ((x1 != XEXP (x, 1) || x0 != XEXP (x, 0))
      && 0 != (tem = simplify_binary_operation (PLUS, GET_MODE (x),
						*constptr, tem))
      && CONST_INT_P (tem))
    {
      *constptr = tem;
      return gen_rtx_PLUS (GET_MODE (x), x0, x1);
    }

  return x;
}

/* Return an rtx for the size in bytes of the value of EXP.  */

rtx
expr_size (tree exp)
{
  tree size;

  if (TREE_CODE (exp) == WITH_SIZE_EXPR)
    size = TREE_OPERAND (exp, 1);
  else
    {
      size = tree_expr_size (exp);
      gcc_assert (size);
      gcc_assert (size == SUBSTITUTE_PLACEHOLDER_IN_EXPR (size, exp));
    }

  return expand_expr (size, NULL_RTX, TYPE_MODE (sizetype), EXPAND_NORMAL);
}

/* Return a wide integer for the size in bytes of the value of EXP, or -1
   if the size can vary or is larger than an integer.  */

HOST_WIDE_INT
int_expr_size (tree exp)
{
  tree size;

  if (TREE_CODE (exp) == WITH_SIZE_EXPR)
    size = TREE_OPERAND (exp, 1);
  else
    {
      size = tree_expr_size (exp);
      gcc_assert (size);
    }

  if (size == 0 || !host_integerp (size, 0))
    return -1;

  return tree_low_cst (size, 0);
}

/* Return a copy of X in which all memory references
   and all constants that involve symbol refs
   have been replaced with new temporary registers.
   Also emit code to load the memory locations and constants
   into those registers.

   If X contains no such constants or memory references,
   X itself (not a copy) is returned.

   If a constant is found in the address that is not a legitimate constant
   in an insn, it is left alone in the hope that it might be valid in the
   address.

   X may contain no arithmetic except addition, subtraction and multiplication.
   Values returned by expand_expr with 1 for sum_ok fit this constraint.  */

static rtx
break_out_memory_refs (rtx x)
{
  if (MEM_P (x)
      || (CONSTANT_P (x) && CONSTANT_ADDRESS_P (x)
	  && GET_MODE (x) != VOIDmode))
    x = force_reg (GET_MODE (x), x);
  else if (GET_CODE (x) == PLUS || GET_CODE (x) == MINUS
	   || GET_CODE (x) == MULT)
    {
      rtx op0 = break_out_memory_refs (XEXP (x, 0));
      rtx op1 = break_out_memory_refs (XEXP (x, 1));

      if (op0 != XEXP (x, 0) || op1 != XEXP (x, 1))
	x = simplify_gen_binary (GET_CODE (x), GET_MODE (x), op0, op1);
    }

  return x;
}

/* Given X, a memory address in address space AS' pointer mode, convert it to
   an address in the address space's address mode, or vice versa (TO_MODE says
   which way).  We take advantage of the fact that pointers are not allowed to
   overflow by commuting arithmetic operations over conversions so that address
   arithmetic insns can be used.  */

rtx
convert_memory_address_addr_space (enum machine_mode to_mode ATTRIBUTE_UNUSED,
				   rtx x, addr_space_t as ATTRIBUTE_UNUSED)
{
#ifndef POINTERS_EXTEND_UNSIGNED
  gcc_assert (GET_MODE (x) == to_mode || GET_MODE (x) == VOIDmode);
  return x;
#else /* defined(POINTERS_EXTEND_UNSIGNED) */
  enum machine_mode pointer_mode, address_mode, from_mode;
  rtx temp;
  enum rtx_code code;

  /* If X already has the right mode, just return it.  */
  if (GET_MODE (x) == to_mode)
    return x;

  pointer_mode = targetm.addr_space.pointer_mode (as);
  address_mode = targetm.addr_space.address_mode (as);
  from_mode = to_mode == pointer_mode ? address_mode : pointer_mode;

  /* Here we handle some special cases.  If none of them apply, fall through
     to the default case.  */
  switch (GET_CODE (x))
    {
    case CONST_INT:
    case CONST_DOUBLE:
      if (GET_MODE_SIZE (to_mode) < GET_MODE_SIZE (from_mode))
	code = TRUNCATE;
      else if (POINTERS_EXTEND_UNSIGNED < 0)
	break;
      else if (POINTERS_EXTEND_UNSIGNED > 0)
	code = ZERO_EXTEND;
      else
	code = SIGN_EXTEND;
      temp = simplify_unary_operation (code, to_mode, x, from_mode);
      if (temp)
	return temp;
      break;

    case SUBREG:
      if ((SUBREG_PROMOTED_VAR_P (x) || REG_POINTER (SUBREG_REG (x)))
	  && GET_MODE (SUBREG_REG (x)) == to_mode)
	return SUBREG_REG (x);
      break;

    case LABEL_REF:
      temp = gen_rtx_LABEL_REF (to_mode, XEXP (x, 0));
      LABEL_REF_NONLOCAL_P (temp) = LABEL_REF_NONLOCAL_P (x);
      return temp;
      break;

    case SYMBOL_REF:
      temp = shallow_copy_rtx (x);
      PUT_MODE (temp, to_mode);
      return temp;
      break;

    case CONST:
      return gen_rtx_CONST (to_mode,
			    convert_memory_address_addr_space
			      (to_mode, XEXP (x, 0), as));
      break;

    case PLUS:
    case MULT:
      /* For addition we can safely permute the conversion and addition
	 operation if one operand is a constant and converting the constant
	 does not change it or if one operand is a constant and we are
	 using a ptr_extend instruction  (POINTERS_EXTEND_UNSIGNED < 0).
	 We can always safely permute them if we are making the address
	 narrower.  */
      if (GET_MODE_SIZE (to_mode) < GET_MODE_SIZE (from_mode)
	  || (GET_CODE (x) == PLUS
	      && CONST_INT_P (XEXP (x, 1))
	      && (XEXP (x, 1) == convert_memory_address_addr_space
				   (to_mode, XEXP (x, 1), as)
                 || POINTERS_EXTEND_UNSIGNED < 0)))
	return gen_rtx_fmt_ee (GET_CODE (x), to_mode,
			       convert_memory_address_addr_space
				 (to_mode, XEXP (x, 0), as),
			       XEXP (x, 1));
      break;

    default:
      break;
    }

  return convert_modes (to_mode, from_mode,
			x, POINTERS_EXTEND_UNSIGNED);
#endif /* defined(POINTERS_EXTEND_UNSIGNED) */
}

/* Return something equivalent to X but valid as a memory address for something
   of mode MODE in the named address space AS.  When X is not itself valid,
   this works by copying X or subexpressions of it into registers.  */

rtx
memory_address_addr_space (enum machine_mode mode, rtx x, addr_space_t as)
{
  rtx oldx = x;
  enum machine_mode address_mode = targetm.addr_space.address_mode (as);

  x = convert_memory_address_addr_space (address_mode, x, as);

  /* By passing constant addresses through registers
     we get a chance to cse them.  */
#ifdef _BUILD_C30_
  if (! cse_not_expected && CONSTANT_P (x) && CONSTANT_ADDRESS_P (x) &&
      (!pic30_neardata_space_operand_p(x)))
#else
  if (! cse_not_expected && CONSTANT_P (x) && CONSTANT_ADDRESS_P (x))
#endif
    x = force_reg (address_mode, x);

  /* We get better cse by rejecting indirect addressing at this stage.
     Let the combiner create indirect addresses where appropriate.
     For now, generate the code so that the subexpressions useful to share
     are visible.  But not if cse won't be done!  */
  else
    {
#ifdef _BUILD_C30_
      if (! cse_not_expected && !REG_P (x) && 
          (!pic30_neardata_space_operand_p(x)))
#else
      if (! cse_not_expected && !REG_P (x))
#endif
	x = break_out_memory_refs (x);

      /* At this point, any valid address is accepted.  */
      if (memory_address_addr_space_p (mode, x, as))
	goto done;

      /* If it was valid before but breaking out memory refs invalidated it,
	 use it the old way.  */
      if (memory_address_addr_space_p (mode, oldx, as))
	{
	  x = oldx;
	  goto done;
	}

      /* Perform machine-dependent transformations on X
	 in certain cases.  This is not necessary since the code
	 below can handle all possible cases, but machine-dependent
	 transformations can make better code.  */
      {
	rtx orig_x = x;
	x = targetm.addr_space.legitimize_address (x, oldx, mode, as);
	if (orig_x != x && memory_address_addr_space_p (mode, x, as))
	  goto done;
      }

      /* PLUS and MULT can appear in special ways
	 as the result of attempts to make an address usable for indexing.
	 Usually they are dealt with by calling force_operand, below.
	 But a sum containing constant terms is special
	 if removing them makes the sum a valid address:
	 then we generate that address in a register
	 and index off of it.  We do this because it often makes
	 shorter code, and because the addresses thus generated
	 in registers often become common subexpressions.  */
      if (GET_CODE (x) == PLUS)
	{
	  rtx constant_term = const0_rtx;
	  rtx y = eliminate_constant_term (x, &constant_term);
	  if (constant_term == const0_rtx
	      || ! memory_address_addr_space_p (mode, y, as))
	    x = force_operand (x, NULL_RTX);
	  else
	    {
	      y = gen_rtx_PLUS (GET_MODE (x), copy_to_reg (y), constant_term);
	      if (! memory_address_addr_space_p (mode, y, as))
		x = force_operand (x, NULL_RTX);
	      else
		x = y;
	    }
	}

      else if (GET_CODE (x) == MULT || GET_CODE (x) == MINUS)
	x = force_operand (x, NULL_RTX);

      /* If we have a register that's an invalid address,
	 it must be a hard reg of the wrong class.  Copy it to a pseudo.  */
      else if (REG_P (x))
	x = copy_to_reg (x);

      /* Last resort: copy the value to a register, since
	 the register is a valid address.  */
      else
	x = force_reg (address_mode, x);
    }

 done:

  gcc_assert (memory_address_addr_space_p (mode, x, as));
  /* If we didn't change the address, we are done.  Otherwise, mark
     a reg as a pointer if we have REG or REG + CONST_INT.  */
  if (oldx == x)
    return x;
  else if (REG_P (x))
    mark_reg_pointer (x, BITS_PER_UNIT);
  else if (GET_CODE (x) == PLUS
	   && REG_P (XEXP (x, 0))
	   && CONST_INT_P (XEXP (x, 1)))
    mark_reg_pointer (XEXP (x, 0), BITS_PER_UNIT);

  /* OLDX may have been the address on a temporary.  Update the address
     to indicate that X is now used.  */
  update_temp_slot_address (oldx, x);

  return x;
}

/* Convert a mem ref into one with a valid memory address.
   Pass through anything else unchanged.  */

rtx
validize_mem (rtx ref)
{
  if (!MEM_P (ref))
    return ref;
  ref = use_anchored_address (ref);
  if (memory_address_addr_space_p (GET_MODE (ref), XEXP (ref, 0),
				   MEM_ADDR_SPACE (ref)))
    return ref;

  /* Don't alter REF itself, since that is probably a stack slot.  */
  return replace_equiv_address (ref, XEXP (ref, 0));
}

/* If X is a memory reference to a member of an object block, try rewriting
   it to use an anchor instead.  Return the new memory reference on success
   and the old one on failure.  */

rtx
use_anchored_address (rtx x)
{
  rtx base;
  HOST_WIDE_INT offset;

  if (!flag_section_anchors)
    return x;

  if (!MEM_P (x))
    return x;

  /* Split the address into a base and offset.  */
  base = XEXP (x, 0);
  offset = 0;
  if (GET_CODE (base) == CONST
      && GET_CODE (XEXP (base, 0)) == PLUS
      && CONST_INT_P (XEXP (XEXP (base, 0), 1)))
    {
      offset += INTVAL (XEXP (XEXP (base, 0), 1));
      base = XEXP (XEXP (base, 0), 0);
    }

  /* Check whether BASE is suitable for anchors.  */
  if (GET_CODE (base) != SYMBOL_REF
      || !SYMBOL_REF_HAS_BLOCK_INFO_P (base)
      || SYMBOL_REF_ANCHOR_P (base)
      || SYMBOL_REF_BLOCK (base) == NULL
      || !targetm.use_anchors_for_symbol_p (base))
    return x;

  /* Decide where BASE is going to be.  */
  place_block_symbol (base);

  /* Get the anchor we need to use.  */
  offset += SYMBOL_REF_BLOCK_OFFSET (base);
  base = get_section_anchor (SYMBOL_REF_BLOCK (base), offset,
			     SYMBOL_REF_TLS_MODEL (base));

  /* Work out the offset from the anchor.  */
  offset -= SYMBOL_REF_BLOCK_OFFSET (base);

  /* If we're going to run a CSE pass, force the anchor into a register.
     We will then be able to reuse registers for several accesses, if the
     target costs say that that's worthwhile.  */
  if (!cse_not_expected)
    base = force_reg (GET_MODE (base), base);

  return replace_equiv_address (x, plus_constant (base, offset));
}

/* Copy the value or contents of X to a new temp reg and return that reg.  */

rtx
copy_to_reg (rtx x)
{
  rtx temp = gen_reg_rtx (GET_MODE (x));

  /* If not an operand, must be an address with PLUS and MULT so
     do the computation.  */
  if (! general_operand (x, VOIDmode))
    x = force_operand (x, temp);

  if (x != temp)
    emit_move_insn (temp, x);

  return temp;
}

/* Like copy_to_reg but always give the new register mode Pmode
   in case X is a constant.  */

rtx
copy_addr_to_reg (rtx x)
{
  return copy_to_mode_reg (Pmode, x);
}

/* Like copy_to_reg but always give the new register mode MODE
   in case X is a constant.  */

rtx
copy_to_mode_reg (enum machine_mode mode, rtx x)
{
  rtx temp = gen_reg_rtx (mode);

  /* If not an operand, must be an address with PLUS and MULT so
     do the computation.  */
  if (! general_operand (x, VOIDmode))
    x = force_operand (x, temp);

  gcc_assert (GET_MODE (x) == mode || GET_MODE (x) == VOIDmode);
  if (x != temp)
    emit_move_insn (temp, x);
  return temp;
}

/* Load X into a register if it is not already one.
   Use mode MODE for the register.
   X should be valid for mode MODE, but it may be a constant which
   is valid for all integer modes; that's why caller must specify MODE.

   The caller must not alter the value in the register we return,
   since we mark it as a "constant" register.  */

rtx
force_reg (enum machine_mode mode, rtx x)
{
  rtx temp, insn, set;

  if (REG_P (x))
    return x;

  if (general_operand (x, mode))
    {
      temp = gen_reg_rtx (mode);
      insn = emit_move_insn (temp, x);
    }
  else
    {
      temp = force_operand (x, NULL_RTX);
      if (REG_P (temp))
	insn = get_last_insn ();
      else
	{
	  rtx temp2 = gen_reg_rtx (mode);
	  insn = emit_move_insn (temp2, temp);
	  temp = temp2;
	}
    }

  /* Let optimizers know that TEMP's value never changes
     and that X can be substituted for it.  Don't get confused
     if INSN set something else (such as a SUBREG of TEMP).  */
  if (CONSTANT_P (x)
      && (set = single_set (insn)) != 0
      && SET_DEST (set) == temp
      && ! rtx_equal_p (x, SET_SRC (set)))
    set_unique_reg_note (insn, REG_EQUAL, x);

  /* Let optimizers know that TEMP is a pointer, and if so, the
     known alignment of that pointer.  */
  {
    unsigned align = 0;
    if (GET_CODE (x) == SYMBOL_REF)
      {
        align = BITS_PER_UNIT;
	if (SYMBOL_REF_DECL (x) && DECL_P (SYMBOL_REF_DECL (x)))
	  align = DECL_ALIGN (SYMBOL_REF_DECL (x));
      }
    else if (GET_CODE (x) == LABEL_REF)
      align = BITS_PER_UNIT;
    else if (GET_CODE (x) == CONST
	     && GET_CODE (XEXP (x, 0)) == PLUS
	     && GET_CODE (XEXP (XEXP (x, 0), 0)) == SYMBOL_REF
	     && CONST_INT_P (XEXP (XEXP (x, 0), 1)))
      {
	rtx s = XEXP (XEXP (x, 0), 0);
	rtx c = XEXP (XEXP (x, 0), 1);
	unsigned sa, ca;

	sa = BITS_PER_UNIT;
	if (SYMBOL_REF_DECL (s) && DECL_P (SYMBOL_REF_DECL (s)))
	  sa = DECL_ALIGN (SYMBOL_REF_DECL (s));

	ca = exact_log2 (INTVAL (c) & -INTVAL (c)) * BITS_PER_UNIT;

	align = MIN (sa, ca);
      }

    if (align || (MEM_P (x) && MEM_POINTER (x)))
      mark_reg_pointer (temp, align);
  }

  return temp;
}

/* If X is a memory ref, copy its contents to a new temp reg and return
   that reg.  Otherwise, return X.  */

rtx
force_not_mem (rtx x)
{
  rtx temp;

  if (!MEM_P (x) || GET_MODE (x) == BLKmode)
    return x;

  temp = gen_reg_rtx (GET_MODE (x));

  if (MEM_POINTER (x))
    REG_POINTER (temp) = 1;

  emit_move_insn (temp, x);
  return temp;
}

/* Copy X to TARGET (if it's nonzero and a reg)
   or to a new temp reg and return that reg.
   MODE is the mode to use for X in case it is a constant.  */

rtx
copy_to_suggested_reg (rtx x, rtx target, enum machine_mode mode)
{
  rtx temp;

  if (target && REG_P (target))
    temp = target;
  else
    temp = gen_reg_rtx (mode);

  emit_move_insn (temp, x);
  return temp;
}

/* Return the mode to use to pass or return a scalar of TYPE and MODE.
   PUNSIGNEDP points to the signedness of the type and may be adjusted
   to show what signedness to use on extension operations.

   FOR_RETURN is nonzero if the caller is promoting the return value
   of FNDECL, else it is for promoting args.  */

enum machine_mode
promote_function_mode (const_tree type, enum machine_mode mode, int *punsignedp,
		       const_tree funtype, int for_return)
{
  switch (TREE_CODE (type))
    {
    case INTEGER_TYPE:   case ENUMERAL_TYPE:   case BOOLEAN_TYPE:
    case REAL_TYPE:      case OFFSET_TYPE:     case FIXED_POINT_TYPE:
    case POINTER_TYPE:   case REFERENCE_TYPE:
      return targetm.calls.promote_function_mode (type, mode, punsignedp, funtype,
						  for_return);

    default:
      return mode;
    }
}
/* Return the mode to use to store a scalar of TYPE and MODE.
   PUNSIGNEDP points to the signedness of the type and may be adjusted
   to show what signedness to use on extension operations.  */

enum machine_mode
promote_mode (const_tree type ATTRIBUTE_UNUSED, enum machine_mode mode,
	      int *punsignedp ATTRIBUTE_UNUSED)
{
  /* FIXME: this is the same logic that was there until GCC 4.4, but we
     probably want to test POINTERS_EXTEND_UNSIGNED even if PROMOTE_MODE
     is not defined.  The affected targets are M32C, S390, SPARC.  */
#ifdef PROMOTE_MODE
  const enum tree_code code = TREE_CODE (type);
  int unsignedp = *punsignedp;

  switch (code)
    {
    case INTEGER_TYPE:   case ENUMERAL_TYPE:   case BOOLEAN_TYPE:
    case REAL_TYPE:      case OFFSET_TYPE:     case FIXED_POINT_TYPE:
      PROMOTE_MODE (mode, unsignedp, type);
      *punsignedp = unsignedp;
      return mode;
      break;

#ifdef POINTERS_EXTEND_UNSIGNED
    case REFERENCE_TYPE:
    case POINTER_TYPE:
      *punsignedp = POINTERS_EXTEND_UNSIGNED;
      return targetm.addr_space.address_mode
	       (TYPE_ADDR_SPACE (TREE_TYPE (type)));
      break;
#endif

    default:
      return mode;
    }
#else
  return mode;
#endif
}


/* Use one of promote_mode or promote_function_mode to find the promoted
   mode of DECL.  If PUNSIGNEDP is not NULL, store there the unsignedness
   of DECL after promotion.  */

enum machine_mode
promote_decl_mode (const_tree decl, int *punsignedp)
{
  tree type = TREE_TYPE (decl);
  int unsignedp = TYPE_UNSIGNED (type);
  enum machine_mode mode = DECL_MODE (decl);
  enum machine_mode pmode;

  if (TREE_CODE (decl) == RESULT_DECL
      || TREE_CODE (decl) == PARM_DECL)
    pmode = promote_function_mode (type, mode, &unsignedp,
                                   TREE_TYPE (current_function_decl), 2);
  else
    pmode = promote_mode (type, mode, &unsignedp);

  if (punsignedp)
    *punsignedp = unsignedp;
  return pmode;
}


/* Adjust the stack pointer by ADJUST (an rtx for a number of bytes).
   This pops when ADJUST is positive.  ADJUST need not be constant.  */

void
adjust_stack (rtx adjust)
{
  rtx temp;

  if (adjust == const0_rtx)
    return;

  /* We expect all variable sized adjustments to be multiple of
     PREFERRED_STACK_BOUNDARY.  */
  if (CONST_INT_P (adjust))
    stack_pointer_delta -= INTVAL (adjust);

#ifdef _BUILD_C30_
  temp = expand_binop (STACK_Pmode,
#else /*_BUILD_C30_*/
  temp = expand_binop (Pmode,
#endif /*_BUILD_C30_*/

#ifdef STACK_GROWS_DOWNWARD
		       add_optab,
#else
		       sub_optab,
#endif
		       stack_pointer_rtx, adjust, stack_pointer_rtx, 0,
		       OPTAB_LIB_WIDEN);

  if (temp != stack_pointer_rtx)
    emit_move_insn (stack_pointer_rtx, temp);
}

/* Adjust the stack pointer by minus ADJUST (an rtx for a number of bytes).
   This pushes when ADJUST is positive.  ADJUST need not be constant.  */

void
anti_adjust_stack (rtx adjust)
{
  rtx temp;

  if (adjust == const0_rtx)
    return;

  /* We expect all variable sized adjustments to be multiple of
     PREFERRED_STACK_BOUNDARY.  */
  if (CONST_INT_P (adjust))
    stack_pointer_delta += INTVAL (adjust);

#ifdef _BUILD_C30_
  temp = expand_binop (STACK_Pmode,
#else /*_BUILD_C30_*/
  temp = expand_binop (Pmode,
#endif /*_BUILD_C30_*/

#ifdef STACK_GROWS_DOWNWARD
		       sub_optab,
#else
		       add_optab,
#endif
		       stack_pointer_rtx, adjust, stack_pointer_rtx, 0,
		       OPTAB_LIB_WIDEN);

  if (temp != stack_pointer_rtx)
    emit_move_insn (stack_pointer_rtx, temp);
}

/* Round the size of a block to be pushed up to the boundary required
   by this machine.  SIZE is the desired size, which need not be constant.  */

static rtx
round_push (rtx size)
{
  int align = PREFERRED_STACK_BOUNDARY / BITS_PER_UNIT;

  if (align == 1)
    return size;

  if (CONST_INT_P (size))
    {
      HOST_WIDE_INT new_size = (INTVAL (size) + align - 1) / align * align;

      if (INTVAL (size) != new_size)
	size = GEN_INT (new_size);
    }
  else
    {
#ifdef _BUILD_C30_
      /* CEIL_DIV_EXPR needs to worry about the addition overflowing,
	 but we know it can't.  So add ourselves and then do
	 TRUNC_DIV_EXPR.  */
      size = expand_binop (STACK_Pmode, add_optab, size, GEN_INT (align - 1),
			   NULL_RTX, 1, OPTAB_LIB_WIDEN);
      size = expand_divmod (0, TRUNC_DIV_EXPR, STACK_Pmode, size, GEN_INT (align),
			    NULL_RTX, 1);
      size = expand_mult (STACK_Pmode, size, GEN_INT (align), NULL_RTX, 1);
#else /*_BUILD_C30_*/
      /* CEIL_DIV_EXPR needs to worry about the addition overflowing,
	 but we know it can't.  So add ourselves and then do
	 TRUNC_DIV_EXPR.  */
      size = expand_binop (Pmode, add_optab, size, GEN_INT (align - 1),
			   NULL_RTX, 1, OPTAB_LIB_WIDEN);
      size = expand_divmod (0, TRUNC_DIV_EXPR, Pmode, size, GEN_INT (align),
			    NULL_RTX, 1);
      size = expand_mult (Pmode, size, GEN_INT (align), NULL_RTX, 1);
#endif /*_BUILD_C30_*/
    }

  return size;
}

/* Save the stack pointer for the purpose in SAVE_LEVEL.  PSAVE is a pointer
   to a previously-created save area.  If no save area has been allocated,
   this function will allocate one.  If a save area is specified, it
   must be of the proper mode.

   The insns are emitted after insn AFTER, if nonzero, otherwise the insns
   are emitted at the current position.  */

void
emit_stack_save (enum save_level save_level, rtx *psave, rtx after)
{
  rtx sa = *psave;
  /* The default is that we use a move insn and save in a Pmode object.  */
  rtx (*fcn) (rtx, rtx) = gen_move_insn;
  enum machine_mode mode = STACK_SAVEAREA_MODE (save_level);

  /* See if this machine has anything special to do for this kind of save.  */
  switch (save_level)
    {
#ifdef HAVE_save_stack_block
    case SAVE_BLOCK:
      if (HAVE_save_stack_block)
	fcn = gen_save_stack_block;
      break;
#endif
#ifdef HAVE_save_stack_function
    case SAVE_FUNCTION:
      if (HAVE_save_stack_function)
	fcn = gen_save_stack_function;
      break;
#endif
#ifdef HAVE_save_stack_nonlocal
    case SAVE_NONLOCAL:
      if (HAVE_save_stack_nonlocal)
	fcn = gen_save_stack_nonlocal;
      break;
#endif
    default:
      break;
    }

  /* If there is no save area and we have to allocate one, do so.  Otherwise
     verify the save area is the proper mode.  */

  if (sa == 0)
    {
      if (mode != VOIDmode)
	{
	  if (save_level == SAVE_NONLOCAL)
	    *psave = sa = assign_stack_local (mode, GET_MODE_SIZE (mode), 0);
	  else
	    *psave = sa = gen_reg_rtx (mode);
	}
    }

  if (after)
    {
      rtx seq;

      start_sequence ();
      do_pending_stack_adjust ();
      /* We must validize inside the sequence, to ensure that any instructions
	 created by the validize call also get moved to the right place.  */
      if (sa != 0)
	sa = validize_mem (sa);
      emit_insn (fcn (sa, stack_pointer_rtx));
      seq = get_insns ();
      end_sequence ();
      emit_insn_after (seq, after);
    }
  else
    {
      do_pending_stack_adjust ();
      if (sa != 0)
	sa = validize_mem (sa);
      emit_insn (fcn (sa, stack_pointer_rtx));
    }
}

/* Restore the stack pointer for the purpose in SAVE_LEVEL.  SA is the save
   area made by emit_stack_save.  If it is zero, we have nothing to do.

   Put any emitted insns after insn AFTER, if nonzero, otherwise at
   current position.  */

void
emit_stack_restore (enum save_level save_level, rtx sa, rtx after)
{
  /* The default is that we use a move insn.  */
  rtx (*fcn) (rtx, rtx) = gen_move_insn;

  /* See if this machine has anything special to do for this kind of save.  */
  switch (save_level)
    {
#ifdef HAVE_restore_stack_block
    case SAVE_BLOCK:
      if (HAVE_restore_stack_block)
	fcn = gen_restore_stack_block;
      break;
#endif
#ifdef HAVE_restore_stack_function
    case SAVE_FUNCTION:
      if (HAVE_restore_stack_function)
	fcn = gen_restore_stack_function;
      break;
#endif
#ifdef HAVE_restore_stack_nonlocal
    case SAVE_NONLOCAL:
      if (HAVE_restore_stack_nonlocal)
	fcn = gen_restore_stack_nonlocal;
      break;
#endif
    default:
      break;
    }

  if (sa != 0)
    {
      sa = validize_mem (sa);
      /* These clobbers prevent the scheduler from moving
	 references to variable arrays below the code
	 that deletes (pops) the arrays.  */
      emit_clobber (gen_rtx_MEM (BLKmode, gen_rtx_SCRATCH (VOIDmode)));
      emit_clobber (gen_rtx_MEM (BLKmode, stack_pointer_rtx));
    }

  discard_pending_stack_adjust ();

  if (after)
    {
      rtx seq;

      start_sequence ();
      emit_insn (fcn (stack_pointer_rtx, sa));
      seq = get_insns ();
      end_sequence ();
      emit_insn_after (seq, after);
    }
  else
    emit_insn (fcn (stack_pointer_rtx, sa));
}

/* Invoke emit_stack_save on the nonlocal_goto_save_area for the current
   function.  This function should be called whenever we allocate or
   deallocate dynamic stack space.  */

void
update_nonlocal_goto_save_area (void)
{
  tree t_save;
  rtx r_save;

  /* The nonlocal_goto_save_area object is an array of N pointers.  The
     first one is used for the frame pointer save; the rest are sized by
     STACK_SAVEAREA_MODE.  Create a reference to array index 1, the first
     of the stack save area slots.  */
  t_save = build4 (ARRAY_REF, ptr_type_node, cfun->nonlocal_goto_save_area,
		   integer_one_node, NULL_TREE, NULL_TREE);
  r_save = expand_expr (t_save, NULL_RTX, VOIDmode, EXPAND_WRITE);

  emit_stack_save (SAVE_NONLOCAL, &r_save, NULL_RTX);
}

/* Return an rtx representing the address of an area of memory dynamically
   pushed on the stack.  This region of memory is always aligned to
   a multiple of BIGGEST_ALIGNMENT.

   Any required stack pointer alignment is preserved.

   SIZE is an rtx representing the size of the area.
   TARGET is a place in which the address can be placed.

   KNOWN_ALIGN is the alignment (in bits) that we know SIZE has.  */

rtx
allocate_dynamic_stack_space (rtx size, rtx target, int known_align)
{
  /* If we're asking for zero bytes, it doesn't matter what we point
     to since we can't dereference it.  But return a reasonable
     address anyway.  */
  if (size == const0_rtx)
    return virtual_stack_dynamic_rtx;

  /* Otherwise, show we're calling alloca or equivalent.  */
  cfun->calls_alloca = 1;

  /* Ensure the size is in the proper mode.  */
#ifdef _BUILD_C30_
  if (GET_MODE (size) != VOIDmode && GET_MODE (size) != STACK_Pmode)
    size = convert_to_mode (STACK_Pmode, size, 1);
#else /*_BUILD_C30_*/
  if (GET_MODE (size) != VOIDmode && GET_MODE (size) != Pmode)
    size = convert_to_mode (Pmode, size, 1);
#endif /*_BUILD_C30_*/

  /* We can't attempt to minimize alignment necessary, because we don't
     know the final value of preferred_stack_boundary yet while executing
     this code.  */
  crtl->preferred_stack_boundary = PREFERRED_STACK_BOUNDARY;

  /* We will need to ensure that the address we return is aligned to
     BIGGEST_ALIGNMENT.  If STACK_DYNAMIC_OFFSET is defined, we don't
     always know its final value at this point in the compilation (it
     might depend on the size of the outgoing parameter lists, for
     example), so we must align the value to be returned in that case.
     (Note that STACK_DYNAMIC_OFFSET will have a default nonzero value if
     STACK_POINTER_OFFSET or ACCUMULATE_OUTGOING_ARGS are defined).
     We must also do an alignment operation on the returned value if
     the stack pointer alignment is less strict that BIGGEST_ALIGNMENT.

     If we have to align, we must leave space in SIZE for the hole
     that might result from the alignment operation.  */

#if defined (STACK_DYNAMIC_OFFSET) || defined (STACK_POINTER_OFFSET)
#define MUST_ALIGN 1
#else
#define MUST_ALIGN (PREFERRED_STACK_BOUNDARY < BIGGEST_ALIGNMENT)
#endif

  if (MUST_ALIGN)
    size
      = force_operand (plus_constant (size,
				      BIGGEST_ALIGNMENT / BITS_PER_UNIT - 1),
		       NULL_RTX);

#ifdef SETJMP_VIA_SAVE_AREA
  /* If setjmp restores regs from a save area in the stack frame,
     avoid clobbering the reg save area.  Note that the offset of
     virtual_incoming_args_rtx includes the preallocated stack args space.
     It would be no problem to clobber that, but it's on the wrong side
     of the old save area.

     What used to happen is that, since we did not know for sure
     whether setjmp() was invoked until after RTL generation, we
     would use reg notes to store the "optimized" size and fix things
     up later.  These days we know this information before we ever
     start building RTL so the reg notes are unnecessary.  */
  if (!cfun->calls_setjmp)
    {
      int align = PREFERRED_STACK_BOUNDARY / BITS_PER_UNIT;

      /* ??? Code below assumes that the save area needs maximal
	 alignment.  This constraint may be too strong.  */
      gcc_assert (PREFERRED_STACK_BOUNDARY == BIGGEST_ALIGNMENT);

      if (CONST_INT_P (size))
	{
	  HOST_WIDE_INT new_size = INTVAL (size) / align * align;

	  if (INTVAL (size) != new_size)
	    size = GEN_INT (new_size);
	}
      else
	{
#ifdef _BUILD_C30_
	  /* Since we know overflow is not possible, we avoid using
	     CEIL_DIV_EXPR and use TRUNC_DIV_EXPR instead.  */
	  size = expand_divmod (0, TRUNC_DIV_EXPR, STACK_Pmode, size,
				GEN_INT (align), NULL_RTX, 1);
	  size = expand_mult (STACK_Pmode, size,
			      GEN_INT (align), NULL_RTX, 1);
#else /*_BUILD_C30_ */
	  /* Since we know overflow is not possible, we avoid using
	     CEIL_DIV_EXPR and use TRUNC_DIV_EXPR instead.  */
	  size = expand_divmod (0, TRUNC_DIV_EXPR, Pmode, size,
				GEN_INT (align), NULL_RTX, 1);
	  size = expand_mult (Pmode, size,
			      GEN_INT (align), NULL_RTX, 1);
#endif /*_BUILD_C30_ */
	}
    }
  else
    {
#ifdef _BUILD_C30_
      rtx dynamic_offset
	= expand_binop (STACK_Pmode, sub_optab, virtual_stack_dynamic_rtx,
			stack_pointer_rtx, NULL_RTX, 1, OPTAB_LIB_WIDEN);

      size = expand_binop (STACK_Pmode, add_optab, size, dynamic_offset,
			   NULL_RTX, 1, OPTAB_LIB_WIDEN);
#else /*_BUILD_C30_ */
      rtx dynamic_offset
	= expand_binop (Pmode, sub_optab, virtual_stack_dynamic_rtx,
			stack_pointer_rtx, NULL_RTX, 1, OPTAB_LIB_WIDEN);

      size = expand_binop (Pmode, add_optab, size, dynamic_offset,
			   NULL_RTX, 1, OPTAB_LIB_WIDEN);
#endif /*_BUILD_C30_ */
    }
#endif /* SETJMP_VIA_SAVE_AREA */

  /* Round the size to a multiple of the required stack alignment.
     Since the stack if presumed to be rounded before this allocation,
     this will maintain the required alignment.

     If the stack grows downward, we could save an insn by subtracting
     SIZE from the stack pointer and then aligning the stack pointer.
     The problem with this is that the stack pointer may be unaligned
     between the execution of the subtraction and alignment insns and
     some machines do not allow this.  Even on those that do, some
     signal handlers malfunction if a signal should occur between those
     insns.  Since this is an extremely rare event, we have no reliable
     way of knowing which systems have this problem.  So we avoid even
     momentarily mis-aligning the stack.  */

  /* If we added a variable amount to SIZE,
     we can no longer assume it is aligned.  */
#if !defined (SETJMP_VIA_SAVE_AREA)
  if (MUST_ALIGN || known_align % PREFERRED_STACK_BOUNDARY != 0)
#endif
    size = round_push (size);

  do_pending_stack_adjust ();

 /* We ought to be called always on the toplevel and stack ought to be aligned
    properly.  */
  gcc_assert (!(stack_pointer_delta
		% (PREFERRED_STACK_BOUNDARY / BITS_PER_UNIT)));

  /* If needed, check that we have the required amount of stack.  Take into
     account what has already been checked.  */
  if (STACK_CHECK_MOVING_SP)
    ;
  else if (flag_stack_check == GENERIC_STACK_CHECK)
    probe_stack_range (STACK_OLD_CHECK_PROTECT + STACK_CHECK_MAX_FRAME_SIZE,
		       size);
  else if (flag_stack_check == STATIC_BUILTIN_STACK_CHECK)
    probe_stack_range (STACK_CHECK_PROTECT, size);

  /* Don't use a TARGET that isn't a pseudo or is the wrong mode.  */
#ifdef _BUILD_C30_
  if (target == 0 || !REG_P (target)
      || REGNO (target) < FIRST_PSEUDO_REGISTER
      || GET_MODE (target) != STACK_Pmode)
    target = gen_reg_rtx (STACK_Pmode);
#else /*_BUILD_C30_ */
  if (target == 0 || !REG_P (target)
      || REGNO (target) < FIRST_PSEUDO_REGISTER
      || GET_MODE (target) != Pmode)
    target = gen_reg_rtx (Pmode);
#endif /*_BUILD_C30_*/

  mark_reg_pointer (target, known_align);

  /* Perform the required allocation from the stack.  Some systems do
     this differently than simply incrementing/decrementing from the
     stack pointer, such as acquiring the space by calling malloc().  */
#ifdef HAVE_allocate_stack
  if (HAVE_allocate_stack)
    {
      enum machine_mode mode = STACK_SIZE_MODE;
      insn_operand_predicate_fn pred;

      /* We don't have to check against the predicate for operand 0 since
	 TARGET is known to be a pseudo of the proper mode, which must
	 be valid for the operand.  For operand 1, convert to the
	 proper mode and validate.  */
      if (mode == VOIDmode)
	mode = insn_data[(int) CODE_FOR_allocate_stack].operand[1].mode;

      pred = insn_data[(int) CODE_FOR_allocate_stack].operand[1].predicate;
      if (pred && ! ((*pred) (size, mode)))
	size = copy_to_mode_reg (mode, convert_to_mode (mode, size, 1));

      emit_insn (gen_allocate_stack (target, size));
    }
  else
#endif
    {
#ifndef STACK_GROWS_DOWNWARD
      emit_move_insn (target, virtual_stack_dynamic_rtx);
#endif

      /* Check stack bounds if necessary.  */
      if (crtl->limit_stack)
	{
	  rtx available;
	  rtx space_available = gen_label_rtx ();
#ifdef _BUILD_C30_
#ifdef STACK_GROWS_DOWNWARD
	  available = expand_binop (STACK_Pmode, sub_optab,
				    stack_pointer_rtx, stack_limit_rtx,
				    NULL_RTX, 1, OPTAB_WIDEN);
#else
	  available = expand_binop (STACK_Pmode, sub_optab,
				    stack_limit_rtx, stack_pointer_rtx,
				    NULL_RTX, 1, OPTAB_WIDEN);
#endif
	  emit_cmp_and_jump_insns (available, size, GEU, NULL_RTX, STACK_Pmode, 1,
				   space_available);
#else /*_BUILD_C30_*/
#ifdef STACK_GROWS_DOWNWARD
	  available = expand_binop (Pmode, sub_optab,
				    stack_pointer_rtx, stack_limit_rtx,
				    NULL_RTX, 1, OPTAB_WIDEN);
#else
	  available = expand_binop (Pmode, sub_optab,
				    stack_limit_rtx, stack_pointer_rtx,
				    NULL_RTX, 1, OPTAB_WIDEN);
#endif
	  emit_cmp_and_jump_insns (available, size, GEU, NULL_RTX, Pmode, 1,
				   space_available);
#endif /*_BUILD_C30_*/

#ifdef HAVE_trap
	  if (HAVE_trap)
	    emit_insn (gen_trap ());
	  else
#endif
	    error ("stack limits not supported on this target");
	  emit_barrier ();
	  emit_label (space_available);
	}

      if (flag_stack_check && STACK_CHECK_MOVING_SP)
	anti_adjust_stack_and_probe (size, false);
      else
	anti_adjust_stack (size);

#ifdef STACK_GROWS_DOWNWARD
      emit_move_insn (target, virtual_stack_dynamic_rtx);
#endif
    }

  if (MUST_ALIGN)
    {
#ifdef _BUILD_C30_
      /* CEIL_DIV_EXPR needs to worry about the addition overflowing,
	 but we know it can't.  So add ourselves and then do
	 TRUNC_DIV_EXPR.  */
      target = expand_binop (STACK_Pmode, add_optab, target,
			     GEN_INT (BIGGEST_ALIGNMENT / BITS_PER_UNIT - 1),
			     NULL_RTX, 1, OPTAB_LIB_WIDEN);
      target = expand_divmod (0, TRUNC_DIV_EXPR, STACK_Pmode, target,
			      GEN_INT (BIGGEST_ALIGNMENT / BITS_PER_UNIT),
			      NULL_RTX, 1);
      target = expand_mult (STACK_Pmode, target,
			    GEN_INT (BIGGEST_ALIGNMENT / BITS_PER_UNIT),
			    NULL_RTX, 1);
#else /*_BUILD_C30_*/
      /* CEIL_DIV_EXPR needs to worry about the addition overflowing,
	 but we know it can't.  So add ourselves and then do
	 TRUNC_DIV_EXPR.  */
      target = expand_binop (Pmode, add_optab, target,
			     GEN_INT (BIGGEST_ALIGNMENT / BITS_PER_UNIT - 1),
			     NULL_RTX, 1, OPTAB_LIB_WIDEN);
      target = expand_divmod (0, TRUNC_DIV_EXPR, Pmode, target,
			      GEN_INT (BIGGEST_ALIGNMENT / BITS_PER_UNIT),
			      NULL_RTX, 1);
      target = expand_mult (Pmode, target,
			    GEN_INT (BIGGEST_ALIGNMENT / BITS_PER_UNIT),
			    NULL_RTX, 1);
#endif /*_BUILD_C30_*/
    }

  /* Record the new stack level for nonlocal gotos.  */
  if (cfun->nonlocal_goto_save_area != 0)
    update_nonlocal_goto_save_area ();

  return target;
}

/* A front end may want to override GCC's stack checking by providing a
   run-time routine to call to check the stack, so provide a mechanism for
   calling that routine.  */

static GTY(()) rtx stack_check_libfunc;

void
set_stack_check_libfunc (rtx libfunc)
{
  stack_check_libfunc = libfunc;
}

/* Emit one stack probe at ADDRESS, an address within the stack.  */

static void
emit_stack_probe (rtx address)
{
  rtx memref = gen_rtx_MEM (word_mode, address);

  MEM_VOLATILE_P (memref) = 1;

  /* See if we have an insn to probe the stack.  */
#ifdef HAVE_probe_stack
  if (HAVE_probe_stack)
    emit_insn (gen_probe_stack (memref));
  else
#endif
    emit_move_insn (memref, const0_rtx);
}

/* Probe a range of stack addresses from FIRST to FIRST+SIZE, inclusive.
   FIRST is a constant and size is a Pmode RTX.  These are offsets from
   the current stack pointer.  STACK_GROWS_DOWNWARD says whether to add
   or subtract them from the stack pointer.  */

#define PROBE_INTERVAL (1 << STACK_CHECK_PROBE_INTERVAL_EXP)

#ifdef STACK_GROWS_DOWNWARD
#define STACK_GROW_OP MINUS
#define STACK_GROW_OPTAB sub_optab
#define STACK_GROW_OFF(off) -(off)
#else
#define STACK_GROW_OP PLUS
#define STACK_GROW_OPTAB add_optab
#define STACK_GROW_OFF(off) (off)
#endif

void
probe_stack_range (HOST_WIDE_INT first, rtx size)
{
#ifdef _BUILD_C30_
  /* First ensure SIZE is Pmode.  */
  if (GET_MODE (size) != VOIDmode && GET_MODE (size) != STACK_Pmode)
    size = convert_to_mode (STACK_Pmode, size, 1);
#else /*_BUILD_C30_*/
  /* First ensure SIZE is Pmode.  */
  if (GET_MODE (size) != VOIDmode && GET_MODE (size) != Pmode)
    size = convert_to_mode (Pmode, size, 1);
#endif /*_BUILD_C30_*/

  /* Next see if we have a function to check the stack.  */
  if (stack_check_libfunc)
    {
#ifdef _BUILD_C30_
      rtx addr = memory_address (STACK_Pmode,
				 gen_rtx_fmt_ee (STACK_GROW_OP, STACK_Pmode,
					         stack_pointer_rtx,
					         plus_constant (size, first)));
      emit_library_call (stack_check_libfunc, LCT_NORMAL, VOIDmode, 1, addr,
			 STACK_Pmode);
#else /*_BUILD_C30_*/
      rtx addr = memory_address (Pmode,
				 gen_rtx_fmt_ee (STACK_GROW_OP, Pmode,
					         stack_pointer_rtx,
					         plus_constant (size, first)));
      emit_library_call (stack_check_libfunc, LCT_NORMAL, VOIDmode, 1, addr,
			 Pmode);
#endif /*_BUILD_C30_*/
    }

  /* Next see if we have an insn to check the stack.  */
#ifdef HAVE_check_stack
  else if (HAVE_check_stack)
    {
#ifdef _BUILD_C30_
      rtx addr = memory_address (STACK_Pmode,
				 gen_rtx_fmt_ee (STACK_GROW_OP, STACK_Pmode,
					         stack_pointer_rtx,
					         plus_constant (size, first)));
      insn_operand_predicate_fn pred
	= insn_data[(int) CODE_FOR_check_stack].operand[0].predicate;
      if (pred && !((*pred) (addr, STACK_Pmode)))
	addr = copy_to_mode_reg (STACK_Pmode, addr);
#else /*_BUILD_C30_*/
      rtx addr = memory_address (Pmode,
				 gen_rtx_fmt_ee (STACK_GROW_OP, Pmode,
					         stack_pointer_rtx,
					         plus_constant (size, first)));
      insn_operand_predicate_fn pred
	= insn_data[(int) CODE_FOR_check_stack].operand[0].predicate;
      if (pred && !((*pred) (addr, Pmode)))
	addr = copy_to_mode_reg (Pmode, addr);
#endif /*_BUILD_C30_*/

      emit_insn (gen_check_stack (addr));
    }
#endif

  /* Otherwise we have to generate explicit probes.  If we have a constant
     small number of them to generate, that's the easy case.  */
  else if (CONST_INT_P (size) && INTVAL (size) < 7 * PROBE_INTERVAL)
    {
      HOST_WIDE_INT isize = INTVAL (size), i;
      rtx addr;

      /* Probe at FIRST + N * PROBE_INTERVAL for values of N from 1 until
	 it exceeds SIZE.  If only one probe is needed, this will not
	 generate any code.  Then probe at FIRST + SIZE.  */
      for (i = PROBE_INTERVAL; i < isize; i += PROBE_INTERVAL)
	{
#ifdef _BUILD_C30_
	  addr = memory_address (STACK_Pmode,
				 plus_constant (stack_pointer_rtx,
				 		STACK_GROW_OFF (first + i)));
#else /*_BUILD_C30_*/
	  addr = memory_address (Pmode,
				 plus_constant (stack_pointer_rtx,
				 		STACK_GROW_OFF (first + i)));
#endif /*_BUILD_C30_*/
	  emit_stack_probe (addr);
	}

#ifdef _BUILD_C30_
      addr = memory_address (STACK_Pmode,
			     plus_constant (stack_pointer_rtx,
					    STACK_GROW_OFF (first + isize)));
#else /*_BUILD_C30_*/
      addr = memory_address (Pmode,
			     plus_constant (stack_pointer_rtx,
					    STACK_GROW_OFF (first + isize)));
#endif /*_BUILD_C30_*/
      emit_stack_probe (addr);
    }

  /* In the variable case, do the same as above, but in a loop.  Note that we
     must be extra careful with variables wrapping around because we might be
     at the very top (or the very bottom) of the address space and we have to
     be able to handle this case properly; in particular, we use an equality
     test for the loop condition.  */
  else
    {
      rtx rounded_size, rounded_size_op, test_addr, last_addr, temp;
      rtx loop_lab = gen_label_rtx ();
      rtx end_lab = gen_label_rtx ();


      /* Step 1: round SIZE to the previous multiple of the interval.  */
#ifdef _BUILD_C30_
      /* ROUNDED_SIZE = SIZE & -PROBE_INTERVAL  */
      rounded_size
	= simplify_gen_binary (AND, STACK_Pmode, size, GEN_INT (-PROBE_INTERVAL));
      rounded_size_op = force_operand (rounded_size, NULL_RTX);


      /* Step 2: compute initial and final value of the loop counter.  */

      /* TEST_ADDR = SP + FIRST.  */
      test_addr = force_operand (gen_rtx_fmt_ee (STACK_GROW_OP, STACK_Pmode,
					 	 stack_pointer_rtx,
					 	 GEN_INT (first)), NULL_RTX);

      /* LAST_ADDR = SP + FIRST + ROUNDED_SIZE.  */
      last_addr = force_operand (gen_rtx_fmt_ee (STACK_GROW_OP, STACK_Pmode,
						 test_addr,
						 rounded_size_op), NULL_RTX);
#else /*_BUILD_C30_*/
      /* ROUNDED_SIZE = SIZE & -PROBE_INTERVAL  */
      rounded_size
	= simplify_gen_binary (AND, Pmode, size, GEN_INT (-PROBE_INTERVAL));
      rounded_size_op = force_operand (rounded_size, NULL_RTX);


      /* Step 2: compute initial and final value of the loop counter.  */

      /* TEST_ADDR = SP + FIRST.  */
      test_addr = force_operand (gen_rtx_fmt_ee (STACK_GROW_OP, Pmode,
					 	 stack_pointer_rtx,
					 	 GEN_INT (first)), NULL_RTX);

      /* LAST_ADDR = SP + FIRST + ROUNDED_SIZE.  */
      last_addr = force_operand (gen_rtx_fmt_ee (STACK_GROW_OP, Pmode,
						 test_addr,
						 rounded_size_op), NULL_RTX);
#endif /*_BUILD_C30_*/

      /* Step 3: the loop

	 while (TEST_ADDR != LAST_ADDR)
	   {
	     TEST_ADDR = TEST_ADDR + PROBE_INTERVAL
	     probe at TEST_ADDR
	   }

	 probes at FIRST + N * PROBE_INTERVAL for values of N from 1
	 until it is equal to ROUNDED_SIZE.  */

      emit_label (loop_lab);
#ifdef _BUILD_C30_
      /* Jump to END_LAB if TEST_ADDR == LAST_ADDR.  */
      emit_cmp_and_jump_insns (test_addr, last_addr, EQ, NULL_RTX, STACK_Pmode, 1,
			       end_lab);

      /* TEST_ADDR = TEST_ADDR + PROBE_INTERVAL.  */
      temp = expand_binop (STACK_Pmode, STACK_GROW_OPTAB, test_addr,
			   GEN_INT (PROBE_INTERVAL), test_addr,
			   1, OPTAB_WIDEN);
#else /*_BUILD_C30_*/
      /* Jump to END_LAB if TEST_ADDR == LAST_ADDR.  */
      emit_cmp_and_jump_insns (test_addr, last_addr, EQ, NULL_RTX, Pmode, 1,
			       end_lab);
      /* TEST_ADDR = TEST_ADDR + PROBE_INTERVAL.  */
      temp = expand_binop (Pmode, STACK_GROW_OPTAB, test_addr,
			   GEN_INT (PROBE_INTERVAL), test_addr,
			   1, OPTAB_WIDEN);
#endif /*_BUILD_C30_*/
      gcc_assert (temp == test_addr);

      /* Probe at TEST_ADDR.  */
      emit_stack_probe (test_addr);

      emit_jump (loop_lab);

      emit_label (end_lab);


      /* Step 4: probe at FIRST + SIZE if we cannot assert at compile-time
	 that SIZE is equal to ROUNDED_SIZE.  */
#ifdef _BUILD_C30_
      /* TEMP = SIZE - ROUNDED_SIZE.  */
      temp = simplify_gen_binary (MINUS, STACK_Pmode, size, rounded_size);
#else /*_BUILD_C30_*/
      /* TEMP = SIZE - ROUNDED_SIZE.  */
      temp = simplify_gen_binary (MINUS, Pmode, size, rounded_size);
#endif /*_BUILD_C30_*/
      if (temp != const0_rtx)
	{
	  rtx addr;

	  if (GET_CODE (temp) == CONST_INT)
	    {
	      /* Use [base + disp] addressing mode if supported.  */
	      HOST_WIDE_INT offset = INTVAL (temp);
#ifdef _BUILD_C30_
	      addr = memory_address (STACK_Pmode,
				     plus_constant (last_addr,
						    STACK_GROW_OFF (offset)));
#else /*_BUILD_C30_*/
	      addr = memory_address (Pmode,
				     plus_constant (last_addr,
						    STACK_GROW_OFF (offset)));
#endif /*_BUILD_C30_*/
	    }
	  else
	    {
#ifdef _BUILD_C30_
	      /* Manual CSE if the difference is not known at compile-time.  */
	      temp = gen_rtx_MINUS (STACK_Pmode, size, rounded_size_op);
	      addr = memory_address (STACK_Pmode,
				     gen_rtx_fmt_ee (STACK_GROW_OP, STACK_Pmode,
						     last_addr, temp));
#else /*_BUILD_C30_*/
	      /* Manual CSE if the difference is not known at compile-time.  */
	      temp = gen_rtx_MINUS (Pmode, size, rounded_size_op);
	      addr = memory_address (Pmode,
				     gen_rtx_fmt_ee (STACK_GROW_OP, Pmode,
						     last_addr, temp));
#endif /*_BUILD_C30_*/
	    }

	  emit_stack_probe (addr);
	}
    }
}

/* Adjust the stack pointer by minus SIZE (an rtx for a number of bytes)
   while probing it.  This pushes when SIZE is positive.  SIZE need not
   be constant.  If ADJUST_BACK is true, adjust back the stack pointer
   by plus SIZE at the end.  */

void
anti_adjust_stack_and_probe (rtx size, bool adjust_back)
{
  /* We skip the probe for the first interval + a small dope of 4 words and
     probe that many bytes past the specified size to maintain a protection
     area at the botton of the stack.  */
  const int dope = 4 * UNITS_PER_WORD;

#ifdef _BUILD_C30_
  /* First ensure SIZE is STACK_Pmode.  */
  if (GET_MODE (size) != VOIDmode && GET_MODE (size) != STACK_Pmode)
    size = convert_to_mode (STACK_Pmode, size, 1);
#else /*_BUILD_C30_*/
  /* First ensure SIZE is Pmode.  */
  if (GET_MODE (size) != VOIDmode && GET_MODE (size) != Pmode)
    size = convert_to_mode (Pmode, size, 1);
#endif /*_BUILD_C30_*/

  /* If we have a constant small number of probes to generate, that's the
     easy case.  */
  if (GET_CODE (size) == CONST_INT && INTVAL (size) < 7 * PROBE_INTERVAL)
    {
      HOST_WIDE_INT isize = INTVAL (size), i;
      bool first_probe = true;

      /* Adjust SP and probe to PROBE_INTERVAL + N * PROBE_INTERVAL for
	 values of N from 1 until it exceeds SIZE.  If only one probe is
	 needed, this will not generate any code.  Then adjust and probe
	 to PROBE_INTERVAL + SIZE.  */
      for (i = PROBE_INTERVAL; i < isize; i += PROBE_INTERVAL)
	{
	  if (first_probe)
	    {
	      anti_adjust_stack (GEN_INT (2 * PROBE_INTERVAL + dope));
	      first_probe = false;
	    }
	  else
	    anti_adjust_stack (GEN_INT (PROBE_INTERVAL));
	  emit_stack_probe (stack_pointer_rtx);
	}

      if (first_probe)
	anti_adjust_stack (plus_constant (size, PROBE_INTERVAL + dope));
      else
	anti_adjust_stack (plus_constant (size, PROBE_INTERVAL - i));
      emit_stack_probe (stack_pointer_rtx);
    }

  /* In the variable case, do the same as above, but in a loop.  Note that we
     must be extra careful with variables wrapping around because we might be
     at the very top (or the very bottom) of the address space and we have to
     be able to handle this case properly; in particular, we use an equality
     test for the loop condition.  */
  else
    {
      rtx rounded_size, rounded_size_op, last_addr, temp;
      rtx loop_lab = gen_label_rtx ();
      rtx end_lab = gen_label_rtx ();


      /* Step 1: round SIZE to the previous multiple of the interval.  */
#ifdef _BUILD_C30_
      /* ROUNDED_SIZE = SIZE & -PROBE_INTERVAL  */
      rounded_size
	= simplify_gen_binary (AND, STACK_Pmode, size, GEN_INT (-PROBE_INTERVAL));
#else /*_BUILD_C30_*/
      /* ROUNDED_SIZE = SIZE & -PROBE_INTERVAL  */
      rounded_size
	= simplify_gen_binary (AND, Pmode, size, GEN_INT (-PROBE_INTERVAL));
#endif /*_BUILD_C30_*/

      rounded_size_op = force_operand (rounded_size, NULL_RTX);


      /* Step 2: compute initial and final value of the loop counter.  */

      /* SP = SP_0 + PROBE_INTERVAL.  */
      anti_adjust_stack (GEN_INT (PROBE_INTERVAL + dope));

#ifdef _BUILD_C30_
      /* LAST_ADDR = SP_0 + PROBE_INTERVAL + ROUNDED_SIZE.  */
      last_addr = force_operand (gen_rtx_fmt_ee (STACK_GROW_OP, STACK_Pmode,
						 stack_pointer_rtx,
						 rounded_size_op), NULL_RTX);
#else /*_BUILD_C30_*/
      /* LAST_ADDR = SP_0 + PROBE_INTERVAL + ROUNDED_SIZE.  */
      last_addr = force_operand (gen_rtx_fmt_ee (STACK_GROW_OP, Pmode,
						 stack_pointer_rtx,
						 rounded_size_op), NULL_RTX);
#endif /*_BUILD_C30_*/


      /* Step 3: the loop

	  while (SP != LAST_ADDR)
	    {
	      SP = SP + PROBE_INTERVAL
	      probe at SP
	    }

	 adjusts SP and probes to PROBE_INTERVAL + N * PROBE_INTERVAL for
	 values of N from 1 until it is equal to ROUNDED_SIZE.  */

      emit_label (loop_lab);

#ifdef _BUILD_C30_
      /* Jump to END_LAB if SP == LAST_ADDR.  */
      emit_cmp_and_jump_insns (stack_pointer_rtx, last_addr, EQ, NULL_RTX,
			       STACK_Pmode, 1, end_lab);
#else /*_BUILD_C30_*/
      /* Jump to END_LAB if SP == LAST_ADDR.  */
      emit_cmp_and_jump_insns (stack_pointer_rtx, last_addr, EQ, NULL_RTX,
			       Pmode, 1, end_lab);
#endif /*_BUILD_C30_*/

      /* SP = SP + PROBE_INTERVAL and probe at SP.  */
      anti_adjust_stack (GEN_INT (PROBE_INTERVAL));
      emit_stack_probe (stack_pointer_rtx);

      emit_jump (loop_lab);

      emit_label (end_lab);


      /* Step 4: adjust SP and probe to PROBE_INTERVAL + SIZE if we cannot
	 assert at compile-time that SIZE is equal to ROUNDED_SIZE.  */

#ifdef _BUILD_C30_
      /* TEMP = SIZE - ROUNDED_SIZE.  */
      temp = simplify_gen_binary (MINUS, Pmode, size, rounded_size);
#else /*_BUILD_C30_*/
      /* TEMP = SIZE - ROUNDED_SIZE.  */
      temp = simplify_gen_binary (MINUS, Pmode, size, rounded_size);
#endif /*_BUILD_C30_*/
      if (temp != const0_rtx)
	{
	  /* Manual CSE if the difference is not known at compile-time.  */
	  if (GET_CODE (temp) != CONST_INT)
	    temp = gen_rtx_MINUS (Pmode, size, rounded_size_op);
	  anti_adjust_stack (temp);
	  emit_stack_probe (stack_pointer_rtx);
	}
    }

  /* Adjust back and account for the additional first interval.  */
  if (adjust_back)
    adjust_stack (plus_constant (size, PROBE_INTERVAL + dope));
  else
    adjust_stack (GEN_INT (PROBE_INTERVAL + dope));
}

/* Return an rtx representing the register or memory location
   in which a scalar value of data type VALTYPE
   was returned by a function call to function FUNC.
   FUNC is a FUNCTION_DECL, FNTYPE a FUNCTION_TYPE node if the precise
   function is known, otherwise 0.
   OUTGOING is 1 if on a machine with register windows this function
   should return the register in which the function will put its result
   and 0 otherwise.  */

rtx
hard_function_value (const_tree valtype, const_tree func, const_tree fntype,
		     int outgoing ATTRIBUTE_UNUSED)
{
  rtx val;

  val = targetm.calls.function_value (valtype, func ? func : fntype, outgoing);

  if (REG_P (val)
      && GET_MODE (val) == BLKmode)
    {
      unsigned HOST_WIDE_INT bytes = int_size_in_bytes (valtype);
      enum machine_mode tmpmode;

      /* int_size_in_bytes can return -1.  We don't need a check here
	 since the value of bytes will then be large enough that no
	 mode will match anyway.  */

      for (tmpmode = GET_CLASS_NARROWEST_MODE (MODE_INT);
	   tmpmode != VOIDmode;
	   tmpmode = GET_MODE_WIDER_MODE (tmpmode))
	{
	  /* Have we found a large enough mode?  */
	  if (GET_MODE_SIZE (tmpmode) >= bytes)
	    break;
	}

      /* No suitable mode found.  */
      gcc_assert (tmpmode != VOIDmode);

      PUT_MODE (val, tmpmode);
    }
  return val;
}

/* Return an rtx representing the register or memory location
   in which a scalar value of mode MODE was returned by a library call.  */

rtx
hard_libcall_value (enum machine_mode mode, rtx fun)
{
  return targetm.calls.libcall_value (mode, fun);
}

/* Look up the tree code for a given rtx code
   to provide the arithmetic operation for REAL_ARITHMETIC.
   The function returns an int because the caller may not know
   what `enum tree_code' means.  */

int
rtx_to_tree_code (enum rtx_code code)
{
  enum tree_code tcode;

  switch (code)
    {
    case PLUS:
      tcode = PLUS_EXPR;
      break;
    case MINUS:
      tcode = MINUS_EXPR;
      break;
    case MULT:
      tcode = MULT_EXPR;
      break;
    case DIV:
      tcode = RDIV_EXPR;
      break;
    case SMIN:
      tcode = MIN_EXPR;
      break;
    case SMAX:
      tcode = MAX_EXPR;
      break;
    default:
      tcode = LAST_AND_UNUSED_TREE_CODE;
      break;
    }
  return ((int) tcode);
}

#include "gt-explow.h"
