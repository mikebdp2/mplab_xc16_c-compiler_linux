/* linker.c -- BFD linker routines
   Copyright 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   Free Software Foundation, Inc.
   Written by Steve Chamberlain and Ian Lance Taylor, Cygnus Support

   This file is part of BFD, the Binary File Descriptor library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "bfd.h"
#include "sysdep.h"
#include "libbfd.h"
#include "bfdlink.h"
#include "genlink.h"
#include "elf-bfd.h"
#include "libiberty.h"
#if PIC30
#include "pic30-utils.h"

/* 
 * make common version of this symbol which will be initialized to NIL 
 * unless we are creating the linker where an initialized definition will
 * be provided by pic30-elf.em or pic30-coff.em
 *
 * we only call this function if it is a valid pointer
 */

unsigned int (*pic30_force_keep_symbol)(char *, char *);
void (*pic30_smartio_symbols)(struct bfd_link_info*);

unsigned int global_signature_mask = 0;
unsigned int global_signature_set = 0;

#endif

/*
SECTION
	Linker Functions

@cindex Linker
	The linker uses three special entry points in the BFD target
	vector.  It is not necessary to write special routines for
	these entry points when creating a new BFD back end, since
	generic versions are provided.  However, writing them can
	speed up linking and make it use significantly less runtime
	memory.

	The first routine creates a hash table used by the other
	routines.  The second routine adds the symbols from an object
	file to the hash table.  The third routine takes all the
	object files and links them together to create the output
	file.  These routines are designed so that the linker proper
	does not need to know anything about the symbols in the object
	files that it is linking.  The linker merely arranges the
	sections as directed by the linker script and lets BFD handle
	the details of symbols and relocs.

	The second routine and third routines are passed a pointer to
	a <<struct bfd_link_info>> structure (defined in
	<<bfdlink.h>>) which holds information relevant to the link,
	including the linker hash table (which was created by the
	first routine) and a set of callback functions to the linker
	proper.

	The generic linker routines are in <<linker.c>>, and use the
	header file <<genlink.h>>.  As of this writing, the only back
	ends which have implemented versions of these routines are
	a.out (in <<aoutx.h>>) and ECOFF (in <<ecoff.c>>).  The a.out
	routines are used as examples throughout this section.

@menu
@* Creating a Linker Hash Table::
@* Adding Symbols to the Hash Table::
@* Performing the Final Link::
@end menu

INODE
Creating a Linker Hash Table, Adding Symbols to the Hash Table, Linker Functions, Linker Functions
SUBSECTION
	Creating a linker hash table

@cindex _bfd_link_hash_table_create in target vector
@cindex target vector (_bfd_link_hash_table_create)
	The linker routines must create a hash table, which must be
	derived from <<struct bfd_link_hash_table>> described in
	<<bfdlink.c>>.  @xref{Hash Tables}, for information on how to
	create a derived hash table.  This entry point is called using
	the target vector of the linker output file.

	The <<_bfd_link_hash_table_create>> entry point must allocate
	and initialize an instance of the desired hash table.  If the
	back end does not require any additional information to be
	stored with the entries in the hash table, the entry point may
	simply create a <<struct bfd_link_hash_table>>.  Most likely,
	however, some additional information will be needed.

	For example, with each entry in the hash table the a.out
	linker keeps the index the symbol has in the final output file
	(this index number is used so that when doing a relocateable
	link the symbol index used in the output file can be quickly
	filled in when copying over a reloc).  The a.out linker code
	defines the required structures and functions for a hash table
	derived from <<struct bfd_link_hash_table>>.  The a.out linker
	hash table is created by the function
	<<NAME(aout,link_hash_table_create)>>; it simply allocates
	space for the hash table, initializes it, and returns a
	pointer to it.

	When writing the linker routines for a new back end, you will
	generally not know exactly which fields will be required until
	you have finished.  You should simply create a new hash table
	which defines no additional fields, and then simply add fields
	as they become necessary.

INODE
Adding Symbols to the Hash Table, Performing the Final Link, Creating a Linker Hash Table, Linker Functions
SUBSECTION
	Adding symbols to the hash table

@cindex _bfd_link_add_symbols in target vector
@cindex target vector (_bfd_link_add_symbols)
	The linker proper will call the <<_bfd_link_add_symbols>>
	entry point for each object file or archive which is to be
	linked (typically these are the files named on the command
	line, but some may also come from the linker script).  The
	entry point is responsible for examining the file.  For an
	object file, BFD must add any relevant symbol information to
	the hash table.  For an archive, BFD must determine which
	elements of the archive should be used and adding them to the
	link.

	The a.out version of this entry point is
	<<NAME(aout,link_add_symbols)>>.

@menu
@* Differing file formats::
@* Adding symbols from an object file::
@* Adding symbols from an archive::
@end menu

INODE
Differing file formats, Adding symbols from an object file, Adding Symbols to the Hash Table, Adding Symbols to the Hash Table
SUBSUBSECTION
	Differing file formats

	Normally all the files involved in a link will be of the same
	format, but it is also possible to link together different
	format object files, and the back end must support that.  The
	<<_bfd_link_add_symbols>> entry point is called via the target
	vector of the file to be added.  This has an important
	consequence: the function may not assume that the hash table
	is the type created by the corresponding
	<<_bfd_link_hash_table_create>> vector.  All the
	<<_bfd_link_add_symbols>> function can assume about the hash
	table is that it is derived from <<struct
	bfd_link_hash_table>>.

	Sometimes the <<_bfd_link_add_symbols>> function must store
	some information in the hash table entry to be used by the
	<<_bfd_final_link>> function.  In such a case the <<creator>>
	field of the hash table must be checked to make sure that the
	hash table was created by an object file of the same format.

	The <<_bfd_final_link>> routine must be prepared to handle a
	hash entry without any extra information added by the
	<<_bfd_link_add_symbols>> function.  A hash entry without
	extra information will also occur when the linker script
	directs the linker to create a symbol.  Note that, regardless
	of how a hash table entry is added, all the fields will be
	initialized to some sort of null value by the hash table entry
	initialization function.

	See <<ecoff_link_add_externals>> for an example of how to
	check the <<creator>> field before saving information (in this
	case, the ECOFF external symbol debugging information) in a
	hash table entry.

INODE
Adding symbols from an object file, Adding symbols from an archive, Differing file formats, Adding Symbols to the Hash Table
SUBSUBSECTION
	Adding symbols from an object file

	When the <<_bfd_link_add_symbols>> routine is passed an object
	file, it must add all externally visible symbols in that
	object file to the hash table.  The actual work of adding the
	symbol to the hash table is normally handled by the function
	<<_bfd_generic_link_add_one_symbol>>.  The
	<<_bfd_link_add_symbols>> routine is responsible for reading
	all the symbols from the object file and passing the correct
	information to <<_bfd_generic_link_add_one_symbol>>.

	The <<_bfd_link_add_symbols>> routine should not use
	<<bfd_canonicalize_symtab>> to read the symbols.  The point of
	providing this routine is to avoid the overhead of converting
	the symbols into generic <<asymbol>> structures.

@findex _bfd_generic_link_add_one_symbol
	<<_bfd_generic_link_add_one_symbol>> handles the details of
	combining common symbols, warning about multiple definitions,
	and so forth.  It takes arguments which describe the symbol to
	add, notably symbol flags, a section, and an offset.  The
	symbol flags include such things as <<BSF_WEAK>> or
	<<BSF_INDIRECT>>.  The section is a section in the object
	file, or something like <<bfd_und_section_ptr>> for an undefined
	symbol or <<bfd_com_section_ptr>> for a common symbol.

	If the <<_bfd_final_link>> routine is also going to need to
	read the symbol information, the <<_bfd_link_add_symbols>>
	routine should save it somewhere attached to the object file
	BFD.  However, the information should only be saved if the
	<<keep_memory>> field of the <<info>> argument is TRUE, so
	that the <<-no-keep-memory>> linker switch is effective.

	The a.out function which adds symbols from an object file is
	<<aout_link_add_object_symbols>>, and most of the interesting
	work is in <<aout_link_add_symbols>>.  The latter saves
	pointers to the hash tables entries created by
	<<_bfd_generic_link_add_one_symbol>> indexed by symbol number,
	so that the <<_bfd_final_link>> routine does not have to call
	the hash table lookup routine to locate the entry.

INODE
Adding symbols from an archive, , Adding symbols from an object file, Adding Symbols to the Hash Table
SUBSUBSECTION
	Adding symbols from an archive

	When the <<_bfd_link_add_symbols>> routine is passed an
	archive, it must look through the symbols defined by the
	archive and decide which elements of the archive should be
	included in the link.  For each such element it must call the
	<<add_archive_element>> linker callback, and it must add the
	symbols from the object file to the linker hash table.

@findex _bfd_generic_link_add_archive_symbols
	In most cases the work of looking through the symbols in the
	archive should be done by the
	<<_bfd_generic_link_add_archive_symbols>> function.  This
	function builds a hash table from the archive symbol table and
	looks through the list of undefined symbols to see which
	elements should be included.
	<<_bfd_generic_link_add_archive_symbols>> is passed a function
	to call to make the final decision about adding an archive
	element to the link and to do the actual work of adding the
	symbols to the linker hash table.

	The function passed to
	<<_bfd_generic_link_add_archive_symbols>> must read the
	symbols of the archive element and decide whether the archive
	element should be included in the link.  If the element is to
	be included, the <<add_archive_element>> linker callback
	routine must be called with the element as an argument, and
	the elements symbols must be added to the linker hash table
	just as though the element had itself been passed to the
	<<_bfd_link_add_symbols>> function.

	When the a.out <<_bfd_link_add_symbols>> function receives an
	archive, it calls <<_bfd_generic_link_add_archive_symbols>>
	passing <<aout_link_check_archive_element>> as the function
	argument. <<aout_link_check_archive_element>> calls
	<<aout_link_check_ar_symbols>>.  If the latter decides to add
	the element (an element is only added if it provides a real,
	non-common, definition for a previously undefined or common
	symbol) it calls the <<add_archive_element>> callback and then
	<<aout_link_check_archive_element>> calls
	<<aout_link_add_symbols>> to actually add the symbols to the
	linker hash table.

	The ECOFF back end is unusual in that it does not normally
	call <<_bfd_generic_link_add_archive_symbols>>, because ECOFF
	archives already contain a hash table of symbols.  The ECOFF
	back end searches the archive itself to avoid the overhead of
	creating a new hash table.

INODE
Performing the Final Link, , Adding Symbols to the Hash Table, Linker Functions
SUBSECTION
	Performing the final link

@cindex _bfd_link_final_link in target vector
@cindex target vector (_bfd_final_link)
	When all the input files have been processed, the linker calls
	the <<_bfd_final_link>> entry point of the output BFD.  This
	routine is responsible for producing the final output file,
	which has several aspects.  It must relocate the contents of
	the input sections and copy the data into the output sections.
	It must build an output symbol table including any local
	symbols from the input files and the global symbols from the
	hash table.  When producing relocateable output, it must
	modify the input relocs and write them into the output file.
	There may also be object format dependent work to be done.

	The linker will also call the <<write_object_contents>> entry
	point when the BFD is closed.  The two entry points must work
	together in order to produce the correct output file.

	The details of how this works are inevitably dependent upon
	the specific object file format.  The a.out
	<<_bfd_final_link>> routine is <<NAME(aout,final_link)>>.

@menu
@* Information provided by the linker::
@* Relocating the section contents::
@* Writing the symbol table::
@end menu

INODE
Information provided by the linker, Relocating the section contents, Performing the Final Link, Performing the Final Link
SUBSUBSECTION
	Information provided by the linker

	Before the linker calls the <<_bfd_final_link>> entry point,
	it sets up some data structures for the function to use.

	The <<input_bfds>> field of the <<bfd_link_info>> structure
	will point to a list of all the input files included in the
	link.  These files are linked through the <<link_next>> field
	of the <<bfd>> structure.

	Each section in the output file will have a list of
	<<link_order>> structures attached to the <<link_order_head>>
	field (the <<link_order>> structure is defined in
	<<bfdlink.h>>).  These structures describe how to create the
	contents of the output section in terms of the contents of
	various input sections, fill constants, and, eventually, other
	types of information.  They also describe relocs that must be
	created by the BFD backend, but do not correspond to any input
	file; this is used to support -Ur, which builds constructors
	while generating a relocateable object file.

INODE
Relocating the section contents, Writing the symbol table, Information provided by the linker, Performing the Final Link
SUBSUBSECTION
	Relocating the section contents

	The <<_bfd_final_link>> function should look through the
	<<link_order>> structures attached to each section of the
	output file.  Each <<link_order>> structure should either be
	handled specially, or it should be passed to the function
	<<_bfd_default_link_order>> which will do the right thing
	(<<_bfd_default_link_order>> is defined in <<linker.c>>).

	For efficiency, a <<link_order>> of type
	<<bfd_indirect_link_order>> whose associated section belongs
	to a BFD of the same format as the output BFD must be handled
	specially.  This type of <<link_order>> describes part of an
	output section in terms of a section belonging to one of the
	input files.  The <<_bfd_final_link>> function should read the
	contents of the section and any associated relocs, apply the
	relocs to the section contents, and write out the modified
	section contents.  If performing a relocateable link, the
	relocs themselves must also be modified and written out.

@findex _bfd_relocate_contents
@findex _bfd_final_link_relocate
	The functions <<_bfd_relocate_contents>> and
	<<_bfd_final_link_relocate>> provide some general support for
	performing the actual relocations, notably overflow checking.
	Their arguments include information about the symbol the
	relocation is against and a <<reloc_howto_type>> argument
	which describes the relocation to perform.  These functions
	are defined in <<reloc.c>>.

	The a.out function which handles reading, relocating, and
	writing section contents is <<aout_link_input_section>>.  The
	actual relocation is done in <<aout_link_input_section_std>>
	and <<aout_link_input_section_ext>>.

INODE
Writing the symbol table, , Relocating the section contents, Performing the Final Link
SUBSUBSECTION
	Writing the symbol table

	The <<_bfd_final_link>> function must gather all the symbols
	in the input files and write them out.  It must also write out
	all the symbols in the global hash table.  This must be
	controlled by the <<strip>> and <<discard>> fields of the
	<<bfd_link_info>> structure.

	The local symbols of the input files will not have been
	entered into the linker hash table.  The <<_bfd_final_link>>
	routine must consider each input file and include the symbols
	in the output file.  It may be convenient to do this when
	looking through the <<link_order>> structures, or it may be
	done by stepping through the <<input_bfds>> list.

	The <<_bfd_final_link>> routine must also traverse the global
	hash table to gather all the externally visible symbols.  It
	is possible that most of the externally visible symbols may be
	written out when considering the symbols of each input file,
	but it is still necessary to traverse the hash table since the
	linker script may have defined some symbols that are not in
	any of the input files.

	The <<strip>> field of the <<bfd_link_info>> structure
	controls which symbols are written out.  The possible values
	are listed in <<bfdlink.h>>.  If the value is <<strip_some>>,
	then the <<keep_hash>> field of the <<bfd_link_info>>
	structure is a hash table of symbols to keep; each symbol
	should be looked up in this hash table, and only symbols which
	are present should be included in the output file.

	If the <<strip>> field of the <<bfd_link_info>> structure
	permits local symbols to be written out, the <<discard>> field
	is used to further controls which local symbols are included
	in the output file.  If the value is <<discard_l>>, then all
	local symbols which begin with a certain prefix are discarded;
	this is controlled by the <<bfd_is_local_label_name>> entry point.

	The a.out backend handles symbols by calling
	<<aout_link_write_symbols>> on each input BFD and then
	traversing the global hash table with the function
	<<aout_link_write_other_symbol>>.  It builds a string table
	while writing out the symbols, which is written to the output
	file at the end of <<NAME(aout,final_link)>>.
*/

