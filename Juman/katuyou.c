/*
==============================================================================
	katuyou.c
		1990/12/17/Mon	Yutaka MYOKI(Nagao Lab., KUEE)
==============================================================================
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "juman.h"

/*
------------------------------------------------------------------------------
	GLOBAL:
	definition of global variables
------------------------------------------------------------------------------
*/

TYPE		Type[TYPE_NO];
FORM		Form[TYPE_NO][FORM_NO];

extern CLASS	Class[CLASSIFY_NO][CLASSIFY_NO];
extern int	LineNo;
extern int	LineNoForError;

/*
------------------------------------------------------------------------------
	PROCEDURE:
	<initialize_type_form>: initialize <TYPE:Type>, <FORM:Form>
------------------------------------------------------------------------------
*/

static void initialize_type_form(void) {
	int	i, j;

	for (i = 0; i < TYPE_NO; i++) {

		if (Type[i].name)
			free(Type[i].name);
		Type[i].name = (U_CHAR *)NULL;

		for (j = 0; j < FORM_NO; j++) {

			if (Form[i][j].name)
				free(Form[i][j].name);
			Form[i][j].name = (U_CHAR *)NULL;

			if (Form[i][j].gobi)
				free(Form[i][j].gobi);
			Form[i][j].gobi = (U_CHAR *)NULL;

			if (Form[i][j].gobi_yomi)
				free(Form[i][j].gobi_yomi);
			Form[i][j].gobi_yomi = (U_CHAR *)NULL;
		}
	}
}

/*
------------------------------------------------------------------------------
	PROCEDURE:
	<print_type_form>: print <TYPE:Type>, <FORM:Form> on <fp>
------------------------------------------------------------------------------
*/

void print_type_form(FILE *fp)
{
     int	i, j;

     for (i = 1; (Type[i].name != NULL) && i < TYPE_NO; i++) {
	  fprintf(fp, "%s\n", Type[i].name);
	  for (j = 1; (Form[i][j].name != NULL) && j < FORM_NO; j++)
	       fprintf(fp, "\t%-30s %-20s\n",
		       Form[i][j].name, Form[i][j].gobi);
	  fputc('\n', fp);
     }
}

/*
------------------------------------------------------------------------------
	PROCEDURE:
	<read_type_form>: read-in <TYPE:Type>, <FORM:Form> from <fp>
------------------------------------------------------------------------------
*/

void read_type_form(FILE *fp)
{
    CELL	*cell1, *cell2;
    int	i, j;

    LineNo = 1;
    i = 1;
    while (! s_feof(fp)) {
	LineNoForError = LineNo;
	cell1 = s_read(fp);
	Type[i].name = (U_CHAR*)my_alloc((int)((sizeof(U_CHAR)*strlen(_Atom(car(cell1)))) + 1));
	strcpy(Type[i].name, _Atom(car(cell1)));
	cell1 = car(cdr(cell1));
	j = 1;
	while (!Null(cell2 = car(cell1))) {
	    Form[i][j].name = (U_CHAR*)my_alloc((int)((sizeof(U_CHAR)*strlen(_Atom(car(cell2)))) + 1));
	    strcpy(Form[i][j].name, _Atom(car(cell2)));

	    Form[i][j].gobi = (U_CHAR*)my_alloc((int)((sizeof(U_CHAR)*strlen(_Atom(car(cdr(cell2))))) + 1));
	    if (strcmp(_Atom(car(cdr(cell2))), "*") == 0)
			strcpy(Form[i][j].gobi, "");
	    else
			strcpy(Form[i][j].gobi, _Atom(car(cdr(cell2))));

	    if (!Null(car(cdr(cdr(cell2))))) {
			/* ??????????????????????????????????????????????????? */
			Form[i][j].gobi_yomi = (U_CHAR*)my_alloc((int)((sizeof(U_CHAR)*strlen(_Atom(car(cdr(cdr(cell2)))))) + 1));
			if (strcmp(_Atom(car(cdr(cdr(cell2)))), "*") == 0)
				strcpy(Form[i][j].gobi_yomi, "");
			else
				strcpy(Form[i][j].gobi_yomi, _Atom(car(cdr(cdr(cell2)))));
	    } else {
			Form[i][j].gobi_yomi = (U_CHAR*)my_alloc((int)(sizeof(U_CHAR)*strlen(Form[i][j].gobi) + 1));
			strcpy(Form[i][j].gobi_yomi, Form[i][j].gobi);
	    }

	    j++;
	    cell1 = cdr(cell1);
	}
	i++;
    }
}

/*
------------------------------------------------------------------------------
	PROCEDURE:
	<katuyou>: call <initialize_type_form> and <read_type_form>

	juman_path??????????????????????????????????????????????????????????????? (2002/11/08)
------------------------------------------------------------------------------
*/

void katuyou ( FILE *fp_out ) {
	FILE	*fp;
	char	cur_path [FILENAME_MAX] ;
	char	juman_path [FILENAME_MAX] ;
	char	katuyoufile_path [FILENAME_MAX] ;

	getpath ( cur_path, juman_path ) ;

	while ( 1 ) {
		if ( ( fp = pathfopen ( KATUYOUFILE, "r", "", katuyoufile_path ) ) != NULL ) 
			break ;
		if ( ( fp = pathfopen ( KATUYOUFILE, "r", cur_path, katuyoufile_path ) ) != NULL ) 
			break ;
		if ( ( fp = pathfopen ( KATUYOUFILE, "r", juman_path, katuyoufile_path ) ) != NULL ) 
			break ;
		error ( (int*(*)(void))OpenError, "can't open", katuyoufile_path, ".", EOA ) ;
	}

	if ( fp_out != NULL ) {
		print_current_time ( fp_out ) ;
		fprintf ( fp_out, "%s parsing... ", katuyoufile_path ) ;
	}

	initialize_type_form ( ) ; 
	read_type_form ( fp ) ;

	if ( fp_out != NULL )
		fputs ( "done.\n\n", fp_out ) ;

	fclose ( fp ) ;
}
