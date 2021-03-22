#pragma once

#include <assert.h>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>

#define DO_NOTHING { int x = 0 ; x ++ ; x -- ; } // Macro to be able to set a breakpoint on.

typedef std::basic_string<wchar_t> STRING ;
typedef unsigned char BYTE ;

inline bool MyLoadString ( STRING& String, FILE *File ) {
	short Length ;
	size_t cb = fread ( &Length, 1, sizeof(Length), File ) ;
	if ( cb < sizeof(Length) )
		return ( false ) ;
	wchar_t Buffer [0x1000] ; 
	assert ( Length < _countof(Buffer) ) ;
	cb = fread ( Buffer, sizeof(wchar_t), Length, File ) ;
	if ( cb < Length )
		return ( false ) ;
	String.assign ( Buffer, Length ) ;
	return ( true ) ;
}

inline bool MySaveString ( const STRING& String, FILE *File ) {
	short Length = (short) String.length ( ) ;
	fwrite ( &Length, sizeof(Length), 1, File ) ;
	fwrite ( String.c_str(), sizeof(wchar_t), Length, File ) ;
	return ( true ) ;
}

class _Counter {
private:
	unsigned Count ;
	size_t cbBytes ;
public:
	_Counter ( ) : Count(0), cbBytes(0) { }
	void Add ( STRING s ) {
		++ Count ;
		cbBytes += s.size() * sizeof(wchar_t) ;
	} /* endmethod */
	void Remove ( STRING s ) {
		Count -- ;
		cbBytes -= s.size() * sizeof(wchar_t) ;
	} /* endmethod */
	void Add ( size_t cb ) {
		++ Count ;
		cbBytes += cb ;
	} /* endmethod */
	void Remove ( size_t cb ) {
		Count -- ;
		cbBytes -= cb ;
	} /* endmethod */
	void Dump ( char *Label ) {
		printf ( "  %hs: %u objects, %zu bytes\n", Label, Count, cbBytes ) ;
	} /* endmethod */
	void Combine ( _Counter& Other ) {
		Count += Other.Count ;
		cbBytes += Other.cbBytes ;
	} /* endmethod */
} ;

extern _Counter DictionaryInfos, Symbols, Sounds, SymbolSounds, DictionaryEntries, IndexEntries ;

enum _ConjugationClass { 
	CC_NULL, 
	CC_ICHIDAN, 
	CC_GODAN, 
	CC_IKU, 
	CC_ARU, 
	CC_SURU, 
	CC_KURU,
	CC_ADJ_I,
	CC_ADJ_NA
} ;

enum _ConjugationType {
	CT_NONE,
	CT_STEM,
	CT_PLAIN_POSITIVE,
	CT_PLAIN_NEGATIVE1,
	CT_PLAIN_NEGATIVE2,
	CT_PLAIN_POSITIVE_PAST,
	CT_PLAIN_NEGATIVE_PAST,
	CT_PLAIN_IMPERATIVE1,
	CT_PLAIN_IMPERATIVE2,
	CT_PLAIN_PRESUMPTIVE1,
	CT_PLAIN_PRESUMPTIVE2,
	CT_PLAIN_CONJUNCTIVE,
	CT_PLAIN_POSITIVE_PROVISIONAL,
	CT_PLAIN_NEGATIVE_PROVISIONAL1,
	CT_PLAIN_NEGATIVE_PROVISIONAL2,
	CT_PLAIN_POSITIVE_CONDITIONAL,
	CT_PLAIN_NEGATIVE_CONDITIONAL,
	CT_POLITE_POSITIVE_CONDITIONAL,
	CT_POLITE_NEGATIVE_CONDITIONAL,
	CT_PLAIN_PASSIVE,
	CT_PLAIN_PASSIVE_PAST,
	CT_PLAIN_WISH,
	CT_POLITE_POSITIVE,
	CT_POLITE_NEGATIVE,
	CT_POLITE_POSITIVE_PAST,
	CT_POLITE_NEGATIVE_PAST,
	CT_POLITE_PRESUMPTIVE,
	CT_ADVERB, // From ADJ_I and ADJ_NA forms.
	CT_POTENTIAL1,
	CT_POTENTIAL2,
	CT_CAUSATIVE,
	CT_NEGATIVE_CONJUNCTIVE,
	CT_PLAIN_POSITIVE_ALTERNATIVE,
	CT_PLAIN_NEGATIVE_ALTERNATIVE,
	CT_POLITE_POSITIVE_ALTERNATIVE,
	CT_POLITE_NEGATIVE_ALTERNATIVE,
	CT_PLAIN_PROGRESSIVE1,
	CT_PLAIN_PROGRESSIVE2,
	CT_POLITE_PROGRESSIVE,
	CT_PLAIN_PROGRESSIVE_PAST1,
	CT_PLAIN_PROGRESSIVE_PAST2,
	CT_PLAIN_PROGRESSIVE_CONJUNCTIVE1,
	CT_PLAIN_PROGRESSIVE_CONJUNCTIVE2,
	CT_POLITE_PROGRESSIVE_PAST,
	CT_ADJ_NOUN,
	CT_ADJ_SEEMS,
	CT_PLAIN_NEGATIVE_PRESUMPTIVE,
	CT_POLITE_NEGATIVE_PRESUMPTIVE,
	CT_POLITE_POSITIVE_CONJUNCTIVE,
	CT_POLITE_NEGATIVE_CONJUNCTIVE,
	CT_MAX
} ;

