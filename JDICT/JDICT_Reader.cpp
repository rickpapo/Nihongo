#include "stdafx.h"
#include <assert.h>
#include <memory>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <set>
#include <map>
#include "..\JIS0208\JIS0208.h"
#include "JDICT_Reader.h"

_Counter DictionaryInfos, Symbols, Sounds, SymbolSounds, DictionaryEntries, IndexEntries ;

extern wchar_t *szConjugationTypes [CT_MAX] = { 
	L"(Unconjugated)", L"Stem",
	L"Plain Positive", L"Plain Negative (1)", L"Plain Negative (2)", 
	L"Plain Positive Past",	L"Plain Negative Past", 
	L"Plain Imperative (1)", L"Plain Imperative (2)", L"Polite Imperative", L"Polite Negative Imperative",
	L"Plain Presumptive (1)", L"Plain Presumptive (2)", 
	L"Plain Conjunctive", 
	L"Plain Positive Provisional", L"Plain Negative Provisional (1)", L"Plain Negative Provisional (2)", 
	L"Plain Positive Conditional", L"Plain Negative Conditional", 
	L"Polite Positive Conditional", L"Polite Negative Conditional", 
	L"Plain Passive", L"Plain Passive Past", L"Plain Wish", 
	L"Polite Positive", L"Polite Negative", 
	L"Polite Positive Past", L"Polite Negative Past", 
	L"Polite Presumptive", L"Adverb", 
	L"Potential (1)", L"Potential (2)", 
	L"Causative", L"Negative Conjunctive", 
	L"Plain Positive Alternative",
	L"Plain Negative Alternative",
	L"Polite Positive Alternative",
	L"Polite Negative Alternative",
	L"Plain Progressive",
	L"Plain Progressive Casual",
	L"Polite Progressive",
	L"Plain Progressive Past (1)",
	L"Plain Progressive Past (2)",
	L"Plain Progressive Conjunctive (1)",
	L"Plain Progressive Conjunctive (2)",
	L"Polite Progressive Past",
	L"Noun (from adjective)",
	L"Seems (from adjective)",
	L"Plain Negative Presumptive",
	L"Polite Negative Presumptive",
	L"Polite Positive Conjunctive",
	L"Polite Negative Conjunctive",
} ;

static bool ParseLine ( wchar_t *szText, wchar_t*&pSymbol, wchar_t*&pSound, wchar_t*&pInfo ) {

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
	if ( !wcscmp ( pSound, L"する" ) ) 
		DO_NOTHING ;
	return ( true ) ;
}

inline unsigned short SwapWord ( unsigned short Word ) {
	unsigned char *p = (unsigned char*) &Word ;
	return ( ( p[0] << 8 ) + p[1] ) ;
}

class _Conjugator {
public:
	void ConjugateSymbol(_DictionaryEntry* pEntry, const wchar_t* szSymbol, const wchar_t* szSound, BYTE Conjugations[]);
	virtual _ConjugationClass GetClass() = 0;
	virtual bool Stem(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) = 0;
	virtual bool PlainPositive(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) {
		wcscpy_s(pBuffer, cbBuffer, szWord);
		return (true);
	} /* endmethod */
	virtual bool PlainNegative1(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) = 0;
	virtual bool PlainNegative2(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) { return (false); }
	virtual bool PlainPositivePast(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) = 0;
	virtual bool PlainImperative1(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) { return (false); }
	virtual bool PlainImperative2(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) { return (false); }
	virtual bool PoliteImperative(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) { return false; }
	virtual bool PoliteNegativeImperative(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) { return false; }
	virtual bool PlainPresumptive1(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) = 0;
	virtual bool PlainPresumptive2(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) { return (false); }
	virtual bool PlainConjunctive(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) = 0;
	virtual bool PlainPositiveProvisional(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) { return (false); }
	virtual bool PlainNegativeProvisional1(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) { return (false); }
	virtual bool PlainNegativeProvisional2(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) { return (false); }
	virtual bool PlainPassive(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) { return (false); }
	virtual bool PlainPassivePast(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) { return (false); }
	virtual bool PlainWish(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) {
		Stem(pBuffer, cbBuffer, szWord);
		wcscat_s(pBuffer, cbBuffer, L"たい");
		return (true);
	} /* endmethod */
	virtual bool PolitePositive(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) {
		Stem(pBuffer, cbBuffer, szWord);
		wcscat_s(pBuffer, cbBuffer, L"ます");
		return (true);
	} /* endmethod */
	virtual bool PoliteNegative(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) {
		Stem(pBuffer, cbBuffer, szWord);
		wcscat_s(pBuffer, cbBuffer, L"ません");
		return (true);
	} /* endmethod */
	virtual bool PolitePositivePast(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) {
		Stem(pBuffer, cbBuffer, szWord);
		wcscat_s(pBuffer, cbBuffer, L"ました");
		return (true);
	} /* endmethod */
	virtual bool PoliteNegativePast(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) {
		Stem(pBuffer, cbBuffer, szWord);
		wcscat_s(pBuffer, cbBuffer, L"ませんでした");
		return (true);
	} /* endmethod */
	virtual bool PolitePresumptive(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) {
		Stem(pBuffer, cbBuffer, szWord);
		wcscat_s(pBuffer, cbBuffer, L"ましょう");
		return (true);
	} /* endmethod */
	virtual bool Adverb(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) { return (false); }
	virtual bool Potential1(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) { return (false); }
	virtual bool Potential2(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) { return (false); }
	virtual bool Causative(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) { return (false); }
	virtual bool NegativeConjunctive(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) { return (false); }
	virtual bool Nominalize(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) { return (false); }
	virtual bool Resembles(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) { return (false); }
	virtual bool PlainNegativePresumptive(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) { return (false); }
	virtual bool PoliteNegativePresumptive(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) { return (false); }
	virtual bool PolitePositiveConjunctive(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) {
		Stem(pBuffer, cbBuffer, szWord);
		wcscat_s(pBuffer, cbBuffer, L"まして");
		return (true);
	} /* endmethod */
	virtual bool PoliteNegativeConjunctive(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) {
		Stem(pBuffer, cbBuffer, szWord);
		wcscat_s(pBuffer, cbBuffer, L"ませんで");
		return (true);
	} /* endmethod */
};

class _Ichidan_Conjugator : public _Conjugator {
public:
	_ConjugationClass GetClass ( ) { return ( CC_ICHIDAN ) ; }
	bool Stem ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		// We must have something left after taking off the ending.
		if ( wcslen(szWord) < 2 ) 
			return ( false ) ;
		// Clip the last letter off the dictionary form to get the stem.
		wcscpy_s ( pBuffer, cbBuffer, szWord ) ;
		pBuffer[wcslen(szWord)-1] = 0 ;
		return ( true ) ;
	} /* endmethod */
	bool PlainNegative1 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"ない" ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainNegative2 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"ぬ" ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainPositivePast ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"た" ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainImperative1 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"ろ" ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainImperative2 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"よ" ) ;
		return ( true ) ;
	} /* endmethod */
	bool PoliteImperative(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) {
		Stem(pBuffer, cbBuffer, szWord);
		wcscat_s(pBuffer, cbBuffer, L"なさい");
		return (true);
	} /* endmethod */
	bool PoliteNegativeImperative(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) {
		Stem(pBuffer, cbBuffer, szWord);
		wcscat_s(pBuffer, cbBuffer, L"なさるな");
		return (true);
	} /* endmethod */
	bool PlainPresumptive1 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"よう" ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainConjunctive ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"て" ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainPositiveProvisional ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"れば" ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainNegativeProvisional1 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"なければ" ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainNegativeProvisional2 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"なきゃ" ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainPassive ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"られる" ) ;
		return ( true ) ;
	} /* endmethod */
	bool Potential1 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) { 
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"られる" ) ;
		return ( true ) ;
	} /* endmethod */
	bool Potential2 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) { 
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"れる" ) ;
		return ( true ) ;
	} /* endmethod */
	bool Causative ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) { 
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"させる" ) ;
		return ( true ) ;
	} /* endmethod */
	bool NegativeConjunctive ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"ず" ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainNegativePresumptive ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"まい" ) ;
		return ( true ) ;
	} /* endmethod */
} ;

enum _KanaRow { ROW_A, ROW_I, ROW_U, ROW_E, ROW_O, ROW_NULL } ;
enum _KanaColumn { COL_U, COL_K, COL_G, COL_S, COL_T, COL_N, COL_B, COL_M, COL_R, COL_NULL } ;
_KanaColumn& operator++ ( _KanaColumn &obj ) {
	int Temp = (int) obj ;
	Temp ++ ;
	if ( Temp > COL_NULL )
		Temp = COL_U ;
	obj = (_KanaColumn) Temp ;
	return ( obj ) ;
} /* endmethod */
_KanaColumn& operator++ ( _KanaColumn &obj, int ) {
	int Temp = (int) obj ;
	Temp ++ ;
	if ( Temp > COL_NULL )
		Temp = COL_U ;
	obj = (_KanaColumn) Temp ;
	return ( obj ) ;
} /* endmethod */