static bfd_boolean generic_link_read_symbols
  PARAMS ((bfd *));
static bfd_boolean generic_link_add_symbols
  PARAMS ((bfd *, struct bfd_link_info *, bfd_boolean collect));
static bfd_boolean generic_link_add_object_symbols
  PARAMS ((bfd *, struct bfd_link_info *, bfd_boolean collect));
static bfd_boolean generic_link_check_archive_element_no_collect
  PARAMS ((bfd *, struct bfd_link_info *, bfd_boolean *pneeded));
static bfd_boolean generic_link_check_archive_element_collect
  PARAMS ((bfd *, struct bfd_link_info *, bfd_boolean *pneeded));
bfd_boolean generic_link_check_archive_element
  PARAMS ((bfd *, struct bfd_link_info *, bfd_boolean *pneeded,
	   bfd_boolean collect));
static bfd_boolean generic_link_add_symbol_list
  PARAMS ((bfd *, struct bfd_link_info *, bfd_size_type count, asymbol **,
	   bfd_boolean collect));
static bfd *hash_entry_bfd
  PARAMS ((struct bfd_link_hash_entry *));
static void set_symbol_from_hash
  PARAMS ((asymbol *, struct bfd_link_hash_entry *));
static bfd_boolean generic_add_output_symbol
  PARAMS ((bfd *, size_t *psymalloc, asymbol *));
static bfd_boolean default_data_link_order
  PARAMS ((bfd *, struct bfd_link_info *, asection *,
	   struct bfd_link_order *));
static bfd_boolean default_indirect_link_order
  PARAMS ((bfd *, struct bfd_link_info *, asection *,
	   struct bfd_link_order *, bfd_boolean));
bfd_boolean (*pic30_bfd_has_signature)
  PARAMS ((bfd *, unsigned int *, unsigned int *, unsigned int *));
void (*pic30_update_object_compatibility)
  PARAMS ((const char *, bfd *, unsigned int *, unsigned int *));
bfd_boolean pic30_is_valid_archive_element
  PARAMS ((struct bfd_link_info *, asymbol *, bfd *, unsigned int *,
             unsigned int *));

/* The link hash table structure is defined in bfdlink.h.  It provides
   a base hash table which the backend specific hash tables are built
   upon.  */

/* Routine to create an entry in the link hash table.  */

struct bfd_hash_entry *
_bfd_link_hash_newfunc (entry, table, string)
     struct bfd_hash_entry *entry;
     struct bfd_hash_table *table;
     const char *string;
{
  /* Allocate the structure if it has not already been allocated by a
     subclass.  */
  if (entry == NULL)
    {
      entry = (struct bfd_hash_entry *)
	bfd_hash_allocate (table, sizeof (struct bfd_link_hash_entry));
      if (entry == NULL)
	return entry;
    }

  /* Call the allocation method of the superclass.  */
  entry = bfd_hash_newfunc (entry, table, string);
  if (entry)
    {
      struct bfd_link_hash_entry *h = (struct bfd_link_hash_entry *) entry;

      /* Initialize the local fields.  */
      h->type = bfd_link_hash_new;
      h->next = NULL;
    }

  return entry;
}

/* Initialize a link hash table.  The BFD argument is the one
   responsible for creating this table.  */

bfd_boolean
_bfd_link_hash_table_init (table, abfd, newfunc)
     struct bfd_link_hash_table *table;
     bfd *abfd;
     struct bfd_hash_entry *(*newfunc) PARAMS ((struct bfd_hash_entry *,
						struct bfd_hash_table *,
						const char *));
{
  table->creator = abfd->xvec;
  table->undefs = NULL;
  table->undefs_tail = NULL;
  table->type = bfd_link_generic_hash_table;

  return bfd_hash_table_init (&table->table, newfunc);
}

/* Look up a symbol in a link hash table.  If follow is TRUE, we
   follow bfd_link_hash_indirect and bfd_link_hash_warning links to
   the real symbol.  */

struct bfd_link_hash_entry *
bfd_link_hash_lookup (table, string, create, copy, follow)
     struct bfd_link_hash_table *table;
     const char *string;
     bfd_boolean create;
     bfd_boolean copy;
     bfd_boolean follow;
{
  struct bfd_link_hash_entry *ret;

  ret = ((struct bfd_link_hash_entry *)
	 bfd_hash_lookup (&table->table, string, create, copy));

  if (follow && ret != (struct bfd_link_hash_entry *) NULL)
    {
      while (ret->type == bfd_link_hash_indirect
	     || ret->type == bfd_link_hash_warning)
	ret = ret->u.i.link;
    }

  return ret;
}

/* Look up a symbol in the main linker hash table if the symbol might
   be wrapped.  This should only be used for references to an
   undefined symbol, not for definitions of a symbol.  */

struct bfd_link_hash_entry *
bfd_wrapped_link_hash_lookup (abfd, info, string, create, copy, follow)
     bfd *abfd;
     struct bfd_link_info *info;
     const char *string;
     bfd_boolean create;
     bfd_boolean copy;
     bfd_boolean follow;
{
  bfd_size_type amt;

  if (info->wrap_hash != NULL)
    {
      const char *l;

      l = string;
      if (*l == bfd_get_symbol_leading_char (abfd))
	++l;

#undef WRAP
#define WRAP "__wrap_"

      if (bfd_hash_lookup (info->wrap_hash, l, FALSE, FALSE) != NULL)
	{
	  char *n;
	  struct bfd_link_hash_entry *h;

	  /* This symbol is being wrapped.  We want to replace all
             references to SYM with references to __wrap_SYM.  */

	  amt = strlen (l) + sizeof WRAP + 1;
	  n = (char *) bfd_malloc (amt);
	  if (n == NULL)
	    return NULL;

	  /* Note that symbol_leading_char may be '\0'.  */
	  n[0] = bfd_get_symbol_leading_char (abfd);
	  n[1] = '\0';
	  strcat (n, WRAP);
	  strcat (n, l);
	  h = bfd_link_hash_lookup (info->hash, n, create, TRUE, follow);
	  free (n);
	  return h;
	}

#undef WRAP

#undef REAL
#define REAL "__real_"

      if (*l == '_'
	  && strncmp (l, REAL, sizeof REAL - 1) == 0
	  && bfd_hash_lookup (info->wrap_hash, l + sizeof REAL - 1,
			      FALSE, FALSE) != NULL)
	{
	  char *n;
	  struct bfd_link_hash_entry *h;

	  /* This is a reference to __real_SYM, where SYM is being
             wrapped.  We want to replace all references to __real_SYM
             with references to SYM.  */

	  amt = strlen (l + sizeof REAL - 1) + 2;
	  n = (char *) bfd_malloc (amt);
	  if (n == NULL)
	    return NULL;

	  /* Note that symbol_leading_char may be '\0'.  */
	  n[0] = bfd_get_symbol_leading_char (abfd);
	  n[1] = '\0';
	  strcat (n, l + sizeof REAL - 1);
	  h = bfd_link_hash_lookup (info->hash, n, create, TRUE, follow);
	  free (n);
	  return h;
	}

#undef REAL
    }

  return bfd_link_hash_lookup (info->hash, string, create, copy, follow);
}

/* Traverse a generic link hash table.  The only reason this is not a
   macro is to do better type checking.  This code presumes that an
   argument passed as a struct bfd_hash_entry * may be caught as a
   struct bfd_link_hash_entry * with no explicit cast required on the
   call.  */

void
bfd_link_hash_traverse (table, func, info)
     struct bfd_link_hash_table *table;
     bfd_boolean (*func) PARAMS ((struct bfd_link_hash_entry *, PTR));
     PTR info;
{
  bfd_hash_traverse (&table->table,
		     ((bfd_boolean (*) PARAMS ((struct bfd_hash_entry *, PTR)))
		      func),
		     info);
}

/* Add a symbol to the linker hash table undefs list.  */

INLINE void
bfd_link_add_undef (table, h)
     struct bfd_link_hash_table *table;
     struct bfd_link_hash_entry *h;
{
  BFD_ASSERT (h->next == NULL);
  if (table->undefs_tail != (struct bfd_link_hash_entry *) NULL)
    table->undefs_tail->next = h;
  if (table->undefs == (struct bfd_link_hash_entry *) NULL)
    table->undefs = h;
  table->undefs_tail = h;
}

/* Routine to create an entry in a generic link hash table.  */

struct bfd_hash_entry *
_bfd_generic_link_hash_newfunc (entry, table, string)
     struct bfd_hash_entry *entry;
     struct bfd_hash_table *table;
     const char *string;
{
  /* Allocate the structure if it has not already been allocated by a
     subclass.  */
  if (entry == NULL)
    {
      entry = (struct bfd_hash_entry *)
	bfd_hash_allocate (table, sizeof (struct generic_link_hash_entry));
      if (entry == NULL)
	return entry;
    }

  /* Call the allocation method of the superclass.  */
  entry = _bfd_link_hash_newfunc (entry, table, string);
  if (entry)
    {
      struct generic_link_hash_entry *ret;

      /* Set local fields.  */
      ret = (struct generic_link_hash_entry *) entry;
      ret->written = FALSE;
      ret->sym = NULL;
    }

  return entry;
}

/* Create a generic link hash table.  */

struct bfd_link_hash_table *
_bfd_generic_link_hash_table_create (abfd)
     bfd *abfd;
{
  struct generic_link_hash_table *ret;
  bfd_size_type amt = sizeof (struct generic_link_hash_table);

  ret = (struct generic_link_hash_table *) bfd_malloc (amt);
  if (ret == NULL)
    return (struct bfd_link_hash_table *) NULL;
  if (! _bfd_link_hash_table_init (&ret->root, abfd,
				   _bfd_generic_link_hash_newfunc))
    {
      free (ret);
      return (struct bfd_link_hash_table *) NULL;
    }
  return &ret->root;
}

void
_bfd_generic_link_hash_table_free (hash)
     struct bfd_link_hash_table *hash;
{
  struct generic_link_hash_table *ret
    = (struct generic_link_hash_table *) hash;

  bfd_hash_table_free (&ret->root.table);
  free (ret);
}

/* Grab the symbols for an object file when doing a generic link.  We
   store the symbols in the outsymbols field.  We need to keep them
   around for the entire link to ensure that we only read them once.
   If we read them multiple times, we might wind up with relocs and
   the hash table pointing to different instances of the symbol
   structure.  */

static bfd_boolean
generic_link_read_symbols (abfd)
     bfd *abfd;
{
  if (bfd_get_outsymbols (abfd) == (asymbol **) NULL)
    {
      long symsize;
      long symcount;

      symsize = bfd_get_symtab_upper_bound (abfd);
      if (symsize < 0)
	return FALSE;
      bfd_get_outsymbols (abfd) =
	(asymbol **) bfd_alloc (abfd, (bfd_size_type) symsize);
      if (bfd_get_outsymbols (abfd) == NULL && symsize != 0)
	return FALSE;
      symcount = bfd_canonicalize_symtab (abfd, bfd_get_outsymbols (abfd));
      if (symcount < 0)
	return FALSE;
      bfd_get_symcount (abfd) = symcount;
    }

  return TRUE;
}

/* Generic function to add symbols to from an object file to the
   global hash table.  This version does not automatically collect
   constructors by name.  */

bfd_boolean
_bfd_generic_link_add_symbols (abfd, info)
     bfd *abfd;
     struct bfd_link_info *info;
{
    
    return generic_link_add_symbols (abfd, info, FALSE);
 
}

/* Generic function to add symbols from an object file to the global
   hash table.  This version automatically collects constructors by
   name, as the collect2 program does.  It should be used for any
   target which does not provide some other mechanism for setting up
   constructors and destructors; these are approximately those targets
   for which gcc uses collect2 and do not support stabs.  */

bfd_boolean
_bfd_generic_link_add_symbols_collect (abfd, info)
     bfd *abfd;
     struct bfd_link_info *info;
{
  return generic_link_add_symbols (abfd, info, TRUE);
}

/* Indicate that we are only retrieving symbol values from this
   section.  We want the symbols to act as though the values in the
   file are absolute.  */

void
_bfd_generic_link_just_syms (sec, info)
     asection *sec;
     struct bfd_link_info *info ATTRIBUTE_UNUSED;
{
  sec->output_section = bfd_abs_section_ptr;
  sec->output_offset = sec->vma;
}

/* Add symbols from an object file to the global hash table.  */

static bfd_boolean
generic_link_add_symbols (abfd, info, collect)
     bfd *abfd;
     struct bfd_link_info *info;
     bfd_boolean collect;
{
  bfd_boolean ret;

  switch (bfd_get_format (abfd))
    {
    case bfd_object:
      ret = generic_link_add_object_symbols (abfd, info, collect);
      break;
    case bfd_archive:
      ret = (_bfd_generic_link_add_archive_symbols
	     (abfd, info,
	      (collect
	       ? generic_link_check_archive_element_collect
	       : generic_link_check_archive_element_no_collect)));
      break;
    default:
      bfd_set_error (bfd_error_wrong_format);
      ret = FALSE;
    }

  return ret;
}

/* Add symbols from an object file to the global hash table.  */

static bfd_boolean
generic_link_add_object_symbols (abfd, info, collect)
     bfd *abfd;
     struct bfd_link_info *info;
     bfd_boolean collect;
{
  bfd_size_type symcount;
  struct symbol_cache_entry **outsyms;

  if (! generic_link_read_symbols (abfd))
    return FALSE;
  symcount = _bfd_generic_link_get_symcount (abfd);
  outsyms = _bfd_generic_link_get_symbols (abfd);
  return generic_link_add_symbol_list (abfd, info, symcount, outsyms, collect);
}

