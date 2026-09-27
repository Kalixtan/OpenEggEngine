/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_GM_GMPARSER_CPP_H_INCLUDED
# define YY_GM_GMPARSER_CPP_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int gmdebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    KEYWORD_LOCAL = 258,           /* KEYWORD_LOCAL  */
    KEYWORD_GLOBAL = 259,          /* KEYWORD_GLOBAL  */
    KEYWORD_MEMBER = 260,          /* KEYWORD_MEMBER  */
    KEYWORD_AND = 261,             /* KEYWORD_AND  */
    KEYWORD_OR = 262,              /* KEYWORD_OR  */
    KEYWORD_IF = 263,              /* KEYWORD_IF  */
    KEYWORD_ELSE = 264,            /* KEYWORD_ELSE  */
    KEYWORD_WHILE = 265,           /* KEYWORD_WHILE  */
    KEYWORD_FOR = 266,             /* KEYWORD_FOR  */
    KEYWORD_FOREACH = 267,         /* KEYWORD_FOREACH  */
    KEYWORD_IN = 268,              /* KEYWORD_IN  */
    KEYWORD_BREAK = 269,           /* KEYWORD_BREAK  */
    KEYWORD_CONTINUE = 270,        /* KEYWORD_CONTINUE  */
    KEYWORD_NULL = 271,            /* KEYWORD_NULL  */
    KEYWORD_DOWHILE = 272,         /* KEYWORD_DOWHILE  */
    KEYWORD_RETURN = 273,          /* KEYWORD_RETURN  */
    KEYWORD_FUNCTION = 274,        /* KEYWORD_FUNCTION  */
    KEYWORD_TABLE = 275,           /* KEYWORD_TABLE  */
    KEYWORD_THIS = 276,            /* KEYWORD_THIS  */
    KEYWORD_TRUE = 277,            /* KEYWORD_TRUE  */
    KEYWORD_FALSE = 278,           /* KEYWORD_FALSE  */
    KEYWORD_FORK = 279,            /* KEYWORD_FORK  */
    KEYWORD_SWITCH = 280,          /* KEYWORD_SWITCH  */
    KEYWORD_CASE = 281,            /* KEYWORD_CASE  */
    KEYWORD_DEFAULT = 282,         /* KEYWORD_DEFAULT  */
    IDENTIFIER = 283,              /* IDENTIFIER  */
    CONSTANT_HEX = 284,            /* CONSTANT_HEX  */
    CONSTANT_BINARY = 285,         /* CONSTANT_BINARY  */
    CONSTANT_INT = 286,            /* CONSTANT_INT  */
    CONSTANT_CHAR = 287,           /* CONSTANT_CHAR  */
    CONSTANT_FLOAT = 288,          /* CONSTANT_FLOAT  */
    CONSTANT_STRING = 289,         /* CONSTANT_STRING  */
    SYMBOL_ASGN_BSR = 290,         /* SYMBOL_ASGN_BSR  */
    SYMBOL_ASGN_BSL = 291,         /* SYMBOL_ASGN_BSL  */
    SYMBOL_ASGN_ADD = 292,         /* SYMBOL_ASGN_ADD  */
    SYMBOL_ASGN_MINUS = 293,       /* SYMBOL_ASGN_MINUS  */
    SYMBOL_ASGN_TIMES = 294,       /* SYMBOL_ASGN_TIMES  */
    SYMBOL_ASGN_DIVIDE = 295,      /* SYMBOL_ASGN_DIVIDE  */
    SYMBOL_ASGN_REM = 296,         /* SYMBOL_ASGN_REM  */
    SYMBOL_ASGN_BAND = 297,        /* SYMBOL_ASGN_BAND  */
    SYMBOL_ASGN_BOR = 298,         /* SYMBOL_ASGN_BOR  */
    SYMBOL_ASGN_BXOR = 299,        /* SYMBOL_ASGN_BXOR  */
    SYMBOL_RIGHT_SHIFT = 300,      /* SYMBOL_RIGHT_SHIFT  */
    SYMBOL_LEFT_SHIFT = 301,       /* SYMBOL_LEFT_SHIFT  */
    SYMBOL_INC = 302,              /* SYMBOL_INC  */
    SYMBOL_DEC = 303,              /* SYMBOL_DEC  */
    SYMBOL_LTE = 304,              /* SYMBOL_LTE  */
    SYMBOL_GTE = 305,              /* SYMBOL_GTE  */
    SYMBOL_EQ = 306,               /* SYMBOL_EQ  */
    SYMBOL_NEQ = 307,              /* SYMBOL_NEQ  */
    TOKEN_ERROR = 308              /* TOKEN_ERROR  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE gmlval;


int gmparse (void);


#endif /* !YY_GM_GMPARSER_CPP_H_INCLUDED  */