static wchar_t szVerb1Endings [ROW_NULL] [COL_NULL] = {
	{ L'わ', L'か', L'が', L'さ', L'た', L'な', L'ば', L'ま', L'ら' },
	{ L'い', L'き', L'ぎ', L'し', L'ち', L'に', L'び', L'み', L'り' },
	{ L'う', L'く', L'ぐ', L'す', L'つ', L'ぬ', L'ぶ', L'む', L'る' },
	{ L'え', L'け', L'げ', L'せ', L'て', L'ね', L'べ', L'め', L'れ' },
	{ L'お', L'こ', L'ご', L'そ', L'と', L'の', L'ぼ', L'も', L'ろ' },
} ;

static wchar_t *szVerb1PositivePastSuffix [COL_NULL] = {
	L"った", L"いた", L"いだ", L"した", L"った", L"んだ", L"んだ", L"んだ", L"った", 
} ;

static wchar_t *szVerb1ConjunctiveSuffix [COL_NULL] = {
	L"って", L"いて", L"いで", L"して", L"って", L"んで", L"んで", L"んで", L"って", 
} ;

class _Godan_Conjugator : public _Conjugator {
	_KanaColumn GetStem ( wchar_t const *pRoot, wchar_t *pBuffer, size_t cbBuffer, _KanaRow Row ) { 
		wcscpy_s ( pBuffer, cbBuffer, pRoot ) ;
		wchar_t *pLastCharacter = pBuffer + wcslen(pBuffer) - 1 ;
		_KanaColumn WhichColumn ;
		for ( WhichColumn = COL_U; WhichColumn<COL_NULL; WhichColumn++ )
			if ( *pLastCharacter == szVerb1Endings[ROW_U][WhichColumn] ) 
				break ;
		if ( WhichColumn >= COL_NULL ) 
			return ( COL_NULL ) ;
		if ( Row == ROW_NULL ) {
			*pLastCharacter = 0 ;
		} else {
			*pLastCharacter = szVerb1Endings [Row] [WhichColumn] ;
		} /* endif */
		return ( WhichColumn ) ;
	} /* endmethod */
public:
	_ConjugationClass GetClass ( ) { return ( CC_GODAN ) ; }
	bool Stem ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		// We must have something left after taking off the ending.
		if ( wcslen(szWord) < 2 ) 
			return ( false ) ;
		// Verb must have a valid ending.
		if ( GetStem ( szWord, pBuffer, cbBuffer, ROW_I ) == COL_NULL )
			return ( false ) ;
		// Proceed if successful.
		return ( true ) ;
	} /* endmethod */
	bool PlainNegative1 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		GetStem ( szWord, pBuffer, cbBuffer, ROW_A ) ;
		wcscat_s ( pBuffer, cbBuffer, L"ない" ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainNegative2 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		GetStem ( szWord, pBuffer, cbBuffer, ROW_A ) ;
		wcscat_s ( pBuffer, cbBuffer, L"ぬ" ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainPositivePast ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		_KanaColumn WhichColumn = GetStem ( szWord, pBuffer, cbBuffer, ROW_NULL ) ;
		wcscat_s ( pBuffer, cbBuffer, szVerb1PositivePastSuffix[WhichColumn] ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainImperative1 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		GetStem ( szWord, pBuffer, cbBuffer, ROW_E ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainImperative2 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		return ( false ) ;
	} /* endmethod */
	bool PoliteImperative(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) {
		GetStem(szWord, pBuffer, cbBuffer, ROW_I);
		wcscat_s(pBuffer, cbBuffer, L"なさい");
		return (true);
	} /* endmethod */
	bool PoliteNegativeImperative(wchar_t* pBuffer, size_t cbBuffer, wchar_t const* szWord) {
		GetStem(szWord, pBuffer, cbBuffer, ROW_I);
		wcscat_s(pBuffer, cbBuffer, L"なさるな");
		return (true);
	} /* endmethod */
	bool PlainPresumptive1 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		GetStem ( szWord, pBuffer, cbBuffer, ROW_O ) ;
		wcscat_s ( pBuffer, cbBuffer, L"う" ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainConjunctive ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		_KanaColumn WhichColumn = GetStem ( szWord, pBuffer, cbBuffer, ROW_NULL ) ;
		wcscat_s ( pBuffer, cbBuffer, szVerb1ConjunctiveSuffix[WhichColumn] ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainPositiveProvisional ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		GetStem ( szWord, pBuffer, cbBuffer, ROW_E ) ;
		wcscat_s ( pBuffer, cbBuffer, L"ば" ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainNegativeProvisional1 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		GetStem ( szWord, pBuffer, cbBuffer, ROW_A ) ;
		wcscat_s ( pBuffer, cbBuffer, L"なければ" ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainNegativeProvisional2 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		GetStem ( szWord, pBuffer, cbBuffer, ROW_A ) ;
		wcscat_s ( pBuffer, cbBuffer, L"なきゃ" ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainPassive ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		GetStem ( szWord, pBuffer, cbBuffer, ROW_A ) ;
		wcscat_s ( pBuffer, cbBuffer, L"れる" ) ;
		return ( true ) ;
	} /* endmethod */
	bool Potential1 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) { 
		GetStem ( szWord, pBuffer, cbBuffer, ROW_E ) ;
		wcscat_s ( pBuffer, cbBuffer, L"る" ) ;
		return ( true ) ;
	} /* endmethod */
	bool Causative ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) { 
		GetStem ( szWord, pBuffer, cbBuffer, ROW_A ) ;
		wcscat_s ( pBuffer, cbBuffer, L"せる" ) ;
		return ( true ) ;
	} /* endmethod */
	bool NegativeConjunctive ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		GetStem ( szWord, pBuffer, cbBuffer, ROW_A ) ;
		wcscat_s ( pBuffer, cbBuffer, L"ず" ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainNegativePresumptive ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		GetStem ( szWord, pBuffer, cbBuffer, ROW_U ) ;
		wcscat_s ( pBuffer, cbBuffer, L"まい" ) ;
		return ( true ) ;
	} /* endmethod */
} ;

class _Iku_Conjugator : public _Godan_Conjugator {
public:
	_ConjugationClass GetClass ( ) { return ( CC_IKU ) ; }
	bool PlainPositivePast ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"った" ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainConjunctive ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"って" ) ;
		return ( true ) ;
	} /* endmethod */
} ;

class _Aru_Conjugator : public _Godan_Conjugator {
public:
	_ConjugationClass GetClass ( ) { return ( CC_ARU ) ; }
	bool PlainNegative1 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		return ( false ) ;
	} /* endmethod */
	bool PlainImperative1 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		return ( false ) ;
	} /* endmethod */
	bool PlainImperative2 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		return ( false ) ;
	} /* endmethod */
	bool PlainPresumptive1 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		return ( false ) ;
	} /* endmethod */
	bool PlainPresumptive2 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		return ( false ) ;
	} /* endmethod */
	bool PlainConjunctive ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		return ( false ) ;
	} /* endmethod */
	bool PlainPassive ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		return ( false ) ;
	} /* endmethod */
} ;

