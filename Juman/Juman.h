// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the JUMAN_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// JUMAN_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef JUMAN_EXPORTS
#define JUMAN_API __declspec(dllexport)
#else
#define JUMAN_API __declspec(dllimport)
#endif

//
// *** NOTE ***  This document MUST BE SAVED IN UTF-8 format, or JUMAN will not work.
//  Incidentally, that is generally true of all the parts of the JUMAN library.
//

#ifdef __cplusplus

class JUMAN_API _Juman {

public:

	_Juman ( ) ;
	~_Juman ( ) ;

	enum _PartOfSpeechCode {
		POSC_SPECIAL = 1,
		POSC_VERB,
		POSC_ADJECTIVE,
		POSC_FINALPARTICLE,
		POSC_AUXILIARY,
		POSC_NOUN,
		POSC_DEMONSTRATIVE,
		POSC_ADVERB,
		POSC_PARTICLE,
		POSC_CONJUNCTION,
		POSC_ADNOMIAL_ADJECTIVE,
		POSC_INTERJECTION,
		POSC_PREFIX,
		POSC_SUFFIX,
		POSC_UNDEFINED,
		POSC_MAX
	} ;

	bool Analyze ( wchar_t *pBuffer, unsigned cbBuffer ) ;
	bool Analyze ( wchar_t *szText ) ;
	unsigned GetResultCount ( ) ;
	bool GetResultString ( unsigned i, wchar_t *pBuffer, unsigned cbBuffer ) ;
	bool isAlternate ( unsigned i, wchar_t *pBuffer, unsigned cbBuffer ) ;
	wchar_t *GetSymbol ( unsigned i, wchar_t *pBuffer, unsigned cbBuffer ) ;
	wchar_t *GetSound ( unsigned i, wchar_t *pBuffer, unsigned cbBuffer ) ;
	_PartOfSpeechCode GetPartOfSpeech ( unsigned i, wchar_t *pBuffer, unsigned cbBuffer ) ;
	static wchar_t *GetPartOfSpeechName ( _PartOfSpeechCode posc ) ;

private:
	char **Result ;
	static wchar_t *szPartofSpeechCodes [POSC_MAX] ;

} ;

#endif

/*
==============================================================================
	juman.h
		1990/12/06/Thu	Yutaka MYOKI(Nagao Lab., KUEE)
		1990/01/09/Wed	Last Modified
                                          >>> 94/02 changed by T.Nakamura <<<
==============================================================================
*/

/*
------------------------------------------------------------------------------
	inclusion of header files
------------------------------------------------------------------------------
*/

#include	<stdio.h>
#include	<ctype.h>

#ifdef HAVE_STRING_H
#include	<string.h>
#endif

#include	<stdarg.h>

#ifdef HAVE_LIMITS_H
#include	<limits.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include	<sys/types.h>
#endif

#ifdef HAVE_SYS_FILE_H
#include	<sys/file.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include	<sys/stat.h>
#endif

#include	<time.h>

/* added by T.Nakamura */
#ifdef HAVE_STDLIB_H
#include        <stdlib.h>
#endif

#ifdef HAVE_FCNTL_H
#include 	<fcntl.h>
#endif

#include	"juman_pat.h"

/*
 * MS Windows ???????????? SJIS??????????????????????????????
 * Added by Taku Kudoh (taku@pine.kuee.kyoto-u.ac.jp)
 * Thu Oct 29 03:42:45 JST 1998
 */
#ifdef _WIN32

#include        <stdarg.h>

#ifdef HAVE_WINDOWS_H
#include <windows.h>

#if ! defined __CYGWIN__
typedef char *	caddr_t;
#endif
#endif

/* ????????? ??????????????????????????? undef (tricky?) */
#undef          TRUE
#undef          FALSE

#endif	//_WIN32

#define EOf		0x0b	/* ??????????????????????????????????????????EOF */

#define NDBM_KEY_MAX 	256
#define NDBM_CON_MAX 	1024

/*
------------------------------------------------------------------------------
	global definition of macros
------------------------------------------------------------------------------
*/

#define 	FALSE		((int)(0))
#define 	TRUE		(!(FALSE))
#define		BOOL		int

#define		U_CHAR		unsigned char
#define		U_INT		unsigned int

