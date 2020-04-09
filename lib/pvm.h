/* pvm.h - Poke Virtual Machine.  */

/* Copyright (C) 2019, 2020 Jose E. Marchesi */

/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* This is the public interface of the Poke Virtual Machine (PVM)
   services as provided by libpoke.  */

#ifndef PVM_H
#define PVM_H

#include <config.h>

#include "ios.h"

/*** PVM values.  ***/

/* The pvm_val type implements values that are native to the poke
   virtual machine:

   - Integers up to 32-bit.
   - Long integers wider than 32-bit up to 64-bit.
   - Strings.
   - Arrays.
   - Structs.
   - Offsets.
   - Closures.

   It is fundamental for pvm_val values to fit in 64 bits, in order to
   avoid expensive allocations and to also improve the performance of
   the virtual machine.  */

typedef uint64_t pvm_val;

/* The least-significative bits of pvm_val are reserved for the tag,
   which specifies the type of the value.  */

#define PVM_VAL_TAG(V) ((V) & 0x7)

#define PVM_VAL_TAG_INT   0x0
#define PVM_VAL_TAG_UINT  0x1
#define PVM_VAL_TAG_LONG  0x2
#define PVM_VAL_TAG_ULONG 0x3
#define PVM_VAL_TAG_BIG   0x4
#define PVM_VAL_TAG_UBIG  0x5
#define PVM_VAL_TAG_BOX   0x6
/* Note that there is no tag 0x7.  It is used to implement PVM_NULL
   below.  */
/* Note also that the tags below are stored in the box, not in
   PVM_VAL_TAG.  See below in this file.  */
#define PVM_VAL_TAG_STR 0x8
#define PVM_VAL_TAG_OFF 0x9
#define PVM_VAL_TAG_ARR 0xa
#define PVM_VAL_TAG_SCT 0xb
#define PVM_VAL_TAG_TYP 0xc
#define PVM_VAL_TAG_CLS 0xd

#define PVM_VAL_BOXED_P(V) (PVM_VAL_TAG((V)) > 1)

/* Integers up to 32-bit are unboxed and encoded the following way:

              val                   bits  tag
              ---                   ----  ---
      vvvv vvvv vvvv vvvv xxxx xxxx bbbb bttt

   BITS+1 is the size of the integral value in bits, from 0 to 31.

   VAL is the value of the integer, sign- or zero-extended to 32 bits.
   Bits marked with `x' are unused and should be always 0.  */

#define PVM_VAL_INT_SIZE(V) (((int) (((V) >> 3) & 0x1f)) + 1)
#define PVM_VAL_INT(V) (((int32_t) ((V) >> 32))                \
                        << (32 - PVM_VAL_INT_SIZE ((V)))       \
                        >> (32 - PVM_VAL_INT_SIZE ((V))))

#define PVM_VAL_UINT_SIZE(V) (((int) (((V) >> 3) & 0x1f)) + 1)
#define PVM_VAL_UINT(V) (((uint32_t) ((V) >> 32)) \
                         & ((uint32_t) (~( ((~0ul) << ((PVM_VAL_UINT_SIZE ((V)))-1)) << 1 ))))


#define PVM_MAX_UINT(size) ((1U << (size)) - 1)

pvm_val pvm_make_int (int32_t value, int size);
pvm_val pvm_make_uint (uint32_t value, int size);

/* Long integers, wider than 32-bit and up to 64-bit, are boxed.  A
   pointer
                                             tag
                                             ---
         pppp pppp pppp pppp pppp pppp pppp pttt

   points to a pair of 64-bit words:

                           val
                           ---
   [0]   vvvv vvvv vvvv vvvv vvvv vvvv vvvv vvvv
                                           bits
                                           ----
   [1]   xxxx xxxx xxxx xxxx xxxx xxxx xxbb bbbb

   BITS+1 is the size of the integral value in bits, from 0 to 63.

   VAL is the value of the integer, sign- or zero-extended to 64 bits.
   Bits marked with `x' are unused.  */

#define _PVM_VAL_LONG_ULONG_VAL(V) (((int64_t *) ((((uintptr_t) V) & ~0x7)))[0])
#define _PVM_VAL_LONG_ULONG_SIZE(V) ((int) (((int64_t *) ((((uintptr_t) V) & ~0x7)))[1]) + 1)

#define PVM_VAL_LONG_SIZE(V) (_PVM_VAL_LONG_ULONG_SIZE (V))
#define PVM_VAL_LONG(V) (_PVM_VAL_LONG_ULONG_VAL ((V))           \
                         << (64 - PVM_VAL_LONG_SIZE ((V)))      \
                         >> (64 - PVM_VAL_LONG_SIZE ((V))))

#define PVM_VAL_ULONG_SIZE(V) (_PVM_VAL_LONG_ULONG_SIZE (V))
#define PVM_VAL_ULONG(V) (_PVM_VAL_LONG_ULONG_VAL ((V))                 \
                          & ((uint64_t) (~( ((~0ull) << ((PVM_VAL_ULONG_SIZE ((V)))-1)) << 1 ))))

#define PVM_MAX_ULONG(size) ((1LU << (size)) - 1)

pvm_val pvm_make_long (int64_t value, int size);
pvm_val pvm_make_ulong (uint64_t value, int size);

/* Big integers, wider than 64-bit, are boxed.  They are implemented
   using the GNU mp library.  */

/* XXX: implement big integers.  */

/* A pointer to a boxed value is encoded in the most significative 61
   bits of pvm_val (32 bits for 32-bit hosts).  Note that this assumes
   all pointers are aligned to 8 bytes.  The allocator for the boxed
   values makes sure this is always the case.  */

#define PVM_VAL_BOX(V) ((pvm_val_box) ((((uintptr_t) V) & ~0x7)))

/* This constructor should be used in order to build boxes.  */

#define PVM_BOX(PTR) (((uint64_t) (uintptr_t) PTR) | PVM_VAL_TAG_BOX)

/* A box is a header for a boxed value, plus that value.  It is of
   type `pvm_val_box'.  */

#define PVM_VAL_BOX_TAG(B) ((B)->tag)
#define PVM_VAL_BOX_STR(B) ((B)->v.string)
#define PVM_VAL_BOX_ARR(B) ((B)->v.array)
#define PVM_VAL_BOX_SCT(B) ((B)->v.sct)
#define PVM_VAL_BOX_TYP(B) ((B)->v.type)
#define PVM_VAL_BOX_CLS(B) ((B)->v.cls)
#define PVM_VAL_BOX_OFF(B) ((B)->v.offset)

struct pvm_val_box
{
  uint8_t tag;
  union
  {
    char *string;
    struct pvm_array *array;
    struct pvm_struct *sct;
    struct pvm_type *type;
    struct pvm_off *offset;
    struct pvm_cls *cls;
  } v;
};

typedef struct pvm_val_box *pvm_val_box;

/* Strings are boxed.  */

#define PVM_VAL_STR(V) (PVM_VAL_BOX_STR (PVM_VAL_BOX ((V))))

pvm_val pvm_make_string (const char *value);
void pvm_print_string (pvm_val string);

/* Arrays values are boxed, and store sequences of homogeneous values
   called array "elements".  They can be mapped in IO, or unmapped.

   IOS is an int<32> value that identifies the IO space where the
   value is mapped.  If the array is not mapped then this is PVM_NULL.

   OFFSET is an ulong<64> value with the bit offset in the current IO
   space where the array is mapped.  If the array is not mapped then
   this is PVM_NULL.

   If the array is mapped, ELEMS_BOUND is an unsigned long containing
   the number of elements to which the map is bounded.  Similarly,
   SIZE_BOUND is an offset indicating the size to which the map is
   bounded.  If the array is not mapped, both ELEMS_BOUND and
   SIZE_BOUND are PVM_NULL.  Note that these two boundaries are
   mutually exclusive, i.e. an array mapped value can be bounded by
   either a given number of elements, or a given size, but not both.

   MAPPER is a closure that gets an offset as an argument and, when
   executed, maps an array from IO.  This field is PVM_NULL if the
   array is not mapped.

   WRITER is a closure that gets an offset and an array of this type
   as arguments and, when executed, writes the array contents to IO.
   This writer can raise PVM_E_CONSTRAINT if some constraint is
   violated during the write.  This field is PVM_NULL if the array is
   not mapped.

   TYPE is the type of the array.  This includes the type of the
   elements of the array and the boundaries of the array, in case it
   is bounded.

   NELEM is the number of elements contained in the array.

   ELEMS is a list of elements.  The order of the elements is
   relevant.  */

#define PVM_VAL_ARR(V) (PVM_VAL_BOX_ARR (PVM_VAL_BOX ((V))))
#define PVM_VAL_ARR_IOS(V) (PVM_VAL_ARR(V)->ios)
#define PVM_VAL_ARR_OFFSET(V) (PVM_VAL_ARR(V)->offset)
#define PVM_VAL_ARR_ELEMS_BOUND(V) (PVM_VAL_ARR(V)->elems_bound)
#define PVM_VAL_ARR_SIZE_BOUND(V) (PVM_VAL_ARR(V)->size_bound)
#define PVM_VAL_ARR_MAPPER(V) (PVM_VAL_ARR(V)->mapper)
#define PVM_VAL_ARR_WRITER(V) (PVM_VAL_ARR(V)->writer)
#define PVM_VAL_ARR_TYPE(V) (PVM_VAL_ARR(V)->type)
#define PVM_VAL_ARR_NELEM(V) (PVM_VAL_ARR(V)->nelem)
#define PVM_VAL_ARR_ELEM(V,I) (PVM_VAL_ARR(V)->elems[(I)])

struct pvm_array
{
  pvm_val ios;
  pvm_val offset;
  pvm_val elems_bound;
  pvm_val size_bound;
  pvm_val mapper;
  pvm_val writer;
  pvm_val type;
  pvm_val nelem;
  struct pvm_array_elem *elems;
};

typedef struct pvm_array *pvm_array;