class _Suru_Conjugator : public _Conjugator {
public:
	_ConjugationClass GetClass ( ) { return ( CC_SURU ) ; }
	bool Stem ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		// These verbs must end in SURU.
		size_t cbWord = wcslen ( szWord ) ;
		if ( ( cbWord < 2 ) || ( wcscmp ( szWord+cbWord-2, L"為る" ) && wcscmp ( szWord+cbWord-2, L"する" ) ) )
			return ( false ) ;
		// Clip off する and replace it with し.
		wcscpy_s ( pBuffer, cbBuffer, szWord ) ;
		pBuffer[cbWord-2] = L'し' ;
		pBuffer[cbWord-1] = 0 ;
		return ( true ) ;
	} /* endmethod */
	bool PlainNegative1 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"ない" ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainPositivePast ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"た" ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainImperative1 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"ろ" ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainImperative2 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		size_t cbWord = wcslen ( szWord ) ;
		assert ( cbBuffer > cbWord + 1 ) ;
		wcscpy_s ( pBuffer, cbBuffer, szWord ) ;
		pBuffer[cbWord-2] = L'せ' ;
		pBuffer[cbWord-1] = L'よ' ;
		return ( true ) ;
	} /* endmethod */
	bool PlainPresumptive1 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"よう" ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainPresumptive2 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		size_t cbWord = wcslen ( szWord ) ;
		assert ( cbBuffer > cbWord + 2 ) ;
		wcscpy_s ( pBuffer, cbBuffer, szWord ) ;
		pBuffer[cbWord-2] = L'せ' ;
		pBuffer[cbWord-1] = L'よ' ;
		pBuffer[cbWord+0] = L'う' ;
		pBuffer[cbWord+1] = 0 ;
		return ( true ) ;
	} /* endmethod */
	bool PlainConjunctive ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"て" ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainPositiveProvisional ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		size_t cbWord = wcslen ( szWord ) ;
		assert ( cbBuffer > cbWord + 2 ) ;
		wcscpy_s ( pBuffer, cbBuffer, szWord ) ;
		pBuffer[cbWord-2] = L'す' ;
		pBuffer[cbWord-1] = L'れ' ;
		pBuffer[cbWord+0] = L'ば' ;
		pBuffer[cbWord+1] = 0 ;
		return ( true ) ;
	} /* endmethod */
	bool PlainNegativeProvisional1 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		size_t cbWord = wcslen ( szWord ) ;
		assert ( cbBuffer > cbWord + 4 ) ;
		wcscpy_s ( pBuffer, cbBuffer, szWord ) ;
		pBuffer[cbWord-2] = L'し' ;
		pBuffer[cbWord-1] = L'な' ;
		pBuffer[cbWord-0] = L'け' ;
		pBuffer[cbWord+1] = L'れ' ;
		pBuffer[cbWord+2] = L'ば' ;
		pBuffer[cbWord+3] = 0 ;
		return ( true ) ;
	} /* endmethod */
	bool PlainNegativeProvisional2 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		size_t cbWord = wcslen ( szWord ) ;
		assert ( cbBuffer > cbWord + 3 ) ;
		wcscpy_s ( pBuffer, cbBuffer, szWord ) ;
		pBuffer[cbWord-2] = L'し' ;
		pBuffer[cbWord-1] = L'な' ;
		pBuffer[cbWord-0] = L'き' ;
		pBuffer[cbWord+1] = L'ゃ' ;
		pBuffer[cbWord+2] = 0 ;
		return ( true ) ;
	} /* endmethod */
	bool PlainPassive ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		size_t cbWord = wcslen ( szWord ) ;
		assert ( cbBuffer > cbWord + 2 ) ;
		wcscpy_s ( pBuffer, cbBuffer, szWord ) ;
		pBuffer[cbWord-2] = L'さ' ;
		pBuffer[cbWord-1] = L'れ' ;
		pBuffer[cbWord+0] = L'る' ;
		pBuffer[cbWord+1] = 0 ;
		return ( true ) ;
	} /* endmethod */
	bool Potential1 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) { 
		size_t cbWord = wcslen ( szWord ) ;
		assert ( cbBuffer > cbWord + 2 ) ;
		wcscpy_s ( pBuffer, cbBuffer, szWord ) ;
		pBuffer[cbWord-2] = L'で' ;
		pBuffer[cbWord-1] = L'き' ;
		pBuffer[cbWord+0] = L'る' ;
		pBuffer[cbWord+1] = 0 ;
		return ( true ) ;
	} /* endmethod */
	bool Causative ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) { 
		size_t cbWord = wcslen ( szWord ) ;
		assert ( cbBuffer > cbWord + 2 ) ;
		wcscpy_s ( pBuffer, cbBuffer, szWord ) ;
		pBuffer[cbWord-2] = L'さ' ;
		pBuffer[cbWord-1] = L'せ' ;
		pBuffer[cbWord+0] = L'る' ;
		pBuffer[cbWord+1] = 0 ;
		return ( true ) ;
	} /* endmethod */
} ;

class _Kuru_Conjugator : public _Conjugator {
public:
	_ConjugationClass GetClass ( ) { return ( CC_KURU ) ; }
	bool Stem ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		// These verbs must end in KURU.
		size_t cbWord = wcslen ( szWord ) ;
		if ( ( cbWord < 2 ) )
			return ( false ) ;
		const wchar_t *pKuru = szWord + cbWord - 2 ;
		if ( wcscmp ( pKuru, L"来る" ) && wcscmp ( pKuru, L"來る" ) && wcscmp ( pKuru, L"くる" ) )
			return ( false ) ;
		// Clip off くる and replace it with き.
		wcscpy_s ( pBuffer, cbBuffer, szWord ) ;
		if ( pKuru[0] == L'く' )
			pBuffer[cbWord-2] = L'き' ;
		pBuffer[cbWord-1] = 0 ;
		return ( true ) ;
	} /* endmethod */
	bool PlainNegative1 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		size_t cbWord = wcslen ( szWord ) ;
		assert ( cbBuffer > cbWord + 2 ) ;
		const wchar_t *pKuru = szWord + cbWord - 2 ;
		bool fKanji = pKuru[0] == L'来' ;
		wcscpy_s ( pBuffer, cbBuffer, szWord ) ;
		pBuffer[cbWord-2] = fKanji ? L'来' : L'こ' ;
		pBuffer[cbWord-1] = L'な' ;
		pBuffer[cbWord+0] = L'い' ;
		pBuffer[cbWord+1] = 0 ;
		return ( true ) ;
	} /* endmethod */
	bool PlainPositivePast ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"た" ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainImperative1 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		size_t cbWord = wcslen ( szWord ) ;
		assert ( cbBuffer > cbWord + 1 ) ;
		const wchar_t *pKuru = szWord + cbWord - 2 ;
		bool fKanji = pKuru[0] == L'来' ;
		wcscpy_s ( pBuffer, cbBuffer, szWord ) ;
		pBuffer[cbWord-2] = fKanji ? L'来' : L'こ' ;
		pBuffer[cbWord-1] = L'い' ;
		return ( true ) ;
	} /* endmethod */
	bool PlainPresumptive1 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		size_t cbWord = wcslen ( szWord ) ;
		assert ( cbBuffer > cbWord + 2 ) ;
		const wchar_t *pKuru = szWord + cbWord - 2 ;
		bool fKanji = pKuru[0] == L'来' ;
		wcscpy_s ( pBuffer, cbBuffer, szWord ) ;
		pBuffer[cbWord-2] = fKanji ? L'来' : L'こ' ;
		pBuffer[cbWord-1] = L'よ' ;
		pBuffer[cbWord+0] = L'う' ;
		pBuffer[cbWord+1] = 0 ;
		return ( true ) ;
	} /* endmethod */
	bool PlainConjunctive ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"て" ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainPositiveProvisional ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		size_t cbWord = wcslen ( szWord ) ;
		assert ( cbBuffer > cbWord + 2 ) ;
		const wchar_t *pKuru = szWord + cbWord - 2 ;
		bool fKanji = pKuru[0] == L'来' ;
		wcscpy_s ( pBuffer, cbBuffer, szWord ) ;
		pBuffer[cbWord-2] = fKanji ? L'来' : L'こ' ;
		pBuffer[cbWord-1] = L'れ' ;
		pBuffer[cbWord+0] = L'ば' ;
		pBuffer[cbWord+1] = 0 ;
		return ( true ) ;
	} /* endmethod */
	bool PlainNegativeProvisional1 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		size_t cbWord = wcslen ( szWord ) ;
		assert ( cbBuffer > cbWord + 4 ) ;
		const wchar_t *pKuru = szWord + cbWord - 2 ;
		bool fKanji = pKuru[0] == L'来' ;
		wcscpy_s ( pBuffer, cbBuffer, szWord ) ;
		pBuffer[cbWord-2] = fKanji ? L'来' : L'こ' ;
		pBuffer[cbWord-1] = L'な' ;
		pBuffer[cbWord-0] = L'け' ;
		pBuffer[cbWord+1] = L'れ' ;
		pBuffer[cbWord+2] = L'ば' ;
		pBuffer[cbWord+3] = 0 ;
		return ( true ) ;
	} /* endmethod */
	bool PlainNegativeProvisional2 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		size_t cbWord = wcslen ( szWord ) ;
		assert ( cbBuffer > cbWord + 3 ) ;
		const wchar_t *pKuru = szWord + cbWord - 2 ;
		bool fKanji = pKuru[0] == L'来' ;
		wcscpy_s ( pBuffer, cbBuffer, szWord ) ;
		pBuffer[cbWord-2] = fKanji ? L'来' : L'こ' ;
		pBuffer[cbWord-1] = L'な' ;
		pBuffer[cbWord-0] = L'き' ;
		pBuffer[cbWord+1] = L'ゃ' ;
		pBuffer[cbWord+2] = 0 ;
		return ( true ) ;
	} /* endmethod */
	bool PlainPassive ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		size_t cbWord = wcslen ( szWord ) ;
		assert ( cbBuffer > cbWord + 3 ) ;
		const wchar_t *pKuru = szWord + cbWord - 2 ;
		bool fKanji = pKuru[0] == L'来' ;
		wcscpy_s ( pBuffer, cbBuffer, szWord ) ;
		pBuffer[cbWord-2] = fKanji ? L'来' : L'こ' ;
		pBuffer[cbWord-1] = L'ら' ;
		pBuffer[cbWord+0] = L'れ' ;
		pBuffer[cbWord+1] = L'る' ;
		pBuffer[cbWord+2] = 0 ;
		return ( true ) ;
	} /* endmethod */
	bool Potential1 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) { 
		size_t cbWord = wcslen ( szWord ) ;
		assert ( cbBuffer > cbWord + 3 ) ;
		const wchar_t *pKuru = szWord + cbWord - 2 ;
		bool fKanji = pKuru[0] == L'来' ;
		wcscpy_s ( pBuffer, cbBuffer, szWord ) ;
		pBuffer[cbWord-2] = fKanji ? L'来' : L'こ' ;
		pBuffer[cbWord-1] = L'ら' ;
		pBuffer[cbWord+0] = L'れ' ;
		pBuffer[cbWord+1] = L'る' ;
		pBuffer[cbWord+2] = 0 ;
		return ( true ) ;
	} /* endmethod */
	bool Potential2 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) { 
		size_t cbWord = wcslen ( szWord ) ;
		assert ( cbBuffer > cbWord + 2 ) ;
		const wchar_t *pKuru = szWord + cbWord - 2 ;
		bool fKanji = pKuru[0] == L'来' ;
		wcscpy_s ( pBuffer, cbBuffer, szWord ) ;
		pBuffer[cbWord-2] = fKanji ? L'来' : L'こ' ;
		pBuffer[cbWord-1] = L'れ' ;
		pBuffer[cbWord+0] = L'る' ;
		pBuffer[cbWord+1] = 0 ;
		return ( true ) ;
	} /* endmethod */
	bool Causative ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) { 
		size_t cbWord = wcslen ( szWord ) ;
		assert ( cbBuffer > cbWord + 3 ) ;
		const wchar_t *pKuru = szWord + cbWord - 2 ;
		bool fKanji = pKuru[0] == L'来' ;
		wcscpy_s ( pBuffer, cbBuffer, szWord ) ;
		pBuffer[cbWord-2] = fKanji ? L'来' : L'こ' ;
		pBuffer[cbWord-1] = L'さ' ;
		pBuffer[cbWord+0] = L'せ' ;
		pBuffer[cbWord+1] = L'る' ;
		pBuffer[cbWord+2] = 0 ;
		return ( true ) ;
	} /* endmethod */
} ;

