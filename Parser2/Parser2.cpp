// Parser2.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <assert.h>
#include <signal.h>
#include <stdlib.h>

#include <list>
#include <map>
#include <set>
#include <string>

#include "..\JIS0208\JIS0208.h"

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

#define DO_NOTHING { int x = 0 ; x ++ ; x -- ; } // Macro to be able to set a breakpoint on.

enum class _WordClass { 
	WC_NULL, 
	WC_ICHIDAN, 
	WC_GODAN, 
	WC_IKU, 
	WC_ARU, 
	WC_SURU, 
	WC_KURU,
	WC_NOUN,
	WC_ADJ_I,
	WC_ADJ_NA,		// Special marker: If word followed by な, then we gather it to the word.
	WC_ADV_TO,		// Special marker: If word followed by と, then we gather it to the word.
	WC_VERB_SURU,	// Special marker: If word followed by conjugate of する, then we gather it to the word.
	WC_PREFERRED
} ;

enum class _RootClass {
	RC_NULL,
	RC_ICHIDAN,
	RC_GODAN_A,
	RC_GODAN_I,
	RC_GODAN_U,
	RC_GODAN_E,
	RC_GODAN_O,
	RC_GODAN_NULL,
	RC_IKU,
	RC_ARU,
	RC_SURU,
	RC_KURU,
	RC_NOUN,
	RC_ADJ_I,
	RC_MAX
} ;

_RootClass& operator++ ( _RootClass &obj ) {
	int Temp = (int) obj ;
	Temp ++ ;
	if ( Temp > (int) _RootClass::RC_MAX )
		Temp = (int) _RootClass::RC_NULL ;
	obj = (_RootClass) Temp ;
	return ( obj ) ;
} /* endmethod */
_RootClass& operator++ ( _RootClass &obj, int ) {
	int Temp = (int) obj ;
	Temp ++ ;
	if ( Temp > (int)_RootClass::RC_MAX )
		Temp = (int)_RootClass::RC_NULL ;
	obj = (_RootClass) Temp ;
	return ( obj ) ;
} /* endmethod */

enum class _KanaRow { ROW_A, ROW_I, ROW_U, ROW_E, ROW_O, ROW_NULL } ;
_KanaRow& operator++ ( _KanaRow &obj ) {
	int Temp = (int) obj ;
	Temp ++ ;
	if ( Temp > (int) _KanaRow::ROW_NULL )
		Temp = (int) _KanaRow::ROW_A ;
	obj = (_KanaRow) Temp ;
	return ( obj ) ;
} /* endmethod */
_KanaRow& operator++ ( _KanaRow &obj, int ) {
	int Temp = (int) obj ;
	Temp ++ ;
	if ( Temp > (int) _KanaRow::ROW_NULL )
		Temp = (int) _KanaRow::ROW_A ;
	obj = (_KanaRow) Temp ;
	return ( obj ) ;
} /* endmethod */

enum class _KanaColumn { COL_U, COL_K, COL_G, COL_S, COL_T, COL_N, COL_B, COL_M, COL_R, COL_NULL } ;
_KanaColumn& operator++ ( _KanaColumn &obj ) {
	int Temp = (int) obj ;
	Temp ++ ;
	if ( Temp > (int) _KanaColumn::COL_NULL )
		Temp = (int)_KanaColumn::COL_U ;
	obj = (_KanaColumn) Temp ;
	return ( obj ) ;
} /* endmethod */
_KanaColumn& operator++ ( _KanaColumn &obj, int ) {
	int Temp = (int) obj ;
	Temp ++ ;
	if ( Temp > (int)_KanaColumn::COL_NULL )
		Temp = (int)_KanaColumn::COL_U ;
	obj = (_KanaColumn) Temp ;
	return ( obj ) ;
} /* endmethod */

static wchar_t szVerb1Endings [(size_t)_KanaRow::ROW_NULL] [(size_t)_KanaColumn::COL_NULL] = {
	{ L'わ', L'か', L'が', L'さ', L'た', L'な', L'ば', L'ま', L'ら' },
	{ L'い', L'き', L'ぎ', L'し', L'ち', L'に', L'び', L'み', L'り' },
	{ L'う', L'く', L'ぐ', L'す', L'つ', L'ぬ', L'ぶ', L'む', L'る' },
	{ L'え', L'け', L'げ', L'せ', L'て', L'ね', L'べ', L'め', L'れ' },
	{ L'お', L'こ', L'ご', L'そ', L'と', L'の', L'ぼ', L'も', L'ろ' },
} ;

static wchar_t *szVerb1PositivePastSuffix [(size_t)_KanaColumn::COL_NULL] = {
	L"った", L"いた", L"いだ", L"した", L"った", L"んだ", L"んだ", L"んだ", L"った", 
} ;

static wchar_t *szVerb1ConjunctiveSuffix [(size_t)_KanaColumn::COL_NULL] = {
	L"って", L"いて", L"いで", L"して", L"って", L"んで", L"んで", L"んで", L"って", 
} ;

inline unsigned short SwapWord ( unsigned short Word ) {
	unsigned char *p = (unsigned char*) &Word ;
	return ( ( p[0] << 8 ) + p[1] ) ;
}

typedef std::basic_string<wchar_t> STRING ;

class _SymbolSound {
	STRING Symbol ;
	STRING Sound ;
	_RootClass RootClass ;
public:
	_SymbolSound ( wchar_t *szSymbol, wchar_t *szSound, _RootClass rc ) : Symbol(szSymbol), Sound(szSound), RootClass(rc) { 
	} /* endmethod */
	_SymbolSound ( STRING sSymbol, STRING sSound, _RootClass rc ) : Symbol(sSymbol), Sound(sSound), RootClass(rc) {
	} /* endmethod */
	bool operator< ( const _SymbolSound &obj ) const {
		if ( (unsigned)RootClass > (unsigned)obj.RootClass )
			return ( false ) ;
		if ( (unsigned)RootClass < (unsigned)obj.RootClass )
			return ( true ) ;
		int Result = Symbol.compare ( obj.Symbol ) ;
		if ( Result > 0 )
			return ( false ) ;
		if ( Result < 0 )
			return ( true ) ;
		Result = Sound.compare ( obj.Sound ) ;
		if ( Result > 0 )
			return ( false ) ;
		if ( Result < 0 )
			return ( true ) ;
		return ( false ) ;
	} /* endmethod */
	const wchar_t *GetSymbol ( ) const { return ( Symbol.c_str() ) ; }
	const wchar_t *GetSound ( ) const { return ( Sound.c_str() ) ; }
	const _RootClass GetRootClass ( ) const { return ( RootClass ) ; }
} ;

typedef std::set<_SymbolSound> SYMBOL_SOUND_SET ;

static struct _Token {
	const wchar_t *pszToken ;
	const size_t cbToken ;
	const _WordClass WordClass ;
	_Token ( const wchar_t *psz, const _WordClass wc=_WordClass::WC_NULL ) : pszToken(psz), cbToken(wcslen(pszToken)), WordClass(wc) {
	} /* endmethod */
} KnownTokens [] = {
	_Token ( L"adj-na", _WordClass::WC_ADJ_NA ),// na-Adjective (without na may be another part of speech)
	_Token ( L"adv-to", _WordClass::WC_ADV_TO ),// Adverb with 'to' particle
	_Token ( L"n-adv", _WordClass::WC_NOUN ),	// Adverbial noun
	_Token ( L"adj-i", _WordClass::WC_ADJ_I ),	// i-Adjective
	_Token ( L"v5aru", _WordClass::WC_ARU ),	// Godan Verb - -aru special class
	_Token ( L"v5k-s", _WordClass::WC_IKU ),	// Godan Verb - iku/yuku special class
	_Token ( L"vs-s", _WordClass::WC_SURU ),	// Suru verb - special class
	_Token ( L"v5b", _WordClass::WC_GODAN ),	// Godan Verb
	_Token ( L"v5g", _WordClass::WC_GODAN ),	// Godan Verb
	_Token ( L"v5k", _WordClass::WC_GODAN ),	// Godan Verb
	_Token ( L"v5m", _WordClass::WC_GODAN ),	// Godan Verb
	_Token ( L"v5n", _WordClass::WC_GODAN ),	// Godan Verb
	_Token ( L"v5r", _WordClass::WC_GODAN ),	// Godan Verb
	_Token ( L"v5s", _WordClass::WC_GODAN ),	// Godan Verb
	_Token ( L"v5t", _WordClass::WC_GODAN ),	// Godan Verb
	_Token ( L"v5u", _WordClass::WC_GODAN ),	// Godan Verb
	_Token ( L"v5z", _WordClass::WC_GODAN ),	// Godan Verb
	_Token ( L"v5", _WordClass::WC_GODAN ),		// Godan Verb
	_Token ( L"v1", _WordClass::WC_ICHIDAN ),	// Ichidan Verb
	_Token ( L"vk", _WordClass::WC_KURU ),		// Kuru
	_Token ( L"vs", _WordClass::WC_VERB_SURU ), // Verb when used with suru
	_Token ( L"n", _WordClass::WC_NOUN ),		// Noun
	_Token ( L"P", _WordClass::WC_PREFERRED ),	// Special flag: This definition is popular or preferred.
} ;

class _DictionaryEntry {
	STRING Info ;
	SYMBOL_SOUND_SET SymbolSounds ;
	_WordClass WordClass ;
	_KanaColumn KanaColumn ; // Used only for Godan verbs.
	bool fPreferred, fAdjectiveNA, fAdverbTO, fVerbSURU ;

	void CheckToken ( wchar_t *szToken ) {
		size_t cbToken = wcslen(szToken) ;
		if ( szToken[0] == L' ' )
			return ;
		if ( cbToken > 8 ) 
			return ;
		for ( unsigned i=0; i<_countof(KnownTokens); i++ ) {
			if ( cbToken == KnownTokens[i].cbToken ) {
				if ( !wcscmp ( szToken, KnownTokens[i].pszToken ) ) {
					if ( KnownTokens[i].WordClass == _WordClass::WC_PREFERRED ) {
						fPreferred = true ;
					} else if ( KnownTokens[i].WordClass == _WordClass::WC_ADJ_NA ) {
						fAdjectiveNA = true ;
					} else if ( KnownTokens[i].WordClass == _WordClass::WC_ADV_TO ) {
						fAdverbTO = true ;
					} else if ( KnownTokens[i].WordClass == _WordClass::WC_VERB_SURU ) {
						fVerbSURU = true ;
					} else if ( KnownTokens[i].WordClass != _WordClass::WC_NULL ) {
						// Special case: When a verb is declared to be both ICHIDAN and GODAN, ICHIDAN wins.
						// As of 2011/02/02, there were two such verbs in EDICT.
						if ( ( WordClass == _WordClass::WC_GODAN ) && ( KnownTokens[i].WordClass == _WordClass::WC_ICHIDAN ) )
							WordClass = _WordClass::WC_ICHIDAN ;
						// Set the word class only once.  First type wins.
						if (WordClass == _WordClass::WC_NULL) {
							WordClass = KnownTokens[i].WordClass;
						} /* endif */
						// And if we get a second contender, complain.
						// As of 2011/02/02, we only get two of these: godans that are sometimes considered nouns.
						else if ( WordClass != KnownTokens[i].WordClass )
							DO_NOTHING ;
					} /* endif */
					return ;
				} /* endif */
			} /* endif */
		} /* endfor */
		// wprintf ( L"Unrecognized token: %s\n", szToken ) ;
	} /* endmethod */