/* Array elements hold the data of the arrays, and/or information on
   how to obtain these values.

   OFFSET is an ulong<64> value holding the bit offset of the element,
   relative to the begginnig of the IO space.  If the array is not
   mapped then this is PVM_NULL.

   VALUE is the value contained in the element.  If the array is
   mapped this is the cached value, which is returned by `aref'.  */

#define PVM_VAL_ARR_ELEM_OFFSET(V,I) (PVM_VAL_ARR_ELEM((V),(I)).offset)
#define PVM_VAL_ARR_ELEM_VALUE(V,I) (PVM_VAL_ARR_ELEM((V),(I)).value)


struct pvm_array_elem
{
  pvm_val offset;
  pvm_val value;
};

pvm_val pvm_make_array (pvm_val nelem, pvm_val type);

/* Struct values are boxed, and store collections of named values
   called structure "elements".  They can be mapped in IO, or
   unmapped.

   IO is an int<32> value that identifies the IO space where the value
   is mapped.  If the structure is not mapped then this is PVM_NULL.

   OFFSET is an ulong<64> value holding the bit offset of in the IO
   space where the structure is mapped.  If the structure is not
   mapped then this is PVM_NULL.

   TYPE is the type of the struct.  This includes the types of the
   struct fields.

   NFIELDS is the number of fields conforming the structure.

   FIELDS is a list of fields.  The order of the fields is
   relevant.

   NMETHODS is the number of methods defined in the structure.

   METHODS is a list of methods.  The order of the methods is
   irrelevant.  */

#define PVM_VAL_SCT(V) (PVM_VAL_BOX_SCT (PVM_VAL_BOX ((V))))
#define PVM_VAL_SCT_IOS(V) (PVM_VAL_SCT((V))->ios)
#define PVM_VAL_SCT_OFFSET(V) (PVM_VAL_SCT((V))->offset)
#define PVM_VAL_SCT_MAPPER(V) (PVM_VAL_SCT((V))->mapper)
#define PVM_VAL_SCT_WRITER(V) (PVM_VAL_SCT((V))->writer)
#define PVM_VAL_SCT_TYPE(V) (PVM_VAL_SCT((V))->type)
#define PVM_VAL_SCT_NFIELDS(V) (PVM_VAL_SCT((V))->nfields)
#define PVM_VAL_SCT_FIELD(V,I) (PVM_VAL_SCT((V))->fields[(I)])
#define PVM_VAL_SCT_NMETHODS(V) (PVM_VAL_SCT((V))->nmethods)
#define PVM_VAL_SCT_METHOD(V,I) (PVM_VAL_SCT((V))->methods[(I)])

struct pvm_struct
{
  pvm_val ios;
  pvm_val offset;
  pvm_val mapper;
  pvm_val writer;
  pvm_val type;
  pvm_val nfields;
  struct pvm_struct_field *fields;
  pvm_val nmethods;
  struct pvm_struct_method *methods;
};

/* Struct fields hold the data of the fields, and/or information on
   how to obtain these values.

   OFFSET is an ulong<64> value containing the bit-offset,
   relative to the beginning of the struct, where the struct field
   resides when stored.

   NAME is a string containing the name of the struct field.  This
   name should be unique in the struct.

   VALUE is the value contained in the field.  If the struct is
   mapped then this is the cached value, which is returned by
   `sref'.

   MODIFIED is a C boolean indicating whether the field value has
   been modified since struct creation, or since last mapping if the
   struct is mapped.  */

#define PVM_VAL_SCT_FIELD_OFFSET(V,I) (PVM_VAL_SCT_FIELD((V),(I)).offset)
#define PVM_VAL_SCT_FIELD_NAME(V,I) (PVM_VAL_SCT_FIELD((V),(I)).name)
#define PVM_VAL_SCT_FIELD_VALUE(V,I) (PVM_VAL_SCT_FIELD((V),(I)).value)
#define PVM_VAL_SCT_FIELD_MODIFIED(V,I) (PVM_VAL_SCT_FIELD((V),(I)).modified)

struct pvm_struct_field
{
  pvm_val offset;
  pvm_val name;
  pvm_val value;
  pvm_val modified;
};

/* Struct methods are closures associated with the struct, which can
   be invoked as functions.

   NAME is a string containing the name of the method.  This name
   should be unique in the struct.

   VALUE is a PVM closure.  */

#define PVM_VAL_SCT_METHOD_NAME(V,I) (PVM_VAL_SCT_METHOD((V),(I)).name)
#define PVM_VAL_SCT_METHOD_VALUE(V,I) (PVM_VAL_SCT_METHOD((V),(I)).value)

struct pvm_struct_method
{
  pvm_val name;
  pvm_val value;
};

typedef struct pvm_struct *pvm_struct;

pvm_val pvm_make_struct (pvm_val nfields, pvm_val nmethods, pvm_val type);
pvm_val pvm_ref_struct (pvm_val sct, pvm_val name);
int pvm_set_struct (pvm_val sct, pvm_val name, pvm_val val);
pvm_val pvm_get_struct_method (pvm_val sct, const char *name);

/* Types are also boxed.  */

#define PVM_VAL_TYP(V) (PVM_VAL_BOX_TYP (PVM_VAL_BOX ((V))))

