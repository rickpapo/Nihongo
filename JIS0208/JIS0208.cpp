// JIS0208.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdlib.h>
#include <string.h>

static struct _ConversionTable {
	unsigned short JIS0208 ;
	unsigned short Unicode ;
} ConversionTable [9000] ;

static unsigned nGlyphs = 0 ;

inline unsigned short SwapWord ( unsigned short Word ) {
	unsigned char *p = (unsigned char*) &Word ;
	return ( ( p[0] << 8 ) + p[1] ) ;
}

int wmain ( int argc, wchar_t* argv[] ) {

	FILE *File ; errno_t Error = _wfopen_s ( &File, L"JIS X 0208 (1990) to Unicode.txt", L"rt" ) ;
	if ( Error ) {
		char Buffer[200]; strerror_s(Buffer, _countof(Buffer), Error);
		fprintf ( stderr, "ERROR: Unable to open source file.  Error %i:%s\n", Error, Buffer ) ;
		return ( 1 ) ;
	} /* endif */

	wchar_t Buffer [200] ; 
	while ( fgetws ( Buffer, _countof(Buffer), File ) ) {
		if ( Buffer[0] == L';' )
			continue ;
		wchar_t *p = wcschr ( Buffer, 0x09 ) ;
		if ( !p )
			continue ;
		p = wcschr ( p+1, 0x09 ) ;
		if ( !p )
			continue ;
		p = wcschr ( p+1, 0x09 ) ;
		if ( !p )
			continue ;
		p = wcschr ( p+1, 0x09 ) ;
		if ( !p )
			continue ;
		// Now we are pointing at the JIS 0208 EUC encoding.
		unsigned Value1 ;
		if ( swscanf_s ( p, L"%x", &Value1 ) < 1 ) 
			continue ;
		p = wcschr ( p+1, 0x09 ) ;
		if ( !p )
			continue ;
		p = wcschr ( p+1, 0x09 ) ;
		if ( !p )
			continue ;
		unsigned Value2 ;
		if ( swscanf_s ( p, L"%x", &Value2 ) < 1 ) 
			continue ;
		ConversionTable[nGlyphs].JIS0208 = SwapWord ( Value1 ) ;
		ConversionTable[nGlyphs].Unicode = SwapWord ( Value2 ) ;
		nGlyphs ++ ;
	} /* endwhile */

	// Done reading the source file.
	fclose ( File ) ;

	// Make a consumable header file from all this.
	Error = _wfopen_s ( &File, L"JIS0208.h", L"wt,ccs=UNICODE" ) ;
	if ( Error || !File ) {
		char Buffer[200]; strerror_s(Buffer, _countof(Buffer), Error);
		fprintf ( stderr, "ERROR: Unable to create header file.  Error %i:%s\n", Error, Buffer ) ;
		return ( 1 ) ;
	} /* endif */
	fprintf ( File, "#pragma once\n" ) ;
	fprintf ( File, "\n" ) ;
	fprintf ( File, "static struct _ConversionTable {\n" ) ;
	fprintf ( File, "    unsigned short JIS0208 ;\n" ) ;
	fprintf ( File, "    unsigned short Unicode ;\n" ) ;
	fprintf ( File, "} JIS0208 [%u] = {\n", nGlyphs ) ;
	for ( unsigned i=0; i<nGlyphs; i++ ) {
		fprintf ( File, "    { 0x%04X, 0x%04X },\n", ConversionTable[i].JIS0208, ConversionTable[i].Unicode ) ;
	} /* endfor */
	fprintf ( File, "} ;\n" ) ;
	fclose ( File ) ;

	// Done OK.
	return ( 0 ) ;
}