	void ParseInfo ( ) {
		// Parse the information to determine the verb class or conjugation type.
		wchar_t *p = (wchar_t*) Info.c_str ( ) ;
		bool fParenthesis = false ;
		while ( *p ) {
			if ( fParenthesis ) {
				wchar_t Token [22] ; unsigned cbToken = 0 ;
				while ( *p ) {
					if ( *p == L')' ) {
						Token[cbToken] = 0 ; 
						CheckToken ( Token ) ;
						cbToken = 0 ; ++ p ;
						fParenthesis = false ; 
						break ;
					} else if ( *p == L',' ) {
						Token[cbToken] = 0 ; 
						CheckToken ( Token ) ;
						cbToken = 0 ; ++ p ;
					} else if ( cbToken < _countof(Token)-1 ) {
						Token[cbToken++] = *p++ ;
					} else {
						++ p ;
					} /* endif */
				} /* endwhile */
			} else if ( *p == '(' ) {
				fParenthesis = true ;
				++ p ;
			} else {
				++ p ;
			} /* endif */
		} /* endwhile */
	} /* endmethod */

public:
	_DictionaryEntry ( STRING sInfo ) : Info(sInfo), WordClass(_WordClass::WC_NULL), KanaColumn(_KanaColumn::COL_NULL), fPreferred(false), fAdjectiveNA(false), fAdverbTO(false), fVerbSURU(false) {
		ParseInfo ( ) ;
	} /* endmethod */
	_DictionaryEntry ( ) : WordClass(_WordClass::WC_NULL), KanaColumn(_KanaColumn::COL_NULL), fPreferred(false), fAdjectiveNA(false), fAdverbTO(false), fVerbSURU(false) {
	} /* endmethod */
	const wchar_t *GetInfo() const { return (Info.c_str()); }
	const SYMBOL_SOUND_SET* GetSymbols() const { return (&SymbolSounds); }
	const _WordClass GetWordClass() { return (WordClass); }
	void SetKanaColumn(_KanaColumn c) { KanaColumn = c; }
	const _KanaColumn GetKanaColumn() const { return (KanaColumn); }
	const bool isPreferred ( ) const { return ( fPreferred ) ; }
	const bool isAdjectiveNA() { return (fAdjectiveNA); }
	const bool isAdverbTO() { return (fAdverbTO); }
	const bool isVerbSURU() { return (fVerbSURU); }
	void AddSymbol ( STRING sSymbol, STRING sSound, _RootClass RootClass ) {
		SymbolSounds.insert ( _SymbolSound ( sSymbol, sSound, RootClass ) ) ;
	} /* endmethod */
	void AddSound ( STRING sSound, _RootClass RootClass ) {
		SymbolSounds.insert ( _SymbolSound ( sSound, sSound, RootClass ) ) ;
	} /* endmethod */
} ;

typedef std::map<STRING,_DictionaryEntry> DICTIONARY ; // Indexed by definition content.

class _IndexEntry {
	STRING Word ;	// In any mixture of Kanji, Hiragana, Katakana or Romaji.
	STRING Kana ;	// In Hiragana.
	_DictionaryEntry *pDictionaryEntry ;
public:
	_IndexEntry ( wchar_t *szWord, wchar_t *szKana, _DictionaryEntry *pEntry ) : Word(szWord), Kana(szKana), pDictionaryEntry(pEntry) { 
		assert ( pDictionaryEntry ) ;
	} /* endmethod */
	bool operator< ( const _IndexEntry &obj ) const {
		size_t cbCompare = __min ( Word.size(), obj.Word.size() ) ;
		int Result = memcmp ( Word.c_str(), obj.Word.c_str(), cbCompare*sizeof(wchar_t) ) ;
		const wchar_t *pHere = Word.c_str() ;
		const wchar_t *pThere = obj.Word.c_str() ;
		for ( size_t i=0; i<cbCompare; i++, pHere++, pThere++ ) {
			int Result = *pHere - *pThere ;
			if ( Result < 0 )
				return ( true ) ;
			if ( Result > 0 )
				return ( false ) ;
		} /* endif */
		Result = (int) ( Word.size() - obj.Word.size() ) ;
		if ( Result > 0 ) 
			return ( true ) ;
		if ( Result < 0 )
			return ( false ) ;
		if ( pDictionaryEntry->isPreferred ( ) && obj.pDictionaryEntry->isPreferred ( ) )
			return ( false ) ;
		if ( pDictionaryEntry->isPreferred ( ) )
			return ( true ) ;
		return ( false ) ;
	} /* endmethod */
	const wchar_t *GetWord ( ) const { return ( (wchar_t*)Word.c_str() ) ; }
	const wchar_t *GetKana ( ) const { return ( (wchar_t*)Kana.c_str() ) ; }
	const _DictionaryEntry *GetDictionaryEntry ( ) const { return ( pDictionaryEntry ) ; }
} ;

typedef std::multiset<_IndexEntry> INDEX ;

typedef std::list<const _IndexEntry*> DEFINITIONS ; 

//
// Conjugation Helper.  Take a word stem and test text against it and its conjugations.
//

enum class _Conjugation {
	CONJ_NONE = 0,
	CONJ_PLAIN_AFFIRMATIVE_INDICATIVE,
	CONJ_POLITE_AFFIRMATIVE_INDICATIVE,
	CONJ_PLAIN_NEGATIVE_INDICATIVE,
	CONJ_POLITE_NEGATIVE_INDICATIVE,
	CONJ_PLAIN_AFFIRMATIVE_PAST_INDICATIVE,
	CONJ_POLITE_AFFIRMATIVE_PAST_INDICATIVE,
	CONJ_PLAIN_NEGATIVE_PAST_INDICATIVE,
	CONJ_POLITE_NEGATIVE_PAST_INDICATIVE,
	CONJ_PLAIN_AFFIRMATIVE_VOLITIONAL,
	CONJ_POLITE_AFFIRMATIVE_VOLITIONAL,
	CONJ_PLAIN_NEGATIVE_VOLITIONAL,
	CONJ_POLITE_NEGATIVE_VOLITIONAL,
	CONJ_PLAIN_AFFIRMATIVE_PRESUMPTIVE,
	CONJ_POLITE_AFFIRMATIVE_PRESUMPTIVE,
	CONJ_PLAIN_NEGATIVE_PRESUMPTIVE,
	CONJ_POLITE_NEGATIVE_PRESUMPTIVE,
	CONJ_PLAIN_AFFIRMATIVE_PAST_PRESUMPTIVE,
	CONJ_POLITE_AFFIRMATIVE_PAST_PRESUMPTIVE,
	CONJ_PLAIN_NEGATIVE_PAST_PRESUMPTIVE,
	CONJ_POLITE_NEGATIVE_PAST_PRESUMPTIVE,
	CONJ_PLAIN_AFFIRMATIVE_CONTINUATIVE,
	CONJ_POLITE_AFFIRMATIVE_CONTINUATIVE,
	CONJ_PLAIN_NEGATIVE_CONTINUATIVE,
	CONJ_POLITE_NEGATIVE_CONTINUATIVE,
	CONJ_PLAIN_AFFIRMATIVE_CONTINUATIVE_RENYOUKEI,
	CONJ_ABRUPT_AFFIRMATIVE_IMPERATIVE1,
	CONJ_ABRUPT_AFFIRMATIVE_IMPERATIVE2,
	CONJ_PLAIN_AFFIRMATIVE_IMPERATIVE,
	CONJ_ABRUPT_NEGATIVE_IMPERATIVE,
	CONJ_PLAIN_NEGATIVE_IMPERATIVE,
	CONJ_POLITE_AFFIRMATIVE_REQUEST1,
	CONJ_POLITE_AFFIRMATIVE_REQUEST2,
	CONJ_HONORIFIC_AFFIRMATIVE_REQUEST,
	CONJ_POLITE_NEGATIVE_REQUEST,
	CONJ_HONORIFIC_NEGATIVE_REQUEST,
	CONJ_AFFIRMATIVE_PROVISIONAL,
	CONJ_NEGATIVE_PROVISIONAL,
	CONJ_PLAIN_AFFIRMATIVE_CONDITIONAL,
	CONJ_POLITE_AFFIRMATIVE_CONDITIONAL,
	CONJ_PLAIN_NEGATIVE_CONDITIONAL,
	CONJ_POLITE_NEGATIVE_CONDITIONAL,
	CONJ_PLAIN_AFFIRMATIVE_ALTERNATIVE,
	CONJ_POLITE_AFFIRMATIVE_ALTERNATIVE,
	CONJ_PLAIN_NEGATIVE_ALTERNATIVE,
	CONJ_POLITE_NEGATIVE_ALTERNATIVE,
	CONJ_POTENTIAL,
	CONJ_MAX
} ;

static wchar_t *szConjugationNames [(size_t)_Conjugation::CONJ_MAX] = {
	L"",
	L"Plain Affirmative Indicative",
	L"Polite Affirmative Indicative",
	L"Plain Negative Indicative",
	L"Polite Negative Indicative",
	L"Plain Affirmative Past Indicative",
	L"Polite Affirmative Past Indicative",
	L"Plain Negative Past Indicative",
	L"Polite Negative Past Indicative",
	L"Plain Affirmative Volitional",
	L"Polite Affirmative Volitional",
	L"Plain Negative Volitional",
	L"Polite Negative Volitional",
	L"Plain Affirmative Presumptive",
	L"Polite Affirmative Presumptive",
	L"Plain Negative Presumptive",
	L"Polite Negative Presumptive",
	L"Plain Affirmative Past Presumptive",
	L"Polite Affirmative Past Presumptive",
	L"Plain Negative Past Presumptive",
	L"Polite Negative Past Presumptive",
	L"Plain Affirmative Continuative",
	L"Polite Affirmative Continuative",
	L"Plain Negative Continuative",
	L"Polite Negative Continuative",
	L"Plain Affirmative Continuative (Renyoukei)",
	L"Abrupt Affirmative Imperative",
	L"Abrupt Affirmative Imperative (yo)",
	L"Plain Affirmative Imperative",
	L"Abrupt Negative Imperative",
	L"Plain Negative Imperative",
	L"Polite Affirmative Request (1)",
	L"Polite Affirmative Request (2)",
	L"Honorific Affirmative Request",
	L"Polite Negative Request",
	L"Honorific Negative Request",
	L"Affirmative Provisional",
	L"Negative Provisional",
	L"Plain Affirmative Conditional",
	L"Polite Affirmative Conditional",
	L"Plain Negative Conditional",
	L"Polite Negative Conditional",
	L"Plain Affirmative Alternative",
	L"Polite Affirmative Alternative",
	L"Plain Negative Alternative",
	L"Polite Negative Alternative",
} ;

static wchar_t *GetConjugationName ( _Conjugation Conjugation ) {
	assert ( ( Conjugation >= _Conjugation::CONJ_NONE ) && ( Conjugation < _Conjugation::CONJ_MAX ) ) ;
	return ( szConjugationNames [ (size_t) Conjugation ] ) ;
}

class _ConjugatableStem {
protected:
	const _DictionaryEntry *pDictionaryEntry ;
	_SymbolSound *pSymbolSound ;
	static bool hasExtension ( wchar_t *pBase, size_t cbBase, size_t cbRemainder, wchar_t *pExtension, size_t cbExtension ) {
		wchar_t *pNext = pBase + cbBase ;
		if ( cbRemainder < cbExtension )
			return ( false ) ;
		return ( !memcmp ( pNext, pExtension, cbExtension*sizeof(wchar_t) ) ) ;
	} /* endmethod */
public:
	_ConjugatableStem ( const _DictionaryEntry *pde, wchar_t *p, size_t cb ) : pDictionaryEntry(pde), pSymbolSound(0) {
		const SYMBOL_SOUND_SET *pSoundSet = pDictionaryEntry->GetSymbols ( ) ;
		SYMBOL_SOUND_SET::iterator Cursor = pSoundSet->begin ( ) ;
		while ( Cursor != pSoundSet->end ( ) ) {
			const wchar_t *pText = (*Cursor).GetSymbol ( ) ;
			if ( !memcmp ( pText, p, cb*sizeof(wchar_t) ) ) 
				break ;
			pText = (*Cursor).GetSound ( ) ;
			if ( !memcmp ( pText, p, cb*sizeof(wchar_t) ) ) 
				break ;
			++ Cursor ;
		} /* endwhile */
		assert ( Cursor != pSoundSet->end() ) ;
		pSymbolSound = (_SymbolSound*) &(*Cursor) ;
	} /* endmethod */
	bool TestConjugations ( wchar_t *pBase, size_t cbBase, size_t cbRemainder, _Conjugation &Conjugation, size_t &cbTotal ) ;
} ;

