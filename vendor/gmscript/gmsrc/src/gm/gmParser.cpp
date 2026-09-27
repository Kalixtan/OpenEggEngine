/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         gmparse
#define yylex           gmlex
#define yyerror         gmerror
#define yydebug         gmdebug
#define yynerrs         gmnerrs
#define yylval          gmlval
#define yychar          gmchar

/* First part of user prologue.  */


#define YYPARSER
#include "gmConfig.h"
#include "gmCodeTree.h"
#define YYSTYPE gmCodeTreeNode *

extern gmCodeTreeNode * g_codeTree;

#define GM_BISON_DEBUG
#ifdef GM_BISON_DEBUG
#define YYDEBUG 1
#define YYERROR_VERBOSE
#endif // GM_BISON_DEBUG

//
// HELPERS
//

void ATTACH(gmCodeTreeNode * &a_res, gmCodeTreeNode * a_a, gmCodeTreeNode * a_b)
{
  YYSTYPE t = a_a;
  if(t != NULL)
  {
    while(t->m_sibling != NULL)
    {
      t = t->m_sibling;
    }
    t->m_sibling = a_b;
    if(a_b) { a_b->m_parent = t; }
    a_res = a_a;
  }
  else
  {
    a_res = a_b;
  }
}

gmCodeTreeNode * CreateOperation(int a_subTypeType, gmCodeTreeNode * a_left = NULL, gmCodeTreeNode * a_right = NULL)
{
  gmCodeTreeNode * node = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_OPERATION, gmlineno, a_subTypeType);
  node->SetChild(0, a_left);
  node->SetChild(1, a_right);
  return node;
}

gmCodeTreeNode * CreateAsignExpression(int a_subTypeType, gmCodeTreeNode * a_left, gmCodeTreeNode * a_right)
{
  // we need to evaluate the complexety of the l-value... if it is a function call, index or dot to the left of a dot or index, we need to cache
  // into a hidden variable.

  // todo

  gmCodeTreeNode * opNode = CreateOperation(a_subTypeType, a_left, a_right);
  return CreateOperation(CTNOT_ASSIGN, a_left, opNode);
}



# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "gmParser.cpp.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_KEYWORD_LOCAL = 3,              /* KEYWORD_LOCAL  */
  YYSYMBOL_KEYWORD_GLOBAL = 4,             /* KEYWORD_GLOBAL  */
  YYSYMBOL_KEYWORD_MEMBER = 5,             /* KEYWORD_MEMBER  */
  YYSYMBOL_KEYWORD_AND = 6,                /* KEYWORD_AND  */
  YYSYMBOL_KEYWORD_OR = 7,                 /* KEYWORD_OR  */
  YYSYMBOL_KEYWORD_IF = 8,                 /* KEYWORD_IF  */
  YYSYMBOL_KEYWORD_ELSE = 9,               /* KEYWORD_ELSE  */
  YYSYMBOL_KEYWORD_WHILE = 10,             /* KEYWORD_WHILE  */
  YYSYMBOL_KEYWORD_FOR = 11,               /* KEYWORD_FOR  */
  YYSYMBOL_KEYWORD_FOREACH = 12,           /* KEYWORD_FOREACH  */
  YYSYMBOL_KEYWORD_IN = 13,                /* KEYWORD_IN  */
  YYSYMBOL_KEYWORD_BREAK = 14,             /* KEYWORD_BREAK  */
  YYSYMBOL_KEYWORD_CONTINUE = 15,          /* KEYWORD_CONTINUE  */
  YYSYMBOL_KEYWORD_NULL = 16,              /* KEYWORD_NULL  */
  YYSYMBOL_KEYWORD_DOWHILE = 17,           /* KEYWORD_DOWHILE  */
  YYSYMBOL_KEYWORD_RETURN = 18,            /* KEYWORD_RETURN  */
  YYSYMBOL_KEYWORD_FUNCTION = 19,          /* KEYWORD_FUNCTION  */
  YYSYMBOL_KEYWORD_TABLE = 20,             /* KEYWORD_TABLE  */
  YYSYMBOL_KEYWORD_THIS = 21,              /* KEYWORD_THIS  */
  YYSYMBOL_KEYWORD_TRUE = 22,              /* KEYWORD_TRUE  */
  YYSYMBOL_KEYWORD_FALSE = 23,             /* KEYWORD_FALSE  */
  YYSYMBOL_KEYWORD_FORK = 24,              /* KEYWORD_FORK  */
  YYSYMBOL_KEYWORD_SWITCH = 25,            /* KEYWORD_SWITCH  */
  YYSYMBOL_KEYWORD_CASE = 26,              /* KEYWORD_CASE  */
  YYSYMBOL_KEYWORD_DEFAULT = 27,           /* KEYWORD_DEFAULT  */
  YYSYMBOL_IDENTIFIER = 28,                /* IDENTIFIER  */
  YYSYMBOL_CONSTANT_HEX = 29,              /* CONSTANT_HEX  */
  YYSYMBOL_CONSTANT_BINARY = 30,           /* CONSTANT_BINARY  */
  YYSYMBOL_CONSTANT_INT = 31,              /* CONSTANT_INT  */
  YYSYMBOL_CONSTANT_CHAR = 32,             /* CONSTANT_CHAR  */
  YYSYMBOL_CONSTANT_FLOAT = 33,            /* CONSTANT_FLOAT  */
  YYSYMBOL_CONSTANT_STRING = 34,           /* CONSTANT_STRING  */
  YYSYMBOL_SYMBOL_ASGN_BSR = 35,           /* SYMBOL_ASGN_BSR  */
  YYSYMBOL_SYMBOL_ASGN_BSL = 36,           /* SYMBOL_ASGN_BSL  */
  YYSYMBOL_SYMBOL_ASGN_ADD = 37,           /* SYMBOL_ASGN_ADD  */
  YYSYMBOL_SYMBOL_ASGN_MINUS = 38,         /* SYMBOL_ASGN_MINUS  */
  YYSYMBOL_SYMBOL_ASGN_TIMES = 39,         /* SYMBOL_ASGN_TIMES  */
  YYSYMBOL_SYMBOL_ASGN_DIVIDE = 40,        /* SYMBOL_ASGN_DIVIDE  */
  YYSYMBOL_SYMBOL_ASGN_REM = 41,           /* SYMBOL_ASGN_REM  */
  YYSYMBOL_SYMBOL_ASGN_BAND = 42,          /* SYMBOL_ASGN_BAND  */
  YYSYMBOL_SYMBOL_ASGN_BOR = 43,           /* SYMBOL_ASGN_BOR  */
  YYSYMBOL_SYMBOL_ASGN_BXOR = 44,          /* SYMBOL_ASGN_BXOR  */
  YYSYMBOL_SYMBOL_RIGHT_SHIFT = 45,        /* SYMBOL_RIGHT_SHIFT  */
  YYSYMBOL_SYMBOL_LEFT_SHIFT = 46,         /* SYMBOL_LEFT_SHIFT  */
  YYSYMBOL_SYMBOL_INC = 47,                /* SYMBOL_INC  */
  YYSYMBOL_SYMBOL_DEC = 48,                /* SYMBOL_DEC  */
  YYSYMBOL_SYMBOL_LTE = 49,                /* SYMBOL_LTE  */
  YYSYMBOL_SYMBOL_GTE = 50,                /* SYMBOL_GTE  */
  YYSYMBOL_SYMBOL_EQ = 51,                 /* SYMBOL_EQ  */
  YYSYMBOL_SYMBOL_NEQ = 52,                /* SYMBOL_NEQ  */
  YYSYMBOL_TOKEN_ERROR = 53,               /* TOKEN_ERROR  */
  YYSYMBOL_54_ = 54,                       /* '{'  */
  YYSYMBOL_55_ = 55,                       /* '}'  */
  YYSYMBOL_56_ = 56,                       /* '('  */
  YYSYMBOL_57_ = 57,                       /* ')'  */
  YYSYMBOL_58_ = 58,                       /* '.'  */
  YYSYMBOL_59_ = 59,                       /* ';'  */
  YYSYMBOL_60_ = 60,                       /* '='  */
  YYSYMBOL_61_ = 61,                       /* ':'  */
  YYSYMBOL_62_ = 62,                       /* '['  */
  YYSYMBOL_63_ = 63,                       /* ']'  */
  YYSYMBOL_64_ = 64,                       /* '|'  */
  YYSYMBOL_65_ = 65,                       /* '^'  */
  YYSYMBOL_66_ = 66,                       /* '&'  */
  YYSYMBOL_67_ = 67,                       /* '<'  */
  YYSYMBOL_68_ = 68,                       /* '>'  */
  YYSYMBOL_69_ = 69,                       /* '+'  */
  YYSYMBOL_70_ = 70,                       /* '-'  */
  YYSYMBOL_71_ = 71,                       /* '*'  */
  YYSYMBOL_72_ = 72,                       /* '/'  */
  YYSYMBOL_73_ = 73,                       /* '%'  */
  YYSYMBOL_74_ = 74,                       /* '~'  */
  YYSYMBOL_75_ = 75,                       /* '!'  */
  YYSYMBOL_76_ = 76,                       /* '?'  */
  YYSYMBOL_77_ = 77,                       /* ','  */
  YYSYMBOL_YYACCEPT = 78,                  /* $accept  */
  YYSYMBOL_program = 79,                   /* program  */
  YYSYMBOL_statement_list = 80,            /* statement_list  */
  YYSYMBOL_statement = 81,                 /* statement  */
  YYSYMBOL_compound_statement = 82,        /* compound_statement  */
  YYSYMBOL_function_statement = 83,        /* function_statement  */
  YYSYMBOL_tablemember_expression = 84,    /* tablemember_expression  */
  YYSYMBOL_var_statement = 85,             /* var_statement  */
  YYSYMBOL_var_type = 86,                  /* var_type  */
  YYSYMBOL_expression_statement = 87,      /* expression_statement  */
  YYSYMBOL_selection_statement = 88,       /* selection_statement  */
  YYSYMBOL_case_selection_statement = 89,  /* case_selection_statement  */
  YYSYMBOL_case_selection_statement_list = 90, /* case_selection_statement_list  */
  YYSYMBOL_postfix_case_expression = 91,   /* postfix_case_expression  */
  YYSYMBOL_case_expression = 92,           /* case_expression  */
  YYSYMBOL_iteration_statement = 93,       /* iteration_statement  */
  YYSYMBOL_jump_statement = 94,            /* jump_statement  */
  YYSYMBOL_assignment_expression = 95,     /* assignment_expression  */
  YYSYMBOL_constant_expression_statement = 96, /* constant_expression_statement  */
  YYSYMBOL_constant_expression = 97,       /* constant_expression  */
  YYSYMBOL_logical_or_expression = 98,     /* logical_or_expression  */
  YYSYMBOL_logical_and_expression = 99,    /* logical_and_expression  */
  YYSYMBOL_inclusive_or_expression = 100,  /* inclusive_or_expression  */
  YYSYMBOL_exclusive_or_expression = 101,  /* exclusive_or_expression  */
  YYSYMBOL_and_expression = 102,           /* and_expression  */
  YYSYMBOL_equality_expression = 103,      /* equality_expression  */
  YYSYMBOL_relational_expression = 104,    /* relational_expression  */
  YYSYMBOL_shift_expression = 105,         /* shift_expression  */
  YYSYMBOL_additive_expression = 106,      /* additive_expression  */
  YYSYMBOL_multiplicative_expression = 107, /* multiplicative_expression  */
  YYSYMBOL_unary_expression = 108,         /* unary_expression  */
  YYSYMBOL_unary_operator = 109,           /* unary_operator  */
  YYSYMBOL_postfix_expression = 110,       /* postfix_expression  */
  YYSYMBOL_argument_expression_list = 111, /* argument_expression_list  */
  YYSYMBOL_table_constructor = 112,        /* table_constructor  */
  YYSYMBOL_function_constructor = 113,     /* function_constructor  */
  YYSYMBOL_field_list = 114,               /* field_list  */
  YYSYMBOL_field = 115,                    /* field  */
  YYSYMBOL_parameter_list = 116,           /* parameter_list  */
  YYSYMBOL_parameter = 117,                /* parameter  */
  YYSYMBOL_primary_expression = 118,       /* primary_expression  */
  YYSYMBOL_identifier = 119,               /* identifier  */
  YYSYMBOL_constant = 120,                 /* constant  */
  YYSYMBOL_constant_string_list = 121      /* constant_string_list  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int16 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  97
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1147

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  78
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  44
/* YYNRULES -- Number of rules.  */
#define YYNRULES  150
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  292

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   308


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    75,     2,     2,     2,    73,    66,     2,
      56,    57,    71,    69,    77,    70,    58,    72,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    61,    59,
      67,    60,    68,    76,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    62,     2,    63,    65,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    54,    64,    55,    74,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   128,   128,   135,   139,   146,   150,   154,   158,   162,
     166,   173,   177,   187,   198,   210,   218,   229,   233,   240,
     245,   251,   260,   273,   277,   281,   287,   291,   298,   304,
     311,   318,   324,   329,   338,   343,   349,   357,   361,   368,
     372,   376,   383,   387,   392,   396,   403,   409,   415,   422,
     430,   437,   448,   452,   456,   460,   468,   476,   480,   484,
     488,   492,   496,   500,   504,   508,   512,   516,   524,   528,
     535,   542,   546,   553,   557,   564,   568,   576,   580,   588,
     592,   600,   604,   608,   615,   619,   623,   627,   631,   638,
     642,   647,   655,   659,   664,   672,   676,   681,   686,   694,
     698,   702,   706,   715,   719,   723,   727,   731,   738,   742,
     746,   751,   757,   763,   770,   777,   781,   788,   792,   797,
     801,   806,   814,   820,   828,   832,   839,   843,   850,   854,
     861,   866,   875,   879,   884,   888,   892,   896,   900,   907,
     916,   921,   926,   931,   936,   941,   987,   992,   996,  1004,
    1018
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "KEYWORD_LOCAL",
  "KEYWORD_GLOBAL", "KEYWORD_MEMBER", "KEYWORD_AND", "KEYWORD_OR",
  "KEYWORD_IF", "KEYWORD_ELSE", "KEYWORD_WHILE", "KEYWORD_FOR",
  "KEYWORD_FOREACH", "KEYWORD_IN", "KEYWORD_BREAK", "KEYWORD_CONTINUE",
  "KEYWORD_NULL", "KEYWORD_DOWHILE", "KEYWORD_RETURN", "KEYWORD_FUNCTION",
  "KEYWORD_TABLE", "KEYWORD_THIS", "KEYWORD_TRUE", "KEYWORD_FALSE",
  "KEYWORD_FORK", "KEYWORD_SWITCH", "KEYWORD_CASE", "KEYWORD_DEFAULT",
  "IDENTIFIER", "CONSTANT_HEX", "CONSTANT_BINARY", "CONSTANT_INT",
  "CONSTANT_CHAR", "CONSTANT_FLOAT", "CONSTANT_STRING", "SYMBOL_ASGN_BSR",
  "SYMBOL_ASGN_BSL", "SYMBOL_ASGN_ADD", "SYMBOL_ASGN_MINUS",
  "SYMBOL_ASGN_TIMES", "SYMBOL_ASGN_DIVIDE", "SYMBOL_ASGN_REM",
  "SYMBOL_ASGN_BAND", "SYMBOL_ASGN_BOR", "SYMBOL_ASGN_BXOR",
  "SYMBOL_RIGHT_SHIFT", "SYMBOL_LEFT_SHIFT", "SYMBOL_INC", "SYMBOL_DEC",
  "SYMBOL_LTE", "SYMBOL_GTE", "SYMBOL_EQ", "SYMBOL_NEQ", "TOKEN_ERROR",
  "'{'", "'}'", "'('", "')'", "'.'", "';'", "'='", "':'", "'['", "']'",
  "'|'", "'^'", "'&'", "'<'", "'>'", "'+'", "'-'", "'*'", "'/'", "'%'",
  "'~'", "'!'", "'?'", "','", "$accept", "program", "statement_list",
  "statement", "compound_statement", "function_statement",
  "tablemember_expression", "var_statement", "var_type",
  "expression_statement", "selection_statement",
  "case_selection_statement", "case_selection_statement_list",
  "postfix_case_expression", "case_expression", "iteration_statement",
  "jump_statement", "assignment_expression",
  "constant_expression_statement", "constant_expression",
  "logical_or_expression", "logical_and_expression",
  "inclusive_or_expression", "exclusive_or_expression", "and_expression",
  "equality_expression", "relational_expression", "shift_expression",
  "additive_expression", "multiplicative_expression", "unary_expression",
  "unary_operator", "postfix_expression", "argument_expression_list",
  "table_constructor", "function_constructor", "field_list", "field",
  "parameter_list", "parameter", "primary_expression", "identifier",
  "constant", "constant_string_list", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-154)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     461,  -154,  -154,  -154,   -24,     2,    12,    18,   -17,    29,
    -154,    35,   522,   -18,    51,  -154,  -154,  -154,    -7,    59,
    -154,  -154,  -154,  -154,  -154,  -154,  -154,  1071,  1071,   583,
    1071,     1,  -154,  -154,  -154,  -154,  -154,  -154,    45,   461,
    -154,  -154,  -154,    36,  -154,  -154,  -154,  -154,    58,    64,
      91,    65,    72,    84,    31,    10,    60,    56,    49,  -154,
    1071,   584,  -154,  -154,  -154,  -154,  -154,   131,  1071,  1071,
     644,     1,  -154,  -154,  1071,   110,  -154,   109,    64,    28,
     -20,   -30,    42,   705,   313,  -154,   116,  1071,  -154,  -154,
    -154,  -154,   -33,  -154,   111,   115,  -154,  -154,  -154,     1,
      76,  -154,  1071,  1071,  1071,  1071,  1071,  1071,  1071,  1071,
    1071,  1071,  1071,  1071,  1071,  1071,  1071,  1071,  1071,  1071,
    -154,  1071,  1071,  1071,  1071,  1071,  1071,  1071,  1071,  1071,
    1071,   766,     1,  1071,     1,  1071,  -154,   117,   118,   827,
      74,   119,  -154,   116,   -44,  -154,   120,   -11,     1,    -9,
       1,  -154,   -41,  -154,   387,  -154,   121,  -154,   888,  1071,
    -154,   123,  -154,  1071,    91,    65,    72,    84,    31,    10,
      10,    60,    60,    60,    60,    56,    56,    49,    49,  -154,
    -154,  -154,    64,    64,    64,    64,    64,    64,    64,    64,
      64,    64,  -154,  -154,   -27,  -154,    64,   126,   114,   116,
     116,  -154,   949,   124,     1,  1071,   116,  -154,   116,     1,
    1071,   116,   -26,  -154,   116,   -16,  -154,  -154,  1071,  -154,
     130,  -154,  -154,  -154,    -8,   127,  -154,  1071,  1010,  -154,
     164,  -154,   116,   128,  -154,   174,   132,  -154,  -154,  -154,
    -154,  -154,   116,  -154,   116,   113,   116,   -14,  -154,  -154,
    -154,    -5,    -1,  -154,   116,  1071,   116,  -154,  -154,    80,
     129,  -154,   -15,  -154,   116,  -154,  -154,  -154,  -154,   134,
    -154,  -154,     1,     8,  -154,  -154,  -154,   116,  -154,  -154,
    -154,   116,  -154,     1,   116,  1071,  -154,  -154,  -154,  -154,
     133,  -154
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    23,    24,    25,     0,     0,     0,     0,     0,     0,
     148,     0,     0,     0,     0,   134,   143,   144,     0,     0,
     139,   140,   141,   142,   145,   146,   149,     0,     0,     0,
       0,     0,    26,   103,   104,   105,   106,   107,     0,     2,
       3,    10,     6,     0,     5,     7,     8,     9,     0,    56,
      71,    73,    75,    77,    79,    81,    84,    89,    92,    95,
       0,    99,   136,   137,   108,   132,   135,   147,     0,     0,
       0,     0,    53,    52,     0,     0,    54,     0,    70,    99,
       0,     0,     0,     0,     0,    32,     0,     0,   100,   101,
     119,   126,     0,   124,   132,     0,   133,     1,     4,     0,
       0,    27,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     102,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   150,     0,     0,     0,
       0,     0,    55,     0,     0,   128,   130,     0,     0,     0,
       0,   117,     0,    11,     0,    31,     0,   120,     0,     0,
     138,     0,    19,     0,    72,    74,    76,    78,    80,    82,
      83,    87,    88,    85,    86,    91,    90,    93,    94,    96,
      97,    98,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,   110,   115,     0,   114,    57,     0,     0,     0,
       0,    68,     0,     0,     0,     0,     0,   123,     0,     0,
       0,     0,     0,    18,     0,     0,    17,   118,     0,    12,
       0,   121,   125,   127,     0,     0,   111,     0,     0,   109,
      28,    46,     0,     0,    69,     0,     0,    47,   122,   129,
     131,    15,     0,    13,     0,     0,     0,     0,    20,   116,
     112,     0,     0,    48,     0,     0,     0,    16,    14,     0,
       0,    37,     0,    21,     0,   113,    29,    30,    49,     0,
      50,    44,     0,     0,    39,    42,    45,     0,    33,    38,
      22,     0,    43,     0,    34,     0,    36,    51,    41,    35,
       0,    40
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -154,  -154,   108,   -38,    38,  -154,  -154,  -154,  -154,   125,
     -58,   -65,  -154,  -154,  -154,  -154,  -154,    -3,  -154,   -12,
      34,    99,   100,    98,   101,   102,    41,    22,    39,    26,
     -25,  -154,    15,   -21,  -154,  -154,   135,  -153,  -143,     0,
    -154,    -4,   -55,  -154
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,    38,    39,    40,    85,    41,    81,    42,    43,    44,
      45,   261,   262,   273,   274,    46,    47,    48,   202,    91,
      78,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    79,   194,    62,    63,    92,    93,   144,   145,
      64,    65,    66,    67
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      77,    98,    88,    89,   212,   222,   215,     4,    20,    82,
      20,   259,   260,   208,    86,    61,   217,    20,    95,    20,
      20,    20,   157,    18,    19,    94,   147,    96,   148,    20,
     226,   242,    68,   209,    49,   120,   218,   143,    80,   100,
     278,   244,    72,   264,   158,    97,   211,    84,   214,   246,
     227,   209,   265,    84,    61,    99,   137,   138,    69,   109,
     110,   209,   141,   209,    20,   222,   283,   140,    70,   284,
     285,   102,   227,    49,    71,   156,   146,   111,   112,    94,
     204,   247,   107,   108,   131,    61,   132,   205,    73,   134,
     135,    74,   179,   180,   181,   161,    10,   103,   149,    61,
     150,   271,    16,    17,    49,   113,   114,    83,    20,    21,
      22,    23,    24,    25,    26,    87,    98,   101,    49,   193,
     117,   118,   119,   198,   155,   115,   116,   203,   195,   104,
     197,   171,   172,   173,   174,   162,   163,   105,   272,   259,
     260,   177,   178,   146,   213,   146,   216,   223,   169,   170,
     106,   225,   175,   176,    94,   182,   183,   184,   185,   186,
     187,   188,   189,   190,   191,   136,    80,   196,   142,    61,
      84,   159,   160,   252,   199,   200,   206,   229,   220,   224,
     210,   207,   228,   234,   245,   254,   248,   255,    49,   256,
     277,   281,   154,   236,   267,   139,   291,   279,   240,   233,
     235,   164,   166,   165,   276,   146,   167,   251,   168,   239,
       0,     0,     0,     0,    94,   249,   193,    61,   152,     0,
     146,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    49,   230,   231,     0,
       0,     0,     0,   269,   237,     0,   238,     0,     0,   241,
       0,     0,   243,     0,     0,   275,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   282,     0,
     253,     0,     0,   290,     0,     0,     0,     0,     0,   288,
     257,     0,   258,     0,   263,     0,     0,     0,     0,     0,
     266,     0,   268,     0,   270,     0,     0,     0,     0,     0,
       0,     0,   280,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   286,     1,     2,     3,   287,
       0,     4,   289,     5,     6,     7,     0,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,     0,
       0,    20,    21,    22,    23,    24,    25,    26,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      27,    28,     0,     0,     0,     0,     0,    29,   153,    30,
       0,    31,    32,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    33,    34,     0,     0,     0,    35,    36,    37,
       1,     2,     3,     0,     0,     4,     0,     5,     6,     7,
       0,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,     0,     0,    20,    21,    22,    23,    24,
      25,    26,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    27,    28,     0,     0,     0,     0,
       0,    29,   219,    30,     0,    31,    32,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    33,    34,     0,     0,
       0,    35,    36,    37,     1,     2,     3,     0,     0,     4,
       0,     5,     6,     7,     0,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,     0,     0,    20,
      21,    22,    23,    24,    25,    26,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    27,    28,
       0,     0,     0,     0,     0,    29,     0,    30,     0,    31,
      32,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      33,    34,     0,     0,     0,    35,    36,    37,    10,     0,
       0,    75,    14,    15,    16,    17,     0,     0,     0,     0,
      20,    21,    22,    23,    24,    25,    26,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    27,
      28,     0,     0,     0,     0,     0,    29,     0,    30,     0,
      31,    76,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    33,    34,     0,     0,     0,    35,    36,    37,    10,
       0,     0,    75,    14,    15,    16,    17,     0,     0,     0,
       0,    20,    21,    22,    23,    24,    25,    26,     0,   121,
     122,   123,   124,   125,   126,   127,   128,   129,   130,     0,
      27,    28,     0,     0,     0,     0,     0,    29,    90,    30,
     131,    31,   132,     0,   133,   134,   135,     0,     0,     0,
       0,     0,    33,    34,     0,     0,     0,    35,    36,    37,
      10,     0,     0,    75,    14,    15,    16,    17,     0,     0,
       0,     0,    20,    21,    22,    23,    24,    25,    26,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    27,    28,     0,     0,     0,     0,     0,    29,     0,
      30,     0,    31,    32,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    33,    34,     0,     0,     0,    35,    36,
      37,    10,     0,     0,    75,    14,    15,    16,    17,     0,
       0,     0,     0,    20,    21,    22,    23,    24,    25,    26,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    27,    28,     0,     0,     0,     0,     0,    29,
       0,    30,   151,    31,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    33,    34,     0,     0,     0,    35,
      36,    37,    10,     0,     0,    75,    14,    15,    16,    17,
       0,     0,     0,     0,    20,    21,    22,    23,    24,    25,
      26,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    27,    28,     0,     0,     0,     0,     0,
      29,     0,    30,   192,    31,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    33,    34,     0,     0,     0,
      35,    36,    37,    10,     0,     0,    75,    14,    15,    16,
      17,     0,     0,     0,     0,    20,    21,    22,    23,    24,
      25,    26,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    27,    28,     0,     0,     0,     0,
       0,    29,     0,    30,     0,    31,   201,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    33,    34,     0,     0,
       0,    35,    36,    37,    10,     0,     0,    75,    14,    15,
      16,    17,     0,     0,     0,     0,    20,    21,    22,    23,
      24,    25,    26,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    27,    28,     0,     0,     0,
       0,     0,    29,   221,    30,     0,    31,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    33,    34,     0,
       0,     0,    35,    36,    37,    10,     0,     0,    75,    14,
      15,    16,    17,     0,     0,     0,     0,    20,    21,    22,
      23,    24,    25,    26,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    27,    28,     0,     0,
       0,     0,     0,    29,     0,    30,   232,    31,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    33,    34,
       0,     0,     0,    35,    36,    37,    10,     0,     0,    75,
      14,    15,    16,    17,     0,     0,     0,     0,    20,    21,
      22,    23,    24,    25,    26,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    27,    28,     0,
       0,     0,     0,     0,    29,     0,    30,   250,    31,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    33,
      34,     0,     0,     0,    35,    36,    37,    10,     0,     0,
      75,    14,    15,    16,    17,     0,     0,     0,     0,    20,
      21,    22,    23,    24,    25,    26,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    27,    28,
       0,     0,     0,     0,     0,    29,     0,    30,     0,    31,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      33,    34,     0,     0,     0,    35,    36,    37
};

