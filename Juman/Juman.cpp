// Juman.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "Juman.h"
#include <stdlib.h>

extern "C" {
	void set_jumanrc_fileptr ( char *option_rcfile, int look_rcdefault_p, int exit_rc_notfound_p ) ;
	FILE *Jumanrc_Fileptr ;
	BOOL juman_init_rc ( FILE *fp ) ;
	void juman_init_etc ( ) ;
	int	juman_sent ( ) ;
	int	juman_close ( ) ;
	BYTE String [LENMAX] ;
	int Show_Opt1 ;
	int Show_Opt2 ;
	char **print_best_path ( FILE* output ) ;
	char **print_all_mrph ( FILE* output ) ;
	char **print_all_path ( FILE* output ) ;
	char **print_homograph_path ( FILE* output ) ;
} ;

_Juman::_Juman ( ) : Result(0) {
	Show_Opt1 = Op_BB ;
	Show_Opt2 = Op_E2 ;
	set_jumanrc_fileptr ( NULL, TRUE, TRUE ) ;
	if ( !juman_init_rc ( Jumanrc_Fileptr ) )
		throw "ERROR: Unable to initialize JUMAN." ;
	juman_init_etc ( ) ;
} /* endmethod */

_Juman::~_Juman ( ) {
	juman_close ( ) ;
} /* endmethod */

bool _Juman::Analyze ( wchar_t *pBuffer, unsigned cbBuffer ) {
	// Encode the text in UTF-8 format and pass it to JUMAN.
	int cbString = WideCharToMultiByte ( CP_UTF8, 0, pBuffer, cbBuffer, (char*)String, _countof(String), 0, 0 ) ;
	String[cbString] = 0 ;
	// Analyze it.
	Result = 0 ;
	if ( juman_sent ( ) != (int) TRUE ) 
		return ( false ) ;
    switch ( Show_Opt1 ) {
	    case Op_B:
			Result = print_best_path ( NULL ) ;
			break ;
		case Op_M:
			Result = print_all_mrph ( NULL ) ; 
			break ;
		case Op_P:
			Result = print_all_path ( NULL ) ; 
			break ;
		case Op_BB:
		case Op_PP:
			Result = print_homograph_path ( NULL ) ; 
			break ;
    } /* endswitch */
	return ( Result != 0 ) ;
} /* endmethod */

bool _Juman::Analyze ( wchar_t *szText ) {
	// Encode the text in UTF-8 format and pass it to JUMAN.
	WideCharToMultiByte ( CP_UTF8, 0, szText, -1, (char*)String, _countof(String), 0, 0 ) ;
	// Analyze it.
	Result = 0 ;
	if ( juman_sent ( ) != (int) TRUE ) 
		return ( false ) ;
    switch ( Show_Opt1 ) {
	    case Op_B:
			Result = print_best_path ( NULL ) ;
			break ;
		case Op_M:
			Result = print_all_mrph ( NULL ) ; 
			break ;
		case Op_P:
			Result = print_all_path ( NULL ) ; 
			break ;
		case Op_BB:
		case Op_PP:
			Result = print_homograph_path ( NULL ) ; 
			break ;
    } /* endswitch */
	return ( Result != 0 ) ;
} /* endmethod */

unsigned _Juman::GetResultCount ( ) {
	unsigned Count = 0 ;
	char **pResult = Result ;
	while ( *pResult ) {
		Count ++ ;
		pResult ++ ;
	} /* endwhile */
	return ( Count ) ;
} /* endmethod */

bool _Juman::GetResultString ( unsigned i, wchar_t *pBuffer, unsigned cbBuffer ) {
	char **pResult = Result ;
	while ( i && *pResult ) {
		++ pResult ;
		-- i ;
	} /* endwhile */
	if ( *pResult ) {
		MultiByteToWideChar ( CP_UTF8, 0, *pResult, -1, pBuffer, cbBuffer ) ;
		return ( true ) ;
	} /* endif */
	return ( false ) ;
} /* endmethod */

bool _Juman::isAlternate ( unsigned i, wchar_t *pBuffer, unsigned cbBuffer ) {
	if ( !GetResultString ( i, pBuffer, cbBuffer ) ) 
		return ( false ) ;
	return ( pBuffer[0] == L'@' ) ;
} /* endmethod */

wchar_t* _Juman::GetSymbol ( unsigned i, wchar_t *pBuffer, unsigned cbBuffer ) {
	if ( !GetResultString ( i, pBuffer, cbBuffer ) ) 
		return ( 0 ) ;
	if ( ( pBuffer[0] == L'@' ) && ( pBuffer[1] == L' ' ) ) 
		pBuffer += 2 ;
	wchar_t *p = wcschr ( pBuffer, L' ' ) ;
	if ( p )
		*p = 0 ;
	return ( pBuffer ) ;
} /* endmethod */