class _IchidanVerb : public _ConjugatableStem {
public:
	bool Test ( wchar_t *pBase, size_t cbBase, size_t cbRemainder, _Conjugation &Conjugation, size_t &cbTotal ) {
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"なかったでしょう", 8 ) ) {
			Conjugation = _Conjugation::CONJ_POLITE_NEGATIVE_PAST_PRESUMPTIVE ;
			cbTotal = cbBase + 8 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ないでください", 8 ) ) {
			Conjugation = _Conjugation::CONJ_POLITE_NEGATIVE_REQUEST ;
			cbTotal = cbBase + 8 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"なかっただろう", 7 ) ) {
			Conjugation = _Conjugation::CONJ_PLAIN_NEGATIVE_PAST_PRESUMPTIVE ;
			cbTotal = cbBase + 7 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ませんでしたら", 7 ) ) {
			Conjugation = _Conjugation::CONJ_POLITE_NEGATIVE_CONDITIONAL ;
			cbTotal = cbBase + 7 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ませんでしたり", 7 ) ) {
			Conjugation = _Conjugation::CONJ_POLITE_NEGATIVE_ALTERNATIVE ;
			cbTotal = cbBase + 7 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ませんでした", 6 ) ) {
			Conjugation = _Conjugation::CONJ_POLITE_NEGATIVE_PAST_INDICATIVE ;
			cbTotal = cbBase + 6 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ないでしょう", 6 ) ) {
			Conjugation = _Conjugation::CONJ_POLITE_NEGATIVE_PRESUMPTIVE ;
			cbTotal = cbBase + 6 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"なさいますな", 6 ) ) {
			Conjugation = _Conjugation::CONJ_HONORIFIC_NEGATIVE_REQUEST ;
			cbTotal = cbBase + 6 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"るでしょう", 5 ) ) {
			Conjugation = _Conjugation::CONJ_POLITE_AFFIRMATIVE_PRESUMPTIVE ;
			cbTotal = cbBase + 5 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"たでしょう", 5 ) ) {
			Conjugation = _Conjugation::CONJ_POLITE_AFFIRMATIVE_PAST_PRESUMPTIVE ;
			cbTotal = cbBase + 5 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ないだろう", 5 ) ) {
			Conjugation = _Conjugation::CONJ_PLAIN_NEGATIVE_PRESUMPTIVE ;
			cbTotal = cbBase + 5 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"てください", 5 ) ) {
			Conjugation = _Conjugation::CONJ_POLITE_AFFIRMATIVE_REQUEST1 ;
			cbTotal = cbBase + 5 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"なさいませ", 5 ) ) {
			Conjugation = _Conjugation::CONJ_HONORIFIC_AFFIRMATIVE_REQUEST ;
			cbTotal = cbBase + 5 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"なかったら", 5 ) ) {
			Conjugation = _Conjugation::CONJ_PLAIN_NEGATIVE_CONDITIONAL ;
			cbTotal = cbBase + 5 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"なかった", 4 ) ) {
			Conjugation = _Conjugation::CONJ_PLAIN_NEGATIVE_PAST_INDICATIVE ;
			cbTotal = cbBase + 4 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ませんで", 4 ) ) {
			Conjugation = _Conjugation::CONJ_POLITE_NEGATIVE_CONTINUATIVE ;
			cbTotal = cbBase + 4 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ましょう", 4 ) ) {
			Conjugation = _Conjugation::CONJ_POLITE_AFFIRMATIVE_VOLITIONAL ;
			cbTotal = cbBase + 4 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ますまい", 4 ) ) {
			Conjugation = _Conjugation::CONJ_POLITE_NEGATIVE_VOLITIONAL ;
			cbTotal = cbBase + 4 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"るだろう", 4 ) ) {
			Conjugation = _Conjugation::CONJ_PLAIN_AFFIRMATIVE_PRESUMPTIVE ;
			cbTotal = cbBase + 4 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ただろう", 4 ) ) {
			Conjugation = _Conjugation::CONJ_PLAIN_AFFIRMATIVE_PAST_PRESUMPTIVE ;
			cbTotal = cbBase + 4 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"なさるな", 4 ) ) {
			Conjugation = _Conjugation::CONJ_PLAIN_NEGATIVE_IMPERATIVE ;
			cbTotal = cbBase + 4 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"なければ", 4 ) ) {
			Conjugation = _Conjugation::CONJ_NEGATIVE_PROVISIONAL ;
			cbTotal = cbBase + 4 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ましたら", 4 ) ) {
			Conjugation = _Conjugation::CONJ_POLITE_AFFIRMATIVE_CONDITIONAL ;
			cbTotal = cbBase + 4 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ましたり", 4 ) ) {
			Conjugation = _Conjugation::CONJ_POLITE_AFFIRMATIVE_ALTERNATIVE ;
			cbTotal = cbBase + 4 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ません", 3 ) ) {
			Conjugation = _Conjugation::CONJ_POLITE_NEGATIVE_INDICATIVE ;
			cbTotal = cbBase + 3 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ました", 3 ) ) {
			Conjugation = _Conjugation::CONJ_POLITE_AFFIRMATIVE_PAST_INDICATIVE ;
			cbTotal = cbBase + 3 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"まして", 3 ) ) {
			Conjugation = _Conjugation::CONJ_POLITE_AFFIRMATIVE_CONTINUATIVE ;
			cbTotal = cbBase + 3 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ないで", 3 ) || hasExtension ( pBase, cbBase, cbRemainder, L"なくて", 3 ) ) {
			Conjugation = _Conjugation::CONJ_PLAIN_NEGATIVE_CONTINUATIVE ;
			cbTotal = cbBase + 3 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"なさい", 3 ) ) {
			Conjugation = _Conjugation::CONJ_PLAIN_AFFIRMATIVE_IMPERATIVE ;
			cbTotal = cbBase + 3 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"られる", 3 ) ) {
			// TODO: This may be conjugated as an Ichidan root.
			Conjugation = _Conjugation::CONJ_POTENTIAL ;
			cbTotal = cbBase + 3 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ない", 2 ) ) {
			// TODO: This may be conjugated as an adjective.
			Conjugation = _Conjugation::CONJ_PLAIN_NEGATIVE_INDICATIVE ;
			cbTotal = cbBase + 2 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ます", 2 ) ) {
			Conjugation = _Conjugation::CONJ_POLITE_AFFIRMATIVE_INDICATIVE ;
			cbTotal = cbBase + 2 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"よう", 2 ) ) {
			Conjugation = _Conjugation::CONJ_PLAIN_AFFIRMATIVE_VOLITIONAL ;
			cbTotal = cbBase + 2 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"まい", 2 ) ) {
			Conjugation = _Conjugation::CONJ_PLAIN_NEGATIVE_VOLITIONAL ;
			cbTotal = cbBase + 2 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"れば", 2 ) ) {
			Conjugation = _Conjugation::CONJ_AFFIRMATIVE_PROVISIONAL ;
			cbTotal = cbBase + 2 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"たら", 2 ) ) {
			Conjugation = _Conjugation::CONJ_PLAIN_AFFIRMATIVE_CONDITIONAL ;
			cbTotal = cbBase + 2 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"たり", 2 ) ) {
			Conjugation = _Conjugation::CONJ_PLAIN_AFFIRMATIVE_ALTERNATIVE ;
			cbTotal = cbBase + 2 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"る", 1 ) ) {
			Conjugation = _Conjugation::CONJ_PLAIN_AFFIRMATIVE_INDICATIVE ;
			cbTotal = cbBase + 1 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"た", 1 ) ) {
			Conjugation = _Conjugation::CONJ_PLAIN_AFFIRMATIVE_PAST_INDICATIVE ;
			cbTotal = cbBase + 1 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"て", 1 ) ) {
			Conjugation = _Conjugation::CONJ_PLAIN_AFFIRMATIVE_CONTINUATIVE ;
			cbTotal = cbBase + 1 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ろ", 1 ) ) {
			Conjugation = _Conjugation::CONJ_ABRUPT_AFFIRMATIVE_IMPERATIVE1 ;
			cbTotal = cbBase + 1 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"よ", 1 ) ) {
			Conjugation = _Conjugation::CONJ_ABRUPT_AFFIRMATIVE_IMPERATIVE2 ;
			cbTotal = cbBase + 1 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"な", 1 ) ) {
			Conjugation = _Conjugation::CONJ_ABRUPT_NEGATIVE_IMPERATIVE ;
			cbTotal = cbBase + 1 ;
			return ( true ) ;
		} /* endif */
		Conjugation = _Conjugation::CONJ_PLAIN_AFFIRMATIVE_CONTINUATIVE_RENYOUKEI ;
		cbTotal = cbBase ;
		return ( true ) ;
	} /* endmethod */
} ;

class _GodanVerbA : public _ConjugatableStem {
public:
	bool Test ( wchar_t *pBase, size_t cbBase, size_t cbRemainder, _Conjugation &Conjugation, size_t &cbTotal ) {
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"なかったでしょう", 8 ) ) {
			Conjugation = _Conjugation::CONJ_POLITE_NEGATIVE_PAST_PRESUMPTIVE ;
			cbTotal = cbBase + 8 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ないでください", 8 ) ) {
			Conjugation = _Conjugation::CONJ_POLITE_NEGATIVE_REQUEST ;
			cbTotal = cbBase + 8 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"なかっただろう", 7 ) ) {
			Conjugation = _Conjugation::CONJ_PLAIN_NEGATIVE_PAST_PRESUMPTIVE ;
			cbTotal = cbBase + 7 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ないでしょう", 6 ) ) {
			Conjugation = _Conjugation::CONJ_POLITE_NEGATIVE_PRESUMPTIVE ;
			cbTotal = cbBase + 6 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ないだろう", 5 ) ) {
			Conjugation = _Conjugation::CONJ_PLAIN_NEGATIVE_PRESUMPTIVE ;
			cbTotal = cbBase + 5 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"なかったら", 5 ) ) {
			Conjugation = _Conjugation::CONJ_PLAIN_NEGATIVE_CONDITIONAL ;
			cbTotal = cbBase + 5 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"なかった", 4 ) ) {
			Conjugation = _Conjugation::CONJ_PLAIN_NEGATIVE_PAST_INDICATIVE ;
			cbTotal = cbBase + 4 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"なければ", 4 ) ) {
			Conjugation = _Conjugation::CONJ_NEGATIVE_PROVISIONAL ;
			cbTotal = cbBase + 4 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ないで", 3 ) || hasExtension ( pBase, cbBase, cbRemainder, L"なくて", 3 ) ) {
			Conjugation = _Conjugation::CONJ_PLAIN_NEGATIVE_CONTINUATIVE ;
			cbTotal = cbBase + 3 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ない", 2 ) ) {
			// TODO: This may be conjugated as an adjective.
			Conjugation = _Conjugation::CONJ_PLAIN_NEGATIVE_INDICATIVE ;
			cbTotal = cbBase + 2 ;
			return ( true ) ;
		} /* endif */
		return ( false ) ;
	} /* endmethod */
} ;

class _GodanVerbI : public _ConjugatableStem {
public:
	bool Test ( wchar_t *pBase, size_t cbBase, size_t cbRemainder, _Conjugation &Conjugation, size_t &cbTotal ) {
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ませんでしたら", 7 ) ) {
			Conjugation = _Conjugation::CONJ_POLITE_NEGATIVE_CONDITIONAL ;
			cbTotal = cbBase + 7 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ませんでしたり", 7 ) ) {
			Conjugation = _Conjugation::CONJ_POLITE_NEGATIVE_ALTERNATIVE ;
			cbTotal = cbBase + 7 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ませんでした", 6 ) ) {
			Conjugation = _Conjugation::CONJ_POLITE_NEGATIVE_PAST_INDICATIVE ;
			cbTotal = cbBase + 6 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"なさいますな", 6 ) ) {
			Conjugation = _Conjugation::CONJ_HONORIFIC_NEGATIVE_REQUEST ;
			cbTotal = cbBase + 6 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"なさいませ", 5 ) ) {
			Conjugation = _Conjugation::CONJ_HONORIFIC_AFFIRMATIVE_REQUEST ;
			cbTotal = cbBase + 5 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ませんで", 4 ) ) {
			Conjugation = _Conjugation::CONJ_POLITE_NEGATIVE_CONTINUATIVE ;
			cbTotal = cbBase + 4 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ましょう", 4 ) ) {
			Conjugation = _Conjugation::CONJ_POLITE_AFFIRMATIVE_VOLITIONAL ;
			cbTotal = cbBase + 4 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ますまい", 4 ) ) {
			Conjugation = _Conjugation::CONJ_POLITE_NEGATIVE_VOLITIONAL ;
			cbTotal = cbBase + 4 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"なさるな", 4 ) ) {
			Conjugation = _Conjugation::CONJ_PLAIN_NEGATIVE_IMPERATIVE ;
			cbTotal = cbBase + 4 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ください", 4 ) ) {
			Conjugation = _Conjugation::CONJ_POLITE_AFFIRMATIVE_REQUEST2 ;
			cbTotal = cbBase + 4 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ましたら", 4 ) ) {
			Conjugation = _Conjugation::CONJ_POLITE_AFFIRMATIVE_CONDITIONAL ;
			cbTotal = cbBase + 4 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ましたり", 4 ) ) {
			Conjugation = _Conjugation::CONJ_POLITE_AFFIRMATIVE_ALTERNATIVE ;
			cbTotal = cbBase + 4 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ません", 3 ) ) {
			Conjugation = _Conjugation::CONJ_POLITE_NEGATIVE_INDICATIVE ;
			cbTotal = cbBase + 3 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ました", 3 ) ) {
			Conjugation = _Conjugation::CONJ_POLITE_AFFIRMATIVE_PAST_INDICATIVE ;
			cbTotal = cbBase + 3 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"まして", 3 ) ) {
			Conjugation = _Conjugation::CONJ_POLITE_AFFIRMATIVE_CONTINUATIVE ;
			cbTotal = cbBase + 3 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"なさい", 3 ) ) {
			Conjugation = _Conjugation::CONJ_PLAIN_AFFIRMATIVE_IMPERATIVE ;
			cbTotal = cbBase + 3 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ます", 2 ) ) {
			Conjugation = _Conjugation::CONJ_POLITE_AFFIRMATIVE_INDICATIVE ;
			cbTotal = cbBase + 2 ;
			return ( true ) ;
		} /* endif */
		Conjugation = _Conjugation::CONJ_PLAIN_AFFIRMATIVE_CONTINUATIVE_RENYOUKEI ;
		cbTotal = cbBase ;
		return ( true ) ;
	} /* endmethod */
} ;

