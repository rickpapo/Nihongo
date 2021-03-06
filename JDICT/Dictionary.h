#pragma once
#include "stdafx.h"
#include "JDICT_Reader.h"

// Platform-specific Definitions
#ifdef _M_AMD64
typedef ULONGLONG ADDRESS ;
#else
typedef DWORD ADDRESS ;
#endif

//
// Previously, the dictionary structure was an STL construct:
//
//   typedef std::map<STRING,_DictionaryEntry> DICTIONARY ; // Indexed by definition content.
//   typedef std::multiset<_IndexEntry> INDEX ;
//     const STRING* psWord ;								// => Heap-allocated null terminated string.
//     const STRING* psKana ;								// => Heap-allocated null terminated string.
//     const _DictionaryEntry *pDictionaryEntry ;			// => Heap-allocated _DictionaryEntry structure.
//     const BYTE* pConjugations ;							// => Conjugations array of SymbolSound structure.
//
// NOTE: The dictionary structure itself was never accessed directly once the index was built.
//   Therefore, there are some things we won't need anymore.
//

// Up front, we need to know how much string space is required, terminators included.  This can be obtained
//   by scanning the index and summing the string sizes for the Symbols, the Sounds and the Infos.

struct _IndexEntry2 {
	wchar_t *pszWord ;
	wchar_t *pszKana ;
	wchar_t *pszInfo ;
	bool fPreferred ;
	BYTE Conjugations [MAX_CONJUGATIONS] ;
	// Constructor for dynamic extensions to read-only dictionary. 
	_IndexEntry2 ( wchar_t *psz, size_t cb, wchar_t *szInfo, bool f ) : pszWord(0), pszKana(0), pszInfo(0), fPreferred(f) {
		pszWord = new wchar_t [cb+1] ; memcpy ( pszWord, psz, cb*sizeof(wchar_t) ) ; pszWord[cb] = 0 ;
		pszKana = new wchar_t [cb+1] ; memcpy ( pszKana, psz, cb*sizeof(wchar_t) ) ; pszKana[cb] = 0 ;
		cb = wcslen(szInfo) + 1 ; pszInfo = new wchar_t [cb] ; wcscpy_s ( pszInfo, cb, szInfo ) ;
		memset ( Conjugations, 0, sizeof(Conjugations) ) ;
	} /* endmethod */
	// Constructor for dynamic extensions to read-only dictionary.
	_IndexEntry2(const _IndexEntry2 &obj, const wchar_t *Suffix) : pszWord(0), pszKana(0), pszInfo(0), fPreferred(obj.fPreferred) {
		size_t cbWord = wcslen(obj.pszWord) + wcslen(Suffix);
		pszWord = new wchar_t[cbWord+1]; 
		wcscpy_s(pszWord, cbWord+1, obj.pszWord);
		wcscat_s(pszWord, cbWord + 1, Suffix);
		size_t cbKana = wcslen(obj.pszKana) + wcslen(Suffix);
		pszKana = new wchar_t[cbKana+1]; 
		wcscpy_s(pszKana, cbKana + 1, obj.pszKana);
		wcscat_s(pszKana, cbKana + 1, Suffix);
		size_t cbInfo = wcslen(obj.pszInfo)+1;
		pszInfo = new wchar_t[cbInfo]; wcscpy_s(pszInfo, cbInfo, obj.pszInfo);
		memset(Conjugations, 0, sizeof(Conjugations));
	} /* endmethod */
	// Default constructor.
	_IndexEntry2 ( ) : pszWord(0), pszKana(0), pszInfo(0), fPreferred(false) {
		memset(Conjugations, 0, sizeof(Conjugations));
	} /* endmethod */
	bool isExtension ( ) ;
	void Release ( ) {
		if ( isExtension ( ) ) {
			delete [] pszWord ;
			delete [] pszKana ;
			delete [] pszInfo ;
		} /* endif */
	} /* endmethod */
	bool operator< ( const _IndexEntry2 &obj ) const {
		size_t cb1 = wcslen ( pszWord ) ; 
		size_t cb2 = wcslen ( obj.pszWord ) ;
		size_t cbCompare = __min ( cb1, cb2 ) ;
		int Result = memcmp ( pszWord, obj.pszWord, cbCompare*sizeof(wchar_t) ) ;
		const wchar_t *pHere = pszWord ;
		const wchar_t *pThere = obj.pszWord ;
		for ( size_t i=0; i<cbCompare; i++, pHere++, pThere++ ) {
			int Result = *pHere - *pThere ;
			if ( Result < 0 )
				return ( true ) ;
			if ( Result > 0 )
				return ( false ) ;
		} /* endif */
		Result = (int) ( cb1 - cb2 ) ;
		if ( Result > 0 ) 
			return ( true ) ;
		return ( false ) ;
	} /* endmethod */
	void Dump ( FILE *File, int Indent, bool fExtra=false ) {
		if ( fExtra ) {
			if ( !wcscmp ( pszWord, pszKana ) ) 
				fwprintf ( File, L"%*ls%0.*ls = ", Indent, L"", (int)wcslen(pszWord), L"　　　　　　　　　　" ) ;
			else
				fwprintf ( File, L"%*ls%0.*ls (%ls) = ", Indent, L"", (int)wcslen(pszWord), L"　　　　　　　　　　", pszKana ) ;
		} else {
			if ( !wcscmp ( pszWord, pszKana ) ) 
				fwprintf ( File, L"%*ls%ls = ", Indent, L"", pszWord ) ;
			else
				fwprintf ( File, L"%*ls%ls (%ls) = ", Indent, L"", pszWord, pszKana ) ;
		} /* endif */
		if ( Conjugations[0] ) {
			fwprintf ( File, L"[" ) ; bool fFirst = true ;
			for ( unsigned i=0; i<MAX_CONJUGATIONS; i++ ) {
				if ( Conjugations[i] == 0 ) 
					break ;
				// Condense "Plain Negative (1), Plain Positive Past" to "Plain Negative Past".
				_ConjugationType ThisType = (_ConjugationType) Conjugations [i] ;
				_ConjugationType NextType = (_ConjugationType) ( i < MAX_CONJUGATIONS ? Conjugations [i+1] : CT_NONE ) ;
				if ( ( ThisType == CT_PLAIN_NEGATIVE1 ) && ( NextType == CT_PLAIN_POSITIVE_PAST ) ) {
					ThisType = CT_PLAIN_NEGATIVE_PAST ;
					++ i ;
				} /* endif */
				fwprintf ( File, L"%ls%ls", fFirst?L"":L", ", szConjugationTypes[ThisType] ) ;
				fFirst = false ;
			} /* endwhile */
			fwprintf ( File, L"] " ) ;
		} /* endif */
		fwprintf ( File, L"%ls\n", pszInfo ) ;
	} /* endmethod */
#ifdef _AFXDLL
	void Dump ( CString& Result, int Indent, bool fExtra=false ) {
		if ( fExtra ) {
			if ( !wcscmp ( pszWord, pszKana ) ) 
				Result.AppendFormat ( L"%*ls%0.*ls = ", Indent, L"", wcslen(pszWord), L"　　　　　　　　　　" ) ;
			else
				Result.AppendFormat ( L"%*ls%0.*ls (%ls) = ", Indent, L"", wcslen(pszWord), L"　　　　　　　　　　", pszKana ) ;
		} else {
			if ( !wcscmp ( pszWord, pszKana ) ) 
				Result.AppendFormat ( L"%*ls%ls = ", Indent, L"", pszWord ) ;
			else
				Result.AppendFormat ( L"%*ls%ls (%ls) = ", Indent, L"", pszWord, pszKana ) ;
		} /* endif */
		if ( Conjugations[0] ) {
			Result.AppendFormat ( L"[" ) ; bool fFirst = true ;
			for ( unsigned i=0; i<MAX_CONJUGATIONS; i++ ) {
				if ( Conjugations[i] == 0 ) 
					break ;
				// Condense "Plain Negative (1), Plain Positive Past" to "Plain Negative Past".
				_ConjugationType ThisType = (_ConjugationType) Conjugations [i] ;
				_ConjugationType NextType = (_ConjugationType) ( i < MAX_CONJUGATIONS ? Conjugations [i+1] : CT_NONE ) ;
				if ( ( ThisType == CT_PLAIN_NEGATIVE1 ) && ( NextType == CT_PLAIN_POSITIVE_PAST ) ) {
					ThisType = CT_PLAIN_NEGATIVE_PAST ;
					++ i ;
				} /* endif */
				Result.AppendFormat ( L"%ls%ls", fFirst?L"":L", ", szConjugationTypes[ThisType] ) ;
				fFirst = false ;
			} /* endwhile */
			Result.AppendFormat ( L"] " ) ;
		} /* endif */
		Result.AppendFormat ( L"%ls\n", pszInfo ) ;
	} /* endmethod */
#endif _AFXDLL

} ;