/* We build a hash table of all symbols defined in an archive.  */

/* An archive symbol may be defined by multiple archive elements.
   This linked list is used to hold the elements.  */

struct archive_list
{
  struct archive_list *next;
  unsigned int indx;
};

/* An entry in an archive hash table.  */

struct archive_hash_entry
{
  struct bfd_hash_entry root;
  /* Where the symbol is defined.  */
  struct archive_list *defs;
};

/* An archive hash table itself.  */

struct archive_hash_table
{
  struct bfd_hash_table table;
};

static struct bfd_hash_entry *archive_hash_newfunc
  PARAMS ((struct bfd_hash_entry *, struct bfd_hash_table *, const char *));
static bfd_boolean archive_hash_table_init
  PARAMS ((struct archive_hash_table *,
	   struct bfd_hash_entry *(*) (struct bfd_hash_entry *,
				       struct bfd_hash_table *,
				       const char *)));

/* Create a new entry for an archive hash table.  */

static struct bfd_hash_entry *
archive_hash_newfunc (entry, table, string)
     struct bfd_hash_entry *entry;
     struct bfd_hash_table *table;
     const char *string;
{
  struct archive_hash_entry *ret = (struct archive_hash_entry *) entry;

  /* Allocate the structure if it has not already been allocated by a
     subclass.  */
  if (ret == (struct archive_hash_entry *) NULL)
    ret = ((struct archive_hash_entry *)
	   bfd_hash_allocate (table, sizeof (struct archive_hash_entry)));
  if (ret == (struct archive_hash_entry *) NULL)
    return NULL;

  /* Call the allocation method of the superclass.  */
  ret = ((struct archive_hash_entry *)
	 bfd_hash_newfunc ((struct bfd_hash_entry *) ret, table, string));

  if (ret)
    {
      /* Initialize the local fields.  */
      ret->defs = (struct archive_list *) NULL;
    }

  return (struct bfd_hash_entry *) ret;
}

/* Initialize an archive hash table.  */

static bfd_boolean
archive_hash_table_init (table, newfunc)
     struct archive_hash_table *table;
     struct bfd_hash_entry *(*newfunc) PARAMS ((struct bfd_hash_entry *,
						struct bfd_hash_table *,
						const char *));
{
  return bfd_hash_table_init (&table->table, newfunc);
}

/* Look up an entry in an archive hash table.  */

#define archive_hash_lookup(t, string, create, copy) \
  ((struct archive_hash_entry *) \
   bfd_hash_lookup (&(t)->table, (string), (create), (copy)))

/* Allocate space in an archive hash table.  */

#define archive_hash_allocate(t, size) bfd_hash_allocate (&(t)->table, (size))

/* Free an archive hash table.  */

#define archive_hash_table_free(t) bfd_hash_table_free (&(t)->table)

/* Generic function to add symbols from an archive file to the global
   hash file.  This function presumes that the archive symbol table
   has already been read in (this is normally done by the
   bfd_check_format entry point).  It looks through the undefined and
   common symbols and searches the archive symbol table for them.  If
   it finds an entry, it includes the associated object file in the
   link.

   The old linker looked through the archive symbol table for
   undefined symbols.  We do it the other way around, looking through
   undefined symbols for symbols defined in the archive.  The
   advantage of the newer scheme is that we only have to look through
   the list of undefined symbols once, whereas the old method had to
   re-search the symbol table each time a new object file was added.

   The CHECKFN argument is used to see if an object file should be
   included.  CHECKFN should set *PNEEDED to TRUE if the object file
   should be included, and must also call the bfd_link_info
   add_archive_element callback function and handle adding the symbols
   to the global hash table.  CHECKFN should only return FALSE if some
   sort of error occurs.

   For some formats, such as a.out, it is possible to look through an
   object file but not actually include it in the link.  The
   archive_pass field in a BFD is used to avoid checking the symbols
   of an object files too many times.  When an object is included in
   the link, archive_pass is set to -1.  If an object is scanned but
   not included, archive_pass is set to the pass number.  The pass
   number is incremented each time a new object file is included.  The
   pass number is used because when a new object file is included it
   may create new undefined symbols which cause a previously examined
   object file to be included.  */

bfd_boolean
_bfd_generic_link_add_archive_symbols (abfd, info, checkfn)
     bfd *abfd;
     struct bfd_link_info *info;
     bfd_boolean (*checkfn)
       PARAMS ((bfd *, struct bfd_link_info *, bfd_boolean *pneeded));
{
  carsym *arsyms;
  carsym *arsym_end;
  register carsym *arsym;
  int pass;
  struct archive_hash_table arsym_hash;
  unsigned int indx;
  struct bfd_link_hash_entry **pundef;


#if PIC30
  { static int smartio_run=0;

    if (smartio_run == 0) {
      /* look through the undef list and adds those symbols that are smartio
         to the undefined list */
      pic30_smartio_symbols(info);
    }
  }
#endif

  if (! bfd_has_map (abfd))
    {
      /* An empty archive is a special case.  */
      if (bfd_openr_next_archived_file (abfd, (bfd *) NULL) == NULL)
	return TRUE;
      bfd_set_error (bfd_error_no_armap);
      return FALSE;
    }

  arsyms = bfd_ardata (abfd)->symdefs;
  arsym_end = arsyms + bfd_ardata (abfd)->symdef_count;

  /* The archive_pass field in the archive itself is used to
     initialize PASS, sine we may search the same archive multiple
     times.  */
  pass = abfd->archive_pass + 1;

  /* In order to quickly determine whether an symbol is defined in
     this archive, we build a hash table of the symbols.  */
  if (! archive_hash_table_init (&arsym_hash, archive_hash_newfunc))
    return FALSE;
  for (arsym = arsyms, indx = 0; arsym < arsym_end; arsym++, indx++)
    {
      struct archive_hash_entry *arh;
      struct archive_list *l, **pp;

      arh = archive_hash_lookup (&arsym_hash, arsym->name, TRUE, FALSE);
      if (arh == (struct archive_hash_entry *) NULL)
	goto error_return;
      l = ((struct archive_list *)
	   archive_hash_allocate (&arsym_hash, sizeof (struct archive_list)));
      if (l == NULL)
	goto error_return;
      l->indx = indx;
      for (pp = &arh->defs;
	   *pp != (struct archive_list *) NULL;
	   pp = &(*pp)->next)
	;
      *pp = l;
      l->next = NULL;
    }

  /* New undefined symbols are added to the end of the list, so we
     only need to look through it once.  */
  pundef = &info->hash->undefs;
  while (*pundef != (struct bfd_link_hash_entry *) NULL)
    {
      struct bfd_link_hash_entry *h;
      struct archive_hash_entry *arh;
      struct archive_list *l;

      h = *pundef;

      /* When a symbol is defined, it is not necessarily removed from
	 the list.  */
#if PIC30
      /* we may have added a new hash */
      if (h->type == bfd_link_hash_new)
        ;
      else
#endif
      if (h->type != bfd_link_hash_undefined
	  && h->type != bfd_link_hash_common)
	{
	  /* Remove this entry from the list, for general cleanliness
	     and because we are going to look through the list again
	     if we search any more libraries.  We can't remove the
	     entry if it is the tail, because that would lose any
	     entries we add to the list later on (it would also cause
	     us to lose track of whether the symbol has been
	     referenced).  */
	  if (*pundef != info->hash->undefs_tail)
	    *pundef = (*pundef)->next;
	  else
	    pundef = &(*pundef)->next;
	  continue;
	}

      /* Look for this symbol in the archive symbol map.  */
      arh = archive_hash_lookup (&arsym_hash, h->root.string, FALSE, FALSE);
      if (arh == (struct archive_hash_entry *) NULL)
	{
	  /* If we haven't found the exact symbol we're looking for,
	     let's look for its import thunk */
	  if (info->pei386_auto_import)
	    {
	      bfd_size_type amt = strlen (h->root.string) + 10;
	      char *buf = (char *) bfd_malloc (amt);
	      if (buf == NULL)
		return FALSE;

	      sprintf (buf, "__imp_%s", h->root.string);
	      arh = archive_hash_lookup (&arsym_hash, buf, FALSE, FALSE);
	      free(buf);
	    }
	  if (arh == (struct archive_hash_entry *) NULL)
	    {
	      pundef = &(*pundef)->next;
	      continue;
	    }
	}
      /* Look at all the objects which define this symbol.  */
      for (l = arh->defs; l != (struct archive_list *) NULL; l = l->next)
	{
	  bfd *element;
	  bfd_boolean needed;

#if PIC30
          /* we may have added a new hash */
          if (h->type == bfd_link_hash_new)
            ;
          else
#endif
	  /* If the symbol has gotten defined along the way, quit.  */
	  if (h->type != bfd_link_hash_undefined
	      && h->type != bfd_link_hash_common)
	    break;

	  element = bfd_get_elt_at_index (abfd, l->indx);
	  if (element == (bfd *) NULL)
	    goto error_return;

	  /* If we've already included this element, or if we've
	     already checked it on this pass, continue.  */
	  if (element->archive_pass == -1
	      || element->archive_pass == pass)
	    continue;

	  /* If we can't figure this element out, just ignore it.  */
	  if (! bfd_check_format (element, bfd_object))
	    {
	      element->archive_pass = -1;
	      continue;
	    }

	  /* CHECKFN will see if this element should be included, and
	     go ahead and include it if appropriate.  */
	  if (! (*checkfn) (element, info, &needed))
	     continue;
	   

	  if (! needed)
	    element->archive_pass = pass;
	  else
	    {
	      element->archive_pass = -1;

	      /* Increment the pass count to show that we may need to
		 recheck object files which were already checked.  */
	      ++pass;
	    }
	}

      pundef = &(*pundef)->next;
    }

  archive_hash_table_free (&arsym_hash);

  /* Save PASS in case we are called again.  */
  abfd->archive_pass = pass;

  return TRUE;

 error_return:
  archive_hash_table_free (&arsym_hash);
  return FALSE;
}

/* See if we should include an archive element.  This version is used
   when we do not want to automatically collect constructors based on
   the symbol name, presumably because we have some other mechanism
   for finding them.  */

static bfd_boolean
generic_link_check_archive_element_no_collect (abfd, info, pneeded)
     bfd *abfd;
     struct bfd_link_info *info;
     bfd_boolean *pneeded;
{
  return generic_link_check_archive_element (abfd, info, pneeded, FALSE);
}

/* See if we should include an archive element.  This version is used
   when we want to automatically collect constructors based on the
   symbol name, as collect2 does.  */

static bfd_boolean
generic_link_check_archive_element_collect (abfd, info, pneeded)
     bfd *abfd;
     struct bfd_link_info *info;
     bfd_boolean *pneeded;
{
  return generic_link_check_archive_element (abfd, info, pneeded, TRUE);
}

#ifdef PIC30
struct pic30_undefsym_table *pic30_undef_symbols;
extern const char *pic30_startup0_file;
extern const char *pic30_startup1_file;
extern bfd_boolean pic30_debug, pic30_data_init;
extern bfd_boolean pic30_select_objects;
extern bfd *pic30_output_bfd;
extern int pic30_is_dsp_machine(const bfd_arch_info_type *);