class _GodanVerbU : public _ConjugatableStem {
public:
	bool Test ( wchar_t *pBase, size_t cbBase, size_t cbRemainder, _Conjugation &Conjugation, size_t &cbTotal ) {
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"でしょう", 4 ) ) {
			Conjugation = _Conjugation::CONJ_POLITE_AFFIRMATIVE_PRESUMPTIVE ;
			cbTotal = cbBase + 4 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"だろう", 3 ) ) {
			Conjugation = _Conjugation::CONJ_PLAIN_AFFIRMATIVE_PRESUMPTIVE ;
			cbTotal = cbBase + 3 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"まい", 2 ) ) {
			Conjugation = _Conjugation::CONJ_PLAIN_NEGATIVE_VOLITIONAL ;
			cbTotal = cbBase + 2 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"な", 1 ) ) {
			Conjugation = _Conjugation::CONJ_ABRUPT_NEGATIVE_IMPERATIVE ;
			cbTotal = cbBase + 1 ;
			return ( true ) ;
		} /* endif */
		Conjugation = _Conjugation::CONJ_PLAIN_AFFIRMATIVE_INDICATIVE ;
		cbTotal = cbBase + 1 ;
		return ( true ) ;
	} /* endmethod */
} ;

class _GodanVerbE : public _ConjugatableStem {
public:
	bool Test ( wchar_t *pBase, size_t cbBase, size_t cbRemainder, _Conjugation &Conjugation, size_t &cbTotal ) {
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"ば", 1 ) ) {
			Conjugation = _Conjugation::CONJ_AFFIRMATIVE_PROVISIONAL ;
			cbTotal = cbBase + 1 ;
			return ( true ) ;
		} /* endif */
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"る", 1 ) ) {
			// TODO: This may be conjugated as an Ichidan root.
			Conjugation = _Conjugation::CONJ_POTENTIAL ;
			cbTotal = cbBase + 1 ;
			return ( true ) ;
		} /* endif */
		Conjugation = _Conjugation::CONJ_ABRUPT_AFFIRMATIVE_IMPERATIVE1 ;
		cbTotal = cbBase + 1 ;
		return ( true ) ;
	} /* endmethod */
} ;

class _GodanVerbO : public _ConjugatableStem {
public:
	bool Test ( wchar_t *pBase, size_t cbBase, size_t cbRemainder, _Conjugation &Conjugation, size_t &cbTotal ) {
		if ( hasExtension ( pBase, cbBase, cbRemainder, L"う", 1 ) ) {
			Conjugation = _Conjugation::CONJ_PLAIN_AFFIRMATIVE_VOLITIONAL ;
			cbTotal = cbBase + 1 ;
			return ( true ) ;
		} /* endif */
		return ( false ) ;
	} /* endmethod */
} ;

class _GodanVerbNULL : public _ConjugatableStem {
public:
	bool Test ( wchar_t *pBase, size_t cbBase, size_t cbRemainder, _Conjugation &Conjugation, size_t &cbTotal ) {
		_KanaColumn WhichColumn = pDictionaryEntry->GetKanaColumn ( ) ;
		if ( hasExtension ( pBase, cbBase, cbRemainder, szVerb1PositivePastSuffix[(size_t)WhichColumn], wcslen(szVerb1PositivePastSuffix[(size_t)WhichColumn]) ) ) {
			size_t cbSuffix = wcslen(szVerb1PositivePastSuffix[(size_t)WhichColumn]) ;
			if ( hasExtension ( pBase, cbBase+cbSuffix, cbRemainder-cbSuffix, L"でしょう", 4 ) ) {
				Conjugation = _Conjugation::CONJ_POLITE_AFFIRMATIVE_PAST_PRESUMPTIVE ;
				cbTotal = cbBase + cbSuffix + 4 ;
				return ( true ) ;
			} /* endif */
			if ( hasExtension ( pBase, cbBase+cbSuffix, cbRemainder-cbSuffix, L"だろう", 3 ) ) {
				Conjugation = _Conjugation::CONJ_PLAIN_AFFIRMATIVE_PAST_PRESUMPTIVE ;
				cbTotal = cbBase + cbSuffix + 3 ;
				return ( true ) ;
			} /* endif */
			if ( hasExtension ( pBase, cbBase+cbSuffix, cbRemainder-cbSuffix, L"ら", 1 ) ) {
				Conjugation = _Conjugation::CONJ_PLAIN_AFFIRMATIVE_CONDITIONAL ;
				cbTotal = cbBase + cbSuffix + 1 ;
				return ( true ) ;
			} /* endif */
			if ( hasExtension ( pBase, cbBase+cbSuffix, cbRemainder-cbSuffix, L"り", 1 ) ) {
				Conjugation = _Conjugation::CONJ_PLAIN_AFFIRMATIVE_ALTERNATIVE ;
				cbTotal = cbBase + cbSuffix + 1 ;
				return ( true ) ;
			} /* endif */
			Conjugation = _Conjugation::CONJ_PLAIN_AFFIRMATIVE_PAST_INDICATIVE ;
			cbTotal = cbBase + wcslen(szVerb1PositivePastSuffix[(size_t)WhichColumn]) ;
			return ( true ) ;
		} else if ( hasExtension ( pBase, cbBase, cbRemainder, szVerb1ConjunctiveSuffix[(size_t)WhichColumn], wcslen(szVerb1ConjunctiveSuffix[(size_t)WhichColumn]) ) ) {
			size_t cbSuffix = wcslen(szVerb1ConjunctiveSuffix[(size_t)WhichColumn]) ;
			if ( hasExtension ( pBase, cbBase+cbSuffix, cbRemainder-cbSuffix, L"ください", 4 ) ) {
				Conjugation = _Conjugation::CONJ_POLITE_AFFIRMATIVE_REQUEST1 ;
				cbTotal = cbBase + cbSuffix + 4 ;
				return ( true ) ;
			} /* endif */
			Conjugation = _Conjugation::CONJ_PLAIN_AFFIRMATIVE_CONTINUATIVE ;
			cbTotal = cbBase + wcslen(szVerb1ConjunctiveSuffix[(size_t)WhichColumn]) ;
			return ( true ) ;
		} /* endif */
		return ( false ) ;
	} /* endmethod */
} ;

class _IkuVerb : public _ConjugatableStem {
public:
	bool Test ( wchar_t *pBase, size_t cbBase, size_t cbRemainder, _Conjugation &Conjugation, size_t &cbTotal ) {
		return ( false ) ;
	} /* endmethod */
} ;

class _AruVerb : public _ConjugatableStem {
public:
	bool Test ( wchar_t *pBase, size_t cbBase, size_t cbRemainder, _Conjugation &Conjugation, size_t &cbTotal ) {
		return ( false ) ;
	} /* endmethod */
} ;

class _SuruVerb : public _ConjugatableStem {
public:
	bool Test ( wchar_t *pBase, size_t cbBase, size_t cbRemainder, _Conjugation &Conjugation, size_t &cbTotal ) {
		return ( false ) ;
	} /* endmethod */
} ;

class _KuruVerb : public _ConjugatableStem {
public:
	bool Test ( wchar_t *pBase, size_t cbBase, size_t cbRemainder, _Conjugation &Conjugation, size_t &cbTotal ) {
		return ( false ) ;
	} /* endmethod */
} ;

class _Adjective_I : public _ConjugatableStem {
public:
	bool Test ( wchar_t *pBase, size_t cbBase, size_t cbRemainder, _Conjugation &Conjugation, size_t &cbTotal ) {
		return ( false ) ;
	} /* endmethod */
} ;

class _Adjective_NA : public _ConjugatableStem {
public:
	bool Test ( wchar_t *pBase, size_t cbBase, size_t cbRemainder, _Conjugation &Conjugation, size_t &cbTotal ) {
		return ( false ) ;
	} /* endmethod */
} ;

class _Adverb : public _ConjugatableStem {
public:
	bool Test ( wchar_t *pBase, size_t cbBase, size_t cbRemainder, _Conjugation &Conjugation, size_t &cbTotal ) {
		return ( false ) ;
	} /* endmethod */
} ;

class _Adverb_TO : public _ConjugatableStem {
public:
	bool Test ( wchar_t *pBase, size_t cbBase, size_t cbRemainder, _Conjugation &Conjugation, size_t &cbTotal ) {
		return ( false ) ;
	} /* endmethod */
} ;

bool _ConjugatableStem::TestConjugations ( wchar_t *pBase, size_t cbBase, size_t cbRemainder, _Conjugation &Conjugation, size_t &cbTotal ) {
	// Initialize results.
	Conjugation = _Conjugation::CONJ_NONE ;
	cbTotal = cbBase ;
	// Use the appropriate conjugator.
	switch ( pSymbolSound->GetRootClass ( ) ) {
		case _RootClass::RC_NULL: return ( true ) ; // We don't know what it is gramatically, so we cannot do anything.  Expressions are like that.
		case _RootClass::RC_ICHIDAN: return ( ((_IchidanVerb*)this)->Test ( pBase, cbBase, cbRemainder, Conjugation, cbTotal ) ) ;
		case _RootClass::RC_GODAN_A: return ( ((_GodanVerbA*)this)->Test ( pBase, cbBase, cbRemainder, Conjugation, cbTotal ) ) ;
		case _RootClass::RC_GODAN_I: return ( ((_GodanVerbI*)this)->Test ( pBase, cbBase, cbRemainder, Conjugation, cbTotal ) ) ;
		case _RootClass::RC_GODAN_U: return ( ((_GodanVerbU*)this)->Test ( pBase, cbBase, cbRemainder, Conjugation, cbTotal ) ) ;
		case _RootClass::RC_GODAN_E: return ( ((_GodanVerbE*)this)->Test ( pBase, cbBase, cbRemainder, Conjugation, cbTotal ) ) ;
		case _RootClass::RC_GODAN_O: return ( ((_GodanVerbO*)this)->Test ( pBase, cbBase, cbRemainder, Conjugation, cbTotal ) ) ;
		case _RootClass::RC_GODAN_NULL: return ( ((_GodanVerbNULL*)this)->Test ( pBase, cbBase, cbRemainder, Conjugation, cbTotal ) ) ;
		case _RootClass::RC_IKU: return ( ((_IkuVerb*)this)->Test ( pBase, cbBase, cbRemainder, Conjugation, cbTotal ) ) ;
		case _RootClass::RC_ARU: return ( ((_AruVerb*)this)->Test ( pBase, cbBase, cbRemainder, Conjugation, cbTotal ) ) ;
		case _RootClass::RC_SURU: return ( ((_SuruVerb*)this)->Test ( pBase, cbBase, cbRemainder, Conjugation, cbTotal ) ) ;
		case _RootClass::RC_KURU: return ( ((_KuruVerb*)this)->Test ( pBase, cbBase, cbRemainder, Conjugation, cbTotal ) ) ;
		case _RootClass::RC_ADJ_I: return ( ((_Adjective_I*)this)->Test ( pBase, cbBase, cbRemainder, Conjugation, cbTotal ) ) ;
		case _RootClass::RC_NOUN: return ( true ) ; // Nouns are not conjugated at all.
		default:
			assert ( !L"ERROR: Unsupported root class." ) ;
			return ( true ) ;
	} /* endswitch */
} /* endmethod */