class _Adjective_Conjugator : public _Conjugator {
public:
	bool PolitePositive ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		return ( false ) ;
	} /* endmethod */
	bool PoliteNegative ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		return ( false ) ;
	} /* endmethod */
	bool PolitePositivePast ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		return ( false ) ;
	} /* endmethod */
	bool PoliteNegativePast ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		return ( false ) ;
	} /* endmethod */
	bool PolitePresumptive ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		return ( false ) ;
	} /* endmethod */
	bool PlainPositive ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		return ( false ) ;
	} /* endmethod */
	bool PlainPassive ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		return ( false ) ;
	} /* endmethod */
	bool PlainWish ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		return ( false ) ;
	} /* endmethod */
} ;

class _AdjectiveI_Conjugator : public _Adjective_Conjugator {
public:
	_ConjugationClass GetClass ( ) { return ( CC_ADJ_I ) ; }
	bool Stem ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		// We must have something left after taking off the ending.
		size_t cbWord = wcslen(szWord) ;
		if ( cbWord < 2 ) 
			return ( false ) ;
		// These words must end in い.
		if ( szWord[cbWord-1] != L'い' )
			return ( false ) ;
		// Clip the last letter off the dictionary form to get the stem.
		wcscpy_s ( pBuffer, cbBuffer, szWord ) ;
		pBuffer[wcslen(szWord)-1] = 0 ;
		return ( true ) ;
	} /* endmethod */
	bool PlainPositive ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"い" ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainNegative1 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"くない" ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainPositivePast ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"かった" ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainPresumptive1 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"かろう" ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainConjunctive ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"くて" ) ;
		return ( true ) ;
	} /* endmethod */
	bool Adverb ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"く" ) ;
		return ( true ) ;
	} /* endmethod */
	bool Causative ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) { 
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"くさせる" ) ;
		return ( true ) ;
	} /* endmethod */
	bool Nominalize ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) { 
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"さ" ) ;
		return ( true ) ;
	} /* endmethod */
	bool Resembles ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) { 
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"そう" ) ;
		return ( true ) ;
	} /* endmethod */
} ;

class _AdjectiveNA_Conjugator : public _Adjective_Conjugator {
public:
	_ConjugationClass GetClass ( ) { return ( CC_ADJ_NA ) ; }
	bool Stem ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		// We aren't using the stem for anything except to indicate that we can do other conjugations.
		wcscpy_s ( pBuffer, cbBuffer, szWord ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainPositive ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"な" ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainNegative1 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		wcscpy_s ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"でわない" ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainNegative2 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		wcscpy_s ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"じゃない" ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainPositivePast ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		wcscpy_s ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"だった" ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainPresumptive1 ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"だろう" ) ;
		return ( true ) ;
	} /* endmethod */
	bool PlainConjunctive ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"で" ) ;
		return ( true ) ;
	} /* endmethod */
	bool Adverb ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) {
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"に" ) ;
		return ( true ) ;
	} /* endmethod */
	bool Causative ( wchar_t *pBuffer, size_t cbBuffer, wchar_t const *szWord ) { 
		Stem ( pBuffer, cbBuffer, szWord ) ;
		wcscat_s ( pBuffer, cbBuffer, L"にさせる" ) ;
		return ( true ) ;
	} /* endmethod */
} ;

void _DictionaryEntry::Conjugate ( const wchar_t *szSymbol, const wchar_t *szSound, BYTE Conjugations[], _ConjugationClass cc ) {
	switch ( ( cc==CC_NULL ) ? ConjugationClass : cc ) {
		case CC_ICHIDAN: {
			IchidanConjugator->ConjugateSymbol ( this, szSymbol, szSound, Conjugations ) ;
			break ; }
		case CC_GODAN:
			GodanConjugator->ConjugateSymbol ( this, szSymbol, szSound, Conjugations ) ;
			break ;
		case CC_IKU:
			IkuConjugator->ConjugateSymbol ( this, szSymbol, szSound, Conjugations ) ;
			break ;
		case CC_ARU:
			AruConjugator->ConjugateSymbol ( this, szSymbol, szSound, Conjugations ) ;
			break ;
		case CC_SURU:
			SuruConjugator->ConjugateSymbol ( this, szSymbol, szSound, Conjugations ) ;
			break ;
		case CC_KURU:
			KuruConjugator->ConjugateSymbol ( this, szSymbol, szSound, Conjugations ) ;
			break ;
		case CC_ADJ_I:
			AdjectiveIConjugator->ConjugateSymbol ( this, szSymbol, szSound, Conjugations ) ;
			break ;
		case CC_ADJ_NA:
			AdjectiveNAConjugator->ConjugateSymbol ( this, szSymbol, szSound, Conjugations ) ;
			break ;
	} /* endswitch */
	fConjugated = true ;
} /* endmethod */

void _DictionaryEntry::CheckToken ( wchar_t *szToken ) {
	size_t cbToken = wcslen(szToken) ;
	if ( cbToken > 8 ) 
		return ;
	if ( szToken[0] == L'v' ) { // verb
		if ( ( cbToken == 2 ) && ( szToken[1] == L'1' ) ) { // Ichidan Verbs
			ConjugationClass = CC_ICHIDAN ;
		} else if ( ( cbToken == 2 ) && ( szToken[1] == L'k' ) ) { // kuru verb - special class
			ConjugationClass = CC_KURU ;
		} else if ( ( cbToken == 2 ) && ( szToken[1] == L'i' ) ) { // Intransitive Verbs
			DO_NOTHING ;
		} else if ( ( cbToken == 2 ) && ( szToken[1] == L't' ) ) { // Transitive Verbs
			DO_NOTHING ;
		} else if ( ( cbToken > 2 ) && ( szToken[1] == L'5' ) ) { // Godan Verbs
			if ( cbToken == 3 ) {
				switch ( szToken[2] ) {
					case 'b': case 'g': case 'k': case 'm': case 'n':
					case 'r': case 's': case 't': case 'u': case 'z':
						ConjugationClass = CC_GODAN ;
				} /* endswitch */
			} else if ( ( cbToken == 5 ) && !wcscmp ( szToken, L"v5k-s" ) ) { // Godan verb - iku/yuku special class
				ConjugationClass = CC_IKU ;
			} else if ( ( cbToken == 5 ) && !wcscmp ( szToken, L"v5aru" ) ) { // Godan verb - -aru special class
				ConjugationClass = CC_ARU ;
			} else {
				DO_NOTHING ;
			} /* endif */
		} else if ( ( cbToken == 4 ) && ( szToken[1] == L'1' ) ) { // 呉れる (くれる) only.
			if ( !wcscmp ( szToken, L"v1-s" ) ) {
				ConjugationClass = CC_ICHIDAN ;
			}
		} else if ( ( cbToken == 4 ) && ( szToken[1] == L's' ) ) { // suru verb - special class
			if ( !wcscmp ( szToken, L"vs-i" ) ) {
				ConjugationClass = CC_SURU ;
			} else if ( !wcscmp ( szToken, L"vs-s" ) ) {
				ConjugationClass = CC_SURU ;
			} else {
				DO_NOTHING ;
			} /* endif */
		} else {
			DO_NOTHING ;
		} /* endif */
	} else if ( ( cbToken == 3 ) && !wcscmp ( szToken, L"exp" ) ) {
		DO_NOTHING ;
	} else if ( ( cbToken == 5 ) && !wcscmp ( szToken, L"adj-i" ) ) {
		ConjugationClass = CC_ADJ_I ;
	} else if ( ( cbToken == 6 ) && !wcscmp ( szToken, L"adj-na" ) ) {
		ConjugationClass = CC_ADJ_NA ;
	} else if ( ( cbToken == 1 ) && ( szToken[0] == L'n' ) ) {
		DO_NOTHING ;
	} else if ( ( cbToken == 1 ) && ( szToken[0] == L'P' ) ) {
		fPreferred = true ;
	} else {
		DO_NOTHING ;
	} /* endif */
} /* endmethod */