extern wchar_t *szConjugationTypes [CT_MAX] ;

#define MAX_CONJUGATIONS (3)

class _SymbolSound {
	STRING Symbol ;
	STRING Sound ;
	BYTE Conjugations [MAX_CONJUGATIONS] ;
public:
	_SymbolSound ( wchar_t *szSymbol, wchar_t *szSound, BYTE conjugations[] ) : Symbol(szSymbol), Sound(szSound) { 
		memcpy ( Conjugations, conjugations, sizeof(Conjugations) ) ;
		SymbolSounds.Add ( sizeof(*this) ) ;
		Symbols.Add ( Symbol ) ;
		Sounds.Add ( Sound ) ;
	} /* endmethod */
	_SymbolSound ( STRING sSymbol, STRING sSound, BYTE conjugations[] ) : Symbol(sSymbol), Sound(sSound) {
		memcpy ( Conjugations, conjugations, sizeof(Conjugations) ) ;
		SymbolSounds.Add ( sizeof(*this) ) ;
		Symbols.Add ( Symbol ) ;
		Sounds.Add ( Sound ) ;
	} /* endmethod */
	_SymbolSound ( STRING sSymbol, STRING sSound ) : Symbol(sSymbol), Sound(sSound) {
		memset ( Conjugations, 0, sizeof(Conjugations) ) ;
		SymbolSounds.Add ( sizeof(*this) ) ;
		Symbols.Add ( Symbol ) ;
		Sounds.Add ( Sound ) ;
	} /* endmethod */
	_SymbolSound ( FILE *File ) {
		if ( Load ( File ) == false )
			throw "ERROR: Unable to load symbol/sound object." ;
	} /* endmethod */
	_SymbolSound ( const _SymbolSound& Other ) : Symbol(Other.Symbol), Sound(Other.Sound) {
		memcpy ( Conjugations, Other.Conjugations, sizeof(Conjugations) ) ;
		SymbolSounds.Add ( sizeof(*this) ) ;
		Symbols.Add ( Symbol ) ;
		Sounds.Add ( Sound ) ;
	} /* endmethod */
	~_SymbolSound ( ) {
		Sounds.Remove ( Sound ) ;
		Symbols.Remove ( Symbol ) ;
		SymbolSounds.Remove ( sizeof(*this) ) ;
	} /* endmethod */
	_SymbolSound& operator= ( const _SymbolSound &Other ) {
		Sounds.Remove ( Sound ) ;
		Symbols.Remove ( Symbol ) ;
		Symbol = Other.Symbol ;
		Sound = Other.Sound ;
		memcpy ( Conjugations, Other.Conjugations, sizeof(Conjugations) ) ;
		Symbols.Add ( Symbol ) ;
		Sounds.Add ( Sound ) ;
	} /* endmethod */
	bool operator< ( const _SymbolSound &obj ) const {
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
		unsigned i ;
		for ( i=0; i<_countof(Conjugations); i++ ) {
			if ( Conjugations[i] == 0 )
				break ;
			if ( Conjugations[i] > obj.Conjugations[i] )
				return ( false ) ;
			if ( Conjugations[i] < obj.Conjugations[i] )
				return ( true ) ;
		} /* endwhile */
		if ( i < _countof(Conjugations) ) 
			return ( obj.Conjugations[i] != 0 ) ;
		return ( false ) ;
	} /* endmethod */
	const STRING *GetSymbol ( ) const { return ( &Symbol ) ; }
	const STRING *GetSound ( ) const { return ( &Sound ) ; }
	const BYTE* GetConjugations ( ) const { return ( Conjugations ) ; }
	bool Load ( FILE *File ) {
		if ( MyLoadString ( Symbol, File ) == false ) 
			return ( false ) ;
		if ( MyLoadString ( Sound, File ) == false ) 
			return ( false ) ;
		size_t cb = fread ( Conjugations, sizeof(BYTE), MAX_CONJUGATIONS, File ) ;
		if ( cb < MAX_CONJUGATIONS )
			return ( false ) ;
		return ( true ) ;
	} /* endmethod */
	void Save ( FILE *File ) const {
		MySaveString ( Symbol, File ) ;
		MySaveString ( Sound, File ) ;
		fwrite ( Conjugations, sizeof(BYTE), MAX_CONJUGATIONS, File ) ;
	} /* endmethod */
} ;