static bool ParseDictionaryLine ( wchar_t *szText, wchar_t*&pSymbol, wchar_t*&pSound, wchar_t*&pInfo ) {

	// Phase 1: Scan for the end of the KANJI, which should start the line.
	bool fParseError = false ;
	pSymbol = szText ;
	wchar_t *p = szText ;
	while ( *p ) {
		if ( *p == 0x0A ) // EOL?
			return ( false ) ;
		if ( *p == L'/' ) // Have we hit the general information?
			return ( false ) ;
		if ( *p == L'[' ) // Have we hit the pronunciation?
			return ( false ) ;
		if ( *p == L' ' ) {
			*p++ = 0 ;
			break ;
		} /* endif */
		p ++ ;
	} /* endwhile */

	// Phase 2: Scan for the start of the KANA, or for the start of general information.
	pSound = 0 ;
	while ( *p ) {
		if ( *p == 0x0A ) // EOL?
			return ( false ) ;
		if ( *p == L'/' ) { // Have we hit the general information?  If so, the KANJI was actually KANA.
			pSound = pSymbol ;
			pSymbol = 0 ;
			break ;
		} /* endif */
		if ( *p == L'[' ) { // Have we hit the start of the KANA?
			pSound = ++p ;
			break ;
		} /* endif */
		p ++ ;
	} /* endwhile */

	// Phase 3: If we have KANJI, then scan for the end of the KANA.
	if ( pSymbol ) {
		while ( *p ) {
			if ( *p == 0x0A ) // EOL?
				return ( false ) ;
			if ( *p == L'/' ) // Have we hit the general information?
				return ( false ) ;
			if ( *p == L']' ) { // Have we hit the end of the KANA?
				*p++ = 0 ;
				break ;
			} /* endif */
			p ++ ;
		} /* endwhile */
	} /* endif */

	// Phase 4: Search for the start of the general information.
	pInfo = 0 ;
	while ( *p ) {
		if ( *p == 0x0A ) // EOL?
			return ( false ) ;
		if ( *p == L'/' ) { // Have we hit the general information?
			pInfo = ++p ;
			break ;
		} /* endif */
		p ++ ;
	} /* endwhile */

	// Phase 5: Mark the end of the general information.
	while ( *p ) {
		if ( *p == 0x0A ) { // EOL?
			*p++ = 0 ;
			break ;
		} /* endif */
		p ++ ;
	} /* endwhile */

	// We have everything now.
	return ( true ) ;
}

static DICTIONARY Dictionary ;

static _KanaColumn GetColumn ( wchar_t const *pRoot, _KanaRow Row ) { 
	const wchar_t *pLastCharacter = pRoot + wcslen(pRoot) - 1 ;
	_KanaColumn WhichColumn ;
	for ( WhichColumn = _KanaColumn::COL_U; WhichColumn< _KanaColumn::COL_NULL; WhichColumn++ )
		if ( *pLastCharacter == szVerb1Endings[(size_t)_KanaRow::ROW_U][(size_t)WhichColumn] )
			break ;
	if ( WhichColumn >= _KanaColumn::COL_NULL )
		return (_KanaColumn::COL_NULL ) ;
	return ( WhichColumn ) ;
}

static _KanaColumn GetStem ( wchar_t const *pRoot, wchar_t *pBuffer, size_t cbBuffer, _KanaRow Row ) { 
	wcscpy_s ( pBuffer, cbBuffer, pRoot ) ;
	wchar_t *pLastCharacter = pBuffer + wcslen(pBuffer) - 1 ;
	_KanaColumn WhichColumn ;
	for ( WhichColumn = _KanaColumn::COL_U; WhichColumn< _KanaColumn::COL_NULL; WhichColumn++ )
		if ( *pLastCharacter == szVerb1Endings[(size_t)_KanaRow::ROW_U][(size_t)WhichColumn] ) 
			break ;
	if ( WhichColumn >= _KanaColumn::COL_NULL )
		return (_KanaColumn::COL_NULL ) ;
	if ( Row == _KanaRow::ROW_NULL ) {
		*pLastCharacter = 0 ;
	} else {
		*pLastCharacter = szVerb1Endings [(size_t)Row] [(size_t)WhichColumn] ;
	} /* endif */
	return ( WhichColumn ) ;
}

static void AddWord ( wchar_t *pSymbol, wchar_t *pSound, wchar_t *pInfo ) {

	// Add the definition to our dictionary.
	STRING sInfo = STRING(pInfo) ;
	DICTIONARY::iterator Cursor = Dictionary.find ( sInfo ) ;
	if ( Cursor == Dictionary.end() ) {
		Dictionary[sInfo] = _DictionaryEntry ( sInfo ) ;
		Cursor = Dictionary.find ( pInfo ) ;
	} /* endif */

	// Insert every root form into the sound list for this definition.
	_WordClass WordClass = (*Cursor).second.GetWordClass ( ) ;
	wchar_t Buffer [2] [100] ;
	switch ( WordClass ) {

		case _WordClass::WC_NULL:
			if ( pSymbol ) 
				Cursor->second.AddSymbol ( STRING(pSymbol), STRING(pSound), _RootClass::RC_NULL ) ;
			else
				Cursor->second.AddSound ( STRING(pSound), _RootClass::RC_NULL ) ;
			break ;

		case _WordClass::WC_ICHIDAN: {
			// Last character must be る.
			if ( pSymbol ) {
				wcscpy_s ( Buffer[0], _countof(Buffer[0]), pSymbol ) ;
				wchar_t *p = Buffer[0] + wcslen(Buffer[0]) - 1 ;
				if (*p != L'る') {
					pSymbol = NULL; // We have a problem here.
				} /* endif */
				*p = 0 ;
			} /* endif */
			wcscpy_s ( Buffer[1], _countof(Buffer[1]), pSound ) ;
			wchar_t *p = Buffer[1] + wcslen(Buffer[1]) - 1 ;
			if (*p != L'る') {
				pSound = NULL ; // We have a problem here.
			} /* endif */
			*p = 0 ;
			if ( pSymbol && pSound ) 
				Cursor->second.AddSymbol ( STRING(Buffer[0]), STRING(Buffer[1]), _RootClass::RC_ICHIDAN ) ;
			else if ( pSound )
				Cursor->second.AddSound ( STRING(Buffer[1]), _RootClass::RC_ICHIDAN ) ;
			break ; }

		case _WordClass::WC_GODAN: {
			Cursor->second.SetKanaColumn ( GetColumn ( pSound, _KanaRow::ROW_NULL ) ) ;
			_RootClass RootClass = _RootClass::RC_GODAN_A ;
			for ( _KanaRow Row = _KanaRow::ROW_A; Row<=_KanaRow::ROW_NULL; Row++, RootClass++ ) {
				if ( pSymbol ) {
					if ( GetStem ( pSymbol, Buffer[0], _countof(Buffer[0]), Row ) == _KanaColumn::COL_NULL ) {
						pSymbol = NULL ; // We have a problem here.
						break ;
					} /* endif */
				} /* endif */
				if ( GetStem ( pSound, Buffer[1], _countof(Buffer[1]), Row ) == _KanaColumn::COL_NULL ) {
					pSound = NULL ; // We have a problem here.
					break ;
				} /* endif */
				if ( pSymbol && pSound ) {
					Cursor->second.AddSymbol ( STRING(Buffer[0]), STRING(Buffer[1]), RootClass ) ;
				} else if ( pSound ) {
					Cursor->second.AddSound ( STRING(Buffer[1]), RootClass ) ;
				} /* endif */
				if ( Row == _KanaRow::ROW_NULL )
					break ;
			} /* endfor */
			break ; }

		case _WordClass::WC_IKU: // -行く（-いく、ゆく)
			if ( pSymbol ) {
				wcscpy_s ( Buffer[0], _countof(Buffer[0]), pSymbol ) ;
				assert ( wcslen(Buffer[0]) >= 2 ) ;
				wchar_t *p = Buffer[0] + wcslen(Buffer[0]) - 2 ;
				assert ( ( ( p[0] == L'行' ) || ( p[0] == L'逝' ) || ( p[0] == L'往' ) || ( p[0] == L'い' ) || ( p[0] == L'ゆ' ) ) && ( p[1] == L'く' ) ) ;
			} /* endif */
			if ( pSound ) {
				wcscpy_s ( Buffer[1], _countof(Buffer[1]), pSound ) ;
				assert ( wcslen(Buffer[1]) >= 2 ) ;
				wchar_t *p = Buffer[1] + wcslen(Buffer[1]) - 2 ;
				assert ( ( ( p[0] == L'い' ) || ( p[0] == L'ゆ' ) || ( p[0] == L'て' ) || ( p[0] == L'で' ) ) && ( p[1] == L'く' ) ) ;
			} /* endif */
			break ;

		case _WordClass::WC_ARU: // -しゃる, -さる, -ざる
			break ;

		case _WordClass::WC_SURU: // -する
			if ( pSymbol ) {
				wcscpy_s ( Buffer[0], _countof(Buffer[0]), pSymbol ) ;
				assert ( wcslen(Buffer[0]) >= 2 ) ;
				wchar_t *p = Buffer[0] + wcslen(Buffer[0]) - 2 ;
				assert ( ( p[0] == L'す' ) && ( p[1] == L'る' ) ) ;
			} /* endif */
			if ( pSound ) {
				wcscpy_s ( Buffer[1], _countof(Buffer[1]), pSound ) ;
				assert ( wcslen(Buffer[1]) >= 2 ) ;
				wchar_t *p = Buffer[1] + wcslen(Buffer[1]) - 2 ;
				assert ( ( p[0] == L'す' ) && ( p[1] == L'る' ) ) ;
			} /* endif */
			break ;

		case _WordClass::WC_KURU: // -来る（-くる)
			if ( pSymbol ) {
				wcscpy_s ( Buffer[0], _countof(Buffer[0]), pSymbol ) ;
				assert ( wcslen(Buffer[0]) >= 2 ) ;
				wchar_t *p = Buffer[0] + wcslen(Buffer[0]) - 2 ;
				assert ( ( ( p[0] == L'来' ) || ( p[0] == L'來' ) || ( p[0] == L'く' ) ) && ( p[1] == L'る' ) ) ;
			} /* endif */
			if ( pSound ) {
				wcscpy_s ( Buffer[1], _countof(Buffer[1]), pSound ) ;
				assert ( wcslen(Buffer[1]) >= 2 ) ;
				wchar_t *p = Buffer[1] + wcslen(Buffer[1]) - 2 ;
				assert ( ( p[0] == L'く' ) && ( p[1] == L'る' ) ) ;
			} /* endif */
			break ;

		case _WordClass::WC_NOUN:
			if ( pSymbol ) 
				Cursor->second.AddSymbol ( STRING(pSymbol), STRING(pSound), _RootClass::RC_NOUN ) ;
			else
				Cursor->second.AddSound ( STRING(pSound), _RootClass::RC_NOUN ) ;
			break ;

		case _WordClass::WC_ADJ_I: {
			// Last character must be い.
			if ( pSymbol ) {
				wcscpy_s ( Buffer[0], _countof(Buffer[0]), pSymbol ) ;
				wchar_t *p = Buffer[0] + wcslen(Buffer[0]) - 1 ;
				if ( *p != L'い' ) 
					pSymbol = 0 ; // We have a problem here.
				*p = 0 ;
			} /* endif */
			wcscpy_s ( Buffer[1], _countof(Buffer[1]), pSound ) ;
			wchar_t *p = Buffer[1] + wcslen(Buffer[1]) - 1 ;
			if ( *p != L'い' ) 
				pSound = 0 ; // We have a problem here.
			*p = 0 ;
			if ( pSymbol && pSound ) {
				Cursor->second.AddSymbol ( STRING(Buffer[0]), STRING(Buffer[1]), _RootClass::RC_ADJ_I ) ;
			} else if ( pSound ) {
				Cursor->second.AddSound ( STRING(Buffer[1]), _RootClass::RC_ADJ_I ) ;
			} /* endif */
			break ; }

		default:
			assert ( !L"ERROR: Unsupported word class." ) ;

	} /* endswitch */
}

