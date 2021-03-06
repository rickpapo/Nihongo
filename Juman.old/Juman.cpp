// Juman.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include "juman.h"

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

class _Juman {

private:
	char **Result ;

public:
	enum _PartOfSpeechCode {
		POSC_SPECIAL = 1,
		POSC_VERB,
		POSC_ADJECTIVE,
		POSC_FINALPARTICLE,
		POSC_AUXILIARY,
		POSC_NOUN,
		POSC_7,
		POSC_ADVERB,
		POSC_PARTICLE,
		POSC_10,
		POSC_11,
		POSC_12,
		POSC_PREFIX,
		POSC_SUFFIX,
		POSC_UNDEFINED,
		POSC_MAX
	} ;

private:
	static wchar_t *szPartofSpeechCodes [POSC_MAX] ;

public:
	_Juman ( ) : Result(0) {
		Show_Opt1 = Op_BB ;
		Show_Opt2 = Op_E2 ;
		set_jumanrc_fileptr ( NULL, TRUE, TRUE ) ;
		if ( !juman_init_rc ( Jumanrc_Fileptr ) )
			throw "ERROR: Unable to initialize JUMAN." ;
		juman_init_etc ( ) ;
	} /* endmethod */

	~_Juman ( ) {
		juman_close ( ) ;
	} /* endmethod */

	bool Analyze ( wchar_t *szText ) {
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

	unsigned GetResultCount ( ) {
		unsigned Count = 0 ;
		char **pResult = Result ;
		while ( *pResult ) {
			Count ++ ;
			pResult ++ ;
		} /* endwhile */
		return ( Count ) ;
	} /* endmethod */

	bool GetResultString ( unsigned i, wchar_t *pBuffer, unsigned cbBuffer ) {
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

	wchar_t *GetSymbol ( unsigned i, wchar_t *pBuffer, unsigned cbBuffer ) {
		if ( !GetResultString ( i, pBuffer, cbBuffer ) ) 
			return ( 0 ) ;
		if ( ( pBuffer[0] == L'@' ) && ( pBuffer[1] == L' ' ) ) 
			pBuffer += 2 ;
		wchar_t *p = wcschr ( pBuffer, L' ' ) ;
		if ( p )
			*p = 0 ;
		return ( pBuffer ) ;
	} /* endmethod */

	wchar_t *GetSound ( unsigned i, wchar_t *pBuffer, unsigned cbBuffer ) {
		if ( !GetResultString ( i, pBuffer, cbBuffer ) ) 
			return ( 0 ) ;
		if ( ( pBuffer[0] == L'@' ) && ( pBuffer[1] == L' ' ) ) 
			pBuffer += 2 ;
		wchar_t *pSound = wcschr ( pBuffer, L' ' ) ;
		if ( !pSound )
			return ( 0 ) ;
		pSound ++ ;
		wchar_t *p = wcschr ( pSound, L' ' ) ;
		if ( p )
			*p = 0 ;
		return ( pSound ) ;
	} /* endmethod */

	_PartOfSpeechCode GetPartOfSpeech ( unsigned i ) {
		wchar_t Buffer [100] ;
		if ( !GetResultString ( i, Buffer, _countof(Buffer) ) ) 
			return ( POSC_MAX ) ;
		wchar_t *pBuffer = Buffer ;
		if ( ( pBuffer[0] == L'@' ) && ( pBuffer[1] == L' ' ) ) 
			pBuffer += 2 ;
		wchar_t *p = wcschr ( pBuffer, L' ' ) ;
		p = wcschr ( p+1, L' ' ) ;
		p = wcschr ( p+1, L' ' ) ;
		p = wcschr ( p+1, L' ' ) ;
		return ( (_PartOfSpeechCode) _wtoi ( p ) ) ;
	} /* endmethod */

	static wchar_t *GetPartOfSpeechName ( _PartOfSpeechCode posc ) {
		return ( szPartofSpeechCodes[posc-1] ) ;
	} /* endmethod */
} ;

wchar_t *_Juman::szPartofSpeechCodes [] = {
	L"Special", L"Verb", L"Adjective", L"Final Particle", L"Auxiliary", L"Noun",
	L"??", L"Adverb", L"Particle", L"??", L"??", L"??", L"Prefix", L"Suffix", L"Undefined",
	L"??"
} ;

int _tmain ( int argc, _TCHAR* argv[] ) {

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

