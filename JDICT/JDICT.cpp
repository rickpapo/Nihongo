// JDICT.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <set>
#include <map>
#include "JDICT_Reader.h"

int _tmain ( int argc, wchar_t* argv[] ) {

	// Load the dictionary.
	printf ( "Loading dictionary file.\n" ) ;
	if ( !BuildDictionary ( L"EDICT", true ) ) 
		return ( 1 ) ;
	printf ( "  %zi dictionary entries.\n", Dictionary.size() ) ;

	// Build the index.
	printf ( "Building word/phrase index.\n" ) ;
	if ( BuildIndex ( ) == false )
		return ( 1 ) ;
	printf ( "  %zi index entries.\n", Index.size() ) ;

	// Dump the dictionary.
	printf ( "Dumping dictionary file.\n" ) ;
	DumpDictionary ( ) ;

	// Done OK.
	printf ( "Done.\n" ) ;
	return ( 0 ) ;
}