void _DictionaryEntry::ParseInfo ( ) {
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

_DictionaryEntry::_DictionaryEntry ( STRING sInfo ) : Info(sInfo), ConjugationClass(CC_NULL), fPreferred(false), fConjugated(false) {
	DictionaryInfos.Add ( Info ) ;
	DictionaryEntries.Add ( sizeof(*this) ) ;
	ParseInfo ( ) ;
} /* endmethod */

_DictionaryEntry::_DictionaryEntry ( const _DictionaryEntry& Other ) : 
	Info(Other.Info), 
	SymbolSounds(Other.SymbolSounds), 
	ConjugationClass(Other.ConjugationClass),
	fPreferred(Other.fPreferred), 
	fConjugated(Other.fConjugated) 
{
	if ( Info.size() ) 
		DictionaryInfos.Add ( Info ) ;
	DictionaryEntries.Add ( sizeof(*this) ) ;
} /* endmethod */

_DictionaryEntry::_DictionaryEntry ( FILE *File ) : ConjugationClass(CC_NULL), fPreferred(false), fConjugated(false) {
	if ( Load ( File ) == false )
		throw "ERROR: Unable to load dictionary entry from file." ;
	DictionaryEntries.Add ( sizeof(*this) ) ;
} /* endmethod */

_DictionaryEntry::_DictionaryEntry ( ) : ConjugationClass(CC_NULL), fPreferred(false), fConjugated(false) {
	DictionaryEntries.Add ( sizeof(*this) ) ;
} /* endmethod */

_DictionaryEntry::~_DictionaryEntry ( ) {
	if ( Info.size() ) 
		DictionaryInfos.Remove ( Info ) ;
	DictionaryEntries.Remove ( sizeof(*this) ) ;
} /* endmethod */

bool _DictionaryEntry::Load ( FILE *File ) {
	if ( MyLoadString ( Info, File ) == false ) 
		return ( false ) ;
	size_t cb = fread ( &ConjugationClass, 1, sizeof(ConjugationClass), File ) ;
	if ( cb < sizeof(ConjugationClass) ) 
		return ( false ) ;
	cb = fread ( &fPreferred, 1, sizeof(fPreferred), File ) ;
	if ( cb < sizeof(fPreferred) ) 
		return ( false ) ;
	cb = fread ( &fConjugated, 1, sizeof(fConjugated), File ) ;
	if ( cb < sizeof(fConjugated) ) 
		return ( false ) ;
	short Count ;
	cb = fread ( &Count, 1, sizeof(Count), File ) ;
	if ( cb < sizeof(Count) )
		return ( false ) ;
	for ( short i=0; i<Count; i++ ) {
		try {
			_SymbolSound Object ( File ) ;
			SymbolSounds.insert ( Object ) ;
		} catch ( char * ) {
			return ( false ) ;
		} /* end try/catch */
	} /* endfor */
	return ( true ) ;
} /* endmethod */

void _DictionaryEntry::Save ( FILE *File ) {
	MySaveString ( Info, File ) ;
	fwrite ( &ConjugationClass, sizeof(ConjugationClass), 1, File ) ;
	fwrite ( &fPreferred, sizeof(fPreferred), 1, File ) ;
	fwrite ( &fConjugated, sizeof(fConjugated), 1, File ) ;
	short Count = (short) SymbolSounds.size ( ) ;
	fwrite ( &Count, sizeof(Count), 1, File ) ;
	for (auto& SymbolSound : SymbolSounds) {
		SymbolSound.Save(File);
	} /* endfor */
} /* endmethod */

_DictionaryEntry& _DictionaryEntry::operator= ( const _DictionaryEntry &Other ) {
	if ( Info.size() ) 
		DictionaryInfos.Remove ( Info ) ;
	Info = Other.Info ;
	SymbolSounds = Other.SymbolSounds ;
	ConjugationClass = Other.ConjugationClass ;
	fPreferred = Other.fPreferred ;
	fConjugated = Other.fConjugated ;
	if ( Info.size() ) 
		DictionaryInfos.Add ( Info ) ;
	return ( *this ) ;
} /* endmethod */

inline bool HasConjugation ( BYTE Conjugations[], _ConjugationType ct ) {
	for ( unsigned i=0; i<MAX_CONJUGATIONS; i++ ) {
		if ( Conjugations[i] == ct )
			return ( true ) ;
		if ( Conjugations[i] == 0 )
			return ( false ) ;
	} /* endfor */
	return ( false ) ;
}

inline bool PushConjugation ( BYTE Conjugations[], _ConjugationType ct ) {
	unsigned i ;
	for ( i=0; i<MAX_CONJUGATIONS; i++ ) {
		if ( Conjugations[i] == 0 )
			break ;
	} /* endfor */
	if ( i >= MAX_CONJUGATIONS )
		return ( false ) ;
	Conjugations[i] = ct ;
	return ( true ) ;
}

inline void PopConjugation ( BYTE Conjugations[] ) {
	unsigned i ;
	for ( i=0; i<MAX_CONJUGATIONS; i++ )
		if ( Conjugations[i] == 0 )
			break ;
	if ( i ) 
		Conjugations[i-1] = 0 ;
}

void _DictionaryEntry::AddSymbol ( STRING sSymbol, STRING sSound, BYTE Conjugations[], _ConjugationType ct ) {
	if ( ct == CT_NONE ) {
		SymbolSounds.insert ( _SymbolSound ( sSymbol, sSound, Conjugations ) ) ;
		if ( Conjugations[0] == 0 )
			Conjugate ( sSymbol.c_str(), sSound.c_str(), Conjugations ) ;
	} else {
		if ( PushConjugation ( Conjugations, ct ) ) {
			SymbolSounds.insert ( _SymbolSound ( sSymbol, sSound, Conjugations ) ) ;
			PopConjugation ( Conjugations ) ;
		} /* endif */
	} /* endif */
} /* endmethod */

void _DictionaryEntry::AddSymbol ( STRING sSymbol, STRING sSound ) {
	SymbolSounds.insert ( _SymbolSound ( sSymbol, sSound ) ) ;
} /* endmethod */

void _DictionaryEntry::AddSound ( STRING sSound, BYTE Conjugations[] ) {
	if ( ConjugationClass == CC_NULL ) {
		SymbolSounds.insert ( _SymbolSound ( sSound, sSound, Conjugations ) ) ;
	} else {
		if ( Conjugations[0] )
			SymbolSounds.insert ( _SymbolSound ( sSound, sSound, Conjugations ) ) ;
		else
			Conjugate ( sSound.c_str(), sSound.c_str(), Conjugations ) ;
	} /* endif */
} /* endmethod */

void _DictionaryEntry::AddSound ( STRING sSound ) {
	SymbolSounds.insert ( _SymbolSound ( sSound, sSound ) ) ;
} /* endmethod */

std::unique_ptr<_Ichidan_Conjugator> _DictionaryEntry::IchidanConjugator ( new _Ichidan_Conjugator ) ;
std::unique_ptr < _Godan_Conjugator> _DictionaryEntry::GodanConjugator ( new _Godan_Conjugator ) ;
std::unique_ptr < _Iku_Conjugator> _DictionaryEntry::IkuConjugator ( new _Iku_Conjugator ) ;
std::unique_ptr < _Aru_Conjugator> _DictionaryEntry::AruConjugator ( new _Aru_Conjugator ) ;
std::unique_ptr < _Suru_Conjugator> _DictionaryEntry::SuruConjugator ( new _Suru_Conjugator ) ;
std::unique_ptr < _Kuru_Conjugator> _DictionaryEntry::KuruConjugator ( new _Kuru_Conjugator ) ;
std::unique_ptr < _AdjectiveI_Conjugator> _DictionaryEntry::AdjectiveIConjugator ( new _AdjectiveI_Conjugator ) ;
std::unique_ptr < _AdjectiveNA_Conjugator> _DictionaryEntry::AdjectiveNAConjugator ( new _AdjectiveNA_Conjugator ) ;

void _Conjugator::ConjugateSymbol ( _DictionaryEntry *pEntry, const wchar_t *szSymbol, const wchar_t *szSound, BYTE Conjugations[] ) {
	wchar_t Buffer [2] [200] ;
	if ( Stem ( Buffer[0], _countof(Buffer[0]), szSymbol ) && Stem ( Buffer[1], _countof(Buffer[1]), szSound ) )
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_STEM ) ;
	else if ( GetClass() != CC_ADJ_I ) 
		return ; // Serious problems here.
	if ( Conjugations[0] == 0 ) { // We don't redo the plain positive, ever.
		if ( PlainPositive ( Buffer[0], _countof(Buffer[0]), szSymbol ) && PlainPositive ( Buffer[1], _countof(Buffer[1]), szSound ) )
			pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_PLAIN_POSITIVE ) ;
	} /* endif */
	if ( !HasConjugation(Conjugations,CT_PLAIN_NEGATIVE1) && !HasConjugation(Conjugations,CT_PLAIN_NEGATIVE2) ) { // Don't do double-negatives.
		if ( PlainNegative1 ( Buffer[0], _countof(Buffer[0]), szSymbol ) && PlainNegative1 ( Buffer[1], _countof(Buffer[1]), szSound ) ) {
			if ( PushConjugation ( Conjugations, CT_PLAIN_NEGATIVE1 ) ) {
				pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations ) ;
				pEntry->Conjugate ( Buffer[0], Buffer[1], Conjugations, CC_ADJ_I ) ;
				PopConjugation ( Conjugations ) ;
			} /* endif */
		} /* endif */
		if ( PlainNegative2 ( Buffer[0], _countof(Buffer[0]), szSymbol ) && PlainNegative2 ( Buffer[1], _countof(Buffer[1]), szSound ) ) {
			pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_PLAIN_NEGATIVE2 ) ;
		} /* endif */
	} /* endif */
	if ( PlainPositivePast ( Buffer[0], _countof(Buffer[0]), szSymbol ) && PlainPositivePast ( Buffer[1], _countof(Buffer[1]), szSound ) ) {
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_PLAIN_POSITIVE_PAST ) ;
		wcscat_s ( Buffer[0], _countof(Buffer[0]), L"り" ) ;
		wcscat_s ( Buffer[1], _countof(Buffer[1]), L"り" ) ;
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_PLAIN_POSITIVE_ALTERNATIVE ) ;
		PlainPositivePast ( Buffer[0], _countof(Buffer[0]), szSymbol ) ; 
		PlainPositivePast ( Buffer[1], _countof(Buffer[1]), szSound ) ;
		wcscat_s ( Buffer[0], _countof(Buffer[0]), L"ら" ) ;
		wcscat_s ( Buffer[1], _countof(Buffer[1]), L"ら" ) ;
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_PLAIN_POSITIVE_CONDITIONAL ) ;
	} /* endif */
	if ( PlainImperative1 ( Buffer[0], _countof(Buffer[0]), szSymbol ) && PlainImperative1 ( Buffer[1], _countof(Buffer[1]), szSound ) )
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_PLAIN_IMPERATIVE1 ) ;
	if ( PlainImperative2 ( Buffer[0], _countof(Buffer[0]), szSymbol ) && PlainImperative2 ( Buffer[1], _countof(Buffer[1]), szSound ) )
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_PLAIN_IMPERATIVE2 ) ;
	if ( PoliteImperative ( Buffer[0], _countof(Buffer[0]), szSymbol ) && PoliteImperative ( Buffer[1], _countof(Buffer[1]), szSound ) )
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_POLITE_IMPERATIVE ) ;
	if (PoliteNegativeImperative(Buffer[0], _countof(Buffer[0]), szSymbol) && PoliteNegativeImperative(Buffer[1], _countof(Buffer[1]), szSound))
		pEntry->AddSymbol(Buffer[0], Buffer[1], Conjugations, CT_POLITE_NEGATIVE_IMPERATIVE);
	if ( PlainPresumptive1 ( Buffer[0], _countof(Buffer[0]), szSymbol ) && PlainPresumptive1 ( Buffer[1], _countof(Buffer[1]), szSound ) )
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_PLAIN_PRESUMPTIVE1 ) ;
	if ( PlainPresumptive2 ( Buffer[0], _countof(Buffer[0]), szSymbol ) && PlainPresumptive2 ( Buffer[1], _countof(Buffer[1]), szSound ) )
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_PLAIN_PRESUMPTIVE2 ) ;
	if ( PlainConjunctive ( Buffer[0], _countof(Buffer[0]), szSymbol ) && PlainConjunctive ( Buffer[1], _countof(Buffer[1]), szSound ) ) {
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_PLAIN_CONJUNCTIVE ) ;
		wcscat_s ( Buffer[0], _countof(Buffer[0]), L"いる" ) ;
		wcscat_s ( Buffer[1], _countof(Buffer[1]), L"いる" ) ;
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_PLAIN_PROGRESSIVE1 ) ;
		PlainConjunctive ( Buffer[0], _countof(Buffer[0]), szSymbol ) ; 
		PlainConjunctive ( Buffer[1], _countof(Buffer[1]), szSound ) ;
		wcscat_s ( Buffer[0], _countof(Buffer[0]), L"る" ) ;
		wcscat_s ( Buffer[1], _countof(Buffer[1]), L"る" ) ;
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_PLAIN_PROGRESSIVE2 ) ;
		PlainConjunctive ( Buffer[0], _countof(Buffer[0]), szSymbol ) ; 
		PlainConjunctive ( Buffer[1], _countof(Buffer[1]), szSound ) ;
		wcscat_s ( Buffer[0], _countof(Buffer[0]), L"います" ) ;
		wcscat_s ( Buffer[1], _countof(Buffer[1]), L"います" ) ;
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_POLITE_PROGRESSIVE ) ;
		PlainConjunctive ( Buffer[0], _countof(Buffer[0]), szSymbol ) ; 
		PlainConjunctive ( Buffer[1], _countof(Buffer[1]), szSound ) ;
		wcscat_s ( Buffer[0], _countof(Buffer[0]), L"いた" ) ;
		wcscat_s ( Buffer[1], _countof(Buffer[1]), L"いた" ) ;
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_PLAIN_PROGRESSIVE_PAST1 ) ;
		PlainConjunctive ( Buffer[0], _countof(Buffer[0]), szSymbol ) ; 
		PlainConjunctive ( Buffer[1], _countof(Buffer[1]), szSound ) ;
		wcscat_s ( Buffer[0], _countof(Buffer[0]), L"た" ) ;
		wcscat_s ( Buffer[1], _countof(Buffer[1]), L"た" ) ;
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_PLAIN_PROGRESSIVE_PAST2 ) ;
		PlainConjunctive ( Buffer[0], _countof(Buffer[0]), szSymbol ) ; 
		PlainConjunctive ( Buffer[1], _countof(Buffer[1]), szSound ) ;
		wcscat_s ( Buffer[0], _countof(Buffer[0]), L"いて" ) ;
		wcscat_s ( Buffer[1], _countof(Buffer[1]), L"いて" ) ;
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_PLAIN_PROGRESSIVE_CONJUNCTIVE1 ) ;
		PlainConjunctive ( Buffer[0], _countof(Buffer[0]), szSymbol ) ; 
		PlainConjunctive ( Buffer[1], _countof(Buffer[1]), szSound ) ;
		wcscat_s ( Buffer[0], _countof(Buffer[0]), L"て" ) ;
		wcscat_s ( Buffer[1], _countof(Buffer[1]), L"て" ) ;
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_PLAIN_PROGRESSIVE_CONJUNCTIVE2 ) ;
		PlainConjunctive ( Buffer[0], _countof(Buffer[0]), szSymbol ) ; 
		PlainConjunctive ( Buffer[1], _countof(Buffer[1]), szSound ) ;
		wcscat_s ( Buffer[0], _countof(Buffer[0]), L"いました" ) ;
		wcscat_s ( Buffer[1], _countof(Buffer[1]), L"いました" ) ;
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_POLITE_PROGRESSIVE_PAST ) ;
	} /* endif */
	if ( PlainPositiveProvisional ( Buffer[0], _countof(Buffer[0]), szSymbol ) && PlainPositiveProvisional ( Buffer[1], _countof(Buffer[1]), szSound ) )
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_PLAIN_POSITIVE_PROVISIONAL ) ;
	if ( PlainNegativeProvisional1 ( Buffer[0], _countof(Buffer[0]), szSymbol ) && PlainNegativeProvisional1 ( Buffer[1], _countof(Buffer[1]), szSound ) )
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_PLAIN_NEGATIVE_PROVISIONAL1 ) ;
	if ( PlainNegativeProvisional2 ( Buffer[0], _countof(Buffer[0]), szSymbol ) && PlainNegativeProvisional2 ( Buffer[1], _countof(Buffer[1]), szSound ) )
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_PLAIN_NEGATIVE_PROVISIONAL2 ) ;
	if ( !HasConjugation(Conjugations,CT_PLAIN_PASSIVE) ) { // Don't do this recursively.
		if ( PlainPassive ( Buffer[0], _countof(Buffer[0]), szSymbol ) && PlainPassive ( Buffer[1], _countof(Buffer[1]), szSound ) ) {
			if ( PushConjugation ( Conjugations, CT_PLAIN_PASSIVE ) ) {
				pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations ) ;
				pEntry->Conjugate ( Buffer[0], Buffer[1], Conjugations, CC_ICHIDAN ) ;
				PopConjugation ( Conjugations ) ;
			} /* endif */
		} /* endif */
	} /* endif */
	if ( PlainPassivePast ( Buffer[0], _countof(Buffer[0]), szSymbol ) && PlainPassivePast ( Buffer[1], _countof(Buffer[1]), szSound ) )
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_PLAIN_PASSIVE_PAST ) ;
	if ( !HasConjugation(Conjugations,CT_PLAIN_WISH) ) { // Don't double wish.
		if ( PlainWish ( Buffer[0], _countof(Buffer[0]), szSymbol ) && PlainWish ( Buffer[1], _countof(Buffer[1]), szSound ) ) {
			if ( PushConjugation ( Conjugations, CT_PLAIN_WISH ) ) {
				pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations ) ;
				pEntry->Conjugate ( Buffer[0], Buffer[1], Conjugations, CC_ADJ_I ) ;
				PopConjugation ( Conjugations ) ;
			} /* endif */
		} /* endif */
	} /* endif */
	if ( PolitePositive ( Buffer[0], _countof(Buffer[0]), szSymbol ) && PolitePositive ( Buffer[1], _countof(Buffer[1]), szSound ) )
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_POLITE_POSITIVE ) ;
	if ( PoliteNegative ( Buffer[0], _countof(Buffer[0]), szSymbol ) && PoliteNegative ( Buffer[1], _countof(Buffer[1]), szSound ) )
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_POLITE_NEGATIVE ) ;
	if ( PolitePositivePast ( Buffer[0], _countof(Buffer[0]), szSymbol ) && PolitePositivePast ( Buffer[1], _countof(Buffer[1]), szSound ) ) {
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_POLITE_POSITIVE_PAST ) ;
		wcscat_s ( Buffer[0], _countof(Buffer[0]), L"り" ) ;
		wcscat_s ( Buffer[1], _countof(Buffer[1]), L"り" ) ;
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_POLITE_POSITIVE_ALTERNATIVE ) ;
		PolitePositivePast ( Buffer[0], _countof(Buffer[0]), szSymbol ) ; 
		PolitePositivePast ( Buffer[1], _countof(Buffer[1]), szSound ) ;
		wcscat_s ( Buffer[0], _countof(Buffer[0]), L"ら" ) ;
		wcscat_s ( Buffer[1], _countof(Buffer[1]), L"ら" ) ;
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_POLITE_POSITIVE_CONDITIONAL ) ;
	} /* endif */
	if ( PoliteNegativePast ( Buffer[0], _countof(Buffer[0]), szSymbol ) && PoliteNegativePast ( Buffer[1], _countof(Buffer[1]), szSound ) ) {
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_POLITE_NEGATIVE_PAST ) ;
		wcscat_s ( Buffer[0], _countof(Buffer[0]), L"り" ) ;
		wcscat_s ( Buffer[1], _countof(Buffer[1]), L"り" ) ;
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_POLITE_NEGATIVE_ALTERNATIVE ) ;
		PoliteNegativePast ( Buffer[0], _countof(Buffer[0]), szSymbol ) ; 
		PoliteNegativePast ( Buffer[1], _countof(Buffer[1]), szSound ) ;
		wcscat_s ( Buffer[0], _countof(Buffer[0]), L"ら" ) ;
		wcscat_s ( Buffer[1], _countof(Buffer[1]), L"ら" ) ;
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_POLITE_NEGATIVE_CONDITIONAL ) ;
	} /* endif */
	if ( PolitePresumptive ( Buffer[0], _countof(Buffer[0]), szSymbol ) && PolitePresumptive ( Buffer[1], _countof(Buffer[1]), szSound ) )
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_POLITE_PRESUMPTIVE ) ;
	if ( Adverb ( Buffer[0], _countof(Buffer[0]), szSymbol ) && Adverb ( Buffer[1], _countof(Buffer[1]), szSound ) )
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_ADVERB ) ;
	if ( !HasConjugation(Conjugations,CT_POTENTIAL1) && !HasConjugation(Conjugations,CT_POTENTIAL2) ) { // Don't do this recursively.
		if ( Potential1 ( Buffer[0], _countof(Buffer[0]), szSymbol ) && Potential1 ( Buffer[1], _countof(Buffer[1]), szSound ) ) {
			if ( PushConjugation ( Conjugations, CT_POTENTIAL1 ) ) {
				pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations ) ;
				// pEntry->Conjugate ( Buffer[0], Buffer[1], Conjugations, CC_ICHIDAN ) ;
				PopConjugation ( Conjugations ) ;
			} /* endif */
		} /* endif */
	} /* endif */
	if ( !HasConjugation(Conjugations,CT_POTENTIAL1) && !HasConjugation(Conjugations,CT_POTENTIAL2) ) { // Don't do this recursively.
		if ( Potential2 ( Buffer[0], _countof(Buffer[0]), szSymbol ) && Potential2 ( Buffer[1], _countof(Buffer[1]), szSound ) ) {
			if ( PushConjugation ( Conjugations, CT_POTENTIAL2 ) ) {
				pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations ) ;
				// pEntry->Conjugate ( Buffer[0], Buffer[1], Conjugations, CC_ICHIDAN ) ;
				PopConjugation ( Conjugations ) ;
			} /* endif */
		} /* endif */
	} /* endif */
	if ( !HasConjugation(Conjugations,CT_CAUSATIVE) ) { // Don't do this recursively.
		if ( Causative ( Buffer[0], _countof(Buffer[0]), szSymbol ) && Causative ( Buffer[1], _countof(Buffer[1]), szSound ) ) {
			if ( PushConjugation ( Conjugations, CT_CAUSATIVE ) ) {
				pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations ) ;
				// pEntry->Conjugate ( Buffer[0], Buffer[1], Conjugations, CC_ICHIDAN ) ;
				PopConjugation ( Conjugations ) ;
			} /* endif */
		} /* endif */
	} /* endif */
	if ( NegativeConjunctive ( Buffer[0], _countof(Buffer[0]), szSymbol ) && NegativeConjunctive ( Buffer[1], _countof(Buffer[1]), szSound ) )
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_NEGATIVE_CONJUNCTIVE ) ;
	if ( Nominalize ( Buffer[0], _countof(Buffer[0]), szSymbol ) && Nominalize ( Buffer[1], _countof(Buffer[1]), szSound ) )
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_ADJ_NOUN ) ;
	if ( Resembles ( Buffer[0], _countof(Buffer[0]), szSymbol ) && Resembles ( Buffer[1], _countof(Buffer[1]), szSound ) )
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_ADJ_SEEMS ) ;
	if ( PlainNegativePresumptive ( Buffer[0], _countof(Buffer[0]), szSymbol ) && PlainNegativePresumptive ( Buffer[1], _countof(Buffer[1]), szSound ) )
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_PLAIN_NEGATIVE_PRESUMPTIVE ) ;
	if ( PoliteNegativePresumptive ( Buffer[0], _countof(Buffer[0]), szSymbol ) && PoliteNegativePresumptive ( Buffer[1], _countof(Buffer[1]), szSound ) )
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_PLAIN_NEGATIVE_PRESUMPTIVE ) ;
	if ( PolitePositiveConjunctive ( Buffer[0], _countof(Buffer[0]), szSymbol ) && PolitePositiveConjunctive ( Buffer[1], _countof(Buffer[1]), szSound ) )
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_POLITE_POSITIVE_CONJUNCTIVE ) ;
	if ( PoliteNegativeConjunctive ( Buffer[0], _countof(Buffer[0]), szSymbol ) && PoliteNegativeConjunctive ( Buffer[1], _countof(Buffer[1]), szSound ) )
		pEntry->AddSymbol ( Buffer[0], Buffer[1], Conjugations, CT_POLITE_NEGATIVE_CONJUNCTIVE ) ;
} /* endmethod */