/* #define		SEEK_SET	0		* use in "fseek()"         */
/* #define		SEEK_CUR	1		* use in "fseek()"         */
/* #define		SEEK_END	2		* use in "fseek()"         */

#define		EOA		((char *)(-1))	/* end of arguments         */

#ifndef 	FILENAME_MAX			/* maximum of filename      */
#define		FILENAME_MAX	1024
#endif
#ifndef		BUFSIZE				/* (char) size of buffer    */
#define 	BUFSIZE		1025
#endif

#define 	NAME_MAX_	1024		/* maximum of various names */
#define 	MIDASI_MAX	129		/* maximum length of MIDASI */
#define 	YOMI_MAX	129		/* maximum length of YOMI   */
#define 	IMI_MAX		1024		/* maximum length of IMI    */


#define		ENV_JUMANPATH	"JUMANPATH"

#define		GRAMMARFILE	"JUMAN.grammar"
#define 	CLASSIFY_NO	128

#define		KATUYOUFILE	"JUMAN.katuyou"
#define		TYPE_NO		128
#define		FORM_NO		128

#define		CONNECTFILE	"JUMAN.connect"
#define		KANKEIFILE	"JUMAN.kankei"

/* added by kuro */
#define		OPTIONFILE	"JUMAN.option"


#define 	S_POSTFIX	".dic"
				/* file postfix for s-expression dic-file */
#define 	I_POSTFIX	".int"
				/* file postfix for intermediate dic-file */
#define		I_FILE_ID	"this is INT-file for makehd.\n"

#define 	CONS		0
#define 	ATOM		1
#define 	NIL		((CELL *)(NULL))

#define 	COMMENTCHAR	';'
#define 	BPARENTHESIS	'('
#define 	BPARENTHESIS2	'<'
#define 	BPARENTHESIS3	'['
#define 	EPARENTHESIS	')'
#define 	EPARENTHESIS2	'>'
#define 	EPARENTHESIS3	']'
#define 	SCANATOM	"%[^(;) \n\t]"
#define 	NILSYMBOL	"NIL"

#define		CELLALLOCSTEP	1024
#define		BLOCKSIZE	16384

#define		Consp(x)	(!Null(x) && (_Tag(x) == CONS))
#define		Atomp(x)	(!Null(x) && (_Tag(x) == ATOM))
#define		_Tag(cell)	(((CELL *)(cell))->tag)
#define		_Car(cell)	(((CELL *)(cell))->value.cons.car)
#define		_Cdr(cell)	(((CELL *)(cell))->value.cons.cdr)
#define		Null(cell)	((cell) == NIL)
#define		new_cell()	(cons(NIL, NIL))
#define		Eq(x, y)	(x == y)
#define		_Atom(cell)	(((CELL *)(cell))->value.atom)


#define 	endchar(str)	(str[strlen(str)-1])

#define		M		30		/* order of B-tree         */
#define		MM		(M*2) 		/* order*2	           */

#define		PTRNIL		(-1L)		/* instead of NULL pointer */

#define		KANJI_CODE	128*128

#define 	BASIC_FORM 	"?????????"

#define		TABLEFILE	"jumandic.tab"
#define		MATRIXFILE	"jumandic.mat"
#define		DICFILE		"jumandic.dat"
#define		PATFILE		"jumandic.pat"

#define		calc_index(x)	((((int)(*(x)&0x7f))<<7)|((int)(*((x)+1))&0x7f))

enum		_ExitCode 	{ NormalExit,
				       SystemError, OpenError, AllocateError,
				       GramError, DicError, ConnError,
				       ConfigError, ProgramError,
				       SyntaxError, UnknownId, OtherError };

/* added by T.Utsuro for weight of rensetu matrix */
#define         DEFAULT_C_WEIGHT  10

/* added by S.Kurohashi for mrph weight default values */
#define		MRPH_DEFAULT_WEIGHT	10

/* .jumanrc default values */

#define         CLASS_COST_DEFAULT   10
#define         RENSETSU_WEIGHT_DEFAULT  100
#define         KEITAISO_WEIGHT_DEFAULT  1
#define         COST_WIDTH_DEFAULT      20
#define         UNDEF_WORD_DEFAULT  10000