bfd_boolean
pic30_is_valid_archive_element(struct bfd_link_info *info,
                               asymbol *p, bfd *abfd,
                               unsigned int *new_mask_bits,
                               unsigned int *new_set_bits) {
  bfd_boolean result = TRUE;
  unsigned int signature_pairs = 0;
  unsigned int signature_mask = 0;
  unsigned int signature_set = 0;
  bfd_boolean has_sig = pic30_bfd_has_signature(abfd,
                                                &signature_pairs,
                                                &signature_mask,
                                                &signature_set);

  if (pic30_debug) 
    printf("pic30_is_valid_archive_element: looking for %s in %s\n",
         p->name, abfd->filename);

  /*
  ** Special processing for CRT0_STARTUP() file
  **
  ** If data_init is disabled, don't bother loading
  ** any object that defines the CRT0 key.
  **
  ** Otherwise check to see if the file name matches
  ** before loading this object.
  */
  if (strcmp(p->name, CRT0_KEY) == 0) {
    if (pic30_data_init == 0) {
      _bfd_generic_link_add_one_symbol (info, pic30_output_bfd, CRT0_KEY,
                                        BSF_GLOBAL, bfd_abs_section_ptr,
                                        0, CRT0_KEY, 1, 0, 0);
      if (pic30_debug)
        printf("\nData init is disabled, so %s defined as NULL\n", CRT0_KEY);
      result = FALSE;
    } else if (pic30_startup0_file && 
               (strcmp(abfd->filename, pic30_startup0_file) != 0)) {
      if (pic30_debug)
        printf("\n%s defines %s, but does not match CRT0 startup module\n",
               abfd->filename, CRT0_KEY);
      result = FALSE;
    }
  }

  /*
  ** Special processing for CRT1_STARTUP() file
  **
  ** If data_init is enabled, don't bother loading
  ** any object that defines the CRT1 key.
  **
  ** Otherwise check to see if the file name matches
  ** before loading this object.
  */
  if (result && strcmp(p->name, CRT1_KEY) == 0) {
    if (pic30_data_init) {
      _bfd_generic_link_add_one_symbol (info, pic30_output_bfd, CRT1_KEY,
                                        BSF_GLOBAL, bfd_abs_section_ptr,
                                        0, CRT1_KEY, 1, 0, 0);
      if (pic30_debug)
        printf("\nData init is enabled, so %s defined as NULL\n", CRT1_KEY);
      result = FALSE;
    } else if (pic30_startup1_file && 
               (strcmp(abfd->filename, pic30_startup1_file) != 0)) {
      if (pic30_debug)
        printf("\n%s defines %s, but does not match CRT1 startup module\n",
               abfd->filename, CRT1_KEY);
      result = FALSE;
    }
  }

#ifdef PIC30ELF
  if (strncmp(p->name, CRTMODE_KEY, sizeof(CRTMODE_KEY)-1) == 0) {
    if (pic30_data_init) {
      if (pic30_preserve_application_info) {
        if (strcmp(p->name,CRTMODE_KEY) == 0) {
          _bfd_generic_link_add_one_symbol (info, pic30_output_bfd, CRTMODE_KEY,
                                            BSF_GLOBAL, bfd_abs_section_ptr,
                                            0, CRTMODE_KEY, 1, 0, 0);
          if (pic30_debug) {
            printf("\nDefined as %s\n", CRTMODE_KEY);
          }
        } else {
          if (pic30_debug) {
            printf("\nSkipping %s\n", CRTMODE_KEY);
          }
          return FALSE;
        }
      } else {
        if (strcmp(p->name,CRTMODEOFF_KEY) == 0) {
          _bfd_generic_link_add_one_symbol (info, pic30_output_bfd,
                                            CRTMODEOFF_KEY, BSF_GLOBAL,
                                            bfd_abs_section_ptr, 0,
                                            CRTMODEOFF_KEY, 1, 0, 0);
        } else return FALSE;
      }
    }
  }
#endif
 
 /* In case when user refernces __reset in their code without defining it,
 *  the linker will look for an definition in the archive. 
 *  __reset is defined in more than one object (built with different options)
 *  as many other symbols. What is different with __reset is that it is defined
 *  in more than one source file form which objects are built in the archive
 *  with the same ptions. So when the linker finds the first object that
 *  defines this symbol and it's arch-compatible and it cannot really decide
 *  based on signature because the symbol didn't have any, it links that object
 *  even though it is not the one specified by the linker script. (bin30-230)
 *  FS */
 
 if (strcmp(p->name, "__reset") == 0) {
    if ((pic30_startup0_file &&
               (strcmp(abfd->filename, pic30_startup0_file) != 0)) &&
        (pic30_startup1_file &&
               (strcmp(abfd->filename, pic30_startup1_file) != 0)))
      return FALSE;
  }

  /*
  ** Compare object signatures
  */
  if (result && pic30_select_objects) {
    const char *dbg_str1 = "\n1) Seeking a definition for %s (0x%x 0x%x)\n";
    const char *dbg_str2 = "  consider %s (0x%x 0x%x) ... ";
    const char *dbg_str3 = "no signature found\n";
    struct pic30_undefsym_entry *usym = 0;
    unsigned int mask;

    /* Except for trivial cases, the undefined symbols table
       should already exist. */

    /* Load the entry for this symbol.
       If no entry exists, the symbol must have been declared
       in a linker script. Take no action here. */
    if (undefsyms) {
      usym = pic30_undefsym_lookup(undefsyms, p->name, 0, 0);
      if (usym) {
        if (usym->external_options_mask)
          if (global_signature_set && !usym->options_set) {
            fprintf(stderr,"Link Error: %s compilation options are not"
                     " compatible with the project global compilation"
                     " options.\n",
                     usym ->most_recent_reference->filename);
             exit(1);
             }
       usym->external_options_mask |= global_signature_mask;
       usym->options_set |= global_signature_set;
      }
    }

    if (pic30_debug && usym) {
      printf(dbg_str1, p->name, usym->external_options_mask,
             usym->options_set);
      printf(dbg_str2, abfd->filename, signature_mask, signature_set);
    }

    /* If the archive object has no signature,
       it's the same as having null signature.
       Then this object is compatible (xc16-100)*/

    /* If the archive object has a signature,
       compare it to the undefined symbol entry */
    if (usym && has_sig) {
      mask = signature_mask & usym->external_options_mask; 
      if ((mask & signature_set) == (mask & usym->options_set)) {
        unsigned int new_mask = global_signature_mask | signature_mask;
        unsigned int new_mask_delta = new_mask ^ global_signature_mask;
        unsigned int new_bits_in_mask = pic30_count_ones(new_mask_delta);

        unsigned int new_set = global_signature_set | signature_set;
        unsigned int new_set_delta = new_set ^ global_signature_set;
        unsigned int new_bits_in_set = pic30_count_ones(new_set_delta);
        

        /* new_bits_set are the number of bits that need to be added to
         *   the signature mask in order to choose this symbol ...
         *   it may not be the best option and it does distinguish between
         *   adding a requirement (and that requirement is clear) and adding
         *   a must-have requirement (1,0) == (1,1) for these purposes
         *
         * in either case it represents a further restriction on future
         * object selection 
         */
        if (pic30_debug)
          printf("OK [+%d bits, +%d]\n", new_bits_in_mask, new_bits_in_set);
        *new_mask_bits = new_bits_in_mask;
        *new_set_bits = new_bits_in_set;
        result = TRUE;
      } else {
        result = FALSE;
        if (pic30_debug)
          printf(" not compatible\n");
      }
    }          
  }

  /* ToDo: inherit boot, secure attributes */

  return result;
} 
#endif


/* See if we should include an archive element.  Optionally collect
   constructors.  */

bfd_boolean
generic_link_check_archive_element (abfd, info, pneeded, collect)
     bfd *abfd;
     struct bfd_link_info *info;
     bfd_boolean *pneeded;
     bfd_boolean collect;
{
  asymbol **pp, **ppend;
#ifdef PIC30
  struct pic30_deferred_archive_members *deferred_member = 0;
#endif

  *pneeded = FALSE;

   

  extern const bfd_arch_info_type *global_PROCESSOR;
    if (((global_PROCESSOR) &&
      (bfd_default_compatible(abfd->arch_info, global_PROCESSOR) == 0)) ||
      (bfd_default_compatible(abfd->arch_info, pic30_output_bfd->arch_info) == 0)){
    return FALSE;
  }

  /* Temporary check for instruction set compatibility. xc16-783*/
   if ((global_PROCESSOR != 0) &&
       (abfd->arch_info->mach != 0) &&
       (global_PROCESSOR != abfd->arch_info))
     if (!pic30_is_dsp_machine(global_PROCESSOR) &&
          pic30_is_dsp_machine(abfd->arch_info))
       return FALSE;

  if (! generic_link_read_symbols (abfd))
    return FALSE;

  pp = _bfd_generic_link_get_symbols (abfd);
  ppend = pp + _bfd_generic_link_get_symcount (abfd);
#ifdef PIC30
  for (; ((pp < ppend) || (deferred_member = pic30_pop_tail_archive())); pp++)
#else
  for (; pp < ppend; pp++)
#endif
    {
      asymbol *p;
      struct bfd_link_hash_entry *h;

#ifdef PIC30
      if (deferred_member) {
        BFD_ASSERT(pp >= ppend);
        p = deferred_member->sym;
      } else 
#endif
      p = *pp;

      /* We are only interested in globally visible symbols.  */
      if (! bfd_is_com_section (p->section)
	  && (p->flags & (BSF_GLOBAL | BSF_INDIRECT | BSF_WEAK)) == 0)
	continue;

      /* We are only interested if we know something about this
	 symbol, and it is undefined or common.  An undefined weak
	 symbol (type bfd_link_hash_undefweak) is not considered to be
	 a reference when pulling files out of an archive.  See the
	 SVR4 ABI, p. 4-27.  */
      h = bfd_link_hash_lookup (info->hash, bfd_asymbol_name (p), FALSE,
				FALSE, TRUE);

      if (h == (struct bfd_link_hash_entry *) NULL
          || (h->type != bfd_link_hash_undefined
              && h->type != bfd_link_hash_common))
        continue;
 
      /* P is a symbol we are looking for.  */

      /* we may need to pull this symbol in because it is a SMARTIO fn */
      if (pic30_force_keep_symbol && 
          pic30_force_keep_symbol(p->name, abfd->filename)) {
        /* I hate to duplicate code, but ... we may have no hash table */
        if (! bfd_is_com_section (p->section))
          {
            bfd_size_type symcount;
            asymbol **symbols;
#ifdef PIC30
            unsigned valid;
            unsigned int new_mask_bits,
                         new_set_bits;

            /* check this element against target-specific rules */
            valid = pic30_is_valid_archive_element (
                         info, p, abfd,
                         &new_mask_bits, &new_set_bits);
            if (valid) {
              if (new_mask_bits == 0) {
                if (pic30_debug) {
                  printf("...perfect match\n");
                  debug_deferred_archive_members();
                }
                pic30_remove_archive(NULL,p,abfd);
              } else if (deferred_member) {
                if (pic30_debug) {
                  printf("...taken match\n");
                }
              } else {
                pic30_defer_archive(NULL, p, abfd, info,
                                    new_mask_bits, new_set_bits, 0);
                continue;
              }
            } else {
              if (pic30_debug) {
                  printf("...No match\n");
              }
              continue;
            }
#endif

            /* This object file defines this symbol, so pull it in.  */
            if (!(*info->callbacks->add_archive_element) (info, abfd,
                                                          bfd_asymbol_name (p)))
              return FALSE;

            symcount = _bfd_generic_link_get_symcount (abfd);
            symbols = _bfd_generic_link_get_symbols (abfd);
            if (! generic_link_add_symbol_list (abfd, info, symcount,
                                                symbols, collect))
              return FALSE;

            *pneeded = TRUE;
            return TRUE;
          }
      }

#ifdef PIC30
      {
        unsigned int valid;
        unsigned int new_mask_bits,
                     new_set_bits;

        valid = pic30_is_valid_archive_element (
                       info, p, abfd,
                       &new_mask_bits, &new_set_bits);
        if (valid) {
          if (new_mask_bits == 0) {
            if (pic30_debug) {
              printf("...perfect match\n");
            }
            pic30_remove_archive(NULL, p,abfd);
          } else if (deferred_member) {
            if (pic30_debug) {
              printf("...taken match\n");
            }
          } else {
            pic30_defer_archive(NULL, p, abfd, info,
                                new_mask_bits, new_set_bits, 0);
            continue;
          }
        } else {
          if (pic30_debug) {
              printf("...No match\n");
          }
          continue;
        }
      }
#endif

      if (! bfd_is_com_section (p->section))
	{
	  bfd_size_type symcount;
	  asymbol **symbols;

	  /* This object file defines this symbol, so pull it in.  */
	  if (! (*info->callbacks->add_archive_element) (info, abfd,
							 bfd_asymbol_name (p)))
	    return FALSE;
	  symcount = _bfd_generic_link_get_symcount (abfd);
	  symbols = _bfd_generic_link_get_symbols (abfd);
	  if (! generic_link_add_symbol_list (abfd, info, symcount,
					      symbols, collect))
	    return FALSE;
	  *pneeded = TRUE;
	  return TRUE;

	}

      /* P is a common symbol.  */

      if (h->type == bfd_link_hash_undefined)
	{
	  bfd *symbfd;
	  bfd_vma size;
	  unsigned int power;

	  symbfd = h->u.undef.abfd;
	  if (symbfd == (bfd *) NULL)
	    {
	      /* This symbol was created as undefined from outside
		 BFD.  We assume that we should link in the object
		 file.  This is for the -u option in the linker.  */
	      if (! (*info->callbacks->add_archive_element)
		  (info, abfd, bfd_asymbol_name (p)))
		return FALSE;
	      *pneeded = TRUE;
	      return TRUE;

	    }

	  /* Turn the symbol into a common symbol but do not link in
	     the object file.  This is how a.out works.  Object
	     formats that require different semantics must implement
	     this function differently.  This symbol is already on the
	     undefs list.  We add the section to a common section
	     attached to symbfd to ensure that it is in a BFD which
	     will be linked in.  */
	  h->type = bfd_link_hash_common;
	  h->u.c.p =
	    ((struct bfd_link_hash_common_entry *)
	     bfd_hash_allocate (&info->hash->table,
				sizeof (struct bfd_link_hash_common_entry)));
	  if (h->u.c.p == NULL)
	    return FALSE;

	  size = bfd_asymbol_value (p);
	  h->u.c.size = size;

	  power = bfd_log2 (size);
	  if (power > 4)
	    power = 4;
	  h->u.c.p->alignment_power = power;

	  if (p->section == bfd_com_section_ptr)
	    h->u.c.p->section = bfd_make_section_old_way (symbfd, "COMMON");
	  else
	    h->u.c.p->section = bfd_make_section_old_way (symbfd,
							  p->section->name);
	  h->u.c.p->section->flags = SEC_ALLOC;
	}
      else
	{
	  /* Adjust the size of the common symbol if necessary.  This
	     is how a.out works.  Object formats that require
	     different semantics must implement this function
	     differently.  */
	  if (bfd_asymbol_value (p) > h->u.c.size)
	    h->u.c.size = bfd_asymbol_value (p);
	}
    }

  /* This archive element is not needed.  */
  return TRUE;
}

/* Add the symbols from an object file to the global hash table.  ABFD
   is the object file.  INFO is the linker information.  SYMBOL_COUNT
   is the number of symbols.  SYMBOLS is the list of symbols.  COLLECT
   is TRUE if constructors should be automatically collected by name
   as is done by collect2.  */