extern const wchar_t *szFilename ;
extern const char *szEyecatcher ;
extern const long lVersion ;

class _Index2 {

	size_t cbBuffer ;		// Size of general string buffer.
	wchar_t *pBuffer ;		// => General string buffer.
	size_t nIndex ;			// Number of Index Entries.
	_IndexEntry2 *pIndex ;	// => Index Entry buffer

	_IndexEntry2* FindEntry ( _IndexEntry2& Key ) {

		// Search the sorted index.
		long Min = 0, Max = (long) ( nIndex - 1 ) ;
		while ( Max >= Min ) {
			long Middle = ( Min + Max ) / 2 ;
			_IndexEntry2 *pMiddle = pIndex + Middle ;
			if ( *pMiddle < Key ) 
				Min = Middle + 1 ;
			else if ( Key < *pMiddle ) 
				Max = Middle - 1 ;
			else
				return ( pMiddle ) ;
		} /* endwhile */

		// Didn't find anything.
		return ( NULL ) ;
	} /* endmethod */

	_IndexEntry2* FindFirstEntry ( _IndexEntry2& Key ) {

		// Search the sorted index.
		_IndexEntry2 *pEntry = FindEntry ( Key ) ;
		if ( pEntry == NULL )
			return ( NULL ) ;

		// If you found something, search back for the first possible match.
		_IndexEntry2 *pPrevious = pEntry - 1 ;
		while ( pIndex <= pPrevious ) {
			if ( wcscmp ( pPrevious->pszWord, Key.pszWord ) ) 
				return ( pEntry ) ;
			pEntry = pPrevious ;
			pPrevious = pEntry - 1 ;
		} /* endwhile */
		return ( pEntry ) ;

	} /* endmethod */