wchar_t* _Juman::GetSound ( unsigned i, wchar_t *pBuffer, unsigned cbBuffer ) {
	wchar_t *pSymbol = GetSymbol ( i, pBuffer, cbBuffer ) ;
	wchar_t *pSound = pSymbol + wcslen(pSymbol) + 1 ;
	wchar_t *p = wcschr ( pSound, L' ' ) ;
	if ( p )
		*p = 0 ;
	return ( pSound ) ;
} /* endmethod */

_Juman::_PartOfSpeechCode _Juman::GetPartOfSpeech ( unsigned i, wchar_t *pBuffer, unsigned cbBuffer ) {
	if ( !GetResultString ( i, pBuffer, cbBuffer ) ) 
		return ( POSC_MAX ) ;
	if ( pBuffer[0] == 0x005C ) { // Workaround for JUMAN bug.
		pBuffer[1] = 0 ;
		pBuffer[2] = 0x005C ;
		pBuffer[3] = 0 ;
		return ( POSC_SPECIAL ) ;
	} /* endif */
	wchar_t *pSound = GetSound ( i, pBuffer, cbBuffer ) ;
	wchar_t *p = pSound + wcslen(pSound) + 1 ;
	p = wcschr ( p+1, L' ' ) ;
	p = wcschr ( p+1, L' ' ) ;
	return ( (_PartOfSpeechCode) _wtoi ( p ) ) ;
} /* endmethod */

wchar_t* _Juman::GetPartOfSpeechName ( _PartOfSpeechCode posc ) {
	return ( szPartofSpeechCodes[posc-1] ) ;
} /* endmethod */

wchar_t *_Juman::szPartofSpeechCodes [] = {
	L"Special", L"Verb", L"Adjective", L"Final Particle", L"Auxiliary", L"Noun",
	L"Demonstrative", L"Adverb", L"Particle", L"Conjunction", L"Prenomial Adjective", 
	L"Interjection", L"Prefix", L"Suffix", L"Undefined",
	L"??"
} ;

#if 0

int _tmain ( int argc, wchar_t* argv[] ) {

	// Create output file.
	FILE *Destin ; errno_t Error = fopen_s ( &Destin, "Output.txt", "wt,ccs=Unicode" ) ;
	if ( Error )
		return ( 1 ) ;

	// Create the analyzer.
	_Juman Juman ;

	// Try analyzing something.
	if ( Juman.Analyze ( L"高須といいます。SONYのだった。" ) ) {
		unsigned Count = Juman.GetResultCount ( ) ;
		for ( unsigned i=0; i<Count; i++ ) {
			wchar_t Buffer [0x400] ;
			if ( Juman.GetResultString ( i, Buffer, _countof(Buffer) ) )
				fwprintf ( Destin, L"%ls", Buffer ) ;
		} /* endfor */
		for ( unsigned i=0; i<Count; i++ ) {
			wchar_t Buffer [2] [0x400] ;
			wchar_t *pSymbol = Juman.GetSymbol ( i, Buffer[0], _countof(Buffer[0]) ) ;
			wchar_t *pSound = Juman.GetSound ( i, Buffer[1], _countof(Buffer[1]) ) ;
			_Juman::_PartOfSpeechCode posc = Juman.GetPartOfSpeech ( i ) ;
			wchar_t *pszPartOfSpeech = Juman.GetPartOfSpeechName ( posc ) ;
			if ( pSymbol && pSound )
				fwprintf ( Destin, L"%ls / %ls / %ls\n", pSymbol, pSound, pszPartOfSpeech ) ;
		} /* endfor */
	} /* endif */

	// Can we do it again?
	if ( Juman.Analyze ( L"宜しくお願いします。いいですか？行ってしまう。聞いちゃう。" ) ) {
		unsigned Count = Juman.GetResultCount ( ) ;
		for ( unsigned i=0; i<Count; i++ ) {
			wchar_t Buffer [0x400] ;
			if ( Juman.GetResultString ( i, Buffer, _countof(Buffer) ) )
				fwprintf ( Destin, L"%ls", Buffer ) ;
		} /* endfor */
		for ( unsigned i=0; i<Count; i++ ) {
			wchar_t Buffer [2] [0x400] ;
			wchar_t *pSymbol = Juman.GetSymbol ( i, Buffer[0], _countof(Buffer[0]) ) ;
			wchar_t *pSound = Juman.GetSound ( i, Buffer[1], _countof(Buffer[1]) ) ;
			_Juman::_PartOfSpeechCode posc = Juman.GetPartOfSpeech ( i ) ;
			wchar_t *pszPartOfSpeech = Juman.GetPartOfSpeechName ( posc ) ;
			if ( pSymbol && pSound )
				fwprintf ( Destin, L"%ls / %ls / %ls\n", pSymbol, pSound, pszPartOfSpeech ) ;
		} /* endfor */
	} /* endif */

	// Done.
	fclose ( Destin ) ;
	return ( 0 ) ;
}

#endif