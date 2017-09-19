/* pcl-ast.c - Abstract Syntax Tree for PCL.  */

/* Copyright (C) 2017 Jose E. Marchesi */

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

#include <config.h>

#ifdef PCL_DEBUG
# include <stdio.h>
#endif

#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "xalloc.h"
#include "pcl-ast.h"

/* Return the endianness of the running system.  */

enum pcl_ast_endian
pcl_ast_default_endian (void)
{
  char buffer[4] = { 0x0, 0x0, 0x0, 0x1 };
  uint32_t canary = *((uint32_t *) buffer);

  return canary == 0x1 ? PCL_AST_MSB : PCL_AST_LSB;
}

/* Allocate and return a new AST node, with the given CODE.  The rest
   of the node is initialized to zero.  */
  
static pcl_ast
pcl_ast_make_node (enum pcl_ast_code code)
{
  pcl_ast ast;

  ast = xmalloc (sizeof (union pcl_ast_s));
  memset (ast, 0, sizeof (ast));
  PCL_AST_CODE (ast) = code;

  return ast;
}

/* Chain AST2 at the end of the tree node chain in AST1.  If AST1 is
   null then it returns AST2.  */

pcl_ast
pcl_ast_chainon (pcl_ast ast1, pcl_ast ast2)
{
  if (ast1)
    {
      pcl_ast tmp;

      for (tmp = ast1; PCL_AST_CHAIN (tmp); tmp = PCL_AST_CHAIN (tmp))
        if (tmp == ast2)
          abort ();

      PCL_AST_CHAIN (tmp) = ast2;
      return ast1;
    }

  return ast2;
}

/* Build and return an AST node for the location counter.  */

pcl_ast
pcl_ast_make_loc (void)
{
  return pcl_ast_make_node (PCL_AST_LOC);
}

/* Build and return an AST node for an integer constant.  */

pcl_ast 
pcl_ast_make_integer (uint64_t value)
{
  pcl_ast new = pcl_ast_make_node (PCL_AST_INTEGER);

  PCL_AST_INTEGER_VALUE (new) = value;
  PCL_AST_LITERAL_P (new) = 1;
  

  return new;
}

/* Build and return an AST node for a string constant.  */

pcl_ast 
pcl_ast_make_string (const char *str)
{
  pcl_ast new = pcl_ast_make_node (PCL_AST_STRING);

  assert (str);
  
  PCL_AST_STRING_POINTER (new) = xstrdup (str);
  PCL_AST_STRING_LENGTH (new) = strlen (str);
  PCL_AST_LITERAL_P (new) = 1;

  return new;
}

/* Build and return an AST node for an identifier.  */

pcl_ast
pcl_ast_make_identifier (const char *str)
{
  pcl_ast id = pcl_ast_make_node (PCL_AST_IDENTIFIER);

  PCL_AST_IDENTIFIER_POINTER (id) = xstrdup (str);
  PCL_AST_IDENTIFIER_LENGTH (id) = strlen (str);

  return id;
}

/* Build and return an AST node for a doc string.  */

pcl_ast
pcl_ast_make_doc_string (const char *str, pcl_ast entity)
{
  pcl_ast doc_string = pcl_ast_make_node (PCL_AST_DOC_STRING);

  assert (str);

  PCL_AST_DOC_STRING_POINTER (doc_string) = xstrdup (str);
  PCL_AST_DOC_STRING_LENGTH (doc_string) = strlen (str);
  PCL_AST_DOC_STRING_ENTITY (doc_string) = entity;

  return doc_string;
}

/* Build and return an AST node for an enumerator.  */

pcl_ast 
pcl_ast_make_enumerator (pcl_ast identifier,
                         pcl_ast value,
                         pcl_ast docstr)
{
  pcl_ast enumerator = pcl_ast_make_node (PCL_AST_ENUMERATOR);

  assert (identifier != NULL);
  
  PCL_AST_ENUMERATOR_IDENTIFIER (enumerator) = identifier;
  PCL_AST_ENUMERATOR_VALUE (enumerator) = value;
  PCL_AST_ENUMERATOR_DOCSTR (enumerator) = docstr;

  return enumerator;
}

/* Build and return an AST node for a conditional expression.  */