	_IndexEntry2* FindPreferredEntry ( _IndexEntry2& Key ) {

		// Search the sorted index.
		_IndexEntry2 *pEntry = FindFirstEntry ( Key ) ;
		if ( pEntry == NULL )
			return ( NULL ) ;

		// If you found something, search forward to first preferred matching entry.
		_IndexEntry2 *pLast = End ( ) ;
		_IndexEntry2 *pNext = pEntry + 1 ;
		while ( pNext < pLast ) {
			if ( pEntry->fPreferred )
				return ( pEntry ) ;
			if ( wcscmp ( pNext->pszWord, Key.pszWord ) )
				return ( NULL ) ;
			pEntry = pNext ;
			pNext = pEntry + 1 ;
		} /* endwhile */

		// Didn't find a preferred entry.
		return ( NULL ) ;

	} /* endmethod */

public:

	// Constructor
	_Index2 ( ) : cbBuffer(0), pBuffer(0), nIndex(0), pIndex(0) { }

	// Destructor
	~_Index2 ( ) {
		if ( pBuffer ) {
			VirtualFree ( pBuffer, 0, MEM_RELEASE ) ;
			pBuffer = 0 ;
		} /* endif */
		if ( pIndex ) {
			VirtualFree ( pIndex, 0, MEM_RELEASE ) ;
			pIndex = 0 ;
		} /* endif */
	} /* endmethod */