DICTIONARY Dictionary ;

extern void AddWord ( wchar_t *pSymbol, wchar_t *pSound, wchar_t *pInfo, bool fConjugate ) {
	STRING sInfo = STRING(pInfo) ;
	DICTIONARY::iterator Cursor = Dictionary.find ( sInfo ) ;
	if ( Cursor == Dictionary.end() ) {
		Dictionary[sInfo] = _DictionaryEntry ( sInfo ) ;
		Cursor = Dictionary.find ( pInfo ) ;
	} /* endif */
	if ( fConjugate ) {
		BYTE Conjugations [MAX_CONJUGATIONS] = { 0 } ;
		if ( pSymbol ) 
			Cursor->second.AddSymbol ( STRING(pSymbol), STRING(pSound), Conjugations ) ;
		else
			Cursor->second.AddSound ( STRING(pSound), Conjugations ) ;
	} else {
		if ( pSymbol ) 
			Cursor->second.AddSymbol ( STRING(pSymbol), STRING(pSound) ) ;
		else
			Cursor->second.AddSound ( STRING(pSound) ) ;
	} /* endif */
}

extern bool BuildDictionary ( wchar_t *szPath, bool fConjugate ) {

	// Open the EDICT file.
	FILE *File ; errno_t Error = _wfopen_s ( &File, szPath, L"rt" ) ;
	if ( Error ) {
		char Buffer[200]; strerror_s(Buffer, _countof(Buffer), Error);
		fwprintf ( stderr, L"ERROR: Unable to open source file.  Error %i:%hs\n", Error, Buffer ) ;
		return ( false ) ;
	} /* endif */

	// Scan the file to discover just how big it is.
	char RawBuffer [0x1000] ; int nLines = 0 ;
	while ( fgets ( RawBuffer, _countof(RawBuffer), File ) )
		++ nLines ;
	fseek ( File, 0, SEEK_SET ) ; // Rewind when done.

	// Read it line by line, converting to Unicode as we go.
	int NumErrors(0), NumWords(0);
	wchar_t ConvertedBuffer [0x1000] ; int iLine = 0 ; int LastPercentage = -1 ;
	while ( fgets ( RawBuffer, _countof(RawBuffer), File ) ) {

		// Announce where we are.
		int PercentageDone = (int) ( ( ( ( iLine * 200 ) + 0.4 ) / nLines ) / 2 ) ;
		if ( PercentageDone != LastPercentage ) {
			printf ( "  %u%% done.\r", PercentageDone ) ;
			LastPercentage = PercentageDone ;
		} /* endif */
		++ iLine ;

		// Convert the raw line to Unicode.
		bool Error = false;
		unsigned char *p1 = (unsigned char*) RawBuffer ; wchar_t *p2 = ConvertedBuffer ;
		while ( *p1 && !Error ) {
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
		if (Error) {
			NumErrors++;
			continue;
		} /* endif */

		// Skip comment lines.
		if ( ConvertedBuffer[0] == L'　' )
			continue ;

		// Parse the line.
		wchar_t *pSymbol(0), *pSound(0), *pInfo(0) ;
		if ( ParseLine ( ConvertedBuffer, pSymbol, pSound, pInfo ) == false ) 
			continue ;

		// Add the information to our dictionary.
		AddWord ( pSymbol, pSound, pInfo, fConjugate ) ;
		NumWords++;

	} /* endwhile */

	// Done reading the source file.
	fclose ( File ) ;
	printf ( "                           \r" ) ;
	printf(
		"Number of words processed: %i\n"
		"Number of words skipped:   %i.\n", 
		NumWords, NumErrors);
	return ( true ) ;
}

INDEX Index ;

extern bool BuildIndex ( ) {

	// Clear the index.
	Index.clear ( ) ;

	// Build the index.
	for (auto& Entry : Dictionary) {
		const SYMBOL_SOUND_SET* pSymbols = Entry.second.GetSymbols();
		for (auto& Symbol : *(Entry.second.GetSymbols())) {
			const STRING* psSymbol = Symbol.GetSymbol();
			const STRING* psSound = Symbol.GetSound();
			const BYTE* Conjugations = Symbol.GetConjugations();
			Index.insert(_IndexEntry(psSymbol, psSound, Conjugations, &Entry.second));
			if (wcscmp(psSymbol->c_str(), psSound->c_str()))
				Index.insert(_IndexEntry(psSound, psSound, Conjugations, &Entry.second));
		} /* endfor */
	} /* endfor */

	return ( true ) ;
}

//
// Scan for symbol prefix.
//
extern INDEX::iterator FindSymbolPrefix ( wchar_t *pLine, size_t cbLine ) {
	wchar_t szKey [2] = { (wchar_t)(pLine[0]-1), 0 } ; 
	STRING sKey ( szKey ) ;
	STRING sNull ( L"" ) ;
	BYTE Dummy1 [MAX_CONJUGATIONS] ; _DictionaryEntry Dummy2 ;
	_IndexEntry Key ( &sKey, &sNull, Dummy1, &Dummy2 ) ;
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
		if ( cb == 0 )
			return ( Cursor ) ;
		++ Cursor ;
	} /* endwhile */
	return ( Index.end() ) ;
}