#define PVM_VAL_TYP_CODE(V) (PVM_VAL_TYP((V))->code)
#define PVM_VAL_TYP_I_SIZE(V) (PVM_VAL_TYP((V))->val.integral.size)
#define PVM_VAL_TYP_I_SIGNED(V) (PVM_VAL_TYP((V))->val.integral.signed_p)
#define PVM_VAL_TYP_A_BOUND(V) (PVM_VAL_TYP((V))->val.array.bound)
#define PVM_VAL_TYP_A_ETYPE(V) (PVM_VAL_TYP((V))->val.array.etype)
#define PVM_VAL_TYP_S_NAME(V) (PVM_VAL_TYP((V))->val.sct.name)
#define PVM_VAL_TYP_S_NFIELDS(V) (PVM_VAL_TYP((V))->val.sct.nfields)
#define PVM_VAL_TYP_S_FNAMES(V) (PVM_VAL_TYP((V))->val.sct.fnames)
#define PVM_VAL_TYP_S_FTYPES(V) (PVM_VAL_TYP((V))->val.sct.ftypes)
#define PVM_VAL_TYP_S_FNAME(V,I) (PVM_VAL_TYP_S_FNAMES((V))[(I)])
#define PVM_VAL_TYP_S_FTYPE(V,I) (PVM_VAL_TYP_S_FTYPES((V))[(I)])
#define PVM_VAL_TYP_O_UNIT(V) (PVM_VAL_TYP((V))->val.off.unit)
#define PVM_VAL_TYP_O_BASE_TYPE(V) (PVM_VAL_TYP((V))->val.off.base_type)
#define PVM_VAL_TYP_C_RETURN_TYPE(V) (PVM_VAL_TYP((V))->val.cls.return_type)
#define PVM_VAL_TYP_C_NARGS(V) (PVM_VAL_TYP((V))->val.cls.nargs)
#define PVM_VAL_TYP_C_ATYPES(V) (PVM_VAL_TYP((V))->val.cls.atypes)
#define PVM_VAL_TYP_C_ATYPE(V,I) (PVM_VAL_TYP_C_ATYPES((V))[(I)])

enum pvm_type_code
{
  PVM_TYPE_INTEGRAL,
  PVM_TYPE_STRING,
  PVM_TYPE_ARRAY,
  PVM_TYPE_STRUCT,
  PVM_TYPE_OFFSET,
  PVM_TYPE_CLOSURE,
  PVM_TYPE_ANY
};

struct pvm_type
{
  enum pvm_type_code code;

  union
  {
    struct
    {
      pvm_val size;
      pvm_val signed_p;
    } integral;

    struct
    {
      pvm_val bound;
      pvm_val etype;
    } array;

    struct
    {
      pvm_val name;
      pvm_val nfields;
      pvm_val *fnames;
      pvm_val *ftypes;
    } sct;

    struct
    {
      pvm_val base_type;
      pvm_val unit;
    } off;

    struct
    {
      pvm_val nargs;
      pvm_val return_type;
      pvm_val *atypes;
    } cls;
  } val;
};

typedef struct pvm_type *pvm_type;

pvm_val pvm_make_integral_type (pvm_val size, pvm_val signed_p);
pvm_val pvm_make_string_type (void);
pvm_val pvm_make_any_type (void);
pvm_val pvm_make_array_type (pvm_val type, pvm_val bound);
pvm_val pvm_make_struct_type (pvm_val nfields, pvm_val name,
                              pvm_val *fnames, pvm_val *ftypes);
pvm_val pvm_make_offset_type (pvm_val base_type, pvm_val unit);
pvm_val pvm_make_closure_type (pvm_val rtype, pvm_val nargs, pvm_val *atypes);

void pvm_allocate_struct_attrs (pvm_val nfields, pvm_val **fnames, pvm_val **ftypes);
void pvm_allocate_closure_attrs (pvm_val nargs, pvm_val **atypes);

pvm_val pvm_dup_type (pvm_val type);
pvm_val pvm_typeof (pvm_val val);
int pvm_type_equal (pvm_val type1, pvm_val type2);

/* Closures are also boxed.  */

#define PVM_VAL_CLS(V) (PVM_VAL_BOX_CLS (PVM_VAL_BOX ((V))))

#define PVM_VAL_CLS_PROGRAM(V) (PVM_VAL_CLS((V))->program)
#define PVM_VAL_CLS_ENTRY_POINT(V) (PVM_VAL_CLS((V))->entry_point)
#define PVM_VAL_CLS_ENV(V) (PVM_VAL_CLS((V))->env)

/* XXX */
typedef struct pvm_program *pvm_program;
typedef void *pvm_program_program_point;

struct pvm_cls
{
  pvm_program program;
  pvm_program_program_point entry_point;
  struct pvm_env *env;
};

typedef struct pvm_cls *pvm_cls;

pvm_val pvm_make_cls (pvm_program program);

/* Offsets are boxed values.  */

#define PVM_VAL_OFF(V) (PVM_VAL_BOX_OFF (PVM_VAL_BOX ((V))))