static bfd_boolean
generic_link_add_symbol_list (abfd, info, symbol_count, symbols, collect)
     bfd *abfd;
     struct bfd_link_info *info;
     bfd_size_type symbol_count;
     asymbol **symbols;
     bfd_boolean collect;
{
  asymbol **pp, **ppend;
#ifdef PIC30
  unsigned int signature_pairs = 0;
  unsigned int signature_mask = 0;
  unsigned int signature_set = 0;
  ivt_record_type *r;
  ivt_record_type *next;
#endif

#ifdef PIC30
  if (pic30_debug)
    printf("\nLoading symbols from %s\n", abfd->filename);

  /*
  ** If pic30_select_objects is enabled, look for a signature section.
  **
  ** Format:
  **   int number_of_pairs;
  **   int signature_pairs[2][];
  */
  if (pic30_select_objects) {
    bfd_boolean has_sig = pic30_bfd_has_signature(abfd,
                                                  &signature_pairs,
                                                  &signature_mask,
                                                  &signature_set);
    global_signature_mask |= signature_mask;
    global_signature_set |= signature_set;
 
    if (pic30_debug) {
      if (has_sig)
        printf("  signature: %d 0x%x 0x%x", signature_pairs,
               signature_mask, signature_set);
      else
        printf("  (no signature)");
    }
  }

  if (pic30_debug)
    printf("\n");
#endif

  pp = symbols;
  ppend = symbols + symbol_count;
  for (; pp < ppend; pp++)
    {
      asymbol *p;

      p = *pp;

      if ((p->flags & (BSF_INDIRECT
		       | BSF_WARNING
		       | BSF_GLOBAL
		       | BSF_CONSTRUCTOR
		       | BSF_WEAK)) != 0
	  || bfd_is_und_section (bfd_get_section (p))
	  || bfd_is_com_section (bfd_get_section (p))
	  || bfd_is_ind_section (bfd_get_section (p)))
	{
	  const char *name;
	  const char *string;
#if defined(PIC30ELF)
	  struct elf_link_hash_entry *h;
#else
          struct generic_link_hash_entry *h;
#endif
	  struct bfd_link_hash_entry *bh;

	  name = bfd_asymbol_name (p);
	  if (((p->flags & BSF_INDIRECT) != 0
	       || bfd_is_ind_section (p->section))
	      && pp + 1 < ppend)
	    {
	      pp++;
	      string = bfd_asymbol_name (*pp);
	    }
	  else if ((p->flags & BSF_WARNING) != 0
		   && pp + 1 < ppend)
	    {
	      /* The name of P is actually the warning string, and the
		 next symbol is the one to warn about.  */
	      string = name;
	      pp++;
	      name = bfd_asymbol_name (*pp);
	    }
	  else
	    string = NULL;

	  bh = NULL;
	  if (! (_bfd_generic_link_add_one_symbol
		 (info, abfd, name, p->flags, bfd_get_section (p),
		  p->value, string, FALSE, collect, &bh)))
	    return FALSE;
#if defined(PIC30ELF)
	  h = (struct elf_link_hash_entry *) bh;
#else
          h = (struct generic_link_hash_entry *) bh;
#endif

	  /* If this is a constructor symbol, and the linker didn't do
             anything with it, then we want to just pass the symbol
             through to the output file.  This will happen when
             linking with -r.  */
	  if ((p->flags & BSF_CONSTRUCTOR) != 0
	      && (h == NULL || h->root.type == bfd_link_hash_new))
	    {
	      p->udata.p = NULL;
	      continue;
	    }

	  /* Save the BFD symbol so that we don't lose any backend
	     specific information that may be attached to it.  We only
	     want this one if it gives more information than the
	     existing one; we don't want to replace a defined symbol
	     with an undefined one.  This routine may be called with a
	     hash table other than the generic hash table, so we only
	     do this if we are certain that the hash table is a
	     generic one.  */
	  if (info->hash->creator == abfd->xvec)
	    {
	      if (h->sym == (asymbol *) NULL
		  || (! bfd_is_und_section (bfd_get_section (p))
		      && (! bfd_is_com_section (bfd_get_section (p))
			  || bfd_is_und_section (bfd_get_section (h->sym)))))
		{
		  h->sym = p;
		  /* BSF_OLD_COMMON is a hack to support COFF reloc
		     reading, and it should go away when the COFF
		     linker is switched to the new version.  */
		  if (bfd_is_com_section (bfd_get_section (p)))
		    p->flags |= BSF_OLD_COMMON;
		}
	    }
          
	  /* Store a back pointer from the symbol to the hash
	     table entry for the benefit of relaxation code until
	     it gets rewritten to not use asymbol structures.
	     Setting this is also used to check whether these
	     symbols were set up by the generic linker.  */
	p->udata.p = (PTR) h;

#ifdef PIC30
          /* If this symbol is undefined, compare the signature of 
             this object to any previous references. If the signatures
             are compatible, create or update a composite signature
             and store it. Otherwise report an error. */
          if (pic30_select_objects &&
              bfd_is_und_section (bfd_get_section (p))) {
            pic30_update_object_compatibility(p->name, abfd,
                                              &signature_mask,
                                              &signature_set);
          }

#endif

	}

        for (r = ivt_records_list; r != NULL; r = next) {
          next = r->next;
          if ((r->name && (strcmp(r->name, p->name+1))) == 0) {
            asection *ivt_sec;
            char *sec_name;
           
            if ((p->flags & BSF_DEBUGGING) != 0) {
              break;
            }
            if (r->is_alternate_vector) {
              sec_name = (char *)
                           xmalloc( strlen(r->name) + strlen(".aivt.") + 2);
              (void) sprintf(sec_name, ".aivt.%s", r->name);
            }
            else {
              sec_name = (char *)
                           xmalloc( strlen(r->name) + strlen(".ivt.") + 2);
              (void) sprintf(sec_name, ".ivt.%s", r->name);
            }
            r->flags |= ivt_seen;
            r->sec_name = sec_name;
            ivt_sec = bfd_make_section(abfd, sec_name);
            ivt_sec->flags |= (SEC_HAS_CONTENTS | SEC_LOAD |
                             SEC_CODE | SEC_ALLOC | SEC_KEEP);
            PIC30_SET_ABSOLUTE_ATTR(ivt_sec);
            ivt_sec->linker_generated = 1;
            ivt_sec->_raw_size = ivt_sec->_cooked_size = 4;
            r->ivt_sec = ivt_sec;
            if ((r->is_alternate_vector) && (pic30_has_fixed_aivt)) {
              if (aivt_base == 0) {
                 fprintf(stderr,"Link Error: can't locate alternate vector "
                                "table base address\n");
                 abort();
              }
              bfd_set_section_vma(abfd, ivt_sec, aivt_base + r->offset);
            } else {
              if (ivt_base == 0) {
                 fprintf(stderr,"Link Error: can't locate vector "
                                "table base address\n");
                 abort();
              }
              bfd_set_section_vma(abfd, ivt_sec, ivt_base + r->offset);
            }
          }
        }

    }

  return TRUE;
}

/* We use a state table to deal with adding symbols from an object
   file.  The first index into the state table describes the symbol
   from the object file.  The second index into the state table is the
   type of the symbol in the hash table.  */

/* The symbol from the object file is turned into one of these row
   values.  */

enum link_row
{
  UNDEF_ROW,		/* Undefined.  */
  UNDEFW_ROW,		/* Weak undefined.  */
  DEF_ROW,		/* Defined.  */
  DEFW_ROW,		/* Weak defined.  */
  COMMON_ROW,		/* Common.  */
  INDR_ROW,		/* Indirect.  */
  WARN_ROW,		/* Warning.  */
  SET_ROW		/* Member of set.  */
};

/* apparently needed for Hitachi 3050R(HI-UX/WE2)? */
#undef FAIL

/* The actions to take in the state table.  */

enum link_action
{
  FAIL,		/* Abort.  */
  UND,		/* Mark symbol undefined.  */
  WEAK,		/* Mark symbol weak undefined.  */
  DEF,		/* Mark symbol defined.  */
  DEFW,		/* Mark symbol weak defined.  */
  COM,		/* Mark symbol common.  */
  REF,		/* Mark defined symbol referenced.  */
  CREF,		/* Possibly warn about common reference to defined symbol.  */
  CDEF,		/* Define existing common symbol.  */
  NOACT,	/* No action.  */
  BIG,		/* Mark symbol common using largest size.  */
  MDEF,		/* Multiple definition error.  */
  MIND,		/* Multiple indirect symbols.  */
  IND,		/* Make indirect symbol.  */
  CIND,		/* Make indirect symbol from existing common symbol.  */
  SET,		/* Add value to set.  */
  MWARN,	/* Make warning symbol.  */
  WARN,		/* Issue warning.  */
  CWARN,	/* Warn if referenced, else MWARN.  */
  CYCLE,	/* Repeat with symbol pointed to.  */
  REFC,		/* Mark indirect symbol referenced and then CYCLE.  */
  WARNC		/* Issue warning and then CYCLE.  */
};

/* The state table itself.  The first index is a link_row and the
   second index is a bfd_link_hash_type.  */

static const enum link_action link_action[8][10] =
{
  /* current\prev    new    undef  undefw def    defw   com    indr   warn  */
  /* UNDEF_ROW 	*/  {UND,   NOACT, UND,   REF,   REF,   NOACT, REFC,  WARNC, REF, REF },
  /* UNDEFW_ROW	*/  {WEAK,  NOACT, NOACT, REF,   REF,   NOACT, REFC,  WARNC, REF, REF },
  /* DEF_ROW 	*/  {DEF,   DEF,   DEF,   MDEF,  DEF,   CDEF,  MDEF,  CYCLE, MDEF, DEF },
  /* DEFW_ROW 	*/  {DEFW,  DEFW,  DEFW,  NOACT, NOACT, NOACT, NOACT, CYCLE, NOACT, NOACT },
  /* COMMON_ROW	*/  {COM,   COM,   COM,   CREF,  COM,   BIG,   REFC,  WARNC, CREF, COM },
  /* INDR_ROW	*/  {IND,   IND,   IND,   MDEF,  IND,   CIND,  MIND,  CYCLE, MDEF, IND },
  /* WARN_ROW   */  {MWARN, WARN,  WARN,  CWARN, CWARN, WARN,  CWARN, NOACT, CWARN, CWARN },
  /* SET_ROW	*/  {SET,   SET,   SET,   SET,   SET,   SET,   CYCLE, CYCLE, SET, SET }
};

/* Most of the entries in the LINK_ACTION table are straightforward,
   but a few are somewhat subtle.

   A reference to an indirect symbol (UNDEF_ROW/indr or
   UNDEFW_ROW/indr) is counted as a reference both to the indirect
   symbol and to the symbol the indirect symbol points to.

   A reference to a warning symbol (UNDEF_ROW/warn or UNDEFW_ROW/warn)
   causes the warning to be issued.

   A common definition of an indirect symbol (COMMON_ROW/indr) is
   treated as a multiple definition error.  Likewise for an indirect
   definition of a common symbol (INDR_ROW/com).

   An indirect definition of a warning (INDR_ROW/warn) does not cause
   the warning to be issued.

   If a warning is created for an indirect symbol (WARN_ROW/indr) no
   warning is created for the symbol the indirect symbol points to.

   Adding an entry to a set does not count as a reference to a set,
   and no warning is issued (SET_ROW/warn).  */

/* Return the BFD in which a hash entry has been defined, if known.  */

static bfd *
hash_entry_bfd (h)
     struct bfd_link_hash_entry *h;
{
  while (h->type == bfd_link_hash_warning)
    h = h->u.i.link;
  switch (h->type)
    {
    default:
      return NULL;
    case bfd_link_hash_undefined:
    case bfd_link_hash_undefweak:
      return h->u.undef.abfd;
    case bfd_link_hash_defined:
    case bfd_link_hash_defweak:
      return h->u.def.section->owner;
    case bfd_link_hash_common:
      return h->u.c.p->section->owner;
    }
  /*NOTREACHED*/
}

/* Add a symbol to the global hash table.
   ABFD is the BFD the symbol comes from.
   NAME is the name of the symbol.
   FLAGS is the BSF_* bits associated with the symbol.
   SECTION is the section in which the symbol is defined; this may be
     bfd_und_section_ptr or bfd_com_section_ptr.
   VALUE is the value of the symbol, relative to the section.
   STRING is used for either an indirect symbol, in which case it is
     the name of the symbol to indirect to, or a warning symbol, in
     which case it is the warning string.
   COPY is TRUE if NAME or STRING must be copied into locally
     allocated memory if they need to be saved.
   COLLECT is TRUE if we should automatically collect gcc constructor
     or destructor names as collect2 does.
   HASHP, if not NULL, is a place to store the created hash table
     entry; if *HASHP is not NULL, the caller has already looked up
     the hash table entry, and stored it in *HASHP.  */

bfd_boolean
_bfd_generic_link_add_one_symbol (info, abfd, name, flags, section, value,
				  string, copy, collect, hashp)
     struct bfd_link_info *info;
     bfd *abfd;
     const char *name;
     flagword flags;
     asection *section;
     bfd_vma value;
     const char *string;
     bfd_boolean copy;
     bfd_boolean collect;
     struct bfd_link_hash_entry **hashp;
{
  enum link_row row;
  struct bfd_link_hash_entry *h;
  bfd_boolean cycle;

  if (bfd_is_ind_section (section)
      || (flags & BSF_INDIRECT) != 0)
    row = INDR_ROW;
  else if ((flags & BSF_WARNING) != 0)
    row = WARN_ROW;
  else if ((flags & BSF_CONSTRUCTOR) != 0)
    row = SET_ROW;
  else if (bfd_is_und_section (section))
    {
      if ((flags & BSF_WEAK) != 0)
	row = UNDEFW_ROW;
      else
	row = UNDEF_ROW;
    }
  else if ((flags & BSF_WEAK) != 0)
    row = DEFW_ROW;
  else if (bfd_is_com_section (section))
    row = COMMON_ROW;
  else
    row = DEF_ROW;

  if (hashp != NULL && *hashp != NULL)
    h = *hashp;
  else
    {
      if (row == UNDEF_ROW || row == UNDEFW_ROW)
	h = bfd_wrapped_link_hash_lookup (abfd, info, name, TRUE, copy, FALSE);
      else
	h = bfd_link_hash_lookup (info->hash, name, TRUE, copy, FALSE);
      if (h == NULL)
	{
	  if (hashp != NULL)
	    *hashp = NULL;
	  return FALSE;
	}
    }

  if (info->notice_all
      || (info->notice_hash != (struct bfd_hash_table *) NULL
	  && (bfd_hash_lookup (info->notice_hash, name, FALSE, FALSE)
	      != (struct bfd_hash_entry *) NULL)))
    {
      if (! (*info->callbacks->notice) (info, h->root.string, abfd, section,
					value))
	return FALSE;
    }

  if (hashp != (struct bfd_link_hash_entry **) NULL)
    *hashp = h;