pcl_ast
pcl_ast_make_cond_exp (pcl_ast cond,
                       pcl_ast thenexp,
                       pcl_ast elseexp)
{
  pcl_ast cond_exp = pcl_ast_make_node (PCL_AST_COND_EXP);

  assert (cond && thenexp && elseexp);

  PCL_AST_COND_EXP_COND (cond_exp) = cond;
  PCL_AST_COND_EXP_THENEXP (thenexp) = thenexp;
  PCL_AST_COND_EXP_ELSEEXP (elseexp) = elseexp;

  PCL_AST_LITERAL_P (cond_exp)
    = PCL_AST_LITERAL_P (thenexp) && PCL_AST_LITERAL_P (elseexp);
  
  return cond_exp;
}

/* Build and return an AST node for a binary expression.  */

pcl_ast
pcl_ast_make_binary_exp (enum pcl_ast_op code,
                         pcl_ast op1,
                         pcl_ast op2)
{
  pcl_ast exp = pcl_ast_make_node (PCL_AST_EXP);

  assert (op1 && op2);

  PCL_AST_EXP_CODE (exp) = code;
  PCL_AST_EXP_NUMOPS (exp) = 2;
  PCL_AST_EXP_OPERAND (exp, 0) = op1;
  PCL_AST_EXP_OPERAND (exp, 1) = op2;

  PCL_AST_LITERAL_P (exp)
    = PCL_AST_LITERAL_P (op1) && PCL_AST_LITERAL_P (op2);

  return exp;
}

/* Build and return an AST node for an unary expression.  */

pcl_ast
pcl_ast_make_unary_exp (enum pcl_ast_op code,
                        pcl_ast op)
{
  pcl_ast exp = pcl_ast_make_node (PCL_AST_EXP);

  PCL_AST_EXP_CODE (exp) = code;
  PCL_AST_EXP_NUMOPS (exp) = 1;
  PCL_AST_EXP_OPERAND (exp, 0) = op;
  PCL_AST_LITERAL_P (exp) = PCL_AST_LITERAL_P (op);
  
  return exp;
}

/* Build and return an AST node for an array reference.  */

pcl_ast
pcl_ast_make_array_ref (pcl_ast base, pcl_ast index)
{
  pcl_ast aref = pcl_ast_make_node (PCL_AST_ARRAY_REF);

  assert (base && index);

  PCL_AST_ARRAY_REF_BASE (aref) = base;
  PCL_AST_ARRAY_REF_INDEX (aref) = index;
  PCL_AST_LITERAL_P (aref) = 0;

  return aref;
}

/* Build and return an AST node for a struct reference.  */

pcl_ast
pcl_ast_make_struct_ref (pcl_ast base, pcl_ast identifier)
{
  pcl_ast sref = pcl_ast_make_node (PCL_AST_STRUCT_REF);

  assert (base && identifier
          && PCL_AST_CODE (identifier) == PCL_AST_IDENTIFIER);

  PCL_AST_STRUCT_REF_BASE (sref) = base;
  PCL_AST_STRUCT_REF_IDENTIFIER (sref) = identifier;

  return sref;
}

/* Build and return an AST node for a type.  */

pcl_ast
pcl_ast_make_type (enum pcl_ast_type_code code, int signed_p,
                   size_t size, pcl_ast enumeration, pcl_ast strct)
{
  pcl_ast type = pcl_ast_make_node (PCL_AST_TYPE);

  PCL_AST_TYPE_CODE (type) = code;
  PCL_AST_TYPE_SIGNED (type) = signed_p;
  PCL_AST_TYPE_SIZE (type) = size;
  PCL_AST_TYPE_ENUMERATION (type) = enumeration;
  PCL_AST_TYPE_STRUCT (type) = strct;

  return type;
}

/* Build and return an AST node for a struct.  */

pcl_ast
pcl_ast_make_struct (pcl_ast tag, pcl_ast docstr,
                     pcl_ast mem)
{
  pcl_ast strct = pcl_ast_make_node (PCL_AST_STRUCT);

  assert (tag);

  PCL_AST_STRUCT_TAG (strct) = tag;
  PCL_AST_STRUCT_DOCSTR (strct) = docstr;
  PCL_AST_STRUCT_MEM (strct) = mem;

  return strct;
}

/* Build and return an AST node for a memory layout.  */

pcl_ast
pcl_ast_make_mem (enum pcl_ast_endian endian,
                  pcl_ast components)
{
  pcl_ast mem = pcl_ast_make_node (PCL_AST_MEM);

  PCL_AST_MEM_ENDIAN (mem) = endian;
  PCL_AST_MEM_COMPONENTS (mem) = components;

  return mem;
}

/* Build and return an AST node for an enum.  */