static const yytype_int16 yycheck[] =
{
      12,    39,    27,    28,   147,   158,   149,     8,    28,    13,
      28,    26,    27,    57,    18,     0,    57,    28,    30,    28,
      28,    28,    55,    24,    25,    29,    56,    31,    58,    28,
      57,    57,    56,    77,     0,    60,    77,    57,    56,    43,
      55,    57,    59,    57,    77,     0,    57,    54,    57,    57,
      77,    77,    57,    54,    39,    19,    68,    69,    56,    49,
      50,    77,    74,    77,    28,   218,    58,    71,    56,    61,
      62,     7,    77,    39,    56,    87,    80,    67,    68,    83,
       6,   224,    51,    52,    56,    70,    58,    13,    59,    61,
      62,    56,   117,   118,   119,    99,    16,     6,    56,    84,
      58,    21,    22,    23,    70,    45,    46,    56,    28,    29,
      30,    31,    32,    33,    34,    56,   154,    59,    84,   131,
      71,    72,    73,   135,    86,    69,    70,   139,   132,    64,
     134,   109,   110,   111,   112,    59,    60,    65,    58,    26,
      27,   115,   116,   147,   148,   149,   150,   159,   107,   108,
      66,   163,   113,   114,   158,   121,   122,   123,   124,   125,
     126,   127,   128,   129,   130,    34,    56,   133,    59,   154,
      54,    60,    57,     9,    57,    57,    57,    63,    57,    56,
      60,   143,    56,    59,    54,    57,    59,    13,   154,    57,
      61,    57,    84,   205,   252,    70,    63,   262,   210,   202,
     204,   102,   104,   103,   259,   209,   105,   228,   106,   209,
      -1,    -1,    -1,    -1,   218,   227,   228,   202,    83,    -1,
     224,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   202,   199,   200,    -1,
      -1,    -1,    -1,   255,   206,    -1,   208,    -1,    -1,   211,
      -1,    -1,   214,    -1,    -1,   259,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   272,    -1,
     232,    -1,    -1,   285,    -1,    -1,    -1,    -1,    -1,   283,
     242,    -1,   244,    -1,   246,    -1,    -1,    -1,    -1,    -1,
     252,    -1,   254,    -1,   256,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   264,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   277,     3,     4,     5,   281,
      -1,     8,   284,    10,    11,    12,    -1,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    -1,
      -1,    28,    29,    30,    31,    32,    33,    34,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      47,    48,    -1,    -1,    -1,    -1,    -1,    54,    55,    56,
      -1,    58,    59,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    69,    70,    -1,    -1,    -1,    74,    75,    76,
       3,     4,     5,    -1,    -1,     8,    -1,    10,    11,    12,
      -1,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    -1,    -1,    28,    29,    30,    31,    32,
      33,    34,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    47,    48,    -1,    -1,    -1,    -1,
      -1,    54,    55,    56,    -1,    58,    59,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    69,    70,    -1,    -1,
      -1,    74,    75,    76,     3,     4,     5,    -1,    -1,     8,
      -1,    10,    11,    12,    -1,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    -1,    -1,    28,
      29,    30,    31,    32,    33,    34,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    47,    48,
      -1,    -1,    -1,    -1,    -1,    54,    -1,    56,    -1,    58,
      59,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      69,    70,    -1,    -1,    -1,    74,    75,    76,    16,    -1,
      -1,    19,    20,    21,    22,    23,    -1,    -1,    -1,    -1,
      28,    29,    30,    31,    32,    33,    34,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    47,
      48,    -1,    -1,    -1,    -1,    -1,    54,    -1,    56,    -1,
      58,    59,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    69,    70,    -1,    -1,    -1,    74,    75,    76,    16,
      -1,    -1,    19,    20,    21,    22,    23,    -1,    -1,    -1,
      -1,    28,    29,    30,    31,    32,    33,    34,    -1,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    -1,
      47,    48,    -1,    -1,    -1,    -1,    -1,    54,    55,    56,
      56,    58,    58,    -1,    60,    61,    62,    -1,    -1,    -1,
      -1,    -1,    69,    70,    -1,    -1,    -1,    74,    75,    76,
      16,    -1,    -1,    19,    20,    21,    22,    23,    -1,    -1,
      -1,    -1,    28,    29,    30,    31,    32,    33,    34,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    47,    48,    -1,    -1,    -1,    -1,    -1,    54,    -1,
      56,    -1,    58,    59,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    69,    70,    -1,    -1,    -1,    74,    75,
      76,    16,    -1,    -1,    19,    20,    21,    22,    23,    -1,
      -1,    -1,    -1,    28,    29,    30,    31,    32,    33,    34,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    47,    48,    -1,    -1,    -1,    -1,    -1,    54,
      -1,    56,    57,    58,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    69,    70,    -1,    -1,    -1,    74,
      75,    76,    16,    -1,    -1,    19,    20,    21,    22,    23,
      -1,    -1,    -1,    -1,    28,    29,    30,    31,    32,    33,
      34,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    47,    48,    -1,    -1,    -1,    -1,    -1,
      54,    -1,    56,    57,    58,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    69,    70,    -1,    -1,    -1,
      74,    75,    76,    16,    -1,    -1,    19,    20,    21,    22,
      23,    -1,    -1,    -1,    -1,    28,    29,    30,    31,    32,
      33,    34,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    47,    48,    -1,    -1,    -1,    -1,
      -1,    54,    -1,    56,    -1,    58,    59,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    69,    70,    -1,    -1,
      -1,    74,    75,    76,    16,    -1,    -1,    19,    20,    21,
      22,    23,    -1,    -1,    -1,    -1,    28,    29,    30,    31,
      32,    33,    34,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    47,    48,    -1,    -1,    -1,
      -1,    -1,    54,    55,    56,    -1,    58,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    69,    70,    -1,
      -1,    -1,    74,    75,    76,    16,    -1,    -1,    19,    20,
      21,    22,    23,    -1,    -1,    -1,    -1,    28,    29,    30,
      31,    32,    33,    34,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    47,    48,    -1,    -1,
      -1,    -1,    -1,    54,    -1,    56,    57,    58,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    69,    70,
      -1,    -1,    -1,    74,    75,    76,    16,    -1,    -1,    19,
      20,    21,    22,    23,    -1,    -1,    -1,    -1,    28,    29,
      30,    31,    32,    33,    34,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    47,    48,    -1,
      -1,    -1,    -1,    -1,    54,    -1,    56,    57,    58,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    69,
      70,    -1,    -1,    -1,    74,    75,    76,    16,    -1,    -1,
      19,    20,    21,    22,    23,    -1,    -1,    -1,    -1,    28,
      29,    30,    31,    32,    33,    34,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    47,    48,
      -1,    -1,    -1,    -1,    -1,    54,    -1,    56,    -1,    58,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      69,    70,    -1,    -1,    -1,    74,    75,    76
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,     4,     5,     8,    10,    11,    12,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      28,    29,    30,    31,    32,    33,    34,    47,    48,    54,
      56,    58,    59,    69,    70,    74,    75,    76,    79,    80,
      81,    83,    85,    86,    87,    88,    93,    94,    95,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   112,   113,   118,   119,   120,   121,    56,    56,
      56,    56,    59,    59,    56,    19,    59,    97,    98,   110,
      56,    84,   119,    56,    54,    82,   119,    56,   108,   108,
      55,    97,   114,   115,   119,    97,   119,     0,    81,    19,
     119,    59,     7,     6,    64,    65,    66,    51,    52,    49,
      50,    67,    68,    45,    46,    69,    70,    71,    72,    73,
     108,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    56,    58,    60,    61,    62,    34,    97,    97,    87,
     119,    97,    59,    57,   116,   117,   119,    56,    58,    56,
      58,    57,   114,    55,    80,    82,    97,    55,    77,    60,
      57,   119,    59,    60,    99,   100,   101,   102,   103,   104,
     104,   105,   105,   105,   105,   106,   106,   107,   107,   108,
     108,   108,    98,    98,    98,    98,    98,    98,    98,    98,
      98,    98,    57,    97,   111,   119,    98,   119,    97,    57,
      57,    59,    96,    97,     6,    13,    57,    82,    57,    77,
      60,    57,   116,   119,    57,   116,   119,    57,    77,    55,
      57,    55,   115,    97,    56,    97,    57,    77,    56,    63,
      82,    82,    57,    95,    59,   119,    97,    82,    82,   117,
      97,    82,    57,    82,    57,    54,    57,   116,    59,    97,
      57,   111,     9,    82,    57,    13,    57,    82,    82,    26,
      27,    89,    90,    82,    57,    57,    82,    88,    82,    97,
      82,    21,    58,    91,    92,   119,   120,    61,    55,    89,
      82,    57,   119,    58,    61,    62,    82,    82,   119,    82,
      97,    63
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    78,    79,    80,    80,    81,    81,    81,    81,    81,
      81,    82,    82,    83,    83,    83,    83,    84,    84,    85,
      85,    85,    85,    86,    86,    86,    87,    87,    88,    88,
      88,    88,    88,    88,    89,    89,    89,    90,    90,    91,
      91,    91,    92,    92,    92,    92,    93,    93,    93,    93,
      93,    93,    94,    94,    94,    94,    95,    95,    95,    95,
      95,    95,    95,    95,    95,    95,    95,    95,    96,    96,
      97,    98,    98,    99,    99,   100,   100,   101,   101,   102,
     102,   103,   103,   103,   104,   104,   104,   104,   104,   105,
     105,   105,   106,   106,   106,   107,   107,   107,   107,   108,
     108,   108,   108,   109,   109,   109,   109,   109,   110,   110,
     110,   110,   110,   110,   110,   111,   111,   112,   112,   112,
     112,   112,   113,   113,   114,   114,   115,   115,   116,   116,
     117,   117,   118,   118,   118,   118,   118,   118,   118,   119,
     120,   120,   120,   120,   120,   120,   120,   120,   120,   121,
     121
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     2,     1,     1,     1,     1,     1,
       1,     2,     3,     5,     6,     5,     6,     3,     3,     3,
       5,     6,     7,     1,     1,     1,     1,     2,     5,     7,
       7,     3,     2,     7,     3,     4,     3,     1,     2,     1,
       4,     3,     1,     2,     1,     1,     5,     5,     6,     7,
       7,     9,     2,     2,     2,     3,     1,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     1,     2,
       1,     1,     3,     1,     3,     1,     3,     1,     3,     1,
       3,     1,     3,     3,     1,     3,     3,     3,     3,     1,
       3,     3,     1,     3,     3,     1,     3,     3,     3,     1,
       2,     2,     2,     1,     1,     1,     1,     1,     1,     4,
       3,     4,     5,     6,     3,     1,     3,     3,     4,     2,
       3,     4,     5,     4,     1,     3,     1,     3,     1,     3,
       1,     3,     1,     2,     1,     1,     1,     1,     3,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       2
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* program: statement_list  */
    {
      g_codeTree = yyvsp[0];
    }
    break;

  case 3: /* statement_list: statement  */
    {
      yyval = yyvsp[0];
    }
    break;

  case 4: /* statement_list: statement_list statement  */
    {
      ATTACH(yyval, yyvsp[-1], yyvsp[0]);
    }
    break;

  case 5: /* statement: expression_statement  */
    {
      yyval = yyvsp[0];
    }
    break;

  case 6: /* statement: var_statement  */
    {
      yyval = yyvsp[0];
    }
    break;

  case 7: /* statement: selection_statement  */
    {
      yyval = yyvsp[0];
    }
    break;

  case 8: /* statement: iteration_statement  */
    {
      yyval = yyvsp[0];
    }
    break;

  case 9: /* statement: jump_statement  */
    {
      yyval = yyvsp[0];
    }
    break;

  case 10: /* statement: function_statement  */
    {
	  yyval = yyvsp[0];
	}
    break;

  case 11: /* compound_statement: '{' '}'  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_COMPOUND, gmlineno);
    }
    break;

  case 12: /* compound_statement: '{' statement_list '}'  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_COMPOUND, gmlineno);
      yyval->SetChild(0, yyvsp[-1]);
    }
    break;

  case 13: /* function_statement: KEYWORD_FUNCTION identifier '(' ')' compound_statement  */
    {
      gmCodeTreeNode* func = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_FUNCTION, gmlineno);
      func->SetChild(1, yyvsp[0]);
      

      yyval = gmCodeTreeNode::Create(CTNT_DECLARATION, CTNDT_VARIABLE, gmlineno, (int)GMMACHINE_DEFAULT_FUNCTION);
      yyval->SetChild(0, yyvsp[-3]);
      ATTACH(yyval, yyval, CreateOperation(CTNOT_ASSIGN, yyvsp[-3], func));
   }
    break;

  case 14: /* function_statement: KEYWORD_FUNCTION identifier '(' parameter_list ')' compound_statement  */
    {
      gmCodeTreeNode* func = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_FUNCTION, gmlineno);
      func->SetChild(0, yyvsp[-2]);
      func->SetChild(1, yyvsp[0]);
      

      yyval = gmCodeTreeNode::Create(CTNT_DECLARATION, CTNDT_VARIABLE, gmlineno, (int)GMMACHINE_DEFAULT_FUNCTION);
      yyval->SetChild(0, yyvsp[-4]);
      ATTACH(yyval, yyval, CreateOperation(CTNOT_ASSIGN, yyvsp[-4], func));
   }
    break;

  case 15: /* function_statement: KEYWORD_FUNCTION tablemember_expression '(' ')' compound_statement  */
    {
      gmCodeTreeNode* func = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_FUNCTION, gmlineno);
      func->SetChild(1, yyvsp[0]);
      
      ATTACH(yyval, yyval, CreateOperation(CTNOT_ASSIGN, yyvsp[-3], func));
   }
    break;

  case 16: /* function_statement: KEYWORD_FUNCTION tablemember_expression '(' parameter_list ')' compound_statement  */
    {
      gmCodeTreeNode* func = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_FUNCTION, gmlineno);
      func->SetChild(0, yyvsp[-2]);
      func->SetChild(1, yyvsp[0]);
      
      ATTACH(yyval, yyval, CreateOperation(CTNOT_ASSIGN, yyvsp[-4], func));
   }
    break;

  case 17: /* tablemember_expression: identifier '.' identifier  */
    {
      yyval = CreateOperation(CTNOT_DOT, yyvsp[-2], yyvsp[0]);
    }
    break;

  case 18: /* tablemember_expression: tablemember_expression '.' identifier  */
    {
      yyval = CreateOperation(CTNOT_DOT, yyvsp[-2], yyvsp[0]);
    }
    break;

  case 19: /* var_statement: var_type identifier ';'  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_DECLARATION, CTNDT_VARIABLE, gmlineno, (int)(gmptr) yyvsp[-2]);
      yyval->SetChild(0, yyvsp[-1]);
    }
    break;

  case 20: /* var_statement: var_type identifier '=' constant_expression ';'  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_DECLARATION, CTNDT_VARIABLE, gmlineno, (int)(gmptr) yyvsp[-4]);
      yyval->SetChild(0, yyvsp[-3]);
      ATTACH(yyval, yyval, CreateOperation(CTNOT_ASSIGN, yyvsp[-3], yyvsp[-1]));
    }
    break;

  case 21: /* var_statement: var_type KEYWORD_FUNCTION identifier '(' ')' compound_statement  */
    {
      gmCodeTreeNode* func = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_FUNCTION, gmlineno);
      func->SetChild(1, yyvsp[0]);

      yyval = gmCodeTreeNode::Create(CTNT_DECLARATION, CTNDT_VARIABLE, gmlineno, (int)(gmptr) yyvsp[-5]);
      yyval->SetChild(0, yyvsp[-3]);
      ATTACH(yyval, yyval, CreateOperation(CTNOT_ASSIGN, yyvsp[-3], func));
    }
    break;

  case 22: /* var_statement: var_type KEYWORD_FUNCTION identifier '(' parameter_list ')' compound_statement  */
    {
      gmCodeTreeNode* func = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_FUNCTION, gmlineno);
      func->SetChild(0, yyvsp[-2]);
      func->SetChild(1, yyvsp[0]);

      yyval = gmCodeTreeNode::Create(CTNT_DECLARATION, CTNDT_VARIABLE, gmlineno, (int)(gmptr) yyvsp[-6]);
      yyval->SetChild(0, yyvsp[-4]);
      ATTACH(yyval, yyval, CreateOperation(CTNOT_ASSIGN, yyvsp[-4], func));
    }
    break;

  case 23: /* var_type: KEYWORD_LOCAL  */
    {
      yyval = (YYSTYPE) CTVT_LOCAL;
    }
    break;

  case 24: /* var_type: KEYWORD_GLOBAL  */
    {
      yyval = (YYSTYPE) CTVT_GLOBAL;
    }
    break;

  case 25: /* var_type: KEYWORD_MEMBER  */
    {
      yyval = (YYSTYPE) CTVT_MEMBER;
    }
    break;

  case 26: /* expression_statement: ';'  */
    {
      yyval = NULL;
    }
    break;

  case 27: /* expression_statement: assignment_expression ';'  */
    {
      yyval = yyvsp[-1];
    }
    break;

  case 28: /* selection_statement: KEYWORD_IF '(' constant_expression ')' compound_statement  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_IF, (yyvsp[-2]) ? yyvsp[-2]->m_lineNumber : gmlineno);
      yyval->SetChild(0, yyvsp[-2]);
      yyval->SetChild(1, yyvsp[0]);
    }
    break;

  case 29: /* selection_statement: KEYWORD_IF '(' constant_expression ')' compound_statement KEYWORD_ELSE compound_statement  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_IF, (yyvsp[-4]) ? yyvsp[-4]->m_lineNumber : gmlineno);
      yyval->SetChild(0, yyvsp[-4]);
      yyval->SetChild(1, yyvsp[-2]);
      yyval->SetChild(2, yyvsp[0]);
    }
    break;

  case 30: /* selection_statement: KEYWORD_IF '(' constant_expression ')' compound_statement KEYWORD_ELSE selection_statement  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_IF, (yyvsp[-4]) ? yyvsp[-4]->m_lineNumber : gmlineno);
      yyval->SetChild(0, yyvsp[-4]);
      yyval->SetChild(1, yyvsp[-2]);
      yyval->SetChild(2, yyvsp[0]);
    }
    break;

  case 31: /* selection_statement: KEYWORD_FORK identifier compound_statement  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_FORK, (yyvsp[-1]) ? yyvsp[-1]->m_lineNumber : gmlineno );
      yyval->SetChild(0, yyvsp[0] );
      yyval->SetChild(1, yyvsp[-1] );
    }
    break;

  case 32: /* selection_statement: KEYWORD_FORK compound_statement  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_FORK, (yyvsp[0]) ? yyvsp[0]->m_lineNumber : gmlineno );
      yyval->SetChild(0, yyvsp[0] );
    }
    break;

  case 33: /* selection_statement: KEYWORD_SWITCH '(' constant_expression ')' '{' case_selection_statement_list '}'  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_SWITCH, (yyvsp[-4]) ? yyvsp[-4]->m_lineNumber : gmlineno);
      yyval->SetChild(0, yyvsp[-4]);
      yyval->SetChild(1, yyvsp[-1]);
    }
    break;

  case 34: /* case_selection_statement: KEYWORD_CASE postfix_case_expression ':'  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_CASE, (yyvsp[-1]) ? yyvsp[-1]->m_lineNumber : gmlineno);
      yyval->SetChild(0, yyvsp[-1]);
    }
    break;

  case 35: /* case_selection_statement: KEYWORD_CASE postfix_case_expression ':' compound_statement  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_CASE, (yyvsp[-2]) ? yyvsp[-2]->m_lineNumber : gmlineno);
      yyval->SetChild(0, yyvsp[-2]);
      yyval->SetChild(1, yyvsp[0]);
    }
    break;

  case 36: /* case_selection_statement: KEYWORD_DEFAULT ':' compound_statement  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_DEFAULT, (yyvsp[0]) ? yyvsp[0]->m_lineNumber : gmlineno);
      yyval->SetChild(0, yyvsp[0]);
    }
    break;

  case 37: /* case_selection_statement_list: case_selection_statement  */
    {
      yyval = yyvsp[0];
    }
    break;

  case 38: /* case_selection_statement_list: case_selection_statement_list case_selection_statement  */
    {
      ATTACH(yyval, yyvsp[-1], yyvsp[0]);
    }
    break;

  case 39: /* postfix_case_expression: case_expression  */
    {
      yyval = yyvsp[0];
    }
    break;

  case 40: /* postfix_case_expression: postfix_case_expression '[' constant_expression ']'  */
    {
      yyval = CreateOperation(CTNOT_ARRAY_INDEX, yyvsp[-3], yyvsp[-1]);
    }
    break;

  case 41: /* postfix_case_expression: postfix_case_expression '.' identifier  */
    {
      yyval = CreateOperation(CTNOT_DOT, yyvsp[-2], yyvsp[0]);
    }
    break;

  case 42: /* case_expression: identifier  */
    {
      yyval = yyvsp[0];
    }
    break;

  case 43: /* case_expression: '.' identifier  */
    {
      yyval = yyvsp[0];
      yyval->m_flags |= gmCodeTreeNode::CTN_MEMBER;
    }
    break;

  case 44: /* case_expression: KEYWORD_THIS  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_THIS, gmlineno);
    }
    break;

  case 45: /* case_expression: constant  */
    {
      yyval = yyvsp[0];
    }
    break;

  case 46: /* iteration_statement: KEYWORD_WHILE '(' constant_expression ')' compound_statement  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_WHILE, (yyvsp[-2]) ? yyvsp[-2]->m_lineNumber : gmlineno);
      yyval->SetChild(0, yyvsp[-2]);
      yyval->SetChild(1, yyvsp[0]);
    }
    break;

  case 47: /* iteration_statement: KEYWORD_DOWHILE '(' constant_expression ')' compound_statement  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_DOWHILE, (yyvsp[-2]) ? yyvsp[-2]->m_lineNumber : gmlineno);
      yyval->SetChild(0, yyvsp[-2]);
      yyval->SetChild(1, yyvsp[0]);
    }
    break;

  case 48: /* iteration_statement: KEYWORD_FOR '(' expression_statement constant_expression_statement ')' compound_statement  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_FOR, (yyvsp[-3]) ? yyvsp[-3]->m_lineNumber : gmlineno);
      yyval->SetChild(0, yyvsp[-3]);
      yyval->SetChild(1, yyvsp[-2]);
      yyval->SetChild(3, yyvsp[0]);
    }
    break;

  case 49: /* iteration_statement: KEYWORD_FOR '(' expression_statement constant_expression_statement assignment_expression ')' compound_statement  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_FOR, (yyvsp[-4]) ? yyvsp[-4]->m_lineNumber : gmlineno);
      yyval->SetChild(0, yyvsp[-4]);
      yyval->SetChild(1, yyvsp[-3]);
      yyval->SetChild(2, yyvsp[-2]);
      yyval->SetChild(3, yyvsp[0]);
    }
    break;

  case 50: /* iteration_statement: KEYWORD_FOREACH '(' identifier KEYWORD_IN constant_expression ')' compound_statement  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_FOREACH, (yyvsp[-2]) ? yyvsp[-2]->m_lineNumber : gmlineno);
      yyval->SetChild(0, yyvsp[-2]);
      yyval->SetChild(1, yyvsp[-4]);
      yyval->SetChild(3, yyvsp[0]);
    }
    break;

  case 51: /* iteration_statement: KEYWORD_FOREACH '(' identifier KEYWORD_AND identifier KEYWORD_IN constant_expression ')' compound_statement  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_FOREACH, (yyvsp[-2]) ? yyvsp[-2]->m_lineNumber : gmlineno);
      yyval->SetChild(0, yyvsp[-2]);
      yyval->SetChild(1, yyvsp[-4]);
      yyval->SetChild(2, yyvsp[-6]);
      yyval->SetChild(3, yyvsp[0]);
    }
    break;

  case 52: /* jump_statement: KEYWORD_CONTINUE ';'  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_CONTINUE, gmlineno);
    }
    break;

  case 53: /* jump_statement: KEYWORD_BREAK ';'  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_BREAK, gmlineno);
    }
    break;

  case 54: /* jump_statement: KEYWORD_RETURN ';'  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_RETURN, gmlineno);
    }
    break;

  case 55: /* jump_statement: KEYWORD_RETURN constant_expression ';'  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_STATEMENT, CTNST_RETURN, gmlineno);
      yyval->SetChild(0, yyvsp[-1]);
    }
    break;

  case 56: /* assignment_expression: logical_or_expression  */
    {
      yyval = yyvsp[0];
      if(yyval)
      {
        yyval->m_flags |= gmCodeTreeNode::CTN_POP;
      }
    }
    break;

  case 57: /* assignment_expression: postfix_expression '=' logical_or_expression  */
    {
      yyval = CreateOperation(CTNOT_ASSIGN, yyvsp[-2], yyvsp[0]);
    }
    break;

  case 58: /* assignment_expression: postfix_expression SYMBOL_ASGN_BSR logical_or_expression  */
    {
      yyval = CreateAsignExpression(CTNOT_SHIFT_RIGHT, yyvsp[-2], yyvsp[0]);
    }
    break;

  case 59: /* assignment_expression: postfix_expression SYMBOL_ASGN_BSL logical_or_expression  */
    {
      yyval = CreateAsignExpression(CTNOT_SHIFT_LEFT, yyvsp[-2], yyvsp[0]);
    }
    break;

  case 60: /* assignment_expression: postfix_expression SYMBOL_ASGN_ADD logical_or_expression  */
    {
      yyval = CreateAsignExpression(CTNOT_ADD, yyvsp[-2], yyvsp[0]);
    }
    break;

  case 61: /* assignment_expression: postfix_expression SYMBOL_ASGN_MINUS logical_or_expression  */
    {
      yyval = CreateAsignExpression(CTNOT_MINUS, yyvsp[-2], yyvsp[0]);
    }
    break;

  case 62: /* assignment_expression: postfix_expression SYMBOL_ASGN_TIMES logical_or_expression  */
    {
      yyval = CreateAsignExpression(CTNOT_TIMES, yyvsp[-2], yyvsp[0]);
    }
    break;

  case 63: /* assignment_expression: postfix_expression SYMBOL_ASGN_DIVIDE logical_or_expression  */
    {
      yyval = CreateAsignExpression(CTNOT_DIVIDE, yyvsp[-2], yyvsp[0]);
    }
    break;

  case 64: /* assignment_expression: postfix_expression SYMBOL_ASGN_REM logical_or_expression  */
    {
      yyval = CreateAsignExpression(CTNOT_REM, yyvsp[-2], yyvsp[0]);
    }
    break;

  case 65: /* assignment_expression: postfix_expression SYMBOL_ASGN_BAND logical_or_expression  */
    {
      yyval = CreateAsignExpression(CTNOT_BIT_AND, yyvsp[-2], yyvsp[0]);
    }
    break;

  case 66: /* assignment_expression: postfix_expression SYMBOL_ASGN_BOR logical_or_expression  */
    {
      yyval = CreateAsignExpression(CTNOT_BIT_OR, yyvsp[-2], yyvsp[0]);
    }
    break;

  case 67: /* assignment_expression: postfix_expression SYMBOL_ASGN_BXOR logical_or_expression  */
    {
      yyval = CreateAsignExpression(CTNOT_BIT_XOR, yyvsp[-2], yyvsp[0]);
    }
    break;

  case 68: /* constant_expression_statement: ';'  */
    {
      yyval = NULL;
    }
    break;

  case 69: /* constant_expression_statement: constant_expression ';'  */
    {
      yyval = yyvsp[-1];
    }
    break;

  case 70: /* constant_expression: logical_or_expression  */
    {
      yyval = yyvsp[0];
    }
    break;

  case 71: /* logical_or_expression: logical_and_expression  */
    {
      yyval = yyvsp[0];
    }
    break;

  case 72: /* logical_or_expression: logical_or_expression KEYWORD_OR logical_and_expression  */
    {
      yyval = CreateOperation(CTNOT_OR, yyvsp[-2], yyvsp[0]);
    }
    break;

  case 73: /* logical_and_expression: inclusive_or_expression  */
    {
      yyval = yyvsp[0];
    }
    break;

  case 74: /* logical_and_expression: logical_and_expression KEYWORD_AND inclusive_or_expression  */
    {
      yyval = CreateOperation(CTNOT_AND, yyvsp[-2], yyvsp[0]);
    }
    break;

  case 75: /* inclusive_or_expression: exclusive_or_expression  */
    {
      yyval = yyvsp[0];
    }
    break;

  case 76: /* inclusive_or_expression: inclusive_or_expression '|' exclusive_or_expression  */
    {
      yyval = CreateOperation(CTNOT_BIT_OR, yyvsp[-2], yyvsp[0]);
      yyval->ConstantFold();
    }
    break;

  case 77: /* exclusive_or_expression: and_expression  */
    {
      yyval = yyvsp[0];
    }
    break;

  case 78: /* exclusive_or_expression: exclusive_or_expression '^' and_expression  */
    {
      yyval = CreateOperation(CTNOT_BIT_XOR, yyvsp[-2], yyvsp[0]);
      yyval->ConstantFold();
    }
    break;

  case 79: /* and_expression: equality_expression  */
    {
      yyval = yyvsp[0];
    }
    break;

  case 80: /* and_expression: and_expression '&' equality_expression  */
    {
      yyval = CreateOperation(CTNOT_BIT_AND, yyvsp[-2], yyvsp[0]);
      yyval->ConstantFold();
    }
    break;

  case 81: /* equality_expression: relational_expression  */
    {
      yyval = yyvsp[0];
    }
    break;

  case 82: /* equality_expression: equality_expression SYMBOL_EQ relational_expression  */
    {
      yyval = CreateOperation(CTNOT_EQ, yyvsp[-2], yyvsp[0]);
    }
    break;

  case 83: /* equality_expression: equality_expression SYMBOL_NEQ relational_expression  */
    {
      yyval = CreateOperation(CTNOT_NEQ, yyvsp[-2], yyvsp[0]);
    }
    break;

  case 84: /* relational_expression: shift_expression  */
    {
      yyval = yyvsp[0];
    }
    break;

  case 85: /* relational_expression: relational_expression '<' shift_expression  */
    {
      yyval = CreateOperation(CTNOT_LT, yyvsp[-2], yyvsp[0]);
    }
    break;

  case 86: /* relational_expression: relational_expression '>' shift_expression  */
    {
      yyval = CreateOperation(CTNOT_GT, yyvsp[-2], yyvsp[0]);
    }
    break;

  case 87: /* relational_expression: relational_expression SYMBOL_LTE shift_expression  */
    {
      yyval = CreateOperation(CTNOT_LTE, yyvsp[-2], yyvsp[0]);
    }
    break;

  case 88: /* relational_expression: relational_expression SYMBOL_GTE shift_expression  */
    {
      yyval = CreateOperation(CTNOT_GTE, yyvsp[-2], yyvsp[0]);
    }
    break;

  case 89: /* shift_expression: additive_expression  */
    {
      yyval = yyvsp[0];
    }
    break;

  case 90: /* shift_expression: shift_expression SYMBOL_LEFT_SHIFT additive_expression  */
    {
      yyval = CreateOperation(CTNOT_SHIFT_LEFT, yyvsp[-2], yyvsp[0]);
      yyval->ConstantFold();
    }
    break;

  case 91: /* shift_expression: shift_expression SYMBOL_RIGHT_SHIFT additive_expression  */
    {
      yyval = CreateOperation(CTNOT_SHIFT_RIGHT, yyvsp[-2], yyvsp[0]);
      yyval->ConstantFold();
    }
    break;

  case 92: /* additive_expression: multiplicative_expression  */
    {
      yyval = yyvsp[0];
    }
    break;

  case 93: /* additive_expression: additive_expression '+' multiplicative_expression  */
    {
      yyval = CreateOperation(CTNOT_ADD, yyvsp[-2], yyvsp[0]);
      yyval->ConstantFold();
    }
    break;

  case 94: /* additive_expression: additive_expression '-' multiplicative_expression  */
    {
      yyval = CreateOperation(CTNOT_MINUS, yyvsp[-2], yyvsp[0]);
      yyval->ConstantFold();
    }
    break;

  case 95: /* multiplicative_expression: unary_expression  */
    {
      yyval = yyvsp[0];
    }
    break;

  case 96: /* multiplicative_expression: multiplicative_expression '*' unary_expression  */
    {
      yyval = CreateOperation(CTNOT_TIMES, yyvsp[-2], yyvsp[0]);
      yyval->ConstantFold();
    }
    break;

  case 97: /* multiplicative_expression: multiplicative_expression '/' unary_expression  */
    {
      yyval = CreateOperation(CTNOT_DIVIDE, yyvsp[-2], yyvsp[0]);
      yyval->ConstantFold();
    }
    break;

  case 98: /* multiplicative_expression: multiplicative_expression '%' unary_expression  */
    {
      yyval = CreateOperation(CTNOT_REM, yyvsp[-2], yyvsp[0]);
      yyval->ConstantFold();
    }
    break;

  case 99: /* unary_expression: postfix_expression  */
    {
      yyval = yyvsp[0];
    }
    break;

  case 100: /* unary_expression: SYMBOL_INC unary_expression  */
    {
      yyval = CreateOperation(CTNOT_PRE_INC, yyvsp[0]);
    }
    break;

  case 101: /* unary_expression: SYMBOL_DEC unary_expression  */
    {
      yyval = CreateOperation(CTNOT_PRE_DEC, yyvsp[0]);
    }
    break;

  case 102: /* unary_expression: unary_operator unary_expression  */
    {
      yyval = yyvsp[-1];
      yyval->SetChild(0, yyvsp[0]);
      yyval->ConstantFold();
    }
    break;

  case 103: /* unary_operator: '+'  */
    {
      yyval = CreateOperation(CTNOT_UNARY_PLUS);
    }
    break;

  case 104: /* unary_operator: '-'  */
    {
      yyval = CreateOperation(CTNOT_UNARY_MINUS);
    }
    break;

  case 105: /* unary_operator: '~'  */
    {
      yyval = CreateOperation(CTNOT_UNARY_COMPLEMENT);
    }
    break;

  case 106: /* unary_operator: '!'  */
    {
      yyval = CreateOperation(CTNOT_UNARY_NOT);
    }
    break;

  case 107: /* unary_operator: '?'  */
    {
      yyval = CreateOperation(CTNOT_UNARY_EXISTS);
    }
    break;

  case 108: /* postfix_expression: primary_expression  */
    {
      yyval = yyvsp[0];
    }
    break;

  case 109: /* postfix_expression: postfix_expression '[' constant_expression ']'  */
    {
      yyval = CreateOperation(CTNOT_ARRAY_INDEX, yyvsp[-3], yyvsp[-1]);
    }
    break;

  case 110: /* postfix_expression: postfix_expression '(' ')'  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_CALL, gmlineno);
      yyval->SetChild(0, yyvsp[-2]);
    }
    break;

  case 111: /* postfix_expression: postfix_expression '(' argument_expression_list ')'  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_CALL, gmlineno);
      yyval->SetChild(0, yyvsp[-3]);
      yyval->SetChild(1, yyvsp[-1]);
    }
    break;

  case 112: /* postfix_expression: postfix_expression ':' identifier '(' ')'  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_CALL, gmlineno);
      yyval->SetChild(0, yyvsp[-2]);
      yyval->SetChild(2, yyvsp[-4]);
    }
    break;

  case 113: /* postfix_expression: postfix_expression ':' identifier '(' argument_expression_list ')'  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_CALL, gmlineno);
      yyval->SetChild(0, yyvsp[-3]);
      yyval->SetChild(1, yyvsp[-1]);
      yyval->SetChild(2, yyvsp[-5]);
    }
    break;

  case 114: /* postfix_expression: postfix_expression '.' identifier  */
    {
      yyval = CreateOperation(CTNOT_DOT, yyvsp[-2], yyvsp[0]);
    }
    break;

  case 115: /* argument_expression_list: constant_expression  */
    {
      yyval = yyvsp[0];
    }
    break;

  case 116: /* argument_expression_list: argument_expression_list ',' constant_expression  */
    {
      ATTACH(yyval, yyvsp[-2], yyvsp[0]);
    }
    break;

  case 117: /* table_constructor: KEYWORD_TABLE '(' ')'  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_TABLE, gmlineno);
    }
    break;

  case 118: /* table_constructor: KEYWORD_TABLE '(' field_list ')'  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_TABLE, gmlineno);
      yyval->SetChild(0, yyvsp[-1]);
    }
    break;

  case 119: /* table_constructor: '{' '}'  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_TABLE, gmlineno);
    }
    break;

  case 120: /* table_constructor: '{' field_list '}'  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_TABLE, gmlineno);
      yyval->SetChild(0, yyvsp[-1]);
    }
    break;

  case 121: /* table_constructor: '{' field_list ',' '}'  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_TABLE, gmlineno);
      yyval->SetChild(0, yyvsp[-2]);
    }
    break;

  case 122: /* function_constructor: KEYWORD_FUNCTION '(' parameter_list ')' compound_statement  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_FUNCTION, gmlineno);
      yyval->SetChild(0, yyvsp[-2]);
      yyval->SetChild(1, yyvsp[0]);
    }
    break;

  case 123: /* function_constructor: KEYWORD_FUNCTION '(' ')' compound_statement  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_FUNCTION, gmlineno);
      yyval->SetChild(1, yyvsp[0]);
    }
    break;

  case 124: /* field_list: field  */
    {
      yyval = yyvsp[0];
    }
    break;

  case 125: /* field_list: field_list ',' field  */
    {
      ATTACH(yyval, yyvsp[-2], yyvsp[0]);
    }
    break;

  case 126: /* field: constant_expression  */
    {
      yyval = yyvsp[0];
    }
    break;

  case 127: /* field: identifier '=' constant_expression  */
    {
      yyval = CreateOperation(CTNOT_ASSIGN_FIELD, yyvsp[-2], yyvsp[0]);
    }
    break;

  case 128: /* parameter_list: parameter  */
    {
      yyval = yyvsp[0];
    }
    break;

  case 129: /* parameter_list: parameter_list ',' parameter  */
    {
      ATTACH(yyval, yyvsp[-2], yyvsp[0]);
    }
    break;

  case 130: /* parameter: identifier  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_DECLARATION, CTNDT_PARAMETER, gmlineno);
      yyval->SetChild(0, yyvsp[0]);
    }
    break;

  case 131: /* parameter: identifier '=' constant_expression  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_DECLARATION, CTNDT_PARAMETER, gmlineno);
      yyval->SetChild(0, yyvsp[-2]);
      yyval->SetChild(1, yyvsp[0]);
    }
    break;

  case 132: /* primary_expression: identifier  */
    {
      yyval = yyvsp[0];
    }
    break;

  case 133: /* primary_expression: '.' identifier  */
    {
      yyval = yyvsp[0];
      yyval->m_flags |= gmCodeTreeNode::CTN_MEMBER;
    }
    break;

  case 134: /* primary_expression: KEYWORD_THIS  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_THIS, gmlineno);
    }
    break;

  case 135: /* primary_expression: constant  */
    {
      yyval = yyvsp[0];
    }
    break;

  case 136: /* primary_expression: table_constructor  */
    {
      yyval = yyvsp[0];
    }
    break;

  case 137: /* primary_expression: function_constructor  */
    {
      yyval = yyvsp[0];
    }
    break;

  case 138: /* primary_expression: '(' constant_expression ')'  */
    {
      yyval = yyvsp[-1];
    }
    break;

  case 139: /* identifier: IDENTIFIER  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_IDENTIFIER, gmlineno);
      yyval->m_data.m_string = (char *) gmCodeTree::Get().Alloc((int)strlen(gmtext) + 1);
      strcpy(yyval->m_data.m_string, gmtext);
    }
    break;

  case 140: /* constant: CONSTANT_HEX  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_CONSTANT, gmlineno, CTNCT_INT);
      yyval->m_data.m_iValue = strtoul(gmtext + 2, NULL, 16);
    }
    break;

  case 141: /* constant: CONSTANT_BINARY  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_CONSTANT, gmlineno, CTNCT_INT);
      yyval->m_data.m_iValue = strtoul(gmtext + 2, NULL, 2);
    }
    break;

  case 142: /* constant: CONSTANT_INT  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_CONSTANT, gmlineno, CTNCT_INT);
      yyval->m_data.m_iValue = (gmint)strtoll(gmtext, NULL, 10);
    }
    break;

  case 143: /* constant: KEYWORD_TRUE  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_CONSTANT, gmlineno, CTNCT_INT);
      yyval->m_data.m_iValue = 1;
    }
    break;

  case 144: /* constant: KEYWORD_FALSE  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_CONSTANT, gmlineno, CTNCT_INT);
      yyval->m_data.m_iValue = 0;
    }
    break;

  case 145: /* constant: CONSTANT_CHAR  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_CONSTANT, gmlineno, CTNCT_INT);

      char * c = (char *) gmCodeTree::Get().Alloc((int)strlen(gmtext) + 1);
      strcpy(c, gmtext);
      int result = 0;
      int shr = 0;

      while(*c)
      {
        if(c[0] == '\'')
        {
          ++c;
          continue;
        }
        else if(c[0] == '\\')
        {
          if(shr) result <<= 8;
          switch(c[1])
          {
            case 'a' : result |= (unsigned char) '\a'; break;
            case 'b' : result |= (unsigned char) '\b'; break;
            case 'f' : result |= (unsigned char) '\f'; break;
            case 'n' : result |= (unsigned char) '\n'; break;
            case 'r' : result |= (unsigned char) '\r'; break;
            case 't' : result |= (unsigned char) '\t'; break;
            case 'v' : result |= (unsigned char) '\v'; break;
            case '\'' : result |= (unsigned char) '\''; break;
            case '\"' : result |= (unsigned char) '\"'; break;
            case '\\' : result |= (unsigned char) '\\'; break;
            default: result |= (unsigned char) c[1];
          }
          ++shr;
          c += 2;
          continue;
        }
        if(shr) result <<= 8;
        result |= (unsigned char) *(c++);
        ++shr;
      }

      if(shr > 4 && gmCodeTree::Get().GetLog()) gmCodeTree::Get().GetLog()->LogEntry("truncated char, line %d", gmlineno);

      yyval->m_data.m_iValue = result;
    }
    break;

  case 146: /* constant: CONSTANT_FLOAT  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_CONSTANT, gmlineno, CTNCT_FLOAT);
      yyval->m_data.m_fValue = (gmfloat)atof(gmtext);
    }
    break;

  case 147: /* constant: constant_string_list  */
    {
      yyval = yyvsp[0];
    }
    break;

  case 148: /* constant: KEYWORD_NULL  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_CONSTANT, gmlineno, CTNCT_NULL);
      yyval->m_data.m_iValue = 0;
    }
    break;

  case 149: /* constant_string_list: CONSTANT_STRING  */
    {
      yyval = gmCodeTreeNode::Create(CTNT_EXPRESSION, CTNET_CONSTANT, gmlineno, CTNCT_STRING);
      yyval->m_data.m_string = (char *) gmCodeTree::Get().Alloc((int)strlen(gmtext) + 1);
      strcpy(yyval->m_data.m_string, gmtext);
      if(gmtext[0] == '"')
      {
        gmProcessDoubleQuoteString(yyval->m_data.m_string);
      }
      else if(gmtext[0] == '`')
      {
        gmProcessSingleQuoteString(yyval->m_data.m_string);
      }
    }
    break;

  case 150: /* constant_string_list: constant_string_list CONSTANT_STRING  */
    {
      yyval = yyvsp[-1];
      int alen = (int)strlen(yyval->m_data.m_string);
      int blen = (int)strlen(gmtext);
      char * str = (char *) gmCodeTree::Get().Alloc(alen + blen + 1);
      if(str)
      {
        memcpy(str, yyvsp[-1]->m_data.m_string, alen);
        memcpy(str + alen, gmtext, blen);
        str[alen + blen] = '\0';
        if(str[alen] == '"')
        {
          gmProcessDoubleQuoteString(str + alen);
        }
        else if(str[alen] == '`')
        {
          gmProcessSingleQuoteString(str + alen);
        }
        yyval->m_data.m_string = str;
      }
    }
    break;



      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}



#include <stdio.h>