#define         RENGO_ID   "999"    /* yamaji */
#define         RENGO_DEFAULT_WEIGHT 0.5

/* for juman_lib.c */
#define		LENMAX			50000
#define         BUFFER_BLOCK_SIZE       1000

#define         Op_B		     	0
#define         Op_M		     	1
#define         Op_P		     	2
#define         Op_BB		     	3
#define         Op_PP		     	4
#define         Op_F		     	0
#define         Op_E		     	1
#define         Op_C		     	2
#define         Op_EE		     	3
#define         Op_E2		     	4
#define         Op_NODEBUG	     	0
#define         Op_DEBUG	     	1
#define         Op_DEBUG2	     	2

#define		MAX_PATHES		500
#define		MAX_PATHES_WK		5000

#define		CONNECT_MATRIX_MAX	1000

enum CharacterCodeBlock 	{ 
	BLOCK_UNKNOWN = -1, 
	BLOCK_NULL = 0,
	HIRAGANA_BLOCK,
	KATAKANA_BLOCK,
	KANJI_BLOCK,
	FULLWIDTH_NUMERAL_BLOCK,
	FULLWIDTH_LATIN_BLOCK,
	JAPANESE_HANKAKU_BLOCK
};
/*
------------------------------------------------------------------------------
	global type definition of structures
------------------------------------------------------------------------------
*/

/* <car> ?????? <cdr> ????????????????????????????????????????????? */
typedef		struct		_BIN {
     void		*car;			/* address of <car> */
     void		*cdr;			/* address of <cdr> */
} BIN;

/* <BIN> ????????? ????????? ?????????????????????????????? */
typedef		struct		_CELL {
     int		tag;			/* tag of <cell> */
                                                /*   0: cons     */
                                                /*   1: atom     */
     union {
	  BIN		cons;
	  U_CHAR	*atom;
     } value;
} CELL;

/* "malloc" ??????????????????????????????????????????????????????????????????????????????????????? */
typedef		struct		_CELLTABLE {
     void		*pre;
     void		*next;
     int		max;
     int		n;
     CELL		*cell;
} CELLTABLE;

/* changed by T.Nakamura and S.Kurohashi
	????????? MRPH ?????????????????????????????????
	????????? MORPHEME ?????????????????? */
typedef         struct          _MRPH {
     U_CHAR             midasi[MIDASI_MAX];
     U_CHAR             yomi[YOMI_MAX];
     U_CHAR             imis[IMI_MAX];
     CELL		*imi;

     char               hinsi;
     char               bunrui;
     char               katuyou1;
     char               katuyou2;

     U_CHAR             weight;
     int                con_tbl;
     int                length;
} MRPH;

/* ????????????????????????????????? */
typedef		struct		_CLASS {
     U_CHAR	*id;
     int        cost;     /*??????????????? by k.n*/
     int	kt;
} CLASS;

/* ????????? */
typedef		struct		_TYPE {
     U_CHAR	*name;
} TYPE;

/* ????????? */
typedef		struct		_FORM {
     U_CHAR	*name;
     U_CHAR	*gobi;
     U_CHAR	*gobi_yomi;	/* ??????????????? ???????????????????????? */
} FORM;

/* ??????????????????????????? */
typedef		struct		_DICOPT {
     int	toroku;
} DICOPT;

/* stat() ?????????????????????????????? */
typedef		struct stat	 STAT;

/* ????????? */
typedef         struct          _RENSETU_PAIR {
     int   i_pos;
     int   j_pos;
     int   hinsi;
     int   bunrui;
     int   type;
     int   form;
     U_CHAR  *goi;
} RENSETU_PAIR;

typedef struct _process_buffer {
    int mrph_p;
    int start;
    int end;
    int score;
    int path[MAX_PATHES];     /* ?????? PROCESS_BUFFER ????????? */
    int connect;	      /* FALSE ??????????????????(?????????????????????????????????) */
} PROCESS_BUFFER;

typedef struct _chk_connect_wk {
  int pre_p;     /* PROCESS_BUFFER ????????????????????? */
  int score;     /* ???????????????????????? */
} CHK_CONNECT_WK;

typedef struct _connect_cost {
    short p_no;     /* PROCESS_BUFFER ????????????????????? */
    short pos;
    int cost;     /* ????????? */
} CONNECT_COST;