#define PVM_VAL_OFF_MAGNITUDE(V) (PVM_VAL_OFF((V))->magnitude)
#define PVM_VAL_OFF_UNIT(V) (PVM_VAL_OFF((V))->unit)
#define PVM_VAL_OFF_BASE_TYPE(V) (PVM_VAL_OFF((V))->base_type)

#define PVM_VAL_OFF_UNIT_BITS 1
#define PVM_VAL_OFF_UNIT_NIBBLES 4
#define PVM_VAL_OFF_UNIT_BYTES (2 * PVM_VAL_OFF_UNIT_NIBBLES)

#define PVM_VAL_OFF_UNIT_KILOBITS (1000 * PVM_VAL_OFF_UNIT_BITS)
#define PVM_VAL_OFF_UNIT_KILOBYTES (1000 * PVM_VAL_OFF_UNIT_BYTES)
#define PVM_VAL_OFF_UNIT_MEGABITS (1000 * PVM_VAL_OFF_UNIT_KILOBITS)
#define PVM_VAL_OFF_UNIT_MEGABYTES (1000 * PVM_VAL_OFF_UNIT_KILOBYTES)
#define PVM_VAL_OFF_UNIT_GIGABITS (1000 * PVM_VAL_OFF_UNIT_MEGABITS)
#define PVM_VAL_OFF_UNIT_GIGABYTES (1000LU * PVM_VAL_OFF_UNIT_MEGABYTES)

#define PVM_VAL_OFF_UNIT_KIBIBITS (1024 * PVM_VAL_OFF_UNIT_BITS)
#define PVM_VAL_OFF_UNIT_KIBIBYTES (1024 * PVM_VAL_OFF_UNIT_BYTES)
#define PVM_VAL_OFF_UNIT_MEBIBITS (1024 * PVM_VAL_OFF_UNIT_KIBIBITS)
#define PVM_VAL_OFF_UNIT_MEBIBYTES (1024 * PVM_VAL_OFF_UNIT_KIBIBYTES)
#define PVM_VAL_OFF_UNIT_GIGIBITS (1024 * PVM_VAL_OFF_UNIT_MEBIBITS)
#define PVM_VAL_OFF_UNIT_GIGIBYTES (1024LU * PVM_VAL_OFF_UNIT_MEBIBYTES)

struct pvm_off
{
  pvm_val base_type;
  pvm_val magnitude;
  pvm_val unit;
};

typedef struct pvm_off *pvm_off;

pvm_val pvm_make_offset (pvm_val magnitude, pvm_val unit);

/* PVM_NULL is an invalid pvm_val.  */

#define PVM_NULL 0x7ULL

/* Public interface.  */

#define PVM_IS_INT(V) (PVM_VAL_TAG(V) == PVM_VAL_TAG_INT)
#define PVM_IS_UINT(V) (PVM_VAL_TAG(V) == PVM_VAL_TAG_UINT)
#define PVM_IS_LONG(V) (PVM_VAL_TAG(V) == PVM_VAL_TAG_LONG)
#define PVM_IS_ULONG(V) (PVM_VAL_TAG(V) == PVM_VAL_TAG_ULONG)
#define PVM_IS_STR(V)                                                   \
  (PVM_VAL_TAG(V) == PVM_VAL_TAG_BOX                                    \
   && PVM_VAL_BOX_TAG (PVM_VAL_BOX ((V))) == PVM_VAL_TAG_STR)
#define PVM_IS_ARR(V)                                                   \
  (PVM_VAL_TAG(V) == PVM_VAL_TAG_BOX                                    \
   && PVM_VAL_BOX_TAG (PVM_VAL_BOX ((V))) == PVM_VAL_TAG_ARR)
#define PVM_IS_SCT(V)                                                   \
  (PVM_VAL_TAG(V) == PVM_VAL_TAG_BOX                                    \
   && PVM_VAL_BOX_TAG (PVM_VAL_BOX ((V))) == PVM_VAL_TAG_SCT)
#define PVM_IS_TYP(V)                                                   \
  (PVM_VAL_TAG(V) == PVM_VAL_TAG_BOX                                    \
   && PVM_VAL_BOX_TAG (PVM_VAL_BOX ((V))) == PVM_VAL_TAG_TYP)
#define PVM_IS_CLS(V)                                                   \
  (PVM_VAL_TAG(V) == PVM_VAL_TAG_BOX                                    \
   && PVM_VAL_BOX_TAG (PVM_VAL_BOX ((V))) == PVM_VAL_TAG_CLS)
#define PVM_IS_OFF(V)                                                   \
  (PVM_VAL_TAG(V) == PVM_VAL_TAG_BOX                                    \
   && PVM_VAL_BOX_TAG (PVM_VAL_BOX ((V))) == PVM_VAL_TAG_OFF)


#define PVM_IS_INTEGRAL(V)                                      \
  (PVM_IS_INT (V) || PVM_IS_UINT (V)                            \
   || PVM_IS_LONG (V) || PVM_IS_ULONG (V))

#define PVM_VAL_INTEGRAL(V)                      \
  (PVM_IS_INT ((V)) ? PVM_VAL_INT ((V))          \
   : PVM_IS_UINT ((V)) ? PVM_VAL_UINT ((V))      \
   : PVM_IS_LONG ((V)) ? PVM_VAL_LONG ((V))      \
   : PVM_IS_ULONG ((V)) ? PVM_VAL_ULONG ((V))    \
   : 0)