  do
    {
      enum link_action action;

      cycle = FALSE;
      action = link_action[(int) row][(int) h->type];
      switch (action)
	{
	case FAIL:
	  abort ();

	case NOACT:
	  /* Do nothing.  */
	  break;

	case UND:
	  /* Make a new undefined symbol.  */
	  h->type = bfd_link_hash_undefined;
	  h->u.undef.abfd = abfd;
	  bfd_link_add_undef (info->hash, h);
	  break;

	case WEAK:
	  /* Make a new weak undefined symbol.  */
	  h->type = bfd_link_hash_undefweak;
	  h->u.undef.abfd = abfd;
	  break;

	case CDEF:
	  /* We have found a definition for a symbol which was
	     previously common.  */
	  BFD_ASSERT (h->type == bfd_link_hash_common);
	  if (! ((*info->callbacks->multiple_common)
		 (info, h->root.string,
		  h->u.c.p->section->owner, bfd_link_hash_common, h->u.c.size,
		  abfd, bfd_link_hash_defined, (bfd_vma) 0)))
	    return FALSE;
	  /* Fall through.  */
	case DEF:
	case DEFW:
	  {
	    enum bfd_link_hash_type oldtype;

	    /* Define a symbol.  */
	    oldtype = h->type;
	    if (action == DEFW) {
#ifdef PIC30
              if (flags & BSF_SHARED)
                h->type = bfd_link_hash_shared_defweak;
              else
#endif
	        h->type = bfd_link_hash_defweak;
            }
	    else {
#ifdef PIC30
              if (flags & BSF_SHARED)
                h->type = bfd_link_hash_shared_defined;
              else
#endif
	        h->type = bfd_link_hash_defined;
            }
	    h->u.def.section = section;
	    h->u.def.value = value;

	    /* If we have been asked to, we act like collect2 and
	       identify all functions that might be global
	       constructors and destructors and pass them up in a
	       callback.  We only do this for certain object file
	       types, since many object file types can handle this
	       automatically.  */
	    if (collect && name[0] == '_')
	      {
		const char *s;

		/* A constructor or destructor name starts like this:
		   _+GLOBAL_[_.$][ID][_.$] where the first [_.$] and
		   the second are the same character (we accept any
		   character there, in case a new object file format
		   comes along with even worse naming restrictions).  */

#define CONS_PREFIX "GLOBAL_"
#define CONS_PREFIX_LEN (sizeof CONS_PREFIX - 1)

		s = name + 1;
		while (*s == '_')
		  ++s;
		if (s[0] == 'G'
		    && strncmp (s, CONS_PREFIX, CONS_PREFIX_LEN - 1) == 0)
		  {
		    char c;

		    c = s[CONS_PREFIX_LEN + 1];
		    if ((c == 'I' || c == 'D')
			&& s[CONS_PREFIX_LEN] == s[CONS_PREFIX_LEN + 2])
		      {
			/* If this is a definition of a symbol which
                           was previously weakly defined, we are in
                           trouble.  We have already added a
                           constructor entry for the weak defined
                           symbol, and now we are trying to add one
                           for the new symbol.  Fortunately, this case
                           should never arise in practice.  */
			if (oldtype == bfd_link_hash_defweak)
			  abort ();

			if (! ((*info->callbacks->constructor)
			       (info, c == 'I',
				h->root.string, abfd, section, value)))
			  return FALSE;
		      }
		  }
	      }
	  }

	  break;

	case COM:
	  /* We have found a common definition for a symbol.  */
	  if (h->type == bfd_link_hash_new)
	    bfd_link_add_undef (info->hash, h);
	  h->type = bfd_link_hash_common;
	  h->u.c.p =
	    ((struct bfd_link_hash_common_entry *)
	     bfd_hash_allocate (&info->hash->table,
				sizeof (struct bfd_link_hash_common_entry)));
	  if (h->u.c.p == NULL)
	    return FALSE;

	  h->u.c.size = value;

#ifdef PIC30
          /* Increase the size to account for phantom bytes,
             and skip the auto-alignment. */
          h->u.c.size *= bfd_octets_per_byte (abfd);
#else
	  /* Select a default alignment based on the size.  This may
             be overridden by the caller.  */
	  {
	    unsigned int power;

	    power = bfd_log2 (value);
	    if (power > 4)
	      power = 4;
	    h->u.c.p->alignment_power = power;
	  }
#endif

	  /* The section of a common symbol is only used if the common
             symbol is actually allocated.  It basically provides a
             hook for the linker script to decide which output section
             the common symbols should be put in.  In most cases, the
             section of a common symbol will be bfd_com_section_ptr,
             the code here will choose a common symbol section named
             "COMMON", and the linker script will contain *(COMMON) in
             the appropriate place.  A few targets use separate common
             sections for small symbols, and they require special
             handling.  */
	  if (section == bfd_com_section_ptr)
	    {
	      h->u.c.p->section = bfd_make_section_old_way (abfd, "COMMON");
	      h->u.c.p->section->flags = SEC_ALLOC;
	    }
	  else if (section->owner != abfd)
	    {
	      h->u.c.p->section = bfd_make_section_old_way (abfd,
							    section->name);
	      h->u.c.p->section->flags = SEC_ALLOC;
	    }
	  else
	    h->u.c.p->section = section;
	  break;

	case REF:
	  /* A reference to a defined symbol.  */
	  if (h->next == NULL && info->hash->undefs_tail != h)
	    h->next = h;
	  break;

	case BIG:
	  /* We have found a common definition for a symbol which
	     already had a common definition.  Use the maximum of the
	     two sizes, and use the section required by the larger symbol.  */
	  BFD_ASSERT (h->type == bfd_link_hash_common);
	  if (! ((*info->callbacks->multiple_common)
		 (info, h->root.string,
		  h->u.c.p->section->owner, bfd_link_hash_common, h->u.c.size,
		  abfd, bfd_link_hash_common, value)))
	    return FALSE;

#ifdef PIC30
          value *= bfd_octets_per_byte (abfd);
#endif
	  if (value > h->u.c.size)
	    {
	      unsigned int power;

	      h->u.c.size = value;

	      /* Select a default alignment based on the size.  This may
		 be overridden by the caller.  */
	      power = bfd_log2 (value);
	      if (power > 4)
		power = 4;
#ifndef PIC30
	      h->u.c.p->alignment_power = power;
#endif

	      /* Some systems have special treatment for small commons,
		 hence we want to select the section used by the larger
		 symbol.  This makes sure the symbol does not go in a
		 small common section if it is now too large.  */
	      if (section == bfd_com_section_ptr)
		{
		  h->u.c.p->section
		    = bfd_make_section_old_way (abfd, "COMMON");
		  h->u.c.p->section->flags = SEC_ALLOC;
		}
	      else if (section->owner != abfd)
		{
		  h->u.c.p->section
		    = bfd_make_section_old_way (abfd, section->name);
		  h->u.c.p->section->flags = SEC_ALLOC;
		}
	      else
		h->u.c.p->section = section;
	    }
	  break;

	case CREF:
	  {
	    bfd *obfd;

	    /* We have found a common definition for a symbol which
	       was already defined.  FIXME: It would nice if we could
	       report the BFD which defined an indirect symbol, but we
	       don't have anywhere to store the information.  */
	    if (h->type == bfd_link_hash_defined
		|| h->type == bfd_link_hash_defweak)
	      obfd = h->u.def.section->owner;
	    else
	      obfd = NULL;
	    if (! ((*info->callbacks->multiple_common)
		   (info, h->root.string, obfd, h->type, (bfd_vma) 0,
		    abfd, bfd_link_hash_common, value)))
	      return FALSE;
	  }
	  break;

	case MIND:
	  /* Multiple indirect symbols.  This is OK if they both point
	     to the same symbol.  */
	  if (strcmp (h->u.i.link->root.string, string) == 0)
	    break;
	  /* Fall through.  */
	case MDEF:
	  /* Handle a multiple definition.  */
#ifdef PIC30
          if (PIC30_IS_SHARED_ATTR(section) &&
              PIC30_IS_SHARED_ATTR(h->u.def.section) &&
              ((section->linked == 1) || (h->u.def.section->linked == 1)) &&
              section->_raw_size == h->u.def.section->_raw_size &&
              strcmp(section->name, h->u.def.section->name) == 0) {
            /* these are the same symbol, in the same section, and the same
               size... allow the user to redefine them in this special
               circumstance.   But ensure that they are allocated at the same
               place */
            h->u.def.section->vma = section->vma;
            h->u.def.section->lma = section->lma;
            PIC30_SET_ABSOLUTE_ATTR(h->u.def.section);
          } else 
#endif
	  if (!info->allow_multiple_definition)
	    {
	      asection *msec = NULL;
	      bfd_vma mval = 0;

	      switch (h->type)
		{
		case bfd_link_hash_defined:
		  msec = h->u.def.section;
		  mval = h->u.def.value;
		  break;
	        case bfd_link_hash_indirect:
		  msec = bfd_ind_section_ptr;
		  mval = 0;
		  break;
		default:
		  abort ();
		}

	      /* Ignore a redefinition of an absolute symbol to the
		 same value; it's harmless.  */
	      if (h->type == bfd_link_hash_defined
		  && bfd_is_abs_section (msec)
		  && bfd_is_abs_section (section)
		  && value == mval)
		break;

	      if (! ((*info->callbacks->multiple_definition)
		     (info, h->root.string, msec->owner, msec, mval,
		      abfd, section, value)))
		return FALSE;
	    }
	  break;

	case CIND:
	  /* Create an indirect symbol from an existing common symbol.  */
	  BFD_ASSERT (h->type == bfd_link_hash_common);
	  if (! ((*info->callbacks->multiple_common)
		 (info, h->root.string,
		  h->u.c.p->section->owner, bfd_link_hash_common, h->u.c.size,
		  abfd, bfd_link_hash_indirect, (bfd_vma) 0)))
	    return FALSE;
	  /* Fall through.  */
	case IND:
	  /* Create an indirect symbol.  */
	  {
	    struct bfd_link_hash_entry *inh;

	    /* STRING is the name of the symbol we want to indirect
	       to.  */
	    inh = bfd_wrapped_link_hash_lookup (abfd, info, string, TRUE,
						copy, FALSE);
	    if (inh == (struct bfd_link_hash_entry *) NULL)
	      return FALSE;
	    if (inh->type == bfd_link_hash_indirect
		&& inh->u.i.link == h)
	      {
		(*_bfd_error_handler)
		  (_("%s: indirect symbol `%s' to `%s' is a loop"),
		   bfd_archive_filename (abfd), name, string);
		bfd_set_error (bfd_error_invalid_operation);
		return FALSE;
	      }
	    if (inh->type == bfd_link_hash_new)
	      {
		inh->type = bfd_link_hash_undefined;
		inh->u.undef.abfd = abfd;
		bfd_link_add_undef (info->hash, inh);
	      }

	    /* If the indirect symbol has been referenced, we need to
	       push the reference down to the symbol we are
	       referencing.  */
	    if (h->type != bfd_link_hash_new)
	      {
		row = UNDEF_ROW;
		cycle = TRUE;
	      }

	    h->type = bfd_link_hash_indirect;
	    h->u.i.link = inh;
	  }
	  break;

	case SET:
	  /* Add an entry to a set.  */
	  if (! (*info->callbacks->add_to_set) (info, h, BFD_RELOC_CTOR,
						abfd, section, value))
	    return FALSE;
	  break;

	case WARNC:
	  /* Issue a warning and cycle.  */
	  if (h->u.i.warning != NULL)
	    {
	      if (! (*info->callbacks->warning) (info, h->u.i.warning,
						 h->root.string, abfd,
						 (asection *) NULL,
						 (bfd_vma) 0))
		return FALSE;
	      /* Only issue a warning once.  */
	      h->u.i.warning = NULL;
	    }
	  /* Fall through.  */
	case CYCLE:
	  /* Try again with the referenced symbol.  */
	  h = h->u.i.link;
	  cycle = TRUE;
	  break;

	case REFC:
	  /* A reference to an indirect symbol.  */
	  if (h->next == NULL && info->hash->undefs_tail != h)
	    h->next = h;
	  h = h->u.i.link;
	  cycle = TRUE;
	  break;

	case WARN:
	  /* Issue a warning.  */
	  if (! (*info->callbacks->warning) (info, string, h->root.string,
					     hash_entry_bfd (h),
					     (asection *) NULL, (bfd_vma) 0))
	    return FALSE;
	  break;

	case CWARN:
	  /* Warn if this symbol has been referenced already,
	     otherwise add a warning.  A symbol has been referenced if
	     the next field is not NULL, or it is the tail of the
	     undefined symbol list.  The REF case above helps to
	     ensure this.  */
	  if (h->next != NULL || info->hash->undefs_tail == h)
	    {
	      if (! (*info->callbacks->warning) (info, string, h->root.string,
						 hash_entry_bfd (h),
						 (asection *) NULL,
						 (bfd_vma) 0))
		return FALSE;
	      break;
	    }
	  /* Fall through.  */
	case MWARN:
	  /* Make a warning symbol.  */
	  {
	    struct bfd_link_hash_entry *sub;

	    /* STRING is the warning to give.  */
	    sub = ((struct bfd_link_hash_entry *)
		   ((*info->hash->table.newfunc)
		    ((struct bfd_hash_entry *) NULL, &info->hash->table,
		     h->root.string)));
	    if (sub == NULL)
	      return FALSE;
	    *sub = *h;
	    sub->type = bfd_link_hash_warning;
	    sub->u.i.link = h;
	    if (! copy)
	      sub->u.i.warning = string;
	    else
	      {
		char *w;
		size_t len = strlen (string) + 1;

		w = bfd_hash_allocate (&info->hash->table, len);
		if (w == NULL)
		  return FALSE;
		memcpy (w, string, len);
		sub->u.i.warning = w;
	      }

	    bfd_hash_replace (&info->hash->table,
			      (struct bfd_hash_entry *) h,
			      (struct bfd_hash_entry *) sub);
	    if (hashp != NULL)
	      *hashp = sub;
	  }
	  break;
	}
    }
  while (cycle);

  return TRUE;
}

/* Generic final link routine.  */

bfd_boolean
_bfd_generic_final_link (abfd, info)
     bfd *abfd;
     struct bfd_link_info *info;
{
  bfd *sub;
  asection *o;
  struct bfd_link_order *p;
  size_t outsymalloc;
  struct generic_write_global_symbol_info wginfo;

  bfd_get_outsymbols (abfd) = (asymbol **) NULL;
  bfd_get_symcount (abfd) = 0;
  outsymalloc = 0;

  /* Mark all sections which will be included in the output file.  */
  for (o = abfd->sections; o != NULL; o = o->next)
    for (p = o->link_order_head; p != NULL; p = p->next)
      if (p->type == bfd_indirect_link_order)
	p->u.indirect.section->linker_mark = TRUE;

  /* Build the output symbol table.  */
  for (sub = info->input_bfds; sub != (bfd *) NULL; sub = sub->link_next)
    if (! _bfd_generic_link_output_symbols (abfd, sub, info, &outsymalloc))
      return FALSE;

  /* Accumulate the global symbols.  */
  wginfo.info = info;
  wginfo.output_bfd = abfd;
  wginfo.psymalloc = &outsymalloc;
  _bfd_generic_link_hash_traverse (_bfd_generic_hash_table (info),
				   _bfd_generic_link_write_global_symbol,
				   (PTR) &wginfo);

  /* Make sure we have a trailing NULL pointer on OUTSYMBOLS.  We
     shouldn't really need one, since we have SYMCOUNT, but some old
     code still expects one.  */
  if (! generic_add_output_symbol (abfd, &outsymalloc, NULL))
    return FALSE;