static bool BuildDictionary ( wchar_t *szPath ) {

	// Open the EDICT file.
	FILE *File ; errno_t Error = _wfopen_s ( &File, szPath, L"rt" ) ;
	if ( Error || ( File == 0 ) ) {
		char Buffer[200]; strerror_s(Buffer, _countof(Buffer), Error);
		fprintf ( stderr, "ERROR: Unable to open source file.  Error %i:%hs\n", Error, Buffer ) ;
		return ( false ) ;
	} /* endif */

	// Read it line by line, converting to Unicode as we go.
	char RawBuffer [0x1000] ; wchar_t ConvertedBuffer [0x1000] ;
	while ( fgets ( RawBuffer, _countof(RawBuffer), File ) ) {

		// Convert the raw line to Unicode.
		bool Error = false;
		unsigned char *p1 = (unsigned char*) RawBuffer ; wchar_t *p2 = ConvertedBuffer ;
		while (*p1 && !Error) {
			if ( *p1 < 0xA1 ) {
				*p2++ = *p1++ ;
			} else {
				unsigned short Glyph = ( p1[1] << 8 ) + p1[0] ;
				unsigned Index ;
				for ( Index=0; Index<_countof(JIS0208); Index++ ) 
					if ( JIS0208[Index].JIS0208 == Glyph ) 
						break ;
				if ((Index < _countof(JIS0208)) && (JIS0208[Index].JIS0208 == Glyph))
					*p2++ = SwapWord(JIS0208[Index].Unicode);
				else
					Error = true;
				p1 += 2 ;
			} /* endif */
		} /* endwhile */
		*p2 = 0 ;

		// Skip lines we cannot fully convert.
		if (Error)
			continue;

		// Skip comment lines.
		if ( ConvertedBuffer[0] == L'　' )
			continue ;

		// Parse the line if we can.
		wchar_t *pSymbol(0), *pSound(0), *pInfo(0) ;
		if ( ParseDictionaryLine ( ConvertedBuffer, pSymbol, pSound, pInfo ) == false ) {
			assert ( !L"Dictionary line failed to parse!" ) ;
			continue ;
		} /* endif */

		// Add the definition to our internal dictionary.
		AddWord ( pSymbol, pSound, pInfo ) ;

	} /* endwhile */

	// Done reading the source file.
	fclose ( File ) ;
	return ( true ) ;
}

static INDEX Index ;

static bool BuildIndex ( ) {

	// Clear the index.
	Index.clear ( ) ;

	// Build the index.
	DICTIONARY::iterator Cursor1 = Dictionary.begin ( ) ;
	while ( Cursor1 != Dictionary.end() ) {
		const SYMBOL_SOUND_SET* pSymbols = Cursor1->second.GetSymbols ( ) ;
		SYMBOL_SOUND_SET::iterator Cursor2 = pSymbols->begin ( ) ;
		while ( Cursor2 != pSymbols->end() ) {
			wchar_t *szSymbol = (wchar_t*) Cursor2->GetSymbol ( ) ;
			wchar_t *szSound = (wchar_t*) Cursor2->GetSound ( ) ;
			Index.insert ( _IndexEntry ( szSymbol, szSound, &Cursor1->second ) ) ;
			if ( wcscmp ( szSymbol, szSound ) ) 
				Index.insert ( _IndexEntry ( szSound, szSound, &Cursor1->second ) ) ;
			++ Cursor2 ;
		} /* endwhile */
		++ Cursor1 ;
	} /* endwhile */

	return ( true ) ;
}

// Scan for the longest symbol that matches the start of the line.
// Once you've found a symbol, scan over all identical symbols to 
//  find the preferred definition and pronunciation.

extern INDEX::iterator FindSymbol ( wchar_t *pLine, size_t cbLine ) {
	wchar_t szKey [2] = { (wchar_t)(pLine[0]-1), 0 } ;	
	_DictionaryEntry Dummy2 ; _IndexEntry Key ( szKey, L"", &Dummy2 ) ;
	INDEX::iterator Cursor = Index.upper_bound ( Key ) ;
	while ( Cursor != Index.end() ) {
		const _IndexEntry* pEntry = &(*Cursor) ;
		const wchar_t *pWord = pEntry->GetWord ( ) ;
		if ( pWord[0] != pLine[0] )
			break ;
		wchar_t *p = pLine ; size_t cb = cbLine ;
		while ( *pWord && cb ) {
			if ( *p != *pWord ) 
				break ;
			++ pWord ; ++ p ; -- cb ;
		} /* endwhile */
		if ( *pWord == 0 ) {
			// We have something.
			INDEX::iterator FirstAnswer = Cursor ++ ;
			while ( Cursor != Index.end() ) {
				if ( wcscmp ( FirstAnswer->GetWord(), Cursor->GetWord() ) )
					return ( FirstAnswer ) ;
				if ( Cursor->GetDictionaryEntry()->isPreferred() ) 
					return ( Cursor ) ;
				++ Cursor ;
			} /* endwhile */
			return ( FirstAnswer ) ;
		} /* endif */
		++ Cursor ;
	} /* endwhile */
	return ( Index.end() ) ;
}

inline bool isHiragana ( wchar_t c ) {	return ( ( c >= 0x3040 ) && ( c <= 0x309F ) ) ; }
inline bool isKatakana ( wchar_t c ) {	return ( ( c >= 0x30A0 ) && ( c <= 0x30FF ) ) ; }
inline bool isKanji ( wchar_t c ) {	return ( ( ( c >= 0x4E00 ) && ( c <= 0x9FBF ) ) || ( c == L'々' ) ) ; }
inline bool isWhite ( wchar_t c ) { return ( ( c == L' ' ) || ( c == L'　' ) || ( c == L'\t' ) || ( c == L'\r' ) || ( c == L'\n' ) ) ; }

inline wchar_t *SkipWhiteSpace ( wchar_t Line[], size_t cbLine, wchar_t* &p ) {
	wchar_t *pLast = Line + cbLine ;
	while ( ( p < pLast ) && isWhite ( p[0] ) ) 
		++ p ;
	return ( p ) ;
} 

inline bool hasExtension ( wchar_t *pBase, size_t cbBase, size_t cbRemainder, wchar_t *pExtension, size_t cbExtension ) {
	wchar_t *pNext = pBase + cbBase ;
	if ( cbRemainder < cbExtension )
		return ( false ) ;
	return ( !memcmp ( pNext, pExtension, cbExtension*sizeof(wchar_t) ) ) ;
}

//
// Analyze sentence.
//

static void ProcessSentence ( FILE *DestinFile, wchar_t* pSentence, size_t cbSentence, wchar_t cPrefix, wchar_t cSuffix ) {

	// Output the sentence we are about to analyze.
	fwprintf ( DestinFile, L"      " ) ;
	if ( cPrefix )
		fwprintf ( DestinFile, L"%c", cPrefix ) ;
	fwprintf ( DestinFile, L"%0.*ls", (int)cbSentence, pSentence ) ;
	if ( cSuffix )
		fwprintf ( DestinFile, L"%c", cSuffix ) ;
	fwprintf ( DestinFile, L"\n" ) ;

	// Scan the sentence.
	wchar_t *p = pSentence ;
	while ( p < pSentence+cbSentence ) {
		if ( isKanji(p[0]) || isHiragana(p[0]) || isKatakana(p[0]) ) {
			INDEX::iterator Cursor = FindSymbol ( p, cbSentence-(p-pSentence) ) ;
			if ( Cursor != Index.end() ) {

				// We found at least one matching definition.  Find the rest now.
				DEFINITIONS Definitions ;
				const _IndexEntry* pIndexEntry = &(*Cursor);
				Definitions.push_back ( pIndexEntry ) ;
				do {
					++ Cursor ;
					if ( Cursor != Index.end() ) {
						const _IndexEntry *pIndexEntry2 = & ( *Cursor ) ;
						if ( wcscmp ( pIndexEntry->GetWord(), pIndexEntry2->GetWord() ) )
							break ;
						Definitions.push_back ( pIndexEntry2 ) ;
					} /* endif */
				} while ( Cursor != Index.end() ) ;

				// Try each of them to see which works best.
				const _IndexEntry *pBestIndexEntry = 0 ; size_t cbBestTotal = 0 ;
				DEFINITIONS::iterator DefCursor = Definitions.begin ( ) ;
				while ( DefCursor != Definitions.end() ) {
					const _IndexEntry *pIndexEntry = *DefCursor ;
					const _DictionaryEntry *pEntry = pIndexEntry->GetDictionaryEntry ( ) ;
					const wchar_t *pWord = pIndexEntry->GetWord ( ) ; 
					size_t cbWord = wcslen(pWord) ;
					wchar_t *pRemainder = p + cbWord ;
					size_t cbRemainder = cbSentence - ( pRemainder - pSentence ) ;
					_ConjugatableStem Stem ( pEntry, p, cbWord ) ; 
					_Conjugation Conjugation ; size_t cbTotal ;
					if ( Stem.TestConjugations ( p, cbWord, cbRemainder, Conjugation, cbTotal ) ) {
						if ( cbTotal > cbBestTotal ) {
							cbBestTotal = cbTotal ;
							pBestIndexEntry = pIndexEntry ;
						} /* endif */
					} /* endif */
					++ DefCursor ;
				} /* endwhile */

				if ( cbBestTotal ) {
					fwprintf ( DestinFile, L"        %0.*ls - %ls\n", (int)cbBestTotal, p, pBestIndexEntry->GetDictionaryEntry()->GetInfo() ) ;
					p += cbBestTotal ;
					continue ;
				} /* endif */

				// Apparently not.
				const _DictionaryEntry *pEntry = pIndexEntry->GetDictionaryEntry ( ) ;
				const wchar_t *pWord = pIndexEntry->GetWord ( ) ; 
				size_t cbWord = wcslen(pWord) ;
				fwprintf ( DestinFile, L"        %0.*ls - [Stem] %ls\n", (int)cbWord, p, pEntry->GetInfo() ) ;
				p += cbWord ;

			// No dictionary entry found.
			} else {
				fwprintf ( DestinFile, L"        %lc - No dictionary match.\n", *p ) ;
				++ p ;

			} /* endif */

		// Not a japanese character.
		} else {
			fwprintf ( DestinFile, L"        %lc - Not hiragana nor katakana nor kanji.\n", *p ) ;
			++ p ;

		} /* endif */

	} /* endwhile */
}