/*
------------------------------------------------------------------------------
 additional type definition  written by K. Yanagi  >>>changed by T.Nakamura<<<
------------------------------------------------------------------------------
*/

typedef struct _DIC_FILES {
  int number;
  int now;
  FILE *dic[MAX_DIC_NUMBER];
  pat_node tree_top[MAX_DIC_NUMBER];
} DIC_FILES;

typedef struct _cost_omomi  {
    int rensetsu;
    int keitaiso;
    int cost_haba;
  } COST_OMOMI;            /*k.n*/
/*
------------------------------------------------------------------------------
	prototype definition of functions
------------------------------------------------------------------------------
*/

/* iotool.c */
FILE	*my_fopen(char *filename, char *mode);
FILE	*pathfopen(char *filename, char *mode,
		   char *path, char *filename_path);
FILE	*my_pathfopen(char *filename, char *mode,
		      char *path, char *filename_path);
int	my_feof(FILE *fp);
void	append_postfix(char *filename, char *affix);
void	change_postfix(char *filename, char *affix1, char *affix2);
void	getpath(char *cur_path, char *juman_path);
void	*my_alloc(int n);
void	*my_realloc(void *ptr, int n);
void	my_exit(int exit_code);
void	error(int errno, char *msg, ...);
char	lower(char c);
char	upper(char c);
int 	my_strlen(U_CHAR *s);
void	my_strcpy(U_CHAR *s1, U_CHAR *s2);
int	my_strcmp(U_CHAR *s1, U_CHAR *s2);
int 	compare_top_str(U_CHAR *s1, U_CHAR *s2);
int 	compare_top_str1(U_CHAR *s1, U_CHAR *s2);
int 	compare_top_str2(U_CHAR *s1, U_CHAR *s2);
int 	compare_end_str(U_CHAR *s1, U_CHAR *s2);

void	ls(FILE *fp, char *p, char *f);
void	print_current_time(FILE *fp);
void	print_execute_time(FILE *fp, int dt, float dp);

/* lisp.c */
int	s_feof(FILE *fp);
int	s_feof_comment(FILE *fp);
CELL	*make_cell(void);
/* CELLTABLE	*make_cell_table(CELLTABLE *pre, int size);*/
CELL	*tmp_atom(U_CHAR *atom);
CELL	*cons(void *car, void *cdr);
CELL	*car(CELL *cell);
CELL	*cdr(CELL *cell);
int	equal(void *x, void *y);
int	length(CELL *list);
int	ifnextchar(FILE *fp, int c);
int	comment(FILE *fp);
CELL	*s_read(FILE *fp);
CELL	*s_read_atom(FILE *fp);
CELL	*s_read_car(FILE *fp);
CELL	*s_read_cdr(FILE *fp);
CELL	*assoc(CELL *item, CELL *alist);
CELL	*s_print(FILE *fp, CELL *cell);
CELL	*_s_print_(FILE *fp, CELL *cell);
CELL	*_s_print_cdr(FILE *fp, CELL *cell);
void	*lisp_alloc(int n);
void 	lisp_alloc_push(void);
void 	lisp_alloc_pop(void);

/* grammar.c */
void	error_in_grammar(int n, int line_no);
void	initialize_class(void);
#define	print_class(fp) 	print_class_(fp, 0, 8, "*")
void 	print_class_(FILE *fp, int tab1, int tab2, char *flag);
void	read_class(FILE *fp);
void	grammar(FILE *fp_out);

/* katuyou.c */
static void	initialize_type_form(void);
void 	print_type_form(FILE *fp);
void	read_type_form(FILE *fp);
void	katuyou(FILE *fp);

/* connect.c */
void connect_table(FILE *fp_out);
void read_table(FILE *fp);
void check_edrtable(MRPH *mrph_p, CELL *x);
void check_table(MRPH *morpheme);
void check_table_for_rengo(MRPH *mrph_p);
int check_table_for_undef(int hinsi, int bunrui);
void connect_matrix(FILE *fp_out);
void read_matrix(FILE *fp);
int check_matrix(int postcon, int precon);
int check_matrix_left(int precon);
int check_matrix_right(int postcon);

/* for edr-dic */
void check_edrtable(MRPH *mrph_p, CELL *x);