  if (info->relocateable)
    {
      /* Allocate space for the output relocs for each section.  */
      for (o = abfd->sections;
	   o != (asection *) NULL;
	   o = o->next)
	{
	  o->reloc_count = 0;
	  for (p = o->link_order_head;
	       p != (struct bfd_link_order *) NULL;
	       p = p->next)
	    {
	      if (p->type == bfd_section_reloc_link_order
		  || p->type == bfd_symbol_reloc_link_order)
		++o->reloc_count;
	      else if (p->type == bfd_indirect_link_order)
		{
		  asection *input_section;
		  bfd *input_bfd;
		  long relsize;
		  arelent **relocs;
		  asymbol **symbols;
		  long reloc_count;

		  input_section = p->u.indirect.section;
		  input_bfd = input_section->owner;
		  relsize = bfd_get_reloc_upper_bound (input_bfd,
						       input_section);
		  if (relsize < 0)
		    return FALSE;
		  relocs = (arelent **) bfd_malloc ((bfd_size_type) relsize);
		  if (!relocs && relsize != 0)
		    return FALSE;
		  symbols = _bfd_generic_link_get_symbols (input_bfd);
		  reloc_count = bfd_canonicalize_reloc (input_bfd,
							input_section,
							relocs,
							symbols);
		  free (relocs);
		  if (reloc_count < 0)
		    return FALSE;
		  BFD_ASSERT ((unsigned long) reloc_count
			      == input_section->reloc_count);
		  o->reloc_count += reloc_count;
		}
	    }
	  if (o->reloc_count > 0)
	    {
	      bfd_size_type amt;

	      amt = o->reloc_count;
	      amt *= sizeof (arelent *);
	      o->orelocation = (arelent **) bfd_alloc (abfd, amt);
	      if (!o->orelocation)
		return FALSE;
	      o->flags |= SEC_RELOC;
	      /* Reset the count so that it can be used as an index
		 when putting in the output relocs.  */
	      o->reloc_count = 0;
	    }
	}
    }

  /* Handle all the link order information for the sections.  */
  for (o = abfd->sections;
       o != (asection *) NULL;
       o = o->next)
    {
      for (p = o->link_order_head;
	   p != (struct bfd_link_order *) NULL;
	   p = p->next)
	{
	  switch (p->type)
	    {
	    case bfd_section_reloc_link_order:
	    case bfd_symbol_reloc_link_order:
	      if (! _bfd_generic_reloc_link_order (abfd, info, o, p))
		return FALSE;
	      break;
	    case bfd_indirect_link_order:
	      if (! default_indirect_link_order (abfd, info, o, p, TRUE))
		return FALSE;
	      break;
	    default:
	      if (! _bfd_default_link_order (abfd, info, o, p))
		return FALSE;
	      break;
	    }
	}
    }

  return TRUE;
}

/* Add an output symbol to the output BFD.  */

static bfd_boolean
generic_add_output_symbol (output_bfd, psymalloc, sym)
     bfd *output_bfd;
     size_t *psymalloc;
     asymbol *sym;
{
  if (bfd_get_symcount (output_bfd) >= *psymalloc)
    {
      asymbol **newsyms;
      bfd_size_type amt;

      if (*psymalloc == 0)
	*psymalloc = 124;
      else
	*psymalloc *= 2;
      amt = *psymalloc;
      amt *= sizeof (asymbol *);
      newsyms = (asymbol **) bfd_realloc (bfd_get_outsymbols (output_bfd), amt);
      if (newsyms == (asymbol **) NULL)
	return FALSE;
      bfd_get_outsymbols (output_bfd) = newsyms;
    }
/*FS We don't want symbols of gc-sections to be added*/
#if defined(PIC30ELF)
  if (sym && !(sym->section->flags & SEC_EXCLUDE))
#endif
  {
    bfd_get_outsymbols (output_bfd) [bfd_get_symcount (output_bfd)] = sym;
    if (sym != NULL)
      ++ bfd_get_symcount (output_bfd);
  }

  return TRUE;
}

/* Handle the symbols for an input BFD.  */

bfd_boolean
_bfd_generic_link_output_symbols (output_bfd, input_bfd, info, psymalloc)
     bfd *output_bfd;
     bfd *input_bfd;
     struct bfd_link_info *info;
     size_t *psymalloc;
{
  asymbol **sym_ptr;
  asymbol **sym_end;

  if (! generic_link_read_symbols (input_bfd))
    return FALSE;

  /* Create a filename symbol if we are supposed to.  */
  if (info->create_object_symbols_section != (asection *) NULL)
    {
      asection *sec;

      for (sec = input_bfd->sections;
	   sec != (asection *) NULL;
	   sec = sec->next)
	{
	  if (sec->output_section == info->create_object_symbols_section)
	    {
	      asymbol *newsym;

	      newsym = bfd_make_empty_symbol (input_bfd);
	      if (!newsym)
		return FALSE;
	      newsym->name = input_bfd->filename;
	      newsym->value = 0;
	      newsym->flags = BSF_LOCAL | BSF_FILE;
	      newsym->section = sec;

	      if (! generic_add_output_symbol (output_bfd, psymalloc,
					       newsym))
		return FALSE;

	      break;
	    }
	}
    }

  /* Adjust the values of the globally visible symbols, and write out
     local symbols.  */
  sym_ptr = _bfd_generic_link_get_symbols (input_bfd);
  sym_end = sym_ptr + _bfd_generic_link_get_symcount (input_bfd);
  for (; sym_ptr < sym_end; sym_ptr++)
    {
      asymbol *sym;
#if defined(PIC30ELF)
      struct elf_link_hash_entry *h;
#else
      struct generic_link_hash_entry *h;
#endif
      bfd_boolean output;

      h = NULL;
      sym = *sym_ptr;

#if defined(PIC30) && 0
      /* not the right way to do this, but checking to see if this is the
           right time */
      if ( (sym->flags & BSF_GLOBAL) &&
          !(sym->flags & (BSF_DEBUGGING | BSF_SECTION_SYM | BSF_FILE))) {
        struct elf_link_hash_entry *h;

        sym->flags &= ~BSF_GLOBAL;
        sym->flags |= BSF_WEAK;
        h = elf_link_hash_lookup ((struct elf_link_hash_table *)info->hash,
                                     sym->name, FALSE, FALSE, TRUE);
        if (h) h->root.type = bfd_link_hash_defweak;

        fprintf(stderr,"weaken: %s %p\n", sym->name, h);
      }
#endif
      if ((sym->flags & (BSF_INDIRECT
			 | BSF_WARNING
			 | BSF_GLOBAL
			 | BSF_CONSTRUCTOR
			 | BSF_WEAK)) != 0
	  || bfd_is_und_section (bfd_get_section (sym))
	  || bfd_is_com_section (bfd_get_section (sym))
	  || bfd_is_ind_section (bfd_get_section (sym)))
	{
	  if (sym->udata.p != NULL)
#if defined(PIC30ELF)
	    h = (struct elf_link_hash_entry *) sym->udata.p;
#else
            h = (struct generic_link_hash_entry *) sym->udata.p;
#endif
	  else if ((sym->flags & BSF_CONSTRUCTOR) != 0)
	    {
	      /* This case normally means that the main linker code
                 deliberately ignored this constructor symbol.  We
                 should just pass it through.  This will screw up if
                 the constructor symbol is from a different,
                 non-generic, object file format, but the case will
                 only arise when linking with -r, which will probably
                 fail anyhow, since there will be no way to represent
                 the relocs in the output format being used.  */
	      h = NULL;
	    }
	  else if (bfd_is_und_section (bfd_get_section (sym)))
#if defined(PIC30ELF)
	    h = ((struct elf_link_hash_entry *)
		 bfd_wrapped_link_hash_lookup (output_bfd, info,
					       bfd_asymbol_name (sym),
					       FALSE, FALSE, TRUE));
#else       
            h = ((struct generic_link_hash_entry *)
                 bfd_wrapped_link_hash_lookup (output_bfd, info,
                                               bfd_asymbol_name (sym),
                                               FALSE, FALSE, TRUE));
#endif
	  else
	    h = _bfd_generic_link_hash_lookup (_bfd_generic_hash_table (info),
					       bfd_asymbol_name (sym),
					       FALSE, FALSE, TRUE);

	  if (h != NULL)
	    {
	      /* Force all references to this symbol to point to
		 the same area in memory.  It is possible that
		 this routine will be called with a hash table
		 other than a generic hash table, so we double
		 check that.  */
              /* Not good enough if we have forced the hash table to be ELF
                 without using the reset of the bfd_elf* functions.  
                 Also check the type. (FS) */
#if !defined(PIC30ELF)
	      if (info->hash->creator == input_bfd->xvec)
                {
		  if (h->sym != (asymbol *) NULL)
		   *sym_ptr = sym = h->sym;
		  
		}
#endif
	      switch (h->root.type)
		{
		default:
		case bfd_link_hash_new:
		  abort ();
		case bfd_link_hash_undefined:
		  break;
		case bfd_link_hash_undefweak:
		  sym->flags |= BSF_WEAK;
		  break;
		case bfd_link_hash_indirect:
#if defined(PIC30ELF)
		  h = (struct elf_link_hash_entry *) h->root.u.i.link;
#else
                  h = (struct generic_link_hash_entry *) h->root.u.i.link;
#endif
		  /* fall through */
		case bfd_link_hash_defined:
#if defined(PIC30ELF) /* Co-resident */
                  if (PIC30_IS_SHARED_ATTR(sym->section)) {
		    sym->flags |= BSF_GLOBAL;
                  } else {
                    h->root.type = bfd_link_hash_defweak;
                    sym->flags |= BSF_WEAK;
                    sym->flags &= ~BSF_GLOBAL;
                  }
#else
		  sym->flags |= BSF_GLOBAL;
#endif
		  sym->flags &=~ BSF_CONSTRUCTOR;
		  sym->value = h->root.u.def.value;
		  sym->section = h->root.u.def.section;
		  break;
		case bfd_link_hash_defweak:
		  sym->flags |= BSF_WEAK;
		  sym->flags &=~ BSF_CONSTRUCTOR;
		  sym->value = h->root.u.def.value;
		  sym->section = h->root.u.def.section;
		  break;
#ifdef PIC30
                case bfd_link_hash_shared_defweak:
                  sym->flags |= BSF_WEAK | BSF_SHARED;
                  sym->flags &=~ BSF_CONSTRUCTOR;
                  sym->value = h->root.u.def.value;
                  sym->section = h->root.u.def.section;
                  break;
                case bfd_link_hash_shared_defined:
                  h->root.type = bfd_link_hash_shared_defweak;
                  sym->flags &= ~BSF_GLOBAL;
                  sym->flags |= BSF_WEAK | BSF_SHARED;
                  sym->flags &=~ BSF_CONSTRUCTOR;
                  sym->value = h->root.u.def.value;
                  sym->section = h->root.u.def.section;
                  break;
#endif
		case bfd_link_hash_common:
		  sym->value = h->root.u.c.size;
		  sym->flags |= BSF_GLOBAL;
		  if (! bfd_is_com_section (sym->section))
		    {
		      BFD_ASSERT (bfd_is_und_section (sym->section));
		      sym->section = bfd_com_section_ptr;
		    }
		  /* We do not set the section of the symbol to
		     h->root.u.c.p->section.  That value was saved so
		     that we would know where to allocate the symbol
		     if it was defined.  In this case the type is
		     still bfd_link_hash_common, so we did not define
		     it, so we do not want to use that section.  */
		  break;
		}
	    }
	}
#ifdef PIC30ELF
      if (h && !h->sym)
        h->sym = sym; 
#endif
      /* This switch is straight from the old code in
	 write_file_locals in ldsym.c.  */
      if (info->strip == strip_all
	  || (info->strip == strip_some
	      && (bfd_hash_lookup (info->keep_hash, bfd_asymbol_name (sym),
				   FALSE, FALSE)
		  == (struct bfd_hash_entry *) NULL)))
	output = FALSE;
      else if ((sym->flags & (BSF_GLOBAL | BSF_WEAK)) != 0)
	{
	  /* If this symbol is marked as occurring now, rather
	     than at the end, output it now.  This is used for
	     COFF C_EXT FCN symbols.  FIXME: There must be a
	     better way.  */
	  if (bfd_asymbol_bfd (sym) == input_bfd
	      && (sym->flags & BSF_NOT_AT_END) != 0) 
                  
	    output = TRUE;
	  else
	    output = FALSE;
	}
      else if (bfd_is_ind_section (sym->section))
	output = FALSE;
      else if ((sym->flags & BSF_DEBUGGING) != 0)
	{
	  if (info->strip == strip_none)
	    output = TRUE;
	  else
	    output = FALSE;
	}
      else if (bfd_is_und_section (sym->section)
	       || bfd_is_com_section (sym->section))
	output = FALSE;
      else if ((sym->flags & BSF_LOCAL) != 0)
	{
	  if ((sym->flags & BSF_WARNING) != 0)
	    output = FALSE;
	  else
	    {
	      switch (info->discard)
		{
		default:
		case discard_all:
		  output = FALSE;
		  break;
		case discard_sec_merge:
		  output = TRUE;
		  if (info->relocateable
		      || ! (sym->section->flags & SEC_MERGE))
		    break;
		  /* FALLTHROUGH */
		case discard_l:
		  if (bfd_is_local_label (input_bfd, sym))
		    output = FALSE;
		  else
		    output = TRUE;
		  break;
		case discard_none:
		  output = TRUE;
		  break;
		}
	    }
	}
      else if ((sym->flags & BSF_CONSTRUCTOR))
	{
	  if (info->strip != strip_all)
	    output = TRUE;
	  else
	    output = FALSE;
	}
      else
	abort ();

      /* If this symbol is in a section which is not being includeid
	 in the output file, then we don't want to output the symbol.

	 Gross.  .bss and similar sections won't have the linker_mark
	 field set.  */
      if ((sym->section->flags & SEC_HAS_CONTENTS) != 0
	  && ! sym->section->linker_mark )
        {
	  output = FALSE;
        }
	
      if (output)
	{
	  if (! generic_add_output_symbol (output_bfd, psymalloc, sym))
	    return FALSE;
	  if (h != NULL)
	    h->written = TRUE;
	}
    }

  return TRUE;
}

/* Set the section and value of a generic BFD symbol based on a linker
   hash table entry.  */

static void
set_symbol_from_hash (sym, h)
     asymbol *sym;
     struct bfd_link_hash_entry *h;
{
  switch (h->type)
    {
    default:
      abort ();
      break;
    case bfd_link_hash_new:
      /* This can happen when a constructor symbol is seen but we are
         not building constructors.  */
      if (sym->section != NULL)
	{
	  BFD_ASSERT ((sym->flags & BSF_CONSTRUCTOR) != 0);
	}
      else
	{
	  sym->flags |= BSF_CONSTRUCTOR;
	  sym->section = bfd_abs_section_ptr;
	  sym->value = 0;
	}
      break;
    case bfd_link_hash_undefined:
      sym->section = bfd_und_section_ptr;
      sym->value = 0;
      break;
    case bfd_link_hash_undefweak:
      sym->section = bfd_und_section_ptr;
      sym->value = 0;
      sym->flags |= BSF_WEAK;
      break;
    case bfd_link_hash_defined:
      sym->section = h->u.def.section;
      sym->value = h->u.def.value;
      break;
    case bfd_link_hash_defweak:
      sym->flags |= BSF_WEAK;
      sym->section = h->u.def.section;
      sym->value = h->u.def.value;
      break;
    case bfd_link_hash_shared_defined:
      sym->flags |= BSF_SHARED;
      sym->section = h->u.def.section;
      sym->value = h->u.def.value;
      break;
    case bfd_link_hash_shared_defweak:
      sym->flags |= BSF_WEAK | BSF_SHARED;
      sym->section = h->u.def.section;
      sym->value = h->u.def.value;
      break;
    case bfd_link_hash_common:
      sym->value = h->u.c.size;
      if (sym->section == NULL)
	sym->section = bfd_com_section_ptr;
      else if (! bfd_is_com_section (sym->section))
	{
	  BFD_ASSERT (bfd_is_und_section (sym->section));
	  sym->section = bfd_com_section_ptr;
	}
      /* Do not set the section; see _bfd_generic_link_output_symbols.  */
      break;
    case bfd_link_hash_indirect:
    case bfd_link_hash_warning:
      /* FIXME: What should we do here?  */
      break;
    }
}

/* Write out a global symbol, if it hasn't already been written out.
   This is called for each symbol in the hash table.  */

bfd_boolean
_bfd_generic_link_write_global_symbol (h, data)
#if defined(PIC30ELF)
     struct elf_link_hash_entry *h;
#else
     struct generic_link_hash_entry *h;
#endif
     PTR data;
{
  struct generic_write_global_symbol_info *wginfo =
    (struct generic_write_global_symbol_info *) data;
  asymbol *sym;
  if (h->root.type == bfd_link_hash_warning)
#if defined(PIC30ELF)
    h = (struct elf_link_hash_entry *) h->root.u.i.link;
#else
    h = (struct generic_link_hash_entry *) h->root.u.i.link;
#endif
    
  if (h->written)
    {
     return TRUE;
    }

  h->written = TRUE;

  if (wginfo->info->strip == strip_all
      || (wginfo->info->strip == strip_some
	  && bfd_hash_lookup (wginfo->info->keep_hash, h->root.root.string,
			      FALSE, FALSE) == NULL))
    return TRUE;

  if (h->sym != (asymbol *) NULL)
    sym = h->sym;
  else
    {
      sym = bfd_make_empty_symbol (wginfo->output_bfd);
      if (!sym)
	return FALSE;
      sym->name = h->root.root.string;
      sym->flags = 0;
    }

  set_symbol_from_hash (sym, &h->root);
  sym->flags |= BSF_GLOBAL;

#if PIC30
  /* don't write fake global symbols */
  if (strstr(sym->name, ":") != (char *) 0)
    return TRUE;
#endif

  if (! generic_add_output_symbol (wginfo->output_bfd, wginfo->psymalloc,
				   sym))
    {
      /* FIXME: No way to return failure.  */
      abort ();
    }
//}
    
  return TRUE;
}

/* Create a relocation.  */

bfd_boolean
_bfd_generic_reloc_link_order (abfd, info, sec, link_order)
     bfd *abfd;
     struct bfd_link_info *info;
     asection *sec;
     struct bfd_link_order *link_order;
{
  arelent *r;

  if (! info->relocateable)
    abort ();
  if (sec->orelocation == (arelent **) NULL)
    abort ();

  r = (arelent *) bfd_alloc (abfd, (bfd_size_type) sizeof (arelent));
  if (r == (arelent *) NULL)
    return FALSE;

  r->address = link_order->offset;
  r->howto = bfd_reloc_type_lookup (abfd, link_order->u.reloc.p->reloc);
  if (r->howto == 0)
    {
      bfd_set_error (bfd_error_bad_value);
      return FALSE;
    }

  /* Get the symbol to use for the relocation.  */
  if (link_order->type == bfd_section_reloc_link_order)
    r->sym_ptr_ptr = link_order->u.reloc.p->u.section->symbol_ptr_ptr;
  else
    {
#if defined(PIC30ELF)
      struct elf_link_hash_entry *h;
       h = ((struct elf_link_hash_entry *)
           bfd_wrapped_link_hash_lookup (abfd, info,
                                         link_order->u.reloc.p->u.name,
                                         FALSE, FALSE, TRUE));
#else
      struct generic_link_hash_entry *h;
      h = ((struct generic_link_hash_entry *)
	   bfd_wrapped_link_hash_lookup (abfd, info,
					 link_order->u.reloc.p->u.name,
					 FALSE, FALSE, TRUE));
#endif
      if (h == NULL
	  || ! h->written)
	{
	  if (! ((*info->callbacks->unattached_reloc)
		 (info, link_order->u.reloc.p->u.name,
		  (bfd *) NULL, (asection *) NULL, (bfd_vma) 0)))
	    return FALSE;
	  bfd_set_error (bfd_error_bad_value);
	  return FALSE;
	}
      r->sym_ptr_ptr = &h->sym;
    }

  /* If this is an inplace reloc, write the addend to the object file.
     Otherwise, store it in the reloc addend.  */
  if (! r->howto->partial_inplace)
    r->addend = link_order->u.reloc.p->addend;
  else
    {
      bfd_size_type size;
      bfd_reloc_status_type rstat;
      bfd_byte *buf;
      bfd_boolean ok;
      file_ptr loc;

      size = bfd_get_reloc_size (r->howto);
      buf = (bfd_byte *) bfd_zmalloc (size);
      if (buf == (bfd_byte *) NULL)
	return FALSE;
      rstat = _bfd_relocate_contents (r->howto, abfd,
				      (bfd_vma) link_order->u.reloc.p->addend,
				      buf);
      switch (rstat)
	{
	case bfd_reloc_ok:
	  break;
	default:
	case bfd_reloc_outofrange:
	  abort ();
	case bfd_reloc_overflow:
	  if (! ((*info->callbacks->reloc_overflow)
		 (info,
		  (link_order->type == bfd_section_reloc_link_order
		   ? bfd_section_name (abfd, link_order->u.reloc.p->u.section)
		   : link_order->u.reloc.p->u.name),
		  r->howto->name, link_order->u.reloc.p->addend,
		  (bfd *) NULL, (asection *) NULL, (bfd_vma) 0)))
	    {
	      free (buf);
	      return FALSE;
	    }
	  break;
	}
      loc = link_order->offset * bfd_octets_per_byte (abfd);
      ok = bfd_set_section_contents (abfd, sec, (PTR) buf, loc,
				     (bfd_size_type) size);
      free (buf);
      if (! ok)
	return FALSE;

      r->addend = 0;
    }

  sec->orelocation[sec->reloc_count] = r;
  ++sec->reloc_count;

  return TRUE;
}

/* Allocate a new link_order for a section.  */

struct bfd_link_order *
bfd_new_link_order (abfd, section)
     bfd *abfd;
     asection *section;
{
  bfd_size_type amt = sizeof (struct bfd_link_order);
  struct bfd_link_order *new;

  new = (struct bfd_link_order *) bfd_zalloc (abfd, amt);
  if (!new)
    return NULL;

  new->type = bfd_undefined_link_order;

  if (section->link_order_tail != (struct bfd_link_order *) NULL)
    section->link_order_tail->next = new;
  else
    section->link_order_head = new;
  section->link_order_tail = new;

  return new;
}

/* Default link order processing routine.  Note that we can not handle
   the reloc_link_order types here, since they depend upon the details
   of how the particular backends generates relocs.  */

bfd_boolean
_bfd_default_link_order (abfd, info, sec, link_order)
     bfd *abfd;
     struct bfd_link_info *info;
     asection *sec;
     struct bfd_link_order *link_order;
{
  switch (link_order->type)
    {
    case bfd_undefined_link_order:
    case bfd_section_reloc_link_order:
    case bfd_symbol_reloc_link_order:
    default:
      abort ();
    case bfd_indirect_link_order:
      return default_indirect_link_order (abfd, info, sec, link_order,
					  FALSE);
    case bfd_data_link_order:
      return default_data_link_order (abfd, info, sec, link_order);
    }
}

/* Default routine to handle a bfd_data_link_order.  */

static bfd_boolean
default_data_link_order (abfd, info, sec, link_order)
     bfd *abfd;
     struct bfd_link_info *info ATTRIBUTE_UNUSED;
     asection *sec;
     struct bfd_link_order *link_order;
{
  bfd_size_type size;
  size_t fill_size;
  bfd_byte *fill;
  file_ptr loc;
  bfd_boolean result;

  BFD_ASSERT ((sec->flags & SEC_HAS_CONTENTS) != 0);

  size = link_order->size;
  if (size == 0)
    return TRUE;

  fill = link_order->u.data.contents;
  fill_size = link_order->u.data.size;
  if (fill_size != 0 && fill_size < size)
    {
      bfd_byte *p;
      fill = (bfd_byte *) bfd_malloc (size);
      if (fill == NULL)
	return FALSE;
      p = fill;
      if (fill_size == 1)
	memset (p, (int) link_order->u.data.contents[0], (size_t) size);
      else
	{
	  do
	    {
	      memcpy (p, link_order->u.data.contents, fill_size);
	      p += fill_size;
	      size -= fill_size;
	    }
	  while (size >= fill_size);
	  if (size != 0)
	    memcpy (p, link_order->u.data.contents, (size_t) size);
	  size = link_order->size;
	}
    }

  loc = link_order->offset * bfd_octets_per_byte (abfd);
  result = bfd_set_section_contents (abfd, sec, fill, loc, size);

  if (fill != link_order->u.data.contents)
    free (fill);
  return result;
}

/* Default routine to handle a bfd_indirect_link_order.  */

static bfd_boolean
default_indirect_link_order (output_bfd, info, output_section, link_order,
			     generic_linker)
     bfd *output_bfd;
     struct bfd_link_info *info;
     asection *output_section;
     struct bfd_link_order *link_order;
     bfd_boolean generic_linker;
{
  asection *input_section;
  bfd *input_bfd;
  bfd_byte *contents = NULL;
  bfd_byte *new_contents;
  bfd_size_type sec_size;
  file_ptr loc;

  BFD_ASSERT ((output_section->flags & SEC_HAS_CONTENTS) != 0);

  if (link_order->size == 0)
    return TRUE;

  input_section = link_order->u.indirect.section;
  input_bfd = input_section->owner;

  BFD_ASSERT (input_section->output_section == output_section);
  BFD_ASSERT (input_section->output_offset == link_order->offset);
  BFD_ASSERT (input_section->_cooked_size == link_order->size);

  if (info->relocateable
      && input_section->reloc_count > 0
      && output_section->orelocation == (arelent **) NULL)
    {
      /* Space has not been allocated for the output relocations.
	 This can happen when we are called by a specific backend
	 because somebody is attempting to link together different
	 types of object files.  Handling this case correctly is
	 difficult, and sometimes impossible.  */
      (*_bfd_error_handler)
	(_("Attempt to do relocateable link with %s input and %s output"),
	 bfd_get_target (input_bfd), bfd_get_target (output_bfd));
      bfd_set_error (bfd_error_wrong_format);
      return FALSE;
    }

  if (! generic_linker)
    {
      asymbol **sympp;
      asymbol **symppend;

      /* Get the canonical symbols.  The generic linker will always
	 have retrieved them by this point, but we are being called by
	 a specific linker, presumably because we are linking
	 different types of object files together.  */
      if (! generic_link_read_symbols (input_bfd))
	return FALSE;

      /* Since we have been called by a specific linker, rather than
	 the generic linker, the values of the symbols will not be
	 right.  They will be the values as seen in the input file,
	 not the values of the final link.  We need to fix them up
	 before we can relocate the section.  */
      sympp = _bfd_generic_link_get_symbols (input_bfd);
      symppend = sympp + _bfd_generic_link_get_symcount (input_bfd);
      for (; sympp < symppend; sympp++)
	{
	  asymbol *sym;
	  struct bfd_link_hash_entry *h;

	  sym = *sympp;

	  if ((sym->flags & (BSF_INDIRECT
			     | BSF_WARNING
			     | BSF_GLOBAL
			     | BSF_CONSTRUCTOR
			     | BSF_WEAK)) != 0
	      || bfd_is_und_section (bfd_get_section (sym))
	      || bfd_is_com_section (bfd_get_section (sym))
	      || bfd_is_ind_section (bfd_get_section (sym)))
	    {
	      /* sym->udata may have been set by
		 generic_link_add_symbol_list.  */
	      if (sym->udata.p != NULL)
		h = (struct bfd_link_hash_entry *) sym->udata.p;
	      else if (bfd_is_und_section (bfd_get_section (sym)))
		h = bfd_wrapped_link_hash_lookup (output_bfd, info,
						  bfd_asymbol_name (sym),
						  FALSE, FALSE, TRUE);
	      else
		h = bfd_link_hash_lookup (info->hash,
					  bfd_asymbol_name (sym),
					  FALSE, FALSE, TRUE);
	      if (h != NULL)
		set_symbol_from_hash (sym, h);
	    }
	}
    }

  /* Get and relocate the section contents.  */
  sec_size = bfd_section_size (input_bfd, input_section);
  contents = ((bfd_byte *) bfd_malloc (sec_size));
  if (contents == NULL && sec_size != 0)
    goto error_return;
  new_contents = (bfd_get_relocated_section_contents
		  (output_bfd, info, link_order, contents, info->relocateable,
		   _bfd_generic_link_get_symbols (input_bfd)));
  if (!new_contents)
    goto error_return;

  /* Output the section contents.  */
  loc = link_order->offset * bfd_octets_per_byte (output_bfd);
  if (! bfd_set_section_contents (output_bfd, output_section,
				  (PTR) new_contents, loc, link_order->size))
    goto error_return;

  if (contents != NULL)
    free (contents);
  return TRUE;

 error_return:
  if (contents != NULL)
    free (contents);
  return FALSE;
}

/* A little routine to count the number of relocs in a link_order
   list.  */

unsigned int
_bfd_count_link_order_relocs (link_order)
     struct bfd_link_order *link_order;
{
  register unsigned int c;
  register struct bfd_link_order *l;

  c = 0;
  for (l = link_order; l != (struct bfd_link_order *) NULL; l = l->next)
    {
      if (l->type == bfd_section_reloc_link_order
	  || l->type == bfd_symbol_reloc_link_order)
	++c;
    }

  return c;
}

/*
FUNCTION
	bfd_link_split_section

SYNOPSIS
        bfd_boolean bfd_link_split_section(bfd *abfd, asection *sec);

DESCRIPTION
	Return nonzero if @var{sec} should be split during a
	reloceatable or final link.

.#define bfd_link_split_section(abfd, sec) \
.       BFD_SEND (abfd, _bfd_link_split_section, (abfd, sec))
.

*/

bfd_boolean
_bfd_generic_link_split_section (abfd, sec)
     bfd *abfd ATTRIBUTE_UNUSED;
     asection *sec ATTRIBUTE_UNUSED;
{
  return FALSE;
}