	// Test if data part of buffer.
	bool inBuffer ( void *p ) {
		if ( p < pBuffer ) 
			return ( false ) ;
		if ( p >= pBuffer + cbBuffer ) 
			return ( false ) ;
		return ( true ) ;
	} /* endmethod */

	// Load from STL version of index.
	void Load ( INDEX& Index ) {

		// Set the number of entries.
		nIndex = Index.size ( ) ;

		// Determine how much buffer space we will need, if there were no duplication at all.
		cbBuffer = 0 ;
		for (auto& Entry : Index) {
			cbBuffer += wcslen(Entry.GetWord()) + 1;
			cbBuffer += wcslen(Entry.GetKana()) + 1;
			cbBuffer += wcslen(Entry.GetDictionaryEntry()->GetInfo()) + 1;
		} /* endfor */

		// Allocate the index space.
		size_t cbIndex = nIndex * sizeof(_IndexEntry2) ;
		pIndex = (_IndexEntry2*) VirtualAlloc ( 0, cbIndex, MEM_COMMIT, PAGE_READWRITE ) ;
		if ( pIndex == 0 )
			throw "ERROR: Unable to obtain memory for index." ;

		// Allocate the buffer space.
		pBuffer = (wchar_t*) VirtualAlloc ( 0, cbBuffer*sizeof(wchar_t), MEM_COMMIT, PAGE_READWRITE ) ;
		if ( pBuffer == 0 ) {
			VirtualFree ( pIndex, 0, MEM_RELEASE ) ;
			throw "ERROR: Unable to obtain memory for general buffer." ;
		} /* endif */

		// Load the index and the buffer.
		_IndexEntry2* pEntry{ pIndex } ;
		wchar_t* p{ pBuffer } ;
		size_t i{ 0 } ;
		for (auto& IndexEntry : Index) {

			// Copy in the word text.
			pEntry->pszWord = p;
			wchar_t* psz = (wchar_t*)IndexEntry.GetWord();
			size_t cb = wcslen(psz) + 1;
			memcpy(p, psz, cb * sizeof(wchar_t));
			p += cb;

			// Copy in the phonetic text.
			pEntry->pszKana = p;
			psz = (wchar_t*)IndexEntry.GetKana();
			cb = wcslen(psz) + 1;
			memcpy(p, psz, cb * sizeof(wchar_t));
			p += cb;

			// Copy in the dictionary definition text.
			pEntry->pszInfo = p;
			psz = (wchar_t*)IndexEntry.GetDictionaryEntry()->GetInfo();
			cb = wcslen(psz) + 1;
			memcpy(p, psz, cb * sizeof(wchar_t));
			p += cb;

			// Copy in the conjugations array.
			pEntry->fPreferred = IndexEntry.GetDictionaryEntry()->isPreferred();
			memcpy(pEntry->Conjugations, IndexEntry.GetConjugations(), sizeof(pEntry->Conjugations));

			// Next entry.
			++pEntry;

		} /* endfor */
		assert ( p == pBuffer + cbBuffer ) ;

		// Write-protect both areas.
		DWORD OldProtect = 0 ;
		VirtualProtect ( pIndex, cbIndex, PAGE_READONLY, &OldProtect ) ;
		VirtualProtect ( pBuffer, cbBuffer*sizeof(wchar_t), PAGE_READONLY, &OldProtect ) ;

		// Save the dictionary.
		Save ( ) ;

	} /* endmethod */

	// Restore from saved file.
	bool Restore ( ) {
		printf ( "Loading dictionary.\n" ) ;

		// Open the file for reading.
		FILE *File ; errno_t Error = _wfopen_s ( &File, szFilename, L"rb" ) ;
		if ( Error || !File ) {
			fwprintf ( stderr, L"ERROR: Unable to open dictionary file '%s' for reading.\n", szFilename ) ;
			return ( false ) ;
		} /* endif */

		// Read and verify the eye-catcher.
		unsigned char Buffer [100] ;
		size_t cbRead = fread ( Buffer, 1, strlen(szEyecatcher), File ) ;
		if ( cbRead < strlen(szEyecatcher) ) {
			fwprintf ( stderr, L"ERROR: Unable to read dictionary eyecatcher.\n" ) ;
			return ( false ) ;
		} /* endif */
		if ( memcmp ( Buffer, szEyecatcher, strlen(szEyecatcher) ) ) {
			fwprintf ( stderr, L"ERROR: Unable to validate eyecatcher.\n" ) ;
			return ( false ) ;
		} /* endif */

		// Read and verify the file version.
		cbRead = fread ( Buffer, 1, sizeof(lVersion), File ) ;
		if ( cbRead < sizeof(lVersion) ) {
			fwprintf ( stderr, L"ERROR: Unable to read dictionary version.\n" ) ;
			return ( false ) ;
		} /* endif */
		if ( *((long*)Buffer) != lVersion ) {
			fwprintf ( stderr, L"ERROR: Unable to validate version.\n" ) ;
			return ( false ) ;
		} /* endif */

		// Read the string buffer size.
		cbRead = fread ( Buffer, 1, sizeof(cbBuffer), File ) ;
		if ( cbRead < sizeof(cbBuffer) ) {
			fwprintf ( stderr, L"ERROR: Unable to read dictionary string buffer size.\n" ) ;
			return ( false ) ;
		} /* endif */
		cbBuffer = *((size_t*)Buffer) ;

		// Allocate the buffer space.
		pBuffer = (wchar_t*) VirtualAlloc ( 0, cbBuffer*sizeof(wchar_t), MEM_COMMIT, PAGE_READWRITE ) ;
		if ( pBuffer == 0 ) {
			fwprintf ( stderr, L"ERROR: Unable to allocate dictionary string buffer.\n" ) ;
			return ( false ) ;
		} /* endif */

		// Read the buffer itself.
		cbRead = fread ( pBuffer, sizeof(wchar_t), cbBuffer, File ) ;
		if ( cbRead < cbBuffer ) {
			fwprintf ( stderr, L"ERROR: Unable to read dictionary string buffer.\n" ) ;
			return ( false ) ;
		} /* endif */

		// Read the number of index entries.
		cbRead = fread ( Buffer, 1, sizeof(nIndex), File ) ;
		if ( cbRead < sizeof(nIndex) ) {
			fwprintf ( stderr, L"ERROR: Unable to read dictionary index size.\n" ) ;
			return ( false ) ;
		} /* endif */
		nIndex = *((size_t*)Buffer) ;

		// Allocate the index space.
		size_t cbIndex = nIndex * sizeof(_IndexEntry2) ;
		pIndex = (_IndexEntry2*) VirtualAlloc ( 0, cbIndex, MEM_COMMIT, PAGE_READWRITE ) ;
		if ( pIndex == 0 ) {
			fwprintf ( stderr, L"ERROR: Unable to allocate dictionary index.\n" ) ;
			return ( false ) ;
		} /* endif */

		// Read the index entries, one by one, adjusting their pointers.
		for ( size_t i=0; i<nIndex; i++ ) {
			_IndexEntry2 Record ;
			cbRead = fread ( Buffer, 1, sizeof(Record), File ) ;
			if ( cbRead < sizeof(Record) ) {
				fwprintf ( stderr, L"ERROR: Unable to read dictionary index record.\n" ) ;
				return ( false ) ;
			} /* endif */
			memcpy ( &Record, Buffer, sizeof(Record) ) ;
			Record.pszWord = (wchar_t*) ( (ADDRESS) Record.pszWord + (ADDRESS) pBuffer ) ;
			Record.pszKana = (wchar_t*) ( (ADDRESS) Record.pszKana + (ADDRESS) pBuffer ) ;
			Record.pszInfo = (wchar_t*) ( (ADDRESS) Record.pszInfo + (ADDRESS) pBuffer ) ;
			pIndex[i] = Record ;
		} /* endfor */

		// Close up.
		fclose ( File ) ;

		// Write-protect both areas.
		DWORD OldProtect = 0 ;
		VirtualProtect ( pIndex, cbIndex, PAGE_READONLY, &OldProtect ) ;
		VirtualProtect ( pBuffer, cbBuffer*sizeof(wchar_t), PAGE_READONLY, &OldProtect ) ;

		// Done OK.
		printf ( "  Dictionary load complete.\n" ) ;
		return ( true ) ;

	} /* endmethod */

	// Save from file.
	bool Save ( ) {
		printf ( "Saving dictionary.\n" ) ;

		// Delete the old dictionary first.
		if ( !_waccess ( szFilename, 0 ) && _wremove ( szFilename ) ) {
			fwprintf ( stderr, L"ERROR: Unable to delete old dictionary file '%s'.\n", szFilename ) ;
			return ( false ) ;
		} /* endif */

		// Open/create the file for writing.
		FILE *File ; errno_t Error = _wfopen_s ( &File, szFilename, L"wb" ) ;
		if ( Error ) {
			fwprintf ( stderr, L"ERROR: Unable to open dictionary file '%s' for writing.\n", szFilename ) ;
			return ( false ) ;
		} /* endif */

		// Write file eyecatcher.
		fwrite ( szEyecatcher, 1, strlen(szEyecatcher), File ) ;

		// Write the file version.
		fwrite ( &lVersion, sizeof(lVersion), 1, File ) ;

		// Write the buffer size.
		fwrite ( &cbBuffer, sizeof(cbBuffer), 1, File ) ;

		// Write the buffer itself.
		fwrite ( pBuffer, sizeof(wchar_t), cbBuffer, File ) ;

		// Write the number of index entries.
		fwrite ( &nIndex, sizeof(nIndex), 1, File ) ;

		// Write the index entries, one by one, adjusting their pointers.
		for ( size_t i=0; i<nIndex; i++ ) {
			_IndexEntry2 Record = pIndex[i] ;
			Record.pszWord = (wchar_t*) ( (ADDRESS) Record.pszWord - (ADDRESS) pBuffer ) ;
			Record.pszKana = (wchar_t*) ( (ADDRESS) Record.pszKana - (ADDRESS) pBuffer ) ;
			Record.pszInfo = (wchar_t*) ( (ADDRESS) Record.pszInfo - (ADDRESS) pBuffer ) ;
			fwrite ( &Record, sizeof(Record), 1, File ) ;
		} /* endfor */

		// Close up.
		fclose ( File ) ;

		// Done OK.
		printf ( "  Dictionary save complete.\n" ) ;
		return ( true ) ;

	} /* endmethod */

	// Rough equivalent to STL "begin" function.
	_IndexEntry2* Begin ( ) { return ( pIndex ) ; }

	// Rough equivalent to STL "end" function.
	_IndexEntry2* End ( ) { return ( pIndex + nIndex ) ; }

	// Find longest match.
	_IndexEntry2* FindLongestMatch ( wchar_t *pszWord, size_t cbWord ) {
		for ( size_t cb = cbWord; cb>0; cb-- ) {
			_IndexEntry2 *Result = FindFirstEntry ( pszWord, cb ) ;
			if ( Result )
				return ( FindPreferredEntry ( pszWord, cb ) ) ;
		} /* endfor */
		return ( NULL ) ;
	} /* endmethod */

	// Find first entry exactly matching a character sequence.
	_IndexEntry2* FindFirstEntry ( wchar_t *pszWord, size_t cbWord ) {

		// Construct search key.
		assert ( cbWord < 100 ) ;
		wchar_t Text [100] ; 
		memcpy ( Text, pszWord, cbWord*sizeof(wchar_t) ) ;
		Text [ cbWord ] = 0 ;
		_IndexEntry2 Key ;
		Key.pszWord = Text ;
		Key.fPreferred = true ;

		// Search the sorted index, returning the first matching entry.
		return ( FindFirstEntry ( Key ) ) ;

	} /* endmethod */

	// Find the first preferred entry of a group of identical entries.
	_IndexEntry2* FindPreferredEntry ( wchar_t *pszWord, size_t cbWord ) {

		// Construct search key.
		wchar_t Text [200] ; 
		assert ( cbWord < _countof(Text) ) ;
		memcpy ( Text, pszWord, cbWord*sizeof(wchar_t) ) ;
		Text [ cbWord ] = 0 ;
		_IndexEntry2 Key ;
		Key.pszWord = Text ;

		// Search the sorted index, returning the first preferred entry, if there is one.
		_IndexEntry2 *pEntry = FindPreferredEntry ( Key ) ;
		if ( pEntry )
			return ( pEntry ) ;

		// Otherwise, return the first entry.
		return ( FindFirstEntry ( Key ) ) ;

	} /* endmethod */

	_IndexEntry2* FindParticle ( wchar_t *pszWord, size_t cbWord ) {

		// Construct search key.
		assert ( cbWord < 100 ) ;
		wchar_t Text [100] ; 
		memcpy ( Text, pszWord, cbWord*sizeof(wchar_t) ) ;
		Text [ cbWord ] = 0 ;
		_IndexEntry2 Key ;
		Key.pszWord = Text ;
		Key.fPreferred = true ;

		// Search the sorted index, returning the first matching entry.
		_IndexEntry2 *pEntry = FindFirstEntry ( Key ) ;
		if ( pEntry == NULL )
			return ( NULL ) ;

		// Search for first matching entry marked with (prt) in the dictionary definition.
		_IndexEntry2 *pLast = End ( ) ;
		_IndexEntry2 *pNext = pEntry + 1 ;
		while ( pNext < pLast ) {
			if ( wcsstr ( pEntry->pszInfo, L"(prt)" ) )
				return ( pEntry ) ;
			if ( wcscmp ( pNext->pszWord, Key.pszWord ) )
				return ( NULL ) ;
			pEntry = pNext ;
			pNext = pEntry + 1 ;
		} /* endwhile */

		// Didn't find one.
		return ( NULL ) ;
	} /* endmethod */

	_IndexEntry2* FindHonorificPrefix ( wchar_t *pszWord, size_t cbWord ) {

		// Construct search key.
		assert ( cbWord < 100 ) ;
		wchar_t Text [100] ; 
		memcpy ( Text, pszWord, cbWord*sizeof(wchar_t) ) ;
		Text [ cbWord ] = 0 ;
		_IndexEntry2 Key ;
		Key.pszWord = Text ;
		Key.fPreferred = true ;

		// Search the sorted index, returning the first matching entry.
		_IndexEntry2 *pEntry = FindFirstEntry ( Key ) ;
		if ( pEntry == NULL )
			return ( NULL ) ;

		// Search for first matching entry marked with (prt) in the dictionary definition.
		_IndexEntry2 *pLast = End ( ) ;
		_IndexEntry2 *pNext = pEntry + 1 ;
		while ( pNext < pLast ) {
			if ( wcsstr ( pEntry->pszInfo, L"(pref)" ) )
				return ( pEntry ) ;
			if ( wcscmp ( pNext->pszWord, Key.pszWord ) )
				return ( NULL ) ;
			pEntry = pNext ;
			pNext = pEntry + 1 ;
		} /* endwhile */

		// Didn't find one.
		return ( NULL ) ;
	} /* endmethod */

} ;

extern _Index2 Index2 ;