/* The following macros allow to handle map-able PVM values (such as
   arrays and structs) polymorphically.

   It is important for the PVM_VAL_SET_{IO,OFFSET,MAPPER,WRITER} to work
   for non-mappeable values, as nops, as they are used in the
   implementation of the `unmap' operator.  */

#define PVM_VAL_OFFSET(V)                               \
  (PVM_IS_ARR ((V)) ? PVM_VAL_ARR_OFFSET ((V))          \
   : PVM_IS_SCT ((V)) ? PVM_VAL_SCT_OFFSET ((V))        \
   : PVM_NULL)

#define PVM_VAL_SET_OFFSET(V,O)                 \
  do                                            \
    {                                           \
      if (PVM_IS_ARR ((V)))                     \
        PVM_VAL_ARR_OFFSET ((V)) = (O);         \
      else if (PVM_IS_SCT ((V)))                \
        PVM_VAL_SCT_OFFSET ((V)) = (O);         \
    } while (0)

#define PVM_VAL_IOS(V)                          \
  (PVM_IS_ARR ((V)) ? PVM_VAL_ARR_IOS ((V))     \
   : PVM_IS_SCT ((V)) ? PVM_VAL_SCT_IOS ((V))   \
   : PVM_NULL)

#define PVM_VAL_SET_IOS(V,I)                     \
  do                                             \
    {                                            \
      if (PVM_IS_ARR ((V)))                      \
        PVM_VAL_ARR_IOS ((V)) = (I);             \
      else if (PVM_IS_SCT (V))                   \
        PVM_VAL_SCT_IOS ((V)) = (I);             \
    } while (0)

#define PVM_VAL_MAPPER(V)                               \
  (PVM_IS_ARR ((V)) ? PVM_VAL_ARR_MAPPER ((V))          \
   : PVM_IS_SCT ((V)) ? PVM_VAL_SCT_MAPPER ((V))        \
   : PVM_NULL)

#define PVM_VAL_ELEMS_BOUND(V)                          \
  (PVM_IS_ARR ((V)) ? PVM_VAL_ARR_ELEMS_BOUND ((V))     \
   : PVM_NULL)

#define PVM_VAL_SIZE_BOUND(V)                           \
  (PVM_IS_ARR ((V)) ? PVM_VAL_ARR_SIZE_BOUND ((V))      \
   : PVM_NULL)

#define PVM_VAL_SET_MAPPER(V,O)                 \
  do                                            \
    {                                           \
      if (PVM_IS_ARR ((V)))                     \
        PVM_VAL_ARR_MAPPER ((V)) = (O);         \
      else if (PVM_IS_SCT ((V)))                \
        PVM_VAL_SCT_MAPPER ((V)) = (O);         \
    } while (0)

#define PVM_VAL_WRITER(V)                               \
  (PVM_IS_ARR ((V)) ? PVM_VAL_ARR_WRITER ((V))          \
   : PVM_IS_SCT ((V)) ? PVM_VAL_SCT_WRITER ((V))        \
   : PVM_NULL)

#define PVM_VAL_SET_WRITER(V,O)                 \
  do                                            \
    {                                           \
      if (PVM_IS_ARR ((V)))                     \
        PVM_VAL_ARR_WRITER ((V)) = (O);         \
      else if (PVM_IS_SCT ((V)))                \
        PVM_VAL_SCT_WRITER ((V)) = (O);         \
    } while (0)

#define PVM_VAL_SET_ELEMS_BOUND(V,O)            \
  do                                            \
    {                                           \
      if (PVM_IS_ARR ((V)))                     \
        PVM_VAL_ARR_ELEMS_BOUND ((V)) = (O);    \
    } while (0)

#define PVM_VAL_SET_SIZE_BOUND(V,O)             \
  do                                            \
    {                                           \
      if (PVM_IS_ARR ((V)))                     \
        PVM_VAL_ARR_SIZE_BOUND ((V)) = (O);     \
    } while (0)

/* Return the size of VAL, in bits.  */
uint64_t pvm_sizeof (pvm_val val);

/* For strings, arrays and structs, return the number of
   elements/fields stored, as an unsigned 64-bits long.  Return 1
   otherwise.  */
pvm_val pvm_elemsof (pvm_val val);

/* Return the mapper function for the given value, and the writer
   function.  If the value is not mapped, return PVM_NULL.  */

pvm_val pvm_val_mapper (pvm_val val);
pvm_val pvm_val_writer (pvm_val val);

/* Return a PVM value for an exception with the given CODE and
   MESSAGE.  */

pvm_val pvm_make_exception (int code, char *message);

/*** PVM programs.  ***/

typedef struct pvm_program *pvm_program;
typedef int pvm_program_label;
typedef unsigned int pvm_register;
typedef void *pvm_program_program_point; /* XXX better name */

/* Create a new PVM program and return it.  */
pvm_program pvm_program_new (void);