static void ProcessParagraph ( FILE *DestinFile, wchar_t Line[], size_t &cbLine, unsigned &nLines ) {

	// Trim CR+LF from end of line if it's there.
	if ( ( cbLine >= 2 ) && ( Line[cbLine-2] == 0x0D ) && ( Line[cbLine-1] == 0x0A ) )
		cbLine -= 2 ;

	// Most paragraphs that are not dialog start with a blank space.  Trim that out.
	wchar_t *p = Line ;
	if ( ( cbLine >= 1 ) && ( Line[0] == L'　' ) ) 
		++ p ;

	// Output the whole line for reference.
	fwprintf ( DestinFile, L"%04i: %0.*ls\n", ++nLines, (int)(cbLine-(p-Line)), p ) ;

	// Null terminate the line.
	Line[cbLine] = 0 ;

	// Parse out the sentences.
	// A sentence can be:
	// (1) A block of text, including embedded quoted strings, up to a period (.), exclamation (!), question (?) or interrobang (!?/?!).
	// (2) A quoted string, if not embedded in another block.
	// If the exclamation, question or interrobang is immediately followed by と or って, then the sentence continues.
	//
	// NOTE: The entire paragraph might be (and often is) a quoted block, so let's treat those quotation marks specially.
	//
	wchar_t *pSentence = SkipWhiteSpace ( Line, cbLine, p ) ;
	int iNesting = 0 ; bool fQuoted = false ;
	wchar_t cPrefix = 0, cSuffix = 0 ;
	if ( ( *p == L'「' ) || ( *p == L'『' ) )
		cPrefix = *p++, ++pSentence, fQuoted = true ; 
	while ( p < Line+cbLine ) {
		if ( !iNesting && ( *p == L'。' ) ) {
			++ p ; // Include the sentence ending character.
			if ( fQuoted && ( p < Line+cbLine ) && ( ( *p == L'」' ) || ( *p == L'』' ) ) )
				cSuffix = *p ;
			ProcessSentence ( DestinFile, pSentence, p-pSentence, cPrefix, cSuffix ) ;
			if ( cSuffix )
				++ p ;
			cPrefix = cSuffix = 0 ;
			pSentence = SkipWhiteSpace ( Line, cbLine, p ) ;
		} else if ( !iNesting && ( ( *p == L'！' ) || ( *p == L'？' ) ) ) {
			if ( ( p < Line+cbLine-2 ) && ( p[1] == L'っ' ) && ( p[2] == L'て' ) ) {
				p += 3 ;
			} else if ( ( p < Line+cbLine-1 ) && ( p[1] == L'と' ) ) {
				p += 2 ;
			} else {
				++ p ; // Include the sentence ending character.
				if ( fQuoted && ( p < Line+cbLine ) && ( ( *p == L'」' ) || ( *p == L'』' ) ) )
					cSuffix = *p ;
				ProcessSentence ( DestinFile, pSentence, p-pSentence, cPrefix, cSuffix ) ;
				if ( cSuffix )
					++ p ;
				cPrefix = cSuffix = 0 ;
				pSentence = SkipWhiteSpace ( Line, cbLine, p ) ;
			} /* endif */
		} else if ( !iNesting && ( p < Line+cbLine-1 ) && ( ( ( p[0] == L'!' ) && ( p[1] == L'?' ) ) || ( ( p[0] == L'?' ) && ( p[1] == L'!' ) ) ) ) {
			if ( ( p < Line+cbLine-3 ) && ( p[2] == L'っ' ) && ( p[3] == L'て' ) ) {
				p += 4 ;
			} else if ( ( p < Line+cbLine-2 ) && ( p[2] == L'と' ) ) {
				p += 3 ;
			} else {
				p += 2 ; // Include the sentence ending characters.
				if ( fQuoted && ( p < Line+cbLine ) && ( ( *p == L'」' ) || ( *p == L'』' ) ) )
					cSuffix = *p ;
				ProcessSentence ( DestinFile, pSentence, p-pSentence, cPrefix, cSuffix ) ;
				if ( cSuffix )
					++ p ;
				cPrefix = cSuffix = 0 ;
				pSentence = SkipWhiteSpace ( Line, cbLine, p ) ;
			} /* endif */
		} else {
			if ( ( *p == L'「' ) || ( *p == L'『' ) ) {
				++ iNesting ;
			} else if ( ( *p == L'」' ) || ( *p == L'』' ) ) {
				-- iNesting ;
			} /* endif */
			++ p ;
		} /* endif */
	} /* endwhile */
	if ( pSentence < Line+cbLine ) {
		// Flush out the rest of the paragraph.
		ProcessSentence ( DestinFile, pSentence, p-pSentence, cPrefix, cSuffix ) ;
	} /* endif */

	// Reset the line size.
	cbLine = 0 ;
}