typedef std::set<_SymbolSound> SYMBOL_SOUND_SET ;

class _Ichidan_Conjugator ;
class _Godan_Conjugator ;
class _Iku_Conjugator ;
class _Aru_Conjugator ;
class _Suru_Conjugator ;
class _Kuru_Conjugator ;
class _AdjectiveI_Conjugator ;
class _AdjectiveNA_Conjugator ;

class _DictionaryEntry {
	STRING Info ;
	SYMBOL_SOUND_SET SymbolSounds ;
	_ConjugationClass ConjugationClass ;
	bool fPreferred, fConjugated ;
	void CheckToken ( wchar_t *szToken ) ;
	void ParseInfo ( ) ;
public:
	static std::unique_ptr<_Ichidan_Conjugator> IchidanConjugator ;
	static std::unique_ptr < _Godan_Conjugator> GodanConjugator ;
	static std::unique_ptr < _Iku_Conjugator> IkuConjugator ;
	static std::unique_ptr < _Aru_Conjugator> AruConjugator ;
	static std::unique_ptr < _Suru_Conjugator> SuruConjugator ;
	static std::unique_ptr < _Kuru_Conjugator> KuruConjugator ;
	static std::unique_ptr < _AdjectiveI_Conjugator> AdjectiveIConjugator ;
	static std::unique_ptr < _AdjectiveNA_Conjugator> AdjectiveNAConjugator ;
	_DictionaryEntry ( STRING sInfo ) ;
	_DictionaryEntry ( const _DictionaryEntry& Other ) ;
	_DictionaryEntry ( FILE *File ) ;
	_DictionaryEntry ( ) ;
	~_DictionaryEntry ( ) ;
	_DictionaryEntry& operator= ( const _DictionaryEntry &Other ) ;
	const wchar_t *GetInfo ( ) const { return ( Info.c_str() ) ; }
	const SYMBOL_SOUND_SET* GetSymbols ( ) const { return ( &SymbolSounds ) ; }
	bool isPreferred ( ) const { return ( fPreferred ) ; }
	bool isConjugated ( ) const { return ( fConjugated ) ; }
	void AddSymbol ( STRING sSymbol, STRING sSound, BYTE Conjugations[], _ConjugationType ct=CT_NONE ) ;
	void AddSymbol ( STRING sSymbol, STRING sSound ) ;
	void AddSound ( STRING sSound, BYTE Conjugations[] ) ;
	void AddSound ( STRING sSound ) ;
	void Conjugate ( wchar_t const *szSymbol, wchar_t const *szSound, BYTE Conjugations[], _ConjugationClass cc=CC_NULL ) ;
	void Dump ( FILE *File, int Indent=0 ) ;
	bool Load ( FILE *File ) ;
	void Save ( FILE *File ) ;
} ;

