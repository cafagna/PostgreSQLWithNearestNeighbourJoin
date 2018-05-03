/* A Bison parser, made by GNU Bison 2.5.  */

/* Bison interface for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2011 Free Software Foundation, Inc.
   
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


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     IDENT = 258,
     FCONST = 259,
     SCONST = 260,
     BCONST = 261,
     XCONST = 262,
     Op = 263,
     ICONST = 264,
     PARAM = 265,
     TYPECAST = 266,
     DOT_DOT = 267,
     COLON_EQUALS = 268,
     ABORT_P = 269,
     ABSOLUTE_P = 270,
     ACCESS = 271,
     ACTION = 272,
     ADD_P = 273,
     ADMIN = 274,
     AFTER = 275,
     AGGR = 276,
     AGGREGATE = 277,
     ALL = 278,
     ALSO = 279,
     ALTER = 280,
     ALWAYS = 281,
     ANALYSE = 282,
     ANALYZE = 283,
     AND = 284,
     ANY = 285,
     ARRAY = 286,
     AS = 287,
     ASC = 288,
     ASSERTION = 289,
     ASSIGNMENT = 290,
     ASYMMETRIC = 291,
     AT = 292,
     ATTRIBUTE = 293,
     AUTHORIZATION = 294,
     BACKTRACK = 295,
     BACKWARD = 296,
     BEFORE = 297,
     BEGIN_P = 298,
     BETWEEN = 299,
     BIGINT = 300,
     BINARY = 301,
     BIT = 302,
     BOOLEAN_P = 303,
     BOTH = 304,
     BY = 305,
     CACHE = 306,
     CALLED = 307,
     CASCADE = 308,
     CASCADED = 309,
     CASE = 310,
     CAST = 311,
     CATALOG_P = 312,
     CHAIN = 313,
     CHAR_P = 314,
     CHARACTER = 315,
     CHARACTERISTICS = 316,
     CHECK = 317,
     CHECKPOINT = 318,
     CLASS = 319,
     CLOSE = 320,
     CLUSTER = 321,
     COALESCE = 322,
     COLLATE = 323,
     COLLATION = 324,
     COLUMN = 325,
     COMMENT = 326,
     COMMENTS = 327,
     COMMIT = 328,
     COMMITTED = 329,
     CONCURRENTLY = 330,
     CONFIGURATION = 331,
     CONNECTION = 332,
     CONSTRAINT = 333,
     CONSTRAINTS = 334,
     CONTENT_P = 335,
     CONTINUE_P = 336,
     CONVERSION_P = 337,
     COPY = 338,
     COST = 339,
     CREATE = 340,
     CROSS = 341,
     CSV = 342,
     CURRENT_P = 343,
     CURRENT_CATALOG = 344,
     CURRENT_DATE = 345,
     CURRENT_ROLE = 346,
     CURRENT_SCHEMA = 347,
     CURRENT_TIME = 348,
     CURRENT_TIMESTAMP = 349,
     CURRENT_USER = 350,
     CURSOR = 351,
     CYCLE = 352,
     DATA_P = 353,
     DATABASE = 354,
     DAY_P = 355,
     DEALLOCATE = 356,
     DEC = 357,
     DECIMAL_P = 358,
     DECLARE = 359,
     DEFAULT = 360,
     DEFAULTS = 361,
     DEFERRABLE = 362,
     DEFERRED = 363,
     DEFINER = 364,
     DELETE_P = 365,
     DELIMITER = 366,
     DELIMITERS = 367,
     DESC = 368,
     DICTIONARY = 369,
     DISABLE_P = 370,
     DISCARD = 371,
     DISTINCT = 372,
     DO = 373,
     DOCUMENT_P = 374,
     DOMAIN_P = 375,
     DOUBLE_P = 376,
     DROP = 377,
     EACH = 378,
     ELSE = 379,
     ENABLE_P = 380,
     ENCODING = 381,
     ENCRYPTED = 382,
     END_P = 383,
     ENUM_P = 384,
     EQUAL = 385,
     ESCAPE = 386,
     EVENT = 387,
     EXCEPT = 388,
     EXCLUDE = 389,
     EXCLUDING = 390,
     EXCLUSIVE = 391,
     EXECUTE = 392,
     EXISTS = 393,
     EXPLAIN = 394,
     EXTENSION = 395,
     EXTERNAL = 396,
     EXTRACT = 397,
     FALSE_P = 398,
     FAMILY = 399,
     FETCH = 400,
     FIRST_P = 401,
     FLOAT_P = 402,
     FOLLOWING = 403,
     FOR = 404,
     FORCE = 405,
     FOREIGN = 406,
     FORWARD = 407,
     FREEZE = 408,
     FROM = 409,
     FULL = 410,
     FUNCTION = 411,
     FUNCTIONS = 412,
     GLOBAL = 413,
     GRANT = 414,
     GRANTED = 415,
     GREATEST = 416,
     GROUP_P = 417,
     HANDLER = 418,
     HAVING = 419,
     HEADER_P = 420,
     HOLD = 421,
     HOUR_P = 422,
     IDENTITY_P = 423,
     IF_P = 424,
     ILIKE = 425,
     IMMEDIATE = 426,
     IMMUTABLE = 427,
     IMPLICIT_P = 428,
     IN_P = 429,
     INCLUDING = 430,
     INCREMENT = 431,
     INDEX = 432,
     INDEXES = 433,
     INHERIT = 434,
     INHERITS = 435,
     INITIALLY = 436,
     INLINE_P = 437,
     INNER_P = 438,
     INOUT = 439,
     INPUT_P = 440,
     INSENSITIVE = 441,
     INSERT = 442,
     INSTEAD = 443,
     INT_P = 444,
     INTEGER = 445,
     INTERSECT = 446,
     INTERVAL = 447,
     INTO = 448,
     INVOKER = 449,
     IS = 450,
     ISNULL = 451,
     ISOLATION = 452,
     JOIN = 453,
     KEY = 454,
     LABEL = 455,
     LANGUAGE = 456,
     LARGE_P = 457,
     LAST_P = 458,
     LATERAL_P = 459,
     LC_COLLATE_P = 460,
     LC_CTYPE_P = 461,
     LEADING = 462,
     LEAKPROOF = 463,
     LEAST = 464,
     LEFT = 465,
     LEVEL = 466,
     LIKE = 467,
     LIMIT = 468,
     LISTEN = 469,
     LOAD = 470,
     LOCAL = 471,
     LOCALTIME = 472,
     LOCALTIMESTAMP = 473,
     LOCATION = 474,
     LOCK_P = 475,
     MAPPING = 476,
     MATCH = 477,
     MATERIALIZED = 478,
     MAXVALUE = 479,
     MINUTE_P = 480,
     MINVALUE = 481,
     MODE = 482,
     MONTH_P = 483,
     MOVE = 484,
     NAME_P = 485,
     NAMES = 486,
     NATIONAL = 487,
     NATURAL = 488,
     NCHAR = 489,
     NEXT = 490,
     NN = 491,
     NO = 492,
     NONE = 493,
     NOT = 494,
     NOTHING = 495,
     NOTIFY = 496,
     NOTNULL = 497,
     NOWAIT = 498,
     NULL_P = 499,
     NULLIF = 500,
     NULLS_P = 501,
     NUMERIC = 502,
     OBJECT_P = 503,
     OF = 504,
     OFF = 505,
     OFFSET = 506,
     OIDS = 507,
     ON = 508,
     ONLY = 509,
     OPERATOR = 510,
     OPTION = 511,
     OPTIONS = 512,
     OR = 513,
     ORDER = 514,
     OUT_P = 515,
     OUTER_P = 516,
     OVER = 517,
     OVERLAPS = 518,
     OVERLAY = 519,
     OWNED = 520,
     OWNER = 521,
     PARSER = 522,
     PARTIAL = 523,
     PARTITION = 524,
     PASSING = 525,
     PASSWORD = 526,
     PLACING = 527,
     PLANS = 528,
     POSITION = 529,
     PRECEDING = 530,
     PRECISION = 531,
     PRESERVE = 532,
     PREPARE = 533,
     PREPARED = 534,
     PRIMARY = 535,
     PRIOR = 536,
     PRIVILEGES = 537,
     PROCEDURAL = 538,
     PROCEDURE = 539,
     PROGRAM = 540,
     QUOTE = 541,
     RANGE = 542,
     READ = 543,
     REAL = 544,
     REASSIGN = 545,
     RECHECK = 546,
     RECURSIVE = 547,
     REF = 548,
     REFERENCES = 549,
     REFRESH = 550,
     REINDEX = 551,
     RELATIVE_P = 552,
     RELEASE = 553,
     RENAME = 554,
     REPEATABLE = 555,
     REPLACE = 556,
     REPLICA = 557,
     RESET = 558,
     RESTART = 559,
     RESTRICT = 560,
     RETURNING = 561,
     RETURNS = 562,
     REVOKE = 563,
     RIGHT = 564,
     RNNJ = 565,
     ROLE = 566,
     ROLLBACK = 567,
     ROW = 568,
     ROWS = 569,
     RULE = 570,
     SAVEPOINT = 571,
     SCHEMA = 572,
     SCROLL = 573,
     SEARCH = 574,
     SECOND_P = 575,
     SECURITY = 576,
     SELECT = 577,
     SEQUENCE = 578,
     SEQUENCES = 579,
     SERIALIZABLE = 580,
     SERVER = 581,
     SESSION = 582,
     SESSION_USER = 583,
     SET = 584,
     SETOF = 585,
     SHARE = 586,
     SHOW = 587,
     SIMILAR = 588,
     SIMPLE = 589,
     SMALLINT = 590,
     SNAPSHOT = 591,
     SOME = 592,
     STABLE = 593,
     STANDALONE_P = 594,
     START = 595,
     STATEMENT = 596,
     STATISTICS = 597,
     STDIN = 598,
     STDOUT = 599,
     STORAGE = 600,
     STRICT_P = 601,
     STRIP_P = 602,
     SUBSTRING = 603,
     SYMMETRIC = 604,
     SYSID = 605,
     SYSTEM_P = 606,
     TABLE = 607,
     TABLES = 608,
     TABLESPACE = 609,
     TEMP = 610,
     TEMPLATE = 611,
     TEMPORARY = 612,
     TEXT_P = 613,
     THEN = 614,
     TIME = 615,
     TIMESTAMP = 616,
     TO = 617,
     TRAILING = 618,
     TRANSACTION = 619,
     TREAT = 620,
     TRIGGER = 621,
     TRIM = 622,
     TRUE_P = 623,
     TRUNCATE = 624,
     TRUSTED = 625,
     TYPE_P = 626,
     TYPES_P = 627,
     UNBOUNDED = 628,
     UNCOMMITTED = 629,
     UNENCRYPTED = 630,
     UNION = 631,
     UNIQUE = 632,
     UNKNOWN = 633,
     UNLISTEN = 634,
     UNLOGGED = 635,
     UNTIL = 636,
     UPDATE = 637,
     USER = 638,
     USING = 639,
     VACUUM = 640,
     VALID = 641,
     VALIDATE = 642,
     VALIDATOR = 643,
     VALUE_P = 644,
     VALUES = 645,
     VARCHAR = 646,
     VARIADIC = 647,
     VARYING = 648,
     VERBOSE = 649,
     VERSION_P = 650,
     VIEW = 651,
     VOLATILE = 652,
     WHEN = 653,
     WHERE = 654,
     WHITESPACE_P = 655,
     WINDOW = 656,
     WITH = 657,
     WITHOUT = 658,
     WORK = 659,
     WRAPPER = 660,
     WRITE = 661,
     XML_P = 662,
     XMLATTRIBUTES = 663,
     XMLCONCAT = 664,
     XMLELEMENT = 665,
     XMLEXISTS = 666,
     XMLFOREST = 667,
     XMLPARSE = 668,
     XMLPI = 669,
     XMLROOT = 670,
     XMLSERIALIZE = 671,
     YEAR_P = 672,
     YES_P = 673,
     ZONE = 674,
     NULLS_FIRST = 675,
     NULLS_LAST = 676,
     WITH_TIME = 677,
     POSTFIXOP = 678,
     UMINUS = 679
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 2068 of yacc.c  */
#line 176 "gram.y"

	core_YYSTYPE		core_yystype;
	/* these fields must match core_YYSTYPE: */
	int					ival;
	char				*str;
	const char			*keyword;

	char				chr;
	bool				boolean;
	JoinType			jtype;
	DropBehavior		dbehavior;
	OnCommitAction		oncommit;
	List				*list;
	Node				*node;
	Value				*value;
	ObjectType			objtype;
	TypeName			*typnam;
	FunctionParameter   *fun_param;
	FunctionParameterMode fun_param_mode;
	FuncWithArgs		*funwithargs;
	DefElem				*defelt;
	SortBy				*sortby;
	WindowDef			*windef;
	JoinExpr			*jexpr;
	IndexElem			*ielem;
	Alias				*alias;
	RangeVar			*range;
	IntoClause			*into;
	WithClause			*with;
	A_Indices			*aind;
	ResTarget			*target;
	struct PrivTarget	*privtarget;
	AccessPriv			*accesspriv;
	InsertStmt			*istmt;
	VariableSetStmt		*vsetstmt;



/* Line 2068 of yacc.c  */
#line 513 "gram.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif



#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif



