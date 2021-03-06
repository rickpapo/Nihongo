// Analyzer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <assert.h>
#include <signal.h>

#include "JDICT_Reader.h"

#define DO_NOTHING { int x = 0 ; x ++ ; x -- ; } // Macro to be able to set a breakpoint on.

static BOOL fAbort = FALSE ;
static void SignalHandler ( int ) { fAbort = TRUE ; }

static BOOL WINAPI handler_routine ( DWORD dwCtrlType ) {
    
    // console control event handler for running server in console window
    switch ( dwCtrlType ) {
        
        // Ctrl+C, Ctrl+Break, Close Button: Raise an interrupt and be done with it.
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        raise ( SIGINT ) ; 
        return ( TRUE ) ;
        
        // Anything else: Let the default handler take it.
    default: 
        return ( FALSE ) ;
        
    } /* endswitch */
}

static const wchar_t QuotePairs [] [2] = { // Things that can be treated like quote pairs.
	{ L'「', L'」' }, { L'\"', L'\"' },
	{ L'『', L'』' }, { L'\'', L'\'' },
	{ L'（', L'）' }, { L'(', L')' },
	{ L'｛', L'｝' }, { L'{', L'}' },
	{ L'［', L'］' }, { L'[', L']' },
	{ L'＜', L'＞' }, { L'<', L'>' },
	{ L'〔', L'〕' }, 
	{ L'《', L'》' },
	{ L'【', L'】' },
} ;

static const wchar_t *SentenceEnders [] = { L"。", L".", L"？", L"?", L"!?", L"！", L"!" } ;

inline bool isHiragana ( wchar_t c ) {	return ( ( c >= 0x3040 ) && ( c <= 0x309F ) ) ; }
inline bool isKatakana ( wchar_t c ) {	return ( ( c >= 0x30A0 ) && ( c <= 0x30FF ) ) ; }
inline bool isKanji ( wchar_t c ) {	return ( ( ( c >= 0x4E00 ) && ( c <= 0x9FBF ) ) || ( c == L'々' ) ) ; }
inline bool isWhite ( wchar_t c ) { return ( ( c == L' ' ) || ( c == L'　' ) || ( c == L'\t' ) || ( c == L'\r' ) || ( c == L'\n' ) ) ; }

enum _WordClass {
	WC_UNKNOWN		= 0,
	WC_NOUN			= 0x00000001,
	WC_VERB_ICHIDAN = 0x00000002,
	WC_VERB_GODAN	= 0x00000004,
	WC_VERB_GODAN_IKU=0x00000008,
	WC_VERB_GODAN_ARU=0x00000010,
	WC_VERB_KURU	= 0x00000020,
	WC_VERB_SURU	= 0x00000040,
	WC_ADJ_F		= 0x00000080,
	WC_ADJ_I		= 0x00000100,
	WC_ADJ_NA		= 0x00000200,
	WC_ADJ_NO		= 0x00000400,
	WC_ADJ_PN		= 0x00000800,
	WC_ADJ_T		= 0x00001000,
	WC_PRIORITY		= 0x00002000,
	WC_ADVERB		= 0x00004000,
	WC_AUXILIARY	= 0x00008000,
	WC_CONJUNCTION  = 0x00010000,
	WC_PREFIX		= 0x00020000,
	WC_SUFFIX		= 0x00040000,
	WC_SURU			= 0x00080000,
	WC_PARTICLE     = 0x00100000
} ;

static struct _Token {
	static unsigned LongestName ;
	wchar_t *szName ;
	size_t cbName ;
	unsigned WordClass ;
	_Token ( wchar_t *name, unsigned wc ) : szName(name), cbName(wcslen(name)), WordClass(wc) { 
		LongestName = (unsigned) __max ( LongestName, cbName ) ;
	} /* endmethod */
	bool Check ( wchar_t *pToken, size_t cbToken ) {
		if ( cbToken != cbName )
			return ( false ) ;
		for ( unsigned i=0; i<cbName; i++ )
			if ( pToken[i] != szName[i] )
				return ( false ) ;
		return ( true ) ;
	} /* endmethod */
} Tokens [] = {
	_Token ( L"n", WC_NOUN ),			// Futsuumeishi
	_Token ( L"name", WC_NOUN ),		// All names are nouns.
	_Token ( L"P", WC_PRIORITY ),
	_Token ( L"v1", WC_VERB_ICHIDAN ),
	_Token ( L"v5", WC_VERB_GODAN ),
	_Token ( L"v5b", WC_VERB_GODAN ),
	_Token ( L"v5g", WC_VERB_GODAN ),
	_Token ( L"v5k", WC_VERB_GODAN ),
	_Token ( L"v5m", WC_VERB_GODAN ),
	_Token ( L"v5n", WC_VERB_GODAN ),
	_Token ( L"v5r", WC_VERB_GODAN ),
	_Token ( L"v5s", WC_VERB_GODAN ),
	_Token ( L"v5t", WC_VERB_GODAN ),
	_Token ( L"v5u", WC_VERB_GODAN ),
	_Token ( L"v5k-s", WC_VERB_GODAN_IKU ),
	_Token ( L"v5aru", WC_VERB_GODAN_ARU ),
	_Token ( L"vk", WC_VERB_KURU ),
	_Token ( L"vs", WC_SURU ),
	_Token ( L"vs-s", WC_VERB_SURU ),
	_Token ( L"n-t", WC_NOUN ),
	_Token ( L"adv", WC_ADVERB ),
	_Token ( L"aux", WC_AUXILIARY ),	// Fukushi
	_Token ( L"conj", WC_CONJUNCTION ),
	_Token ( L"adj-f", WC_ADJ_F ),
	_Token ( L"adj-i", WC_ADJ_I ),		// Keiyoshi
	_Token ( L"adj-na", WC_ADJ_NA ),	// Keiyoudoushi
	_Token ( L"adj-no", WC_ADJ_NO ),
	_Token ( L"adj-pn", WC_ADJ_PN ),	// Rentaishi
	_Token ( L"adj-t", WC_ADJ_T ),
	_Token ( L"pref", WC_PREFIX ),
	_Token ( L"suf", WC_SUFFIX ),
	_Token ( L"prt", WC_PARTICLE ),
	_Token ( L"1", WC_UNKNOWN ),
	_Token ( L"2", WC_UNKNOWN ),
	_Token ( L"3", WC_UNKNOWN ),
	_Token ( L"4", WC_UNKNOWN ),
	_Token ( L"5", WC_UNKNOWN ),
	_Token ( L"6", WC_UNKNOWN ),
	_Token ( L"7", WC_UNKNOWN ),
	_Token ( L"8", WC_UNKNOWN ),
	_Token ( L"9", WC_UNKNOWN ),
	_Token ( L"uk", WC_UNKNOWN ),
	_Token ( L"col", WC_UNKNOWN ),
	_Token ( L"int", WC_UNKNOWN ),
	_Token ( L"obs", WC_UNKNOWN ),
	_Token ( L"abbr", WC_UNKNOWN ),
	_Token ( L"obsc", WC_UNKNOWN ),
	_Token ( L"gikun", WC_UNKNOWN ),
} ;

unsigned _Token::LongestName = 0 ;

static void CheckToken ( unsigned &wc, wchar_t *szToken ) {
	size_t cbToken = wcslen(szToken) ;
	if ( cbToken > _Token::LongestName )
		return ;
	for ( unsigned i=0; i<_countof(Tokens); i++ ) {
		if ( Tokens[i].Check ( szToken, cbToken ) ) {
			wc |= Tokens[i].WordClass ;
			return ;
		} /* endif */
	} /* endfor */
	DO_NOTHING ;
}

static unsigned GetWordClass ( const wchar_t *pInfo ) {
	bool fParenthesis = false ; unsigned WordClass = 0 ;
	while ( *pInfo ) {
		if ( fParenthesis ) {
			wchar_t Token [22] ; unsigned cbToken = 0 ; 
			while ( *pInfo ) {
				if ( *pInfo == L')' ) {
					Token[cbToken] = 0 ; 
					CheckToken ( WordClass, Token ) ;
					cbToken = 0 ; ++ pInfo ;
					fParenthesis = false ; 
					break ;
				} else if ( *pInfo == L',' ) {
					Token[cbToken] = 0 ; 
					CheckToken ( WordClass, Token ) ;
					cbToken = 0 ; ++ pInfo ;
				} else if ( cbToken < _countof(Token)-1 ) {
					Token[cbToken++] = *pInfo++ ;
				} else {
					++ pInfo ;
				} /* endif */
			} /* endwhile */
		} else if ( *pInfo == '(' ) {
			fParenthesis = true ;
			++ pInfo ;
		} else {
			++ pInfo ;
		} /* endif */
	} /* endwhile */
	return ( WordClass ) ;
}

static const _IndexEntry *FindParticle ( wchar_t *p, size_t cb ) {

	INDEX::iterator IndexCursor = FindSymbolExact ( p, cb ) ;
	if ( IndexCursor == Index.end() ) 
		return ( 0 ) ;

	const _IndexEntry *pIndexEntry = &(*IndexCursor) ;
	const _DictionaryEntry *pDictionaryEntry = pIndexEntry->GetDictionaryEntry ( ) ;
	unsigned WordClass = GetWordClass ( pDictionaryEntry->GetInfo ( ) ) ;
	while ( ( WordClass & WC_PARTICLE ) == 0 ) {
		++ IndexCursor ;
		if ( IndexCursor == Index.end() ) 
			break ;
		const _IndexEntry *pIndexEntry2 = & ( *IndexCursor ) ;
		if ( wcscmp ( pIndexEntry->GetWord(), pIndexEntry2->GetWord() ) )
			break ;
		pIndexEntry = pIndexEntry2 ;
		pDictionaryEntry = pIndexEntry->GetDictionaryEntry ( ) ;
		WordClass = GetWordClass ( pDictionaryEntry->GetInfo ( ) ) ;
	} /* endwhile */
	if ( WordClass & WC_PARTICLE ) 
		return ( pIndexEntry ) ;
	return ( 0 ) ;
}

static const _IndexEntry *pParticleDE = 0 ;
static const _IndexEntry *pParticleGA = 0;
static const _IndexEntry *pParticleKA = 0;
static const _IndexEntry *pParticleNI = 0;
static const _IndexEntry *pParticleNO = 0;
static const _IndexEntry *pParticleWA = 0;
static const _IndexEntry *pParticleWO = 0;

class _Word {
private:
	const wchar_t *pWord ;
	const size_t cbWord ;
	const _IndexEntry *pIndexEntry ;
	bool fSpecificEntry ;
public:
	_Word ( const wchar_t *p, const size_t cb, const _IndexEntry *pEntry=0 ) : pWord(p), cbWord(cb), pIndexEntry(pEntry), fSpecificEntry(false) {
	} /* endmethod */
	_Word ( ) : pWord(0), cbWord(0), pIndexEntry(0), fSpecificEntry(false) {
	} /* endmethod */
	const wchar_t *GetValue ( ) { return ( pWord ) ; }
	const size_t GetLength ( ) { return ( cbWord ) ; }
	const _IndexEntry *GetIndexEntry ( ) { return ( pIndexEntry ) ; }
	void SetIndexEntry ( const _IndexEntry *p ) { pIndexEntry = p ; fSpecificEntry = true ; }
	void Dump ( FILE *Destin, const unsigned Indent ) {
		if ( fSpecificEntry ) {
			((_IndexEntry*)pIndexEntry)->Dump ( Destin, Indent ) ;
		} else { 
			if ( pIndexEntry ) {
				INDEX::iterator IndexCursor = FindSymbolExact ( (wchar_t*)pWord, cbWord ) ;
				assert ( IndexCursor != Index.end() ) ;
				if ( IndexCursor != Index.end() ) {
					// Just what kind of word have we found?
					const _IndexEntry *pIndexEntry = &(*IndexCursor) ;
					pIndexEntry->Dump ( Destin, Indent ) ;
					do {
						++ IndexCursor ;
						if ( IndexCursor != Index.end() ) {
							const _IndexEntry *pIndexEntry2 = & ( *IndexCursor ) ;
							if ( wcscmp ( pIndexEntry->GetWord(), pIndexEntry2->GetWord() ) )
								break ;
							pIndexEntry2->Dump ( Destin, Indent, true ) ;
						} /* endif */
					} while ( IndexCursor != Index.end() ) ;
				} else {
					fwprintf ( Destin, L"%*s%0.*s\n", Indent, L"", (int)cbWord, pWord ) ;
				} /* endif */
			} else {
				fwprintf ( Destin, L"%*s%0.*s\n", Indent, L"", (int)cbWord, pWord ) ;
			} /* endif */
		} /* endif */
	} /* endmethod */
} ;

static size_t ParseWord ( FILE *Destin, const wchar_t *p, const size_t cb, const unsigned Indent, _Word& Word ) {

	// Extract any furigana.  NOTE: This format is specific to the Toradora transcript file.
	if ( p[0] == L'《' ) {
		for ( unsigned i=1; i<cb; i++ ) {
			if ( p[i] == L'》' ) {
				// fwprintf ( Destin, L"%*sFurigana: %0.*s\n", Indent, L"", i-1, p+1 ) ;
				// Do nothing to the word, as we are not adding it to the sentence word list.
				return ( i+1 ) ;
			} /* endif */
		} /* endfor */
	} /* endif */

	// Generally speaking, a Kanji character is a word or word root.
	// A Kanji will be followed by one of the following:
	//   (1) Another kanji, forming a compound word.
	//   (2) Depending on the type of kanji, a hiragana conjugation.
	//   (3) A hiragana particle.  Which ones are valid depends on the kanji itself.
	//   (4) Punctuation or end of sentence.
	if ( isKanji ( p[0] ) ) {

		// How many characters do we have before the end, or any punctuation at all?
		unsigned nChar = 1 ;
		while ( isKanji(p[nChar]) || isHiragana(p[nChar]) || isKatakana(p[nChar]) )
			nChar ++ ;

		// Get the longest word we can find from that string.
		while ( nChar ) {
			INDEX::iterator IndexCursor = FindSymbolExact ( (wchar_t*)p, nChar ) ;
			if ( IndexCursor != Index.end() ) {
				const _IndexEntry *pIndexEntry = &(*IndexCursor) ;
				new ( &Word ) _Word ( p, nChar, pIndexEntry ) ;
				return ( Word.GetLength() ) ;
			} /* endif */
			-- nChar ;
		} /* endfor */

		// We have a kanji, and nothing matched from the dictionary.
		// Perhaps its a conjugation?  Add all valid conjugations to the dictionary so we can try again.
		bool fAddedConjugations = false ;
		INDEX::iterator IndexCursor = FindSymbolPrefix ( (wchar_t*)p, 1 ) ;
		assert ( IndexCursor != Index.end() ) ;
		const _IndexEntry *pIndexEntry = &(*IndexCursor) ;
		while ( IndexCursor != Index.end() ) {
			const _IndexEntry *pIndexEntry2 = & ( *IndexCursor ) ;
			const _DictionaryEntry *pDictionaryEntry = pIndexEntry2->GetDictionaryEntry ( ) ;
			unsigned WordClass = GetWordClass ( pDictionaryEntry->GetInfo ( ) ) ;
			if ( WordClass & ( WC_VERB_ICHIDAN | WC_VERB_GODAN | WC_VERB_GODAN_IKU | WC_VERB_GODAN_ARU | WC_VERB_KURU | WC_VERB_SURU | WC_SURU | WC_ADJ_I | WC_ADJ_NA ) ) {
				if ( pDictionaryEntry->isConjugated ( ) == false ) {
					BYTE Conjugations [MAX_CONJUGATIONS] = { 0 } ;
					((_DictionaryEntry*)pDictionaryEntry)->Conjugate ( pIndexEntry2->GetWord(), pIndexEntry2->GetKana(), Conjugations, CC_NULL ) ;
					for (auto& Symbol : *pDictionaryEntry->GetSymbols()) {
						const STRING* psSymbol = Symbol.GetSymbol();
						const STRING* psSound = Symbol.GetSound();
						const BYTE* Conjugations = Symbol.GetConjugations();
						Index.insert(_IndexEntry(psSymbol, psSound, Conjugations, pDictionaryEntry));
						if (wcscmp(psSymbol->c_str(), psSound->c_str()))
							Index.insert(_IndexEntry(psSound, psSound, Conjugations, pDictionaryEntry));
					} /* endfor */
					fAddedConjugations = true ;
				} /* endif */
			} else {
				DO_NOTHING ;
			} /* endif */
			++ IndexCursor ;
			if ( pIndexEntry->GetWord()[0] != pIndexEntry2->GetWord()[0] )
				break ;
		} /* endwhile */

		// If we updated the dictionary, return zero, forcing a new search.
		if ( fAddedConjugations )
			// Do nothing to the word, as we are not adding anything to the sentence word list this time around.
			return ( 0 ) ;

		// Add a single character word to the sentence list.  For now, its meaning is unknown.
		new ( &Word ) _Word ( p, 1 ) ;
		return ( 1 ) ;

	} else {
		// Add a single character word to the sentence list.  For now, its meaning is unknown.
		new ( &Word ) _Word ( p, 1 ) ;
		return ( 1 ) ;

	} /* endif */

}

static void ParseSentence ( FILE *Destin, const wchar_t Line[], const size_t cbLine, const unsigned Indent ) {
	fwprintf ( Destin, L"%*s%0.*s\n", Indent, L"", (int)cbLine, Line ) ;

	// Ignore empty lines.
	if ( cbLine == 0 )
		return ;

	// Remove sentence ender, if any.  Remember what it was for later.
	size_t cb = cbLine ;
	const wchar_t *pszEnder = SentenceEnders [0] ;
	for ( unsigned i=0; i<_countof(SentenceEnders); i++ ) {
		size_t cbEnder = wcslen ( SentenceEnders[i] ) ;
		if ( !memcmp ( Line+cbLine-cbEnder, SentenceEnders[i], cbEnder*sizeof(wchar_t) ) ) {
			pszEnder = SentenceEnders[i] ;
			cb -= cbEnder ;
			break ;
		} /* endif */
	} /* endfor */

	// Bite off words until we run out of sentence.
	std::list<_Word> Words ;
	wchar_t *p = (wchar_t*) Line ;
	while ( cb ) {
		_Word Word ;
		size_t cbWord = ParseWord ( Destin, p, cb, Indent+2, Word ) ;
		if ( Word.GetLength ( ) ) {
			// Add word to sentence.  It may or may not have a meaning yet.
			Words.push_back ( Word ) ;
		} /* endif */
		p += cbWord ;
		cb -= cbWord ;
	} /* endwhile */

	// TODO: Try some easy particle/copula detections.
	// (1) If we find の between two fully detected words, then it is extremely likely to be the particle.
	std::list<_Word>::iterator Cursor = Words.begin ( ) ;
	while ( Cursor != Words.end() ) {
		if ( Cursor->GetIndexEntry ( ) ) {
			std::list<_Word>::iterator Next1 = Cursor ; Next1 ++ ;
			if ( ( Next1 != Words.end ( ) ) && ( Next1->GetIndexEntry ( ) == 0 ) ) {
				std::list<_Word>::iterator Next2 = Next1 ; Next2 ++ ;
				if ( ( Next2 != Words.end ( ) ) && Next2->GetIndexEntry ( ) ) {
					if ( ( Next1->GetLength() == 1 ) && ( Next1->GetValue()[0] == L'の' ) ) {
						Next1->SetIndexEntry ( pParticleNO ) ;
					} else if ( ( Next1->GetLength() == 1 ) && ( Next1->GetValue()[0] == L'に' ) ) {
						Next1->SetIndexEntry ( pParticleNI ) ;
					} else if ( ( Next1->GetLength() == 1 ) && ( Next1->GetValue()[0] == L'を' ) ) {
						Next1->SetIndexEntry ( pParticleWO ) ;
					} else if ( ( Next1->GetLength() == 1 ) && ( Next1->GetValue()[0] == L'で' ) ) {
						Next1->SetIndexEntry ( pParticleDE ) ;
					} else {
						DO_NOTHING ;
					} /* endif */
				} /* endif */
			} /* endif */
		} /* endif */
		++ Cursor ;
	} /* endwhile */

	// Dump the sentence word list.
	for (auto& Word : Words) {
		Word.Dump(Destin, Indent + 2);
	} /* endwhile */

}

static bool FindSentence ( FILE *Destin, const wchar_t Line[], const size_t cbLine, const unsigned Indent ) {

	// Exit if nothing left to analyze.
	if ( cbLine == 0 )
		return ( true ) ;

	// Set to the start of the line.
	wchar_t *p = (wchar_t*) Line ;
	size_t cb = (size_t) cbLine ;

	// Trim leading white-space.
	while ( cb && isWhite ( p[0] ) ) ++ p, -- cb ;

	// Are we dealing with a fully quoted sentence?
	bool fFullyQuoted = false ;
	wchar_t StartFullQuote = 0, EndFullQuote = 0 ;
	for ( unsigned i=0; i<_countof(QuotePairs); i++ ) {
		if ( Line[0] == QuotePairs[i][0] ) {
			fFullyQuoted = true ;
			StartFullQuote = QuotePairs[i][0] ;
			EndFullQuote = QuotePairs[i][1] ;
			wchar_t *pEnd = p + 1 ;
			size_t cbLeft = cb - 1 ;
			while ( cbLeft && ( *pEnd != EndFullQuote ) ) {
				++ pEnd ; -- cbLeft ;
			} /* endwhile */
			assert ( cbLeft ) ; // This will only fire with malformed text.  Fix the text.
			if ( pEnd > p + 1 ) {
				fwprintf ( Destin, L"%*s%c\n", Indent, L"", StartFullQuote ) ;
				FindSentence ( Destin, p+1, pEnd-p-1, Indent+2 ) ; // Analyze what's inside the quotes.
				fwprintf ( Destin, L"%*s%c\n", Indent, L"", EndFullQuote ) ;
				return ( FindSentence ( Destin, pEnd+1, cbLeft-1, Indent ) ) ; // Analyze what's after the closing quotes.
			} /* endif */
		} /* endif */
	} /* endfor */

	// Look for a sentence ending mark, skipping anything quoted inbetween.
	wchar_t *pEnd = p ; size_t cbLeft = cb ; 
	unsigned nQuotes = 0 ; wchar_t EndingQuotes [10] ; 
	while ( cbLeft ) {

		// Look for closing quotes.
		if ( nQuotes && ( pEnd[0] == EndingQuotes[nQuotes-1] ) ) {
			-- nQuotes ; ++ pEnd ; -- cbLeft ; 
			continue ;
		} /* endif */

		// Look for opening quotes.
		unsigned i ;
		for ( i=0; i<_countof(QuotePairs); i++ ) {
			if ( pEnd[0] == QuotePairs[i][0] ) {
				EndingQuotes[nQuotes++] = QuotePairs[i][1] ;
				break ;
			} /* endif */
		} /* endfor */
		if ( i < _countof(QuotePairs) ) {
			++ pEnd ; -- cbLeft ;
			continue ;
		} /* endif */

		// If not in the middle of a quote...
		if ( nQuotes == 0 ) {
			for ( unsigned i=0; i<_countof(SentenceEnders); i++ ) {
				size_t cbEnder = wcslen(SentenceEnders[i]) ;
				if ( ( cbLeft >= cbEnder ) && !memcmp(pEnd,SentenceEnders[i],cbEnder*sizeof(wchar_t)) ) {
					// We've found the end of a sentence.
					ParseSentence ( Destin, p, pEnd - p + cbEnder, Indent+2 ) ;
					p = pEnd + cbEnder ;
					cbLeft = cbLine - ( p - Line ) ;
					return ( FindSentence ( Destin, p, cbLeft, Indent ) ) ; // Analyze what's after the closing quotes.
				} /* endif */
			} /* endfor */
		} /* endif */

		// Move to next character.
		++ pEnd ; -- cbLeft ;

	} /* endwhile */
	assert ( nQuotes == 0 ) ;
	ParseSentence ( Destin, p, cb, Indent+2 ) ;
	return ( true ) ;
}

static void ProcessLine ( FILE *Destin, const wchar_t Line [], const size_t cbLine, const unsigned nLine ) {

	// Trace where we are.
	printf ( "Line %i...\r", nLine ) ;

	// Any line starting with a space starts a paragraph.  
	// Mark it as such for reference, so we can tell later.
	wchar_t *p = (wchar_t*) Line ;
	unsigned cb = (unsigned) cbLine ;
	if ( ( p[0] == L' ' ) && ( p[1] != L' ' ) ) {
		fwprintf ( Destin, L"%c", 0x00B6 ) ;
		++ p ; -- cb ;
	} /* endif */

	// Here we process one or more sentences.  We hope they are gramatically complete...
	// A sentence ends in a period, an exclamation point, a question mark,
	//  or a group of exclamation points and question marks.
	// If a sentence starts with an opening quote of any sort, then
	//   it ends with a close quote of the same sort.
	// If an opening quote appears after the start of a sentence,
	//   then it is an embedded quotation, and the close quote does
	//   not end the sentence.
	fwprintf ( Destin, L"%0.*s\n", cb, p ) ;

	FindSentence ( Destin, p, cb, 2 ) ;
} 

int _tmain ( int argc, wchar_t* argv[] ) {

    // Set up console interrupt handler.
    SetConsoleCtrlHandler ( handler_routine, TRUE ) ;
    signal ( SIGINT, SignalHandler ) ;
    
	// TODO: Decipher the command line.
	wchar_t szSource [_MAX_PATH] ;
	wcscpy_s ( szSource, L"Text.txt" ) ;
	wchar_t szDestin [_MAX_PATH] ;
	wcscpy_s ( szDestin, L"Sentences.txt" ) ;

	// Load the dictionary.
	printf ( "Loading dictionary file.\n" ) ;
	if ( !BuildDictionary ( L"JDICT\\EDICT", false ) ) 
		return ( 1 ) ;
	printf ( "  %zi dictionary entries.\n", Dictionary.size() ) ;

	// Add some custom words.
	AddWord ( L"とらドラ",		L"とらドラ",		L"(title) Toradora" ) ;
	AddWord ( L"竹宮",			L"たけみや",		L"(name) Takemiya" ) ;
	AddWord ( L"ゆゆこ",		L"ゆゆこ",			L"(name) Yuyuko" ) ;
	AddWord ( L"ヤス",			L"ヤス",			L"(pseudonym) Yasu (Illustrator)" ) ;
	AddWord ( L"高須",			L"たかす",			L"(name) Takasu" ) ;	
	AddWord ( L"竜児",			L"りゅうじ",		L"(name) Ryuuji" ) ;
	AddWord ( L"泰子",			L"やすこ",			L"(name) Yasuko" ) ;
	AddWord ( L"逢坂",			L"あいさか",		L"(name) Aisaka" ) ;
	AddWord ( L"大河",			L"たいが",			L"(name) Taiga" ) ;
	AddWord ( L"北村",			L"きたむら",		L"(name) Kitamura" ) ;		
	AddWord ( L"祐作",			L"ゆうさく",		L"(name) Yuusaku" ) ;
	AddWord ( L"櫛枝",			L"くしえだ",		L"(name) Kushieda" ) ;		
	AddWord ( L"実乃梨",		L"みのり",			L"(name) Minori" ) ;
	AddWord ( L"川嶋",			L"かわしま",		L"(name) Kawashima" ) ;
	AddWord ( L"亜美",			L"あみ",			L"(name) Ami" ) ;
	AddWord ( L"恋ヶ窪ゆり",	L"こいがくぼゆり",	L"(name) Koigakubo Yuri" ) ;
	AddWord ( L"恋ヶ窪",		L"こいがくぼ",		L"(name) Koigakubo" ) ;
	AddWord ( L"久光",			L"のと",			L"(name) Noto" ) ;
	AddWord ( L"能登",			L"ひさみつ",		L"(name) Hisamitsu" ) ;
	AddWord ( L"木原",			L"きはら",			L"(name) Kihara" ) ;
	AddWord ( L"麻耶",			L"まや",			L"(name) Maya" ) ;
	AddWord ( L"香椎",			L"かしい",			L"(name) Kashii" ) ;	
	AddWord ( L"奈々子",		L"ななこ",			L"(name) Nanako" ) ;
	AddWord ( L"春田",			L"はるた",			L"(name) Haruta" ) ;
	AddWord ( L"浩次",			L"こうじ",			L"(name) Kouji" ) ;
	AddWord ( L"亮輔",			L"りょうすけ",		L"(name) Ryousuke" ) ;
	AddWord ( L"濱田",			L"はまだ",			L"(name) Hamada" ) ;
	AddWord ( L"瀬奈",			L"せな",			L"(name) Sena" ) ;
	AddWord ( L"富家",			L"とみや",			L"(name) Tomiya" ) ;
	AddWord ( L"幸太",			L"こた",			L"(name) Kota" ) ;
	AddWord ( L"狩野",			L"かの",			L"(name) Kano" ) ;
	AddWord ( L"多田",			L"ただ",			L"(name) Tada" ) ;
	AddWord ( L"万里",			L"ばんり",			L"(name) Banri" ) ;
	AddWord ( L"加賀",			L"かが",			L"(name) Kaga" ) ;
	AddWord ( L"香子",			L"こうこ",			L"(name) Kouko" ) ;
	AddWord ( L"柳澤",			L"やなぎさわ",		L"(name) Yanagisawa" ) ;
	AddWord ( L"澤",			L"さわ",			L"(n) swamp" ) ;
	AddWord ( L"ヤナっさん",	L"ヤナっさん",		L"(name) Yana-ssan (Banri's name for Mitsuo)" ) ;
	AddWord ( L"光央",			L"みつお",			L"(name) Mitsuo" ) ;
	AddWord ( L"林田",			L"はやしだ",		L"(name) Hayashita" ) ;
	AddWord ( L"奈々",			L"なな",			L"(name) Nana" ) ;
	AddWord ( L"岡",			L"おか",			L"(name) Oka" ) ;
	AddWord ( L"千波",			L"ちなみ",			L"(name) Chinami" ) ;
	AddWord ( L"電撃文庫",		L"でんげきぶんこ",	L"(name) Dengeki Bunko" ) ;
	AddWord ( L"フライングシャイン", L"フライングシャイン", L"(name) FlyingShine (a company)" ) ;
	AddWord ( L"毘沙門夭国",	L"びしゃもんてんごく",L"(name) Bishamontengoku, a fictional hostess bar in Toradora!" ) ;
	AddWord ( L"表参道",		L"おもてさんどう",  L"(place) Omotesandou, an upscale district of Tokyo." ) ;
	AddWord ( L"麻布",			L"あざぶ",			L"(place) Azabu, an area within Minato, Tokyo." ) ;
	AddWord ( L"白金",			L"しろかね",        L"(place) Shirokane, a district of Minato, Tokyo." ) ;
	AddWord ( L"手乗りタイガー",L"てのりたいが",    L"(name) Palmtop Tiger, Aisaka Taiga's nickname" ) ;
	AddWord ( L"ばかちー",		L"ばかちい",		L"(name) Taiga's derogatory nickname for Ami: Stupid Chihuahua" ) ;
	AddWord ( L"摑む",			L"つかむ",			L"(v5m,vt) box one's ears; slap/(P)/", true ) ;
	AddWord ( L"搔く",			L"かく",			L"(v5k) to scratch", true ) ;
	AddWord ( L"引き剝がす",	L"ひきはがす",		L"(v5s) to tear off", true ) ;
	AddWord ( L"剝がす",		L"はがす",			L"(v5s) peel/peel off/shell/strip", true ) ;
	AddWord ( L"噓",			L"うそ",			L"(n) lie; praise; flatter; hiss; exhale; deep sigh; blow out;" ) ;

	// Build the index.
	printf ( "Building word/phrase index.\n" ) ;
	if ( BuildIndex ( ) == false )
		return ( 1 ) ;
	printf ( "  %zi index entries.\n", Index.size() ) ;

	// Some special lookups.
	pParticleDE = FindParticle ( L"で", 1 ) ;
	pParticleGA = FindParticle ( L"が", 1 ) ;
	pParticleKA = FindParticle ( L"か", 1 ) ;
	pParticleNI = FindParticle ( L"に", 1 ) ;
	pParticleNO = FindParticle ( L"の", 1 ) ;
	pParticleWA = FindParticle ( L"は", 1 ) ;
	pParticleWO = FindParticle ( L"を", 1 ) ;

	// Open the source file.
	printf ( "Processing document.\n" ) ;
	FILE *Source ; errno_t Error = _wfopen_s ( &Source, szSource, L"rb" ) ;
	if ( Error ) {
		fwprintf ( stderr, L"ERROR: Unable to open source file '%ls'.\n", szSource ) ;
		return ( 1 ) ;
	} /* endif */

	// Validate the code type tag.  Must be UNICODE.
	unsigned short Tag ;
	if ( ( fread ( &Tag, sizeof(Tag), 1, Source ) < 1 ) || ( Tag != 0xFEFF ) ) {
		fwprintf ( stderr, L"ERROR: Invalid source file '%ls'.\n", szSource ) ;
		fclose ( Source ) ;
		return ( 1 ) ;
	} /* endif */

	// Open the destination file.
	FILE *Destin ; Error = _wfopen_s ( &Destin, szDestin, L"wt,ccs=UNICODE" ) ;
	if ( Error ) {
		fwprintf ( stderr, L"ERROR: Unable to open destination file '%ls'.\n", szDestin ) ;
		fclose ( Source ) ;
		return ( 1 ) ;
	} /* endif */

	wchar_t Line [0x1000] ;  size_t cbLine = 0 ; unsigned nLines = 0 ;
	wchar_t Buffer [0x1000] ; wchar_t *pBuffer = Buffer ;
	size_t cbBuffer = fread ( Buffer, sizeof(Buffer[0]), _countof(Buffer), Source ) ;
	while ( !fAbort && cbBuffer  ) {

		// Add character to output line buffer.
		Line[cbLine++] = *pBuffer++ ; 

		// If we've completed a line, flush it now.
		if ( ( Line[cbLine-2] == 0x0D ) && ( Line[cbLine-1] == 0x0A ) ) {
			cbLine -= 2 ;

			// If the line is blank, skip it.
			if ( !cbLine || ( ( cbLine == 2 ) && ( Line[0] == 0x0D ) && ( Line[1] == 0x0A ) ) ) {
				fwprintf ( Destin, L"\n" ) ;
				cbLine = 0 ;
				++ nLines ;

			// Otherwise, do it.
			} else {
				Line[cbLine] = 0 ;
				ProcessLine ( Destin, Line, cbLine, ++nLines ) ;
				cbLine = 0 ;

			} /* endif */

		} /* endif */

		// Get more data if we need to.
		if ( pBuffer >= Buffer + cbBuffer ) {
			pBuffer = Buffer ;
			cbBuffer = fread ( Buffer, sizeof(Buffer[0]), _countof(Buffer), Source ) ;
		} /* endif */

	} /* endwhile */

	// If the line buffer still has something in it, write it out now.
	if ( !fAbort && cbLine ) {

		// If the line is blank, skip it.
		if ( ( Line[cbLine-2] == 0x0D ) && ( Line[cbLine-1] == 0x0A ) )
			cbLine -= 2 ;
		if ( !cbLine || ( ( cbLine == 2 ) && ( Line[0] == 0x0D ) && ( Line[1] == 0x0A ) ) ) {
			fwprintf ( Destin, L"\n" ) ;
			cbLine = 0 ;
			++ nLines ;

		// Otherwise, do it.
		} else {
			Line[cbLine] = 0 ;
			ProcessLine ( Destin, Line, cbLine, ++nLines ) ;
			cbLine = 0 ;

		} /* endif */

	} /* endif */

	// Clean up.
	fclose ( Destin ) ;
	fclose ( Source ) ;

	// Done OK.
	ExitProcess ( 0 ) ;
}