int _tmain ( int argc, wchar_t* argv[] ) {

	// Parse the command line.
	wchar_t SourcePath [_MAX_PATH] = { 0 } ;
	wchar_t DestinPath [_MAX_PATH] = { 0 } ;
	while ( --argc ) {
		argv ++ ;
		if ( !wcscmp(argv[0],L"?") || !wcscmp(argv[0],L"/?") || !wcscmp(argv[0],L"-?") ) {
			printf ( "PARSER2: New Japanese Text Parser\n"
				"\n"
				"Syntax: PARSER2 sourcefile destinfile\n"
				"\n"
				"Where:\n"
				"  'sourcefile' specifies the file to be parsed.\n"
				"  'destinfile' specifies the file to be output.\n"
				"\n"
			) ;
			return ( 0 ) ;
		} /* endif */
		if ( SourcePath[0] == 0 ) {
			WIN32_FILE_ATTRIBUTE_DATA Attributes ;
			if ( !GetFileAttributesEx ( argv[0], GetFileExInfoStandard, &Attributes ) ) {
				fwprintf ( stderr, L"ERROR: File '%ls' not found.\n", argv[0] ) ;
				return ( 1 ) ;
			} /* endif */
			if ( Attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
				fwprintf ( stderr, L"ERROR: File '%ls' is a directory.\n", argv[0] ) ;
				return ( 1 ) ;
			} /* endif */
			wcscpy_s ( SourcePath, _countof(SourcePath), argv[0] ) ;
			continue ;
		} else if ( DestinPath[0] == 0 ) {
			WIN32_FILE_ATTRIBUTE_DATA Attributes ;
			if ( GetFileAttributesEx ( argv[0], GetFileExInfoStandard, &Attributes ) ) {
				if ( Attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
					fwprintf ( stderr, L"ERROR: File '%ls' is a directory.\n", argv[0] ) ;
					return ( 1 ) ;
				} /* endif */
				if ( Attributes.dwFileAttributes & FILE_ATTRIBUTE_READONLY ) {
					fwprintf ( stderr, L"ERROR: File '%ls' is a read-only.\n", argv[0] ) ;
					return ( 1 ) ;
				} /* endif */
			} /* endif */
			wcscpy_s ( DestinPath, _countof(SourcePath), argv[0] ) ;
			continue ;
		} /* endif */
		fwprintf ( stderr, L"ERROR: Unrecognized parameter '%ls'.\n", argv[0] ) ;
		return ( 1 ) ;
	} /* endwhile */
	if ( SourcePath[0] == 0 ) {
		fprintf ( stderr, "ERROR: No source file specified.\n" ) ;
		return ( 1 ) ;
	} else if ( DestinPath[0] == 0 ) {
		fprintf ( stderr, "ERROR: No destination file specified.\n" ) ;
		return ( 1 ) ;
	} /* endif */

	// Load the dictionary.
	printf ( "Loading dictionary file.\n" ) ;
	BuildDictionary ( L"JDICT\\EDICT" ) ;
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
	AddWord ( L"能登",			L"のと",			L"(name) Noto" ) ;
	AddWord ( L"久光",			L"ひさみつ",		L"(name) Hisamitsu" ) ;
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
	AddWord ( L"リンダ",		L"リンダ",			L"(name) Linda" ) ;
	AddWord ( L"奈々",			L"なな",			L"(name) Nana" ) ;
	AddWord ( L"岡",			L"おか",			L"(name) Oka" ) ;
	AddWord ( L"千波",			L"ちなみ",			L"(name) Chinami" ) ;
	AddWord ( L"二次元くん",	L"にじげんくん",	L"(name) Mr. Two Dimensions" ) ;
	AddWord ( L"異次元くん",	L"いじげんくん",	L"(name) Mr. Different Dimensions" ) ;
	AddWord ( L"佐藤",			L"さとう",			L"(name) Satou" ) ;
	AddWord ( L"隆哉",			L"たかや",			L"(name) Takaya" ) ;
	AddWord ( L"さおちゃん",	L"さおちゃん",		L"(name) Sao-chan" ) ;
	AddWord ( L"しーちゃん",	L"しいちゃん",		L"(name) Shii-chan" ) ;
	AddWord ( L"伊集院",		L"いじゅういん",	L"(name) Ijuuin" ) ;
	AddWord ( L"次元大介",		L"じげんだいすけ",	L"(name) Jigen Daisuke" ) ;
	AddWord ( L"電撃文庫",		L"でんげきぶんこ",	L"(name) Dengeki Bunko" ) ;
	AddWord ( L"フライングシャイン", L"フライングシャイン", L"(name) FlyingShine (a company)" ) ;
	AddWord ( L"毘沙門夭国",	L"びしゃもんてんごく",L"(name) Bishamontengoku, a fictional hostess bar in Toradora!" ) ;
	AddWord ( L"表参道",		L"おもてさんどう",  L"(place) Omotesandou, an upscale district of Tokyo." ) ;
	AddWord ( L"麻布",			L"あざぶ",			L"(place) Azabu, an area within Minato, Tokyo." ) ;
	AddWord ( L"白金",			L"しろかね",        L"(place) Shirokane, a district of Minato, Tokyo." ) ;
	AddWord ( L"手乗りタイガー",L"てのりたいが",    L"(name) Palmtop Tiger, Aisaka Taiga's nickname" ) ;
	AddWord ( L"ばかちー",		L"ばかちい",		L"(name) Taiga's derogatory nickname for Ami: Stupid Chihuahua" ) ;
	AddWord ( L"ぶたっ鼻",      L"ぶたはな",		L"(n) pig-nose" ) ;
	AddWord ( L"租",			L"そ",				L"(n) tariff; crop tax; borrowing;" ) ;
	AddWord ( L"卑",			L"ひ",				L"(adj) base; lowly; vile; vulgar;" ) ;
	AddWord ( L"カゴ",			L"かご",			L"(n) basket; cage; (P)" ) ;
	AddWord ( L"ペコペコ",      L"ぺこぺこ",        L"(adj-na,adv,n,vs) (1) (See ぺこん) (on-mim) fawning; being obsequious; cringing; (2) being very hungry; (3) giving in; being dented; (P)" ) ;
	AddWord ( L"意地汚さ",		L"いじきたなさ",    L"(adj-i) gluttonous; greedy" ) ; // NOTE: Add "sa" and "so" forms of i-adjectives?
	AddWord ( L"ハァト",		L"ハァト",          L"(n) heart (P)" ) ;
	AddWord ( L"ヒップホップ",  L"ヒップホップ",	L"(kana) hip-hop" ) ;
	AddWord ( L"アティチュード",L"アティチュード",	L"(n) attitude" ) ;
	AddWord ( L"Ｔシャツ",		L"ティーシャツ",	L"(n) T-shirt; (P)" ) ;
	AddWord ( L"竦む",          L"すくむ",          L"(v5m) cower; crouch (P)" ) ;
	AddWord ( L"猛々しさ",		L"たけだけしさ",	L"(adj-i) rather ferocious" ) ;
	AddWord ( L"真撃",			L"しんし",			L"(adv) sincerely" ) ;
	AddWord ( L"摑む",			L"つかむ",			L"(v5m,vt) to seize/to catch/to grasp/to grip/to grab/to hold/to catch hold of/to lay one's hands on/(P)/" ) ;
	AddWord ( L"摑まる",		L"つかまる",		L"(v5r) to be caught; to be arrested" ) ;
	AddWord ( L"摑まえる",		L"つかまえる",		L"(v1,vt) to catch; to arrest; to sieze" ) ;
	AddWord ( L"うま煮そば",	L"うまにそば",		L"(n) fish/meat boiled in soy sauce and sugar over buckwheat noodles" ) ;
	AddWord ( L"捻じ曲げる",	L"ねじまげる",		L"(v1,vt) to twist; to distort" ) ;
	AddWord ( L"激厚",			L"げきあつ",		L"(adj) very thick; awfully thick" ) ;
	AddWord ( L"搔く",			L"かく",			L"(v5k) to scratch" ) ;
	AddWord ( L"引き剝がす",	L"ひきはがす",		L"(v5s) to tear off" ) ;
	AddWord ( L"剝がす",		L"はがす",			L"(v5s) peel/peel off/shell/strip" ) ;
	AddWord ( L"噓",			L"うそ",			L"(n) lie; praise; flatter; hiss; exhale; deep sigh; blow out;" ) ;
	AddWord ( L"捻じ曲がる",	L"ねじまがる",		L"(io) (v5r,vt) (1) (uk) to screw/to twist/(2) to distort/to parody/to make a pun/(3) to torture/to wrest/" ) ;
	AddWord ( L"麵",			L"むぎこ",			L"(n) dough; flour; noodles;" ) ;
	AddWord ( L"慄く",			L"おののく",		L"(v5k,vi) to shake; to shudder " ) ;
	AddWord ( L"描線",			L"びょうせん",		L"(n) line" ) ;
	AddWord ( L"群馬",			L"ぐんま",			L"(n) Gunma (prefecture)" ) ;
	AddWord ( L"黙禱",			L"もくとう",		L"(n,vs) silent prayer; (P)" ) ;
	AddWord ( L"ユニクロ",		L"ユニクロ",		L"(name) UNIQLO, a clothing brand." ) ;

	// Sloppy or exaggerated speech
	AddWord ( L"そういや",      L"そういや",		L"(slur) そういえば = (exp) (uk) which reminds me .../come to think of it .../now that you mention it .../on that subject .../so, if you say .../(P)/" ) ;
	AddWord ( L"ですぅー",		L"ですぅー",		L"copula, polite form (exaggerated)" ) ;
	AddWord ( L"こんにちはー",	L"こんにちはー",	L"(exp) Good Afternoooon" ) ;
	AddWord ( L"モン",			L"モン",			L"(n) (1) thing/object/(2) (uk) the natural thing/a frequently done thing/(3) (uk) used to express emotional involvement/(4) (uk) used in giving a reason/" ) ;
	AddWord ( L"いやあ",		L"いやあ",			L"(adj-na,n) disagreeable/detestable/unpleasant/reluctant/(P)/" ) ;
	AddWord ( L"なーに",		L"なーに",			L"(int,pn,adj-no) (1) what/(n) (2) (col) euph. for genitals or sex/(P)/" ) ;
	AddWord ( L"そこのち",		L"そこのち",		L"(slur) that house/that home (from そこの家)" ) ;
	AddWord ( L"ノリ",			L"ノリ",			L"(n,n-suf) (1) riding; ride; (2) spread (of paints); (3) (two)-seater; (4) (uk) (possibly from 気乗り) mood (as in to pick up on and join in with someone's mood);" ) ;
	AddWord ( L"よぉぉ",		L"よぉぉ",			L"(prt) (1) (at sentence end) indicates indicates certainty, emphasis, contempt, request, etc./(2) (after a noun) used when calling out to someone/(3) (in mid-sentence) used to catch one's breath or get someone's attention/(int) (4) yo!/(P)/" ) ;
	AddWord ( L"よぉ",			L"よぉ",			L"(prt) (1) (at sentence end) indicates indicates certainty, emphasis, contempt, request, etc./(2) (after a noun) used when calling out to someone/(3) (in mid-sentence) used to catch one's breath or get someone's attention/(int) (4) yo!/(P)/" ) ;
	AddWord ( L"ねぇぇ",		L"ねぇぇ",			L"(prt) (1) (sentence end) indicates emphasis, agreement, request for confirmation, etc./is it so/(int) (2) hey/come on/(P)/" ) ;
	AddWord ( L"ねぇ",			L"ねぇ",			L"(prt) (1) (sentence end) indicates emphasis, agreement, request for confirmation, etc./is it so/(int) (2) hey/come on/(P)/" ) ;

	// Colloquial Speech
	AddWord ( L"真面目くさる",	L"まじめくさる",	L"(v5r,vi) to become extremely serious (in attitude)/to look solemn/to assume a solemn air/" ) ;
	AddWord ( L"おばちゃん",    L"おばちゃん",      L"(n,coll) lady (older)" ) ;
	AddWord ( L"ツラ",			L"ツラ",			L"(n) (1) face (often derog. or vulg.)/mug/(2) surface/(3) (arch) surrounding area/(4) (arch) cheek/cheeks/" ) ;
	AddWord ( L"グッと",		L"ぐっと",			L"(adv) (on-mim) firmly; fast; much; more" ) ;
	AddWord ( L"死ぬっほど",	L"死ぬっほど",		L"(exp) to death; to the utmost extreme/(P)/" ) ;
	AddWord ( L"すっごい",		L"すっごい",		L"(adj-i) (1) (uk) terrible/dreadful/(2) amazing (e.g. of strength)/great (e.g. of skills)/wonderful/terrific/(3) to a great extent/vast (in numbers)/(P)/" ) ;
	AddWord ( L"ほんっと",		L"ほんっと",		L"(adj-na,n) truth/reality/(P)/" ) ;
	AddWord ( L"だーれ",		L"だーれ",			L"(pn,adj-no) who/(P)/" ) ;
	AddWord ( L"チラっと",      L"ちらっと",        L"(adv) (See ちらりと) (on-mim) at a glance; by accident; (P)" ) ;
	AddWord ( L"ぶたバラ",		L"ぶたバラ",		L"(n) pork block" ) ;
	AddWord ( L"やーだー",		L"やーだー",		L"(exp) No way!" ) ;
	AddWord ( L"はっ",          L"はっ",            L"(int) Ha" ) ;
	AddWord ( L"ゴク",			L"ゴク",			L"gulp; swallow" ) ;
	AddWord ( L"ケチくさい",	L"ケチくさい",		L"(adj-i) mean; stingy" ) ;
	AddWord ( L"ビックゥ",      L"ビックゥ",        L"shock;surprise (derived from bikkuri)" ) ;
	AddWord ( L"知らねぇ",      L"しらねぇ",        L"(adj-i) (1) unknown/strange/(2) (id) I don't care/I don't know and I feel nothing/" ) ;
	AddWord ( L"わっがまま",	L"わっがまま",		L"(adj-na,n) (1) selfishness/egoism/self-indulgence/wilfulness/willfulness/(2) disobedience/(3) whim/" ) ;
	AddWord ( L"ステーップ",    L"ステーップ",		L"(n,vs) (1) step/(n) (2) steppe/(P)/" ) ;
	AddWord ( L"買う人",		L"かうひと",		L"(n) Buyer(s)" ) ;
	AddWord ( L"汚部屋",		L"おべや",			L"(n) dirty room" ) ;
	AddWord ( L"さっすが",		L"さっすが",		L"(adj-na,adv,n,adj-no) (1) (uk) as one would expect/(2) still/all the same/(3) even... (e.g. \"even a genius...\")/(P)/" ) ;
	AddWord ( L"してんの",      L"してんの",        L"(coll) shite iru no = doing" ) ; 
	AddWord ( L"してる",        L"してる",          L"(coll) shite iru = doing" ) ; 
	AddWord ( L"るんの",        L"るんの",          L"(coll) suru no = do" ) ;
	AddWord ( L"っつって",		L"っつって",		L"(coll) ... to itte = ... said" ) ;
	AddWord ( L"くんない",		L"くんない",		L"(coll, adv, from 呉れない) (uk) to not give/to not let one have/to not do for one/to not be given/(P)/" ) ;
	AddWord ( L"の事",			L"のこと",			L"(exp) things related to what was just named" ) ;
	AddWord ( L"切羽詰った状況",L"せっぱつまったじょうきょう",L"every man for himself" ) ;
	AddWord ( L"けてる",		L"けてる",			L"(col,exp,from 行くことができている) (uk) being able to go do ..." ) ;

	// Copula forms
	AddWord ( L"です",			L"です",			L"copula, polite form" ) ;
	AddWord ( L"でわありません",L"でわありません",	L"copula, polite negative form" ) ;
	AddWord ( L"じゃありません",L"じゃありません",	L"copula, polite negative form" ) ;
	AddWord ( L"でした",		L"でした",			L"copula, polite past form" ) ;
	AddWord ( L"だ",			L"だ",				L"copula, plain form" ) ;
	AddWord ( L"じゃーん",		L"じゃーん",		L"copula, plain form (very colloquial)" ) ;
	AddWord ( L"でわない",		L"でわない",		L"copula, plain negative form" ) ;
	AddWord ( L"ではない",		L"ではない",		L"copula, plain negative form (alt spelling)" ) ;
	AddWord ( L"じゃない",		L"じゃない",		L"copula, plain negative form" ) ;
	AddWord ( L"じゃねぇ",		L"じゃねぇ",		L"copula, plain negative form (colloquial form)" ) ;
	AddWord ( L"だった",		L"だった",			L"copula, plain past form" ) ;

	// Unknown words added to speed the parser.
	AddWord ( L"フヒヒ",		L"フヒヒ",			L"Hehe! (laughter)" ) ;
	AddWord ( L"ひゃー",		L"ひゃー",			L"Hyaa! (like a karate chop)" ) ;
	AddWord ( L"ぎゃあ",		L"ぎゃあ",			L"Yipes! (exclamation of dismay)" ) ;
	AddWord ( L"どよん",		L"どよん",			L"(mim) sluggish and exhausted; depressed" ) ;
	AddWord ( L"クラ",			L"クラ",			L"(mim) dizziness" ) ;
	AddWord ( L"ダサダサ",		L"ださださ",		L"(adj,col) very awkward; very uncool" ) ;
	AddWord ( L"ズバ",			L"ズバ",			L"(col) Sound of arrow hitting target; Bingo!; I know!;" ) ;
	AddWord ( L"サマ",			L"サマ",			L"(n,suf) (1) (pol) Mr, Mrs or Ms/(2) used (gen. in fixed expressions) to make words more polite/(3) manner/kind/appearance/(P)/" ) ;
	AddWord ( L"グネグネ",		L"グネグネ",		L"(on,mim) zig-zag" ) ;
	AddWord ( L"ユサユサ",		L"ゆさゆさ",		L"(adv,n,vs) (on-mim) large thing swaying" ) ;
	AddWord ( L"もく",			L"もく",			L"(contraction) short for 目的: (n) purpose; goal; aim; objective; intention; (P)" ) ;

	// Odd words suffixed by -tto
//	AddWord ( L"チラっと",		L"チラっと",		L"(adv) (See ちらりと) (on-mim) at a glance; by accident; (P)" ) ;
	AddWord ( L"ずぼっと",		L"ずぼっと",		L"???" ) ;
	AddWord ( L"どべっと",		L"どべっと",		L"???" ) ;
	AddWord ( L"どべーっと",	L"どべーっと",		L"???" ) ;
	AddWord ( L"ぷりっと",		L"ぷりっと",		L"Fresh (pure-like)???" ) ;
	AddWord ( L"ペロっと",		L"ペロっと",		L"Doglike???" ) ;
	AddWord ( L"ポツーン",		L"ポツーン",		L"(mim) loneliness, separation" ) ;
	AddWord ( L"ばしっと",		L"ばしっと",		L"(mim) thumpingly" ) ;

	// Build the dictionary index.
	printf ( "Indexing the dictionary.\n" ) ;
	BuildIndex ( ) ;
	printf ( "  %zi index entries.\n", Index.size() ) ;

	// Announce start of file processing.
	printf ( "Processing file '%ls' to '%ls'...\n", SourcePath, DestinPath ) ;

	// Open the source file.
	FILE* SourceFile ; errno_t Error = _wfopen_s ( &SourceFile, SourcePath, L"rb" ) ;
	if ( Error ) {
		char Buffer[200]; strerror_s(Buffer, _countof(Buffer), Error);
		fwprintf ( stderr, L"ERROR: Unable to open source file.  Error %i:%hs\n", Error, Buffer ) ;
		return ( 1 ) ;
	} /* endif */

	// Validate the code type tag.  Must be UNICODE.
	unsigned short Tag ;
	if ( ( fread ( &Tag, sizeof(Tag), 1, SourceFile ) < 1 ) || ( Tag != 0xFEFF ) ) {
		fwprintf ( stderr, L"ERROR: Invalid source file '%s'.  Not tagged as Unicode.\n", SourcePath ) ;
		fclose ( SourceFile ) ;
		return ( 1 ) ;
	} /* endif */

	// Open/Create the destination file.
	FILE *DestinFile ; Error = _wfopen_s ( &DestinFile, DestinPath, L"wt,ccs=UNICODE" ) ;
	if ( Error ) {
		char Buffer[200]; strerror_s(Buffer, _countof(Buffer), Error);
		fprintf ( stderr, "ERROR: Unable to open/create destination file.  Error %i:%s\n", Error, Buffer ) ;
		fclose ( SourceFile ) ;
		return ( 1 ) ;
	} /* endif */

	// Read the source file and process its contents.
	wchar_t Line [0x1000] ;  size_t cbLine = 0 ; unsigned nLines = 0 ;
	wchar_t Buffer [0x1000] ; wchar_t *pBuffer = Buffer ;
	size_t cbBuffer = fread ( Buffer, sizeof(Buffer[0]), _countof(Buffer), SourceFile ) ;
	while ( !fAbort && cbBuffer  ) {

		// Add character to output line buffer.
		Line[cbLine++] = *pBuffer++ ; 

		// If we've completed a line, flush it now.
		if ( ( cbLine >= 2 ) && ( Line[cbLine-2] == 0x0D ) && ( Line[cbLine-1] == 0x0A ) )
			ProcessParagraph ( DestinFile, Line, cbLine, nLines ) ;

		// Get more data if we need to.
		if ( pBuffer >= Buffer + cbBuffer ) {
			pBuffer = Buffer ;
			cbBuffer = fread ( Buffer, sizeof(Buffer[0]), _countof(Buffer), SourceFile ) ;
		} /* endif */

	} /* endwhile */

	// If the line buffer still has something in it, write it out now.
	if ( !fAbort )
		ProcessParagraph ( DestinFile, Line, cbLine, nLines ) ;

	// Done.  Clean up.
	fclose ( DestinFile ) ;
	fclose ( SourceFile ) ;
	printf ( "Done.  %i lines processed.\n", nLines ) ;

	// Exit rather than return.  This sidesteps the entire STL teardown process.
	ExitProcess ( 0 ) ;
}