/* Append an instruction to a PVM program.  */
void pvm_program_append_instruction (pvm_program program,
                                     const char *insn_name);

/* Append a `push' instruction to a PVM program.  */
void pvm_program_append_push_instruction (pvm_program program,
                                          pvm_val val);

/* Append instruction parameters, of several kind, to a PVM
   program.  */

void pvm_program_append_val_parameter (pvm_program program, pvm_val val);
void pvm_program_append_unsigned_parameter (pvm_program program, unsigned int n);
void pvm_program_append_register_parameter (pvm_program program, pvm_register reg);
void pvm_program_append_label_parameter (pvm_program program, pvm_program_label label);

/* Get a fresh PVM program label.  */
pvm_program_label pvm_program_fresh_label (pvm_program program);

/* Append a label to the given PVM program.  */
void pvm_program_append_label (pvm_program program, pvm_program_label label);

/* Return the program point corresponding to the beginning of the
   given program.  */
pvm_program_program_point pvm_program_beginning (pvm_program program);

/* Make the given PVM program executable.  */
void pvm_program_make_executable (pvm_program program);

/* Print a native disassembly of the given program in the standard
   output.  */
void pvm_disassemble_program_nat (pvm_program program);

/* Print a disassembly of the given program in the standard
   output.  */
void pvm_disassemble_program (pvm_program program);

/* Destroy the given PVM program.  */
void pvm_destroy_program (pvm_program program);

/*** Run-Time environment.  ***/

/* The poke virtual machine (PVM) maintains a data structure called
   the run-time environment.  This structure contains run-time frames,
   which in turn store the variables of PVM routines.

   A set of PVM instructions are provided to allow programs to
   manipulate the run-time environment.  These are implemented in
   pvm.jitter in the "Environment instructions" section, and
   summarized here:

   `pushf' pushes a new frame to the run-time environment.  This is
   used when entering a new environment, such as a function.

   `popf' pops a frame from the run-time environment.  After this
   happens, if no references are left to the popped frame, both the
   frame and the variables stored in the frame are eventually
   garbage-collected.

   `popvar' pops the value at the top of the main stack and creates a
   new variable in the run-time environment to hold that value.

   `pushvar BACK, OVER' retrieves the value of a variable from the
   run-time environment and pushes it in the main stack.  BACK is the
   number of frames to traverse and OVER is the order of the variable
   in its containing frame.  The BACK,OVER pairs (also known as
   lexical addresses) are produced by the compiler; see `pkl-env.h'
   for a description of the compile-time environment.

   This header file provides the prototypes for the functions used to
   implement the above-mentioned PVM instructions.  */

typedef struct pvm_env *pvm_env;  /* Struct defined in pvm-env.c */

/* Create a new run-time environment, containing an empty top-level
   frame, and return it.

   HINT specifies the expected number of variables that will be
   registered in this environment.  If HINT is 0 it indicates that we
   can't provide an estimation.  */

pvm_env pvm_env_new (int hint);

/* Push a new empty frame to ENV and return the modified run-time
   environment.

   HINT provides a hint on the number of entries that will be stored
   in the frame.  If HINT is 0, it indicates the number can't be
   estimated at all.  */

pvm_env pvm_env_push_frame (pvm_env env, int hint);

/* Pop a frame from ENV and return the modified run-time environment.
   The popped frame will eventually be garbage-collected if there are
   no more references to it.  Trying to pop the top-level frame is an
   error.  */

pvm_env pvm_env_pop_frame (pvm_env env);

/* Create a new variable in the current frame of ENV, whose value is
   VAL.  */

void pvm_env_register (pvm_env env, pvm_val val);

/* Return the value for the variable occupying the position BACK, OVER
   in the run-time environment ENV.  Return PVM_NULL if the variable
   is not found.  */

pvm_val pvm_env_lookup (pvm_env env, int back, int over);

/* Set the value of the variable occupying the position BACK, OVER in
   the run-time environment ENV to VAL.  */

void pvm_env_set_var (pvm_env env, int back, int over, pvm_val val);

/* Return 1 if the given run-time environment ENV contains only one
   frame.  Return 0 otherwise.  */

int pvm_env_toplevel_p (pvm_env env);

/*** Other Definitions.  ***/

enum pvm_omode
  {
    PVM_PRINT_FLAT,
    PVM_PRINT_TREE,
  };

/* The following enumeration contains every possible exit code
   resulting from the execution of a routine in the PVM.

   PVM_EXIT_OK is returned if the routine was executed successfully,
   and every raised exception was properly handled.

   PVM_EXIT_ERROR is returned in case of an unhandled exception.  */

enum pvm_exit_code
  {
    PVM_EXIT_OK,
    PVM_EXIT_ERROR
  };

/* Exceptions.  These should be in sync with the exception code
   variables, and the exception messages, declared in pkl-rt.pkl */

#define PVM_E_GENERIC       0
#define PVM_E_GENERIC_MSG "generic"

#define PVM_E_DIV_BY_ZERO   1
#define PVM_E_DIV_BY_ZERO_MSG "division by zero"

#define PVM_E_NO_IOS        2
#define PVM_E_NO_IOS_MSG "no IOS"