//
// Scan for an exact symbol.
//

extern INDEX::iterator FindSymbolExact ( wchar_t *pLine, size_t cbLine ) {
	wchar_t szKey [2] = { (wchar_t)(pLine[0]-1), 0 } ;	
	STRING sKey ( szKey ) ;
	STRING sNull ( L"" ) ;
	BYTE Dummy1 [MAX_CONJUGATIONS] ; _DictionaryEntry Dummy2 ;
	_IndexEntry Key ( &sKey, &sNull, Dummy1, &Dummy2 ) ;
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
		if ( ( *pWord == 0 ) && ( cb == 0 ) ) {
			// We have something.
			INDEX::iterator FirstAnswer = Cursor ;
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

void _DictionaryEntry::Dump ( FILE *File, int Indent ) {
	fwprintf ( File, L"%*ls%ls\n", Indent, L"", Info.c_str() ) ;
	for (auto& Entry : SymbolSounds) {
		const wchar_t* pszSymbol = Entry.GetSymbol()->c_str();
		const wchar_t* pszSound = Entry.GetSound()->c_str();
		if (!wcscmp(pszSymbol, pszSound))
			fwprintf(File, L"%*ls  %ls ", Indent, L"", pszSymbol);
		else
			fwprintf(File, L"%*ls  %ls (%ls) ", Indent, L"", pszSymbol, pszSound);
		const BYTE* Conjugations = Entry.GetConjugations();
		if (Conjugations[0]) {
			fwprintf(File, L"["); bool fFirst = true;
			for (unsigned i = 0; i < MAX_CONJUGATIONS; i++) {
				if (Conjugations[i] == 0)
					break;
				// Condense "Plain Negative (1), Plain Positive Past" to "Plain Negative Past".
				_ConjugationType ThisType = (_ConjugationType)Conjugations[i];
				_ConjugationType NextType = (_ConjugationType)(i < MAX_CONJUGATIONS ? Conjugations[i + 1] : CT_NONE);
				if ((ThisType == CT_PLAIN_NEGATIVE1) && (NextType == CT_PLAIN_POSITIVE_PAST)) {
					ThisType = CT_PLAIN_NEGATIVE_PAST;
					++i;
				} /* endif */
				if ((ThisType == CT_PLAIN_NEGATIVE1) && (NextType == CT_PLAIN_POSITIVE_CONDITIONAL)) {
					ThisType = CT_PLAIN_NEGATIVE_CONDITIONAL;
					++i;
				} /* endif */
				// TODO: Condense "Plain Causative, Plain Passive" to "Causative Passive".
				fwprintf(File, L"%ls%ls", fFirst ? L"" : L", ", szConjugationTypes[ThisType]);
				fFirst = false;
			} /* endwhile */
			fwprintf(File, L"] ");
		} /* endif */
		fwprintf(File, L"\n");
	} /* endfor */
} /* endmethod */

void _IndexEntry::Dump ( FILE *File, int Indent, bool fExtra ) const {
	if ( fExtra ) {
		if ( !wcscmp ( psWord->c_str(), psKana->c_str() ) ) 
			fwprintf ( File, L"%*ls%0.*ls = ", Indent, L"", (int)psWord->length(), L"　　　　　　　　　　" ) ;
		else
			fwprintf ( File, L"%*ls%0.*ls (%ls) = ", Indent, L"", (int)psWord->length(), L"　　　　　　　　　　", psKana->c_str() ) ;
	} else {
		if ( !wcscmp ( psWord->c_str(), psKana->c_str() ) ) 
			fwprintf ( File, L"%*ls%ls = ", Indent, L"", psWord->c_str() ) ;
		else
			fwprintf ( File, L"%*ls%ls (%ls) = ", Indent, L"", psWord->c_str(), psKana->c_str() ) ;
	} /* endif */
	if ( pConjugations[0] ) {
		fwprintf ( File, L"[" ) ; bool fFirst = true ;
		for ( unsigned i=0; i<MAX_CONJUGATIONS; i++ ) {
			if ( pConjugations[i] == 0 ) 
				break ;
			// Condense "Plain Negative (1), Plain Positive Past" to "Plain Negative Past".
			_ConjugationType ThisType = (_ConjugationType) pConjugations [i] ;
			_ConjugationType NextType = (_ConjugationType) ( i < MAX_CONJUGATIONS ? pConjugations [i+1] : CT_NONE ) ;
			if ( ( ThisType == CT_PLAIN_NEGATIVE1 ) && ( NextType == CT_PLAIN_POSITIVE_PAST ) ) {
				ThisType = CT_PLAIN_NEGATIVE_PAST ;
				++ i ;
			} /* endif */
			fwprintf ( File, L"%ls%ls", fFirst?L"":L", ", szConjugationTypes[ThisType] ) ;
			fFirst = false ;
		} /* endwhile */
		fwprintf ( File, L"] " ) ;
	} /* endif */
	fwprintf ( File, L"%ls\n", pDictionaryEntry->GetInfo() ) ;
} /* endmethod */

extern bool DumpDictionary ( ) {

	// Dump the dictionary.
	FILE *File ; errno_t Error = _wfopen_s ( &File, L"Dictionary.txt", L"wt,ccs=UNICODE" ) ;
	if ( !Error && File ) {
		for (auto& Entry : Dictionary) {
			Entry.second.Dump(File);
		} /* endfor */
		fclose ( File ) ;
	} /* endif */

	// Done OK.
	return ( true ) ;
}

extern bool DumpIndex ( ) {

	// Dump the index.
	FILE *File ; errno_t Error = _wfopen_s ( &File, L"Index.txt", L"wt,ccs=UNICODE" ) ;
	if ( !Error && File ) {
		for (auto& Entry : Index) {
			Entry.Dump(File);
		} /* endfor */
		fclose ( File ) ;
	} /* endif */

	// Additional information, for now.
	printf ( "String counts, sizes:\n" ) ;
	DictionaryInfos.Dump ( "Dictionary Info" ) ;
	Symbols.Dump ( "Symbols" ) ;
	Sounds.Dump ( "Sounds" ) ;
	SymbolSounds.Dump ( "SymbolSound Structures" ) ;
	DictionaryEntries.Dump ( "DictionaryEntry Structures" ) ;
	IndexEntries.Dump ( "IndexEntry Structures" ) ;
	_Counter Total ;
	Total.Combine ( DictionaryInfos ) ;
	Total.Combine ( Symbols ) ;
	Total.Combine ( Sounds ) ;
	Total.Combine ( SymbolSounds ) ;
	Total.Combine ( DictionaryEntries ) ;
	Total.Combine ( IndexEntries ) ;
	Total.Dump ( "TOTAL" ) ;

	// Done OK.
	return ( true ) ;
}