pcl_ast
pcl_ast_make_enum (pcl_ast tag, pcl_ast values, pcl_ast docstr)
{
  pcl_ast enumeration = pcl_ast_make_node (PCL_AST_ENUM);

  assert (tag && values);

  PCL_AST_ENUM_TAG (enumeration) = tag;
  PCL_AST_ENUM_VALUES (enumeration) = values;
  PCL_AST_ENUM_DOCSTR (enumeration) = docstr;

  return enumeration;
}

/* Build and return an AST node for a struct field.  */

pcl_ast
pcl_ast_make_field (pcl_ast name, pcl_ast type, pcl_ast docstr,
                    enum pcl_ast_endian endian, pcl_ast num_ents,
                    pcl_ast size)
{
  pcl_ast field = pcl_ast_make_node (PCL_AST_FIELD);

  assert (name);

  PCL_AST_FIELD_NAME (field) = name;
  PCL_AST_FIELD_TYPE (field) = type;
  PCL_AST_FIELD_DOCSTR (field) = docstr;
  PCL_AST_FIELD_ENDIAN (field) = endian;
  PCL_AST_FIELD_NUM_ENTS (field) = num_ents;
  PCL_AST_FIELD_SIZE (field) = size;

  return field;
}

/* Build and return an AST node for a struct conditional.  */

pcl_ast
pcl_ast_make_cond (pcl_ast exp, pcl_ast thenpart, pcl_ast elsepart)
{
  pcl_ast cond = pcl_ast_make_node (PCL_AST_COND);

  assert (exp);

  PCL_AST_COND_EXP (cond) = exp;
  PCL_AST_COND_THENPART (cond) = thenpart;
  PCL_AST_COND_ELSEPART (cond) = elsepart;

  return cond;
}

/* Build and return an AST node for a struct loop.  */

pcl_ast
pcl_ast_make_loop (pcl_ast pre, pcl_ast cond, pcl_ast post,
                   pcl_ast body)
{
  pcl_ast loop = pcl_ast_make_node (PCL_AST_LOOP);

  PCL_AST_LOOP_PRE (loop) = pre;
  PCL_AST_LOOP_COND (loop) = cond;
  PCL_AST_LOOP_POST (loop) = post;
  PCL_AST_LOOP_BODY (loop) = body;

  return loop;
}

/* Build and return an AST node for an assert.  */

pcl_ast
pcl_ast_make_assertion (pcl_ast exp)
{
  pcl_ast assertion = pcl_ast_make_node (PCL_AST_ASSERTION);

  PCL_AST_ASSERTION_EXP (assertion) = exp;
  return assertion;
}

/* Build and return an AST node for a PCL program.  */

pcl_ast
pcl_ast_make_program (void)
{
  pcl_ast program = pcl_ast_make_node (PCL_AST_PROGRAM);

  return program;
}

#ifdef PCL_DEBUG

/* The following macros are commodities to be used to keep the
   `pcl_ast_print' function as readable and easy to update as
   possible.  Do not use them anywhere else.  */

#define IPRINTF(...)                            \
  do                                            \
    {                                           \
      int i;                                    \
      for (i = 0; i < indent; i++)              \
        if (indent >= 2 && i % 2 == 0)          \
          printf ("|");                         \
        else                                    \
          printf (" ");                         \
      printf (__VA_ARGS__);                     \
    } while (0)