typedef std::map<STRING,_DictionaryEntry> DICTIONARY ; // Indexed by definition content.

class _IndexEntry {
	const STRING* psWord ;	// In any mixture of Kanji, Hiragana, Katakana or Romaji.
	const STRING* psKana ;	// In Hiragana.
	const _DictionaryEntry *pDictionaryEntry ;
	const BYTE* pConjugations ;
public:
	_IndexEntry ( const STRING* psw, const STRING* psk, const BYTE* cjgs, const _DictionaryEntry *pEntry ) : psWord(psw), psKana(psk), pDictionaryEntry(pEntry), pConjugations(cjgs) { 
		assert ( pDictionaryEntry ) ;
		IndexEntries.Add ( sizeof(*this) ) ;
	} /* endmethod */
	_IndexEntry ( const _IndexEntry& Other ) : psWord(Other.psWord), psKana(Other.psKana), pDictionaryEntry(Other.pDictionaryEntry), pConjugations(Other.pConjugations) {
		IndexEntries.Add ( sizeof(*this) ) ;
	} /* endmethod */
	_IndexEntry ( ) : psWord(0), psKana(0), pDictionaryEntry(0), pConjugations(0) {
		IndexEntries.Add ( sizeof(*this) ) ;
	} /* endmethod */
	~_IndexEntry ( ) {
		IndexEntries.Remove ( sizeof(*this) ) ;
	} /* endmethod */
	_IndexEntry& operator= ( const _IndexEntry &Other ) {
		psWord = Other.psWord ;
		psKana = Other.psKana ;
		pDictionaryEntry = Other.pDictionaryEntry ;
		pConjugations = Other.pConjugations ;
	} /* endmethod */
	bool operator< ( const _IndexEntry &obj ) const {
		size_t cbCompare = __min ( psWord->size(), obj.psWord->size() ) ;
		int Result = memcmp ( psWord->c_str(), obj.psWord->c_str(), cbCompare*sizeof(wchar_t) ) ;
		const wchar_t *pHere = psWord->c_str() ;
		const wchar_t *pThere = obj.psWord->c_str() ;
		for ( size_t i=0; i<cbCompare; i++, pHere++, pThere++ ) {
			int Result = *pHere - *pThere ;
			if ( Result < 0 )
				return ( true ) ;
			if ( Result > 0 )
				return ( false ) ;
		} /* endif */
		Result = (int) ( psWord->size() - obj.psWord->size() ) ;
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
	const wchar_t *GetWord ( ) const { return ( (wchar_t*)psWord->c_str() ) ; }
	const wchar_t *GetKana ( ) const { return ( (wchar_t*)psKana->c_str() ) ; }
	const BYTE* GetConjugations ( ) const { return ( pConjugations ) ; }
	const _DictionaryEntry *GetDictionaryEntry ( ) const { return ( pDictionaryEntry ) ; }
	void Dump (FILE *File, int Indent = 0, bool fExtra = false) const ;
} ;

typedef std::multiset<_IndexEntry> INDEX ;
extern INDEX Index ;

typedef std::map<STRING,_DictionaryEntry> DICTIONARY ; // Indexed by definition content.
extern DICTIONARY Dictionary ;

extern void AddWord ( wchar_t *pSymbol, wchar_t *pSound, wchar_t *pInfo, bool fConjugate=false ) ;
extern bool BuildDictionary ( wchar_t *szPath, bool fConjugate ) ;
extern bool BuildIndex ( ) ;
extern INDEX::iterator FindSymbolPrefix ( wchar_t *pLine, size_t cbLine ) ;
extern INDEX::iterator FindSymbolExact ( wchar_t *pLine, size_t cbLine ) ;
extern INDEX::iterator FindSymbol ( wchar_t *pLine, size_t cbLine ) ;
extern bool DumpDictionary ( ) ;
extern bool DumpIndex ( ) ;

extern bool SaveDictionary ( ) ;
extern bool LoadDictionary ( ) ;
