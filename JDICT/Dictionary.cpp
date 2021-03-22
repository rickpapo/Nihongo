#include "stdafx.h"
#include "Dictionary.h"

_Index2 Index2 ;
const wchar_t *szFilename = L"Dictionary.dat" ;
const char *szEyecatcher = "_NIHONGO" ;
const long lVersion = 100 ;

//
// Is this index entry a non-indexed extension?
//

bool _IndexEntry2::isExtension ( ) {
	return ( Index2.inBuffer ( pszWord ) == false ) ;
} /* endmethod */