#define PRINT_AST_IMM(NAME,MACRO,FMT)                    \
  do                                                     \
    {                                                    \
      IPRINTF (#NAME ":\n");                             \
      IPRINTF ("  " FMT "\n", PCL_AST_##MACRO (ast));    \
    }                                                    \
  while (0)

#define PRINT_AST_SUBAST(NAME,MACRO)                            \
  do                                                            \
    {                                                           \
      IPRINTF (#NAME ":\n");                                    \
      pcl_ast_print_1 (fd, PCL_AST_##MACRO (ast), indent + 2);  \
    }                                                           \
  while (0)

#define PRINT_AST_OPT_IMM(NAME,MACRO,FMT)       \
  if (PCL_AST_##MACRO (ast))                    \
    {                                           \
      PRINT_AST_IMM (NAME, MACRO, FMT);         \
    }

#define PRINT_AST_OPT_SUBAST(NAME,MACRO)        \
  if (PCL_AST_##MACRO (ast))                    \
    {                                           \
      PRINT_AST_SUBAST (NAME, MACRO);           \
    }

#define PRINT_AST_SUBAST_CHAIN(MACRO)           \
  for (child = PCL_AST_##MACRO (ast);           \
       child;                                   \
       child = PCL_AST_CHAIN (child))           \
    {                                           \
      pcl_ast_print_1 (fd, child, indent + 2);  \
    }

/* Auxiliary function used by `pcl_ast_print', defined below.  */

static void
pcl_ast_print_1 (FILE *fd, pcl_ast ast, int indent)
{
  pcl_ast child;
  int i;
 
  if (ast == NULL)
    {
      IPRINTF ("NULL::\n");
      return;
    }
  
  switch (PCL_AST_CODE (ast))
    {
    case PCL_AST_PROGRAM:
      IPRINTF ("PROGRAM::\n");

      PRINT_AST_SUBAST_CHAIN (PROGRAM_DECLARATIONS);
      break;

    case PCL_AST_IDENTIFIER:
      IPRINTF ("IDENTIFIER::\n");

      PRINT_AST_IMM (length, IDENTIFIER_LENGTH, "%d");
      PRINT_AST_IMM (pointer, IDENTIFIER_POINTER, "0x%lx");
      PRINT_AST_OPT_IMM (*pointer, IDENTIFIER_POINTER, "'%s'");
      break;

    case PCL_AST_INTEGER:
      IPRINTF ("INTEGER::\n");

      PRINT_AST_IMM (value, INTEGER_VALUE, "%lu");
      break;

    case PCL_AST_STRING:
      IPRINTF ("STRING::\n");

      PRINT_AST_IMM (length, STRING_LENGTH, "%lu");
      PRINT_AST_IMM (pointer, STRING_POINTER, "0x%lx");
      PRINT_AST_OPT_IMM (*pointer, STRING_POINTER, "'%s'");
      break;

    case PCL_AST_DOC_STRING:
      IPRINTF ("DOCSTR::\n");

      PRINT_AST_IMM (length, DOC_STRING_LENGTH, "%lu");
      PRINT_AST_IMM (pointer, DOC_STRING_POINTER, "0x%lx");
      PRINT_AST_OPT_IMM (*pointer, DOC_STRING_POINTER, "'%s'");
      break;

    case PCL_AST_EXP:
      {

#define PCL_DEF_OP(SYM, STRING) STRING,
        static char *pcl_ast_op_name[] =
          {
#include "pcl-ops.def"
          };
#undef PCL_DEF_OP

        IPRINTF ("EXPRESSION::\n");
        IPRINTF ("opcode: %s\n",
                 pcl_ast_op_name[PCL_AST_EXP_CODE (ast)]);
        PRINT_AST_IMM (numops, EXP_NUMOPS, "%d");
        IPRINTF ("operands:\n");
        for (i = 0; i < PCL_AST_EXP_NUMOPS (ast); i++)
          pcl_ast_print_1 (fd, PCL_AST_EXP_OPERAND (ast, i),
                         indent + 2);
        break;
      }

    case PCL_AST_COND_EXP:
      IPRINTF ("COND_EXPRESSION::\n");

      PRINT_AST_SUBAST (condition, COND_EXP_COND);
      PRINT_AST_OPT_SUBAST (thenexp, COND_EXP_THENEXP);
      PRINT_AST_OPT_SUBAST (elseexp, COND_EXP_ELSEEXP);
      break;

    case PCL_AST_ENUMERATOR:
      IPRINTF ("ENUMERATOR::\n");

      PRINT_AST_SUBAST (identifier, ENUMERATOR_IDENTIFIER);
      PRINT_AST_SUBAST (value, ENUMERATOR_VALUE);
      PRINT_AST_OPT_SUBAST (docstr, ENUMERATOR_DOCSTR);
      break;

    case PCL_AST_ENUM:
      IPRINTF ("ENUM::\n");

      PRINT_AST_SUBAST (tag, ENUM_TAG);
      PRINT_AST_OPT_SUBAST (docstr, ENUM_DOCSTR);
      IPRINTF ("values:\n");
      PRINT_AST_SUBAST_CHAIN (ENUM_VALUES);
      break;

    case PCL_AST_STRUCT:
      IPRINTF ("STRUCT::\n");

      PRINT_AST_SUBAST (tag, STRUCT_TAG);
      PRINT_AST_OPT_SUBAST (docstr, STRUCT_DOCSTR);
      PRINT_AST_SUBAST (mem, STRUCT_MEM);

      break;

    case PCL_AST_MEM:
      IPRINTF ("MEM::\n");
      
      IPRINTF ("endian:\n");
      IPRINTF ("  %s\n",
               PCL_AST_MEM_ENDIAN (ast)
               == PCL_AST_MSB ? "msb" : "lsb");
      IPRINTF ("components:\n");
      PRINT_AST_SUBAST_CHAIN (MEM_COMPONENTS);

      break;
      
    case PCL_AST_FIELD:
      IPRINTF ("FIELD::\n");

      IPRINTF ("endian:\n");
      IPRINTF ("  %s\n",
               PCL_AST_FIELD_ENDIAN (ast) == PCL_AST_MSB
               ? "msb" : "lsb");
      PRINT_AST_SUBAST (name, FIELD_NAME);
      PRINT_AST_SUBAST (type, FIELD_TYPE);
      PRINT_AST_OPT_SUBAST (num_ents, FIELD_NUM_ENTS);
      PRINT_AST_OPT_SUBAST (size, FIELD_SIZE);
      PRINT_AST_OPT_SUBAST (docstr, FIELD_DOCSTR);
      
      break;

    case PCL_AST_COND:
      IPRINTF ("COND::\n");

      PRINT_AST_SUBAST (exp, COND_EXP);
      PRINT_AST_SUBAST (thenpart, COND_THENPART);
      PRINT_AST_OPT_SUBAST (elsepart, COND_ELSEPART);

      break;

    case PCL_AST_LOOP:
      IPRINTF ("LOOP::\n");

      PRINT_AST_SUBAST (pre, LOOP_PRE);
      PRINT_AST_SUBAST (cond, LOOP_COND);
      PRINT_AST_SUBAST (post, LOOP_POST);
      PRINT_AST_SUBAST (body, LOOP_BODY);

      break;

    case PCL_AST_TYPE:
      IPRINTF ("TYPE::\n");

      IPRINTF ("code:\n");
      switch (PCL_AST_TYPE_CODE (ast))
        {
        case PCL_TYPE_CHAR: IPRINTF ("  char\n"); break;
        case PCL_TYPE_SHORT: IPRINTF ("  short\n"); break;
        case PCL_TYPE_INT: IPRINTF ("  int\n"); break;
        case PCL_TYPE_LONG: IPRINTF ("  long\n"); break;
        case PCL_TYPE_ENUM: IPRINTF (" enum\n"); break;
        case PCL_TYPE_STRUCT: IPRINTF ("  struct\n"); break;
        };
      PRINT_AST_IMM (signed_p, TYPE_SIGNED, "%d");
      PRINT_AST_IMM (size, TYPE_SIZE, "%lu");

      if (PCL_AST_TYPE_ENUMERATION (ast))
        {
          pcl_ast enumeration = PCL_AST_TYPE_ENUMERATION (ast);
          const char *tag
            = PCL_AST_IDENTIFIER_POINTER (PCL_AST_ENUM_TAG (enumeration));

          IPRINTF ("enumeration:\n");
          IPRINTF ("  'enum %s'\n", tag);
        }

      if (PCL_AST_TYPE_STRUCT (ast))
        {
          pcl_ast strct = PCL_AST_TYPE_STRUCT (ast);
          const char *tag
            = PCL_AST_IDENTIFIER_POINTER (PCL_AST_STRUCT_TAG (strct));
          IPRINTF ("struct:\n");
          IPRINTF ("  'struct %s'\n", tag);
        }
      break;

    case PCL_AST_ASSERTION:
      IPRINTF ("ASSERTION::\n");

      PRINT_AST_SUBAST (exp, ASSERTION_EXP);
      break;

    case PCL_AST_LOC:
      IPRINTF ("LOC::\n");
      break;

    case PCL_AST_STRUCT_REF:
      IPRINTF ("STRUCT_REF::\n");

      PRINT_AST_SUBAST (base, STRUCT_REF_BASE);
      PRINT_AST_SUBAST (identifier, STRUCT_REF_IDENTIFIER);
      break;

    case PCL_AST_ARRAY_REF:
      IPRINTF ("ARRAY_REF::\n");

      PRINT_AST_SUBAST (base, ARRAY_REF_BASE);
      PRINT_AST_SUBAST (index, ARRAY_REF_INDEX);
      break;

    default:
      IPRINTF ("UNKNOWN:: code=%d\n", PCL_AST_CODE (ast));
      break;
    }
}

/* Dump a printable representation of AST to the file descriptor FD.
   This function is intended to be useful to debug the PCL
   compiler.  */

void
pcl_ast_print (FILE *fd, pcl_ast ast)
{
  pcl_ast_print_1 (fd, ast, 0);
}

#undef IPRINTF
#undef PRINT_AST_IMM
#undef PRINT_AST_SUBAST
#undef PRINT_AST_OPT_FIELD
#undef PRINT_AST_OPT_SUBAST

#endif /* PCL_DEBUG */