#define PVM_E_NO_RETURN     3
#define PVM_E_NO_RETURN_MSG "no return"

#define PVM_E_OUT_OF_BOUNDS 4
#define PVM_E_OUT_OF_BOUNDS_MSG "out of bounds"

#define PVM_E_MAP_BOUNDS    5
#define PVM_E_MAP_BOUNDS_MSG "out of map bounds"

#define PVM_E_EOF           6
#define PVM_E_EOF_MSG "EOF"

#define PVM_E_MAP           7
#define PVM_E_MAP_MSG "no map"

#define PVM_E_CONV          8
#define PVM_E_CONV_MSG "conversion error"

#define PVM_E_ELEM          9
#define PVM_E_ELEM_MSG "invalid element"

#define PVM_E_CONSTRAINT   10
#define PVM_E_CONSTRAINT_MSG "constraint violation"

#define PVM_E_IO           11
#define PVM_E_IO_MSG "generic IO"

#define PVM_E_SIGNAL       12
#define PVM_E_SIGNAL_MSG ""

#define PVM_E_IOFLAGS      13
#define PVM_E_IOFLAGS_MSG "invalid IO flags"

#define PVM_E_INVAL        14
#define PVM_E_INVAL_MSG "invalid argument"

typedef struct pvm *pvm;

/* Initialize a new Poke Virtual Machine and return it.  */

pvm pvm_init (void);

/* Finalize a Poke Virtual Machine, freeing all used resources.  */

void pvm_shutdown (pvm pvm);

/* Get the current run-time environment of PVM.  */

pvm_env pvm_get_env (pvm pvm);

/* Run a PVM program in a given Poke Virtual Machine.  Put the
   resulting value in RES, if any, and return an exit code.  */

enum pvm_exit_code pvm_run (pvm pvm,
                            pvm_program program,
                            pvm_val *res);

/* Get and set the current endianness, negative encoding and other
   global flags for the given PVM.  */

enum ios_endian pvm_endian (pvm pvm);
void pvm_set_endian (pvm pvm, enum ios_endian endian);

/* Get/set the current negative encoding for PVM.  NENC should be one
   of the IOS_NENC_* values defined in ios.h */

enum ios_nenc pvm_nenc (pvm pvm);
void pvm_set_nenc (pvm pvm, enum ios_nenc nenc);

int pvm_pretty_print (pvm pvm);
void pvm_set_pretty_print (pvm pvm, int flag);

int pvm_obase (pvm apvm);
void pvm_set_obase (pvm apvm, int obase);

enum pvm_omode pvm_omode (pvm apvm);
void pvm_set_omode (pvm apvm, enum pvm_omode omode);

int pvm_omaps (pvm apvm);
void pvm_set_omaps (pvm apvm, int omaps);

unsigned int pvm_oindent (pvm apvm);
void pvm_set_oindent (pvm apvm, unsigned int oindent);

unsigned int pvm_odepth (pvm apvm);
void pvm_set_odepth (pvm apvm, unsigned int odepth);

unsigned int pvm_oacutoff (pvm apvm);
void pvm_set_oacutoff (pvm apvm, unsigned int cutoff);

/* Get/set the compiler associated to the given virtual machine.  */

typedef struct pkl_compiler *pkl_compiler;

pkl_compiler pvm_compiler (pvm apvm);
void pvm_set_compiler (pvm apvm, pkl_compiler compiler);

/* The following function is to be used in pvm.jitter, because the
   system `assert' may expand to a macro and is therefore
   non-wrappeable.  */

void pvm_assert (int expression);

/* This is defined in the late-c block in pvm.jitter.  */

void pvm_handle_signal (int signal_number);

/* Call the pretty printer of the given value VAL.  */

int pvm_call_pretty_printer (pvm vm, pvm_val val);

/* Print a PVM value.

   DEPTH is a number that specifies the maximum depth used when
   printing composite values, i.e. arrays and structs.  If it is 0
   then it means there is no maximum depth.

   MODE is one of the PVM_PRINT_* values defined in pvm_omode, and
   specifies the output mode to use when printing the value.

   BASE is the numeration base to use when printing numbers.  It may
   be one of 2, 8, 10 or 16.

   INDENT is the step value to use when indenting nested structured
   when printin in tree mode.

   ACUTOFF is the maximum number of elements of arrays to print.
   Elements beyond are not printed.

   FLAGS is a 32-bit unsigned integer, that encodes certain properties
   to be used while printing:

   If PVM_PRINT_F_MAPS is specified then the attributes of mapped
   values (notably their offsets) are also printed out.  When
   PVM_PRINT_F_MAPS is not specified, mapped values are printed
   exactly the same way than non-mapped values.

   If PVM_PRINT_F_PPRINT is specified then pretty-printers are used to
   print struct values, if they are defined.  */

#define PVM_PRINT_F_MAPS   1
#define PVM_PRINT_F_PPRINT 2

void pvm_print_val (pvm vm, pvm_val val);
void pvm_print_val_with_params (pvm vm, pvm_val val,
                                int depth,int mode, int base,
                                int indent, int acutoff,
                                uint32_t flags);

#endif /* ! PVM_H */
