// Nihongo.cpp : Defines the entry point for the console application.
//
// TO DO:
// (1) When we encounter furigana, consume them before doing the dictionary lookup, but use the value to 
//   help in selecting which dictionary definition amongst many to choose.
// (2) Add conjugations: potential, causitive, passive causitive.
// (3) Add ARU conjugations.
// (4) If there are two or more preferred readings, and there is are uk versus uK markings, select 
//   the correct preferred reading depending on whether kanji or kana were using in the original text.
//
// NEW ALGORITHM:
//
// (1) Break text into phrases, delimeted by EOL and punctuation of any kind.
// (2) Scan phrase from left, building words or expressions as long as possible.
//  (a) If nothing can be built from the current character, pass it through and try again with the next character.
//

#include "stdafx.h"
#include <Windows.h>
#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <list>
#include <vector>
#include "JDICT\JDICT_Reader.h"
#include "JDICT\Dictionary.h"
#include "Juman\Juman.h"

static struct _Glyph {
	wchar_t *Hiragana ;
	wchar_t *Romaji ;
} Glyphs [] = {

	// Hiragana Doubled consonants
	{ L"っしゃ", L"ssha" },{ L"っしゅ", L"sshu" },{ L"っしょ", L"ssho" },
	{ L"っちゃ", L"ccha" },{ L"っちゅ", L"cchu" },{ L"っちょ", L"ccho" },
	{ L"っきゃ", L"kkya" },{ L"っきゅ", L"kkyu" },{ L"っきょ", L"kkyo" },
	{ L"っぴゃ", L"ppya" },{ L"っぴゅ", L"ppyu" },{ L"っぴょ", L"ppyo" },
	{ L"っが", L"gga" }, { L"っぎ",  L"ggi" }, { L"っぐ", L"ggu" }, { L"っげ", L"gge" }, { L"っご", L"ggo" }, 
	{ L"っか", L"kka" }, { L"っき",  L"kki" }, { L"っく", L"kku" }, { L"っけ", L"kke" }, { L"っこ", L"kko" }, 
	{ L"っぱ", L"ppa" }, { L"っぴ",  L"ppi" }, { L"っぷ", L"ppu" }, { L"っぺ", L"ppe" }, { L"っぽ", L"ppo" }, 
	{ L"っさ", L"ssa" }, { L"っし", L"sshi" }, { L"っす", L"ssu" }, { L"っせ", L"sse" }, { L"っそ", L"sso" }, 
	{ L"った", L"tta" }, { L"っち", L"cchi" }, { L"っつ", L"ttsu" }, { L"って", L"tte" }, { L"っと", L"tto" }, 

	// Hiragana Doubled Vowels (mostly as a side-effect of Juman)
	{ L"あー",     L"aa" }, { L"いー",   L"ii" }, { L"うー",     L"uu" }, { L"えー",  L"ee" }, { L"おー",     L"oo" },
	{ L"ぁー",     L"aa" }, { L"ぃー",   L"ii" }, { L"ぅー",     L"uu" }, { L"ぇー",  L"ee" }, { L"ぉー",     L"oo" },
	{ L"ばー",    L"baa" }, { L"びー",  L"bii" }, { L"ぶー",    L"buu" }, { L"べー", L"bee" }, { L"ぼー",    L"boo" },
	{ L"だー",    L"daa" }, { L"ぢー",  L"jii" }, { L"づー",    L"juu" }, { L"でー", L"dee" }, { L"どー",    L"doo" },
	{ L"がー",    L"gaa" }, { L"ぎー",  L"jii" }, { L"ぐー",    L"guu" }, { L"げー", L"gee" }, { L"ごー",    L"goo" },
	{ L"はー",    L"haa" }, { L"ひー",  L"hii" }, { L"ふー",    L"fuu" }, { L"へー", L"hee" }, { L"ほー",    L"hoo" },
	{ L"かー",    L"kaa" }, { L"きー",  L"kii" }, { L"くー",    L"kuu" }, { L"けー", L"kee" }, { L"こー",    L"koo" },
	{ L"まー",    L"maa" }, { L"みー",  L"mii" }, { L"むー",    L"muu" }, { L"めー", L"mee" }, { L"もー",    L"moo" },
	{ L"なー",    L"naa" }, { L"にー",  L"nii" }, { L"ぬー",    L"nuu" }, { L"ねー", L"nee" }, { L"のー",    L"noo" },
	{ L"ぱー",    L"paa" }, { L"ぴー",  L"pii" }, { L"ぷー",    L"puu" }, { L"ぺー", L"pee" }, { L"ぽー",    L"poo" },
	{ L"らー",    L"raa" }, { L"りー",  L"rii" }, { L"るー",    L"ruu" }, { L"れー", L"ree" }, { L"ろー",    L"roo" },
	{ L"さー",    L"saa" }, { L"しー", L"shii" }, { L"すー",    L"suu" }, { L"せー", L"see" }, { L"そー",    L"soo" },
	{ L"たー",    L"taa" }, { L"ちー", L"chii" }, { L"つー",   L"tsuu" }, { L"てー", L"tee" }, { L"とー",    L"too" },
	{ L"やー",    L"yaa" },                       { L"ゆー",    L"yuu" },					 { L"よー",    L"yoo" },
	{ L"ゃー",    L"yaa" },                       { L"ゅー",    L"yuu" },                     { L"ょー",    L"yoo" },
	{ L"わー",    L"waa" },                                                                   { L"をー",    L"woo" },
	{ L"ざー",    L"zaa" }, { L"じー",  L"zii" }, { L"ずー",    L"zuu" }, { L"ぜー", L"zee" }, { L"ぞー",    L"zoo" },
	{ L"ちゃー", L"chaa" },                       { L"ちゅー", L"chuu" },                      { L"ちょー", L"choo" }, 
	{ L"しゃー", L"shaa" },	                      { L"しゅー", L"shuu" },                      { L"しょー", L"shoo" }, 
	{ L"じゃー",  L"jaa" },                       { L"じゅー",  L"juu" },                      { L"じょー",  L"joo" }, 
	{ L"にゃー",  L"nyaa" },                      { L"にゅー",  L"nyuu" },                     { L"にょー",  L"nyoo" },

	// Double Glyph Hiragana
	{ L"きゃ", L"kya" },{ L"きゅ", L"kyu" },{ L"きょ", L"kyo" },
	{ L"しゃ", L"sha" },{ L"しゅ", L"shu" },{ L"しょ", L"sho" },
	{ L"ちゃ", L"cha" },{ L"ちゅ", L"chu" },{ L"ちょ", L"cho" },
	{ L"にゃ", L"nya" },{ L"にゅ", L"nyu" },{ L"にょ", L"nyo" },
	{ L"ひゃ", L"hya" },{ L"ひゅ", L"hyu" },{ L"ひょ", L"hyo" },
	{ L"みゃ", L"mya" },{ L"みゅ", L"myu" },{ L"みょ", L"myo" },
	{ L"りゃ", L"rya" },{ L"りゅ", L"ryu" },{ L"りょ", L"ryo" },
	{ L"ぎゃ", L"gya" },{ L"ぎゅ", L"gyu" },{ L"ぎょ", L"gyo" },
	{ L"じゃ",  L"ja" },{ L"じゅ",  L"ju" },{ L"じょ",  L"jo" },
	{ L"びゃ", L"bya" },{ L"びゅ", L"byu" },{ L"びょ", L"byo" },
	{ L"ぴゃ", L"pya" },{ L"ぴゅ", L"pyu" },{ L"ぴょ", L"pyo" },

	// Single Glyph Hiragana
	{ L"あ",  L"a" },{ L"い",   L"i" },{ L"う",   L"u" },{ L"え",  L"e" },{ L"お",  L"o" },
	{ L"ぁ",  L"a" },{ L"ぃ",   L"i" },{ L"ぅ",   L"u" },{ L"ぇ",  L"e" },{ L"ぉ",  L"o" },
	{ L"か", L"ka" },{ L"き",  L"ki" },{ L"く",  L"ku" },{ L"け", L"ke" },{ L"こ", L"ko" },
	{ L"さ", L"sa" },{ L"し", L"shi" },{ L"す",  L"su" },{ L"せ", L"se" },{ L"そ", L"so" },
	{ L"た", L"ta" },{ L"ち", L"chi" },{ L"つ", L"tsu" },{ L"て", L"te" },{ L"と", L"to" },
	{ L"な", L"na" },{ L"に",  L"ni" },{ L"ぬ",  L"nu" },{ L"ね", L"ne" },{ L"の", L"no" },
	{ L"は", L"ha" },{ L"ひ",  L"hi" },{ L"ふ",  L"fu" },{ L"へ", L"he" },{ L"ほ", L"ho" },
	{ L"ま", L"ma" },{ L"み",  L"mi" },{ L"む",  L"mu" },{ L"め", L"me" },{ L"も", L"mo" },
	{ L"や", L"ya" },                  { L"ゆ",  L"yu" },                 { L"よ", L"yo" },
	{ L"ゃ", L"ya" },                  { L"ゅ",  L"yu" },                 { L"ょ", L"yo" },
	{ L"ら", L"ra" },{ L"り",  L"ri" },{ L"る",  L"ru" },{ L"れ", L"re" },{ L"ろ", L"ro" },
	{ L"わ", L"wa" },                                                     { L"を", L"wo" },
	{ L"が", L"ga" },{ L"ぎ",  L"gi" },{ L"ぐ",  L"gu" },{ L"げ", L"ge" },{ L"ご", L"go" },
	{ L"ざ", L"za" },{ L"じ",  L"ji" },{ L"ず",  L"zu" },{ L"ぜ", L"ze" },{ L"ぞ", L"zo" },
	{ L"だ", L"da" },{ L"ぢ",  L"ji" },{ L"\x3065",L"zu"},{L"で", L"de" },{ L"ど", L"do" },
	{ L"ば", L"ba" },{ L"び",  L"bi" },{ L"ぶ",  L"bu" },{ L"べ", L"be" },{ L"ぼ", L"bo" },
	{ L"ぱ", L"pa" },{ L"ぴ",  L"pi" },{ L"ぷ",  L"pu" },{ L"ぺ", L"pe" },{ L"ぽ", L"po" },
	{ L"ん",  L"n" },

	// Special Katakana
	{ L"キェ", L"kye" },
	{ L"クァ", L"kwa" },
	{ L"クヮ", L"kwa" },
	{ L"クィ", L"kwi" },
	{ L"クェ", L"kwe" },
	{ L"クォ", L"kwo" },
	{ L"ギェ", L"gye" },
	{ L"グァ", L"gwa" },
	{ L"グヮ", L"gwa" },
	{ L"グィ", L"gwi" },
	{ L"グェ", L"gwe" },
	{ L"グォ", L"gwo" },
	{ L"スィ", L"si"  },
	{ L"シェ", L"she" },
	{ L"ズィ", L"zi"  },
	{ L"ジェ", L"je"  },
	{ L"ティ", L"ti"  },
	{ L"テゥ", L"tu"  },
	{ L"テュ", L"tyu" },
	{ L"チャ", L"cha" },
	{ L"チェ", L"che" },
	{ L"ツァ", L"tsa" },
	{ L"ツィ", L"tsi" },
	{ L"ツェ", L"tse" },
	{ L"ツォ", L"tso" },
	{ L"ディ", L"di"  },
	{ L"ドゥ", L"du"  },
	{ L"デュ", L"dyu" },
	{ L"ニェ", L"nye" },
	{ L"ホゥ", L"hu"  },
	{ L"ヒェ", L"hye" },
	{ L"フィェ",L"fye"}, { L"フョ", L"fyo" },
	{ L"ファ", L"fa"  }, { L"フィ", L"fi"  }, { L"フェ", L"fe"  }, { L"フォ", L"fo"  },
	{ L"フャ", L"fya" }, { L"フュ", L"fyu" },
	{ L"ビェ", L"bye" }, { L"ピェ", L"pye" }, { L"ミェ", L"mye" },
	{ L"イィ", L"yi"  }, { L"イェ", L"ye"  },
	{ L"ラ゜", L"la"  }, { L"リ゜", L"li"  }, { L"ル゜", L"lu"  }, { L"レ゜", L"le"  },	{ L"ロ゜", L"lo"  },
	{ L"リェ", L"rye" },
	{ L"ウァ", L"wa"  }, { L"ウィ", L"wi"  }, { L"ウゥ", L"wu"  }, { L"ウェ", L"we"  }, { L"ウォ", L"wo"  },
	{ L"ウュ", L"wyu" },
	{ L"ヴァ", L"va"  }, { L"ヴィ", L"vi"  }, { L"ヴ",   L"vu"  }, { L"ヴェ", L"ve"  }, { L"ヴォ", L"vo"  },
	{ L"ヴャ", L"vya" }, { L"ヴュ", L"vyu" }, { L"ヴェ", L"vye" }, { L"ヴョ", L"vyo" },

	// Katakana - Three-character combinations.
	{ L"ッダー", L"ddaa" }, { L"ッヂー",  L"jjii" }, { L"ッヅー",  L"jjuu" }, { L"ッデー", L"ddee" }, { L"ッドー", L"ddoo" }, 
	{ L"ッガー", L"ggaa" }, { L"ッギー",  L"ggii" }, { L"ッグー",  L"gguu" }, { L"ッゲー", L"ggee" }, { L"ッゴー", L"ggoo" }, 
	{ L"ッカー", L"kkaa" }, { L"ッキー",  L"kkii" }, { L"ックー",  L"kkuu" }, { L"ッケー", L"kkee" }, { L"ッコー", L"kkoo" }, 
	{ L"ッサー", L"ssaa" }, { L"ッシー", L"sshii" }, { L"ッスー",  L"ssuu" }, { L"ッセー", L"ssee" }, { L"ッソー", L"ssoo" }, 
	{ L"ッザー", L"zzaa" }, { L"ッジー",  L"zzii" }, { L"ッズー",  L"zzuu" }, { L"ッゼー", L"zzee" }, { L"ッゾー", L"zzoo" }, 
	{ L"ッター", L"ttaa" }, { L"ッチー", L"cchii" }, { L"ッツー", L"ttsuu" }, { L"ッテー", L"ttee" }, { L"ットー", L"ttoo" }, 
	{ L"ッパー", L"ppaa" }, { L"ッピー",  L"ppii" }, { L"ップー",  L"ppuu" }, { L"ッペー", L"ppee" }, { L"ッポー", L"ppoo" }, 
	{ L"チャー", L"chaa" },                          { L"チュー",  L"chuu" },                         { L"チョー", L"choo" }, 
	{ L"シャー", L"shaa" },	                         { L"シュー",  L"shuu" },                         { L"ショー", L"shoo" }, 
	{ L"ッシャ", L"ssha" },                          { L"ッシュ",  L"sshu" },                         { L"ッショ", L"ssho" },
	{ L"ジャー",  L"jaa" },                          { L"ジュー",  L"juu" },                          { L"ジョー",  L"joo" }, 
	{ L"キャー", L"kyaa" },                          { L"キュー",  L"kyuu" },                         { L"キョー", L"kyoo" },
	{ L"ミャー", L"myaa" },                          { L"ミュー",  L"myuu" },                         { L"ミョー", L"myoo" },
	{ L"リャー", L"ryaa" },                          { L"リュー",  L"ryuu" },                         { L"リョー", L"ryoo" },
	{ L"ヒャー", L"hyaa" },                          { L"ヒュー",  L"hyuu" },                         { L"ヒョー", L"hyoo" },
	{ L"ビャー", L"byaa" },                          { L"ビュー",  L"byuu" },                         { L"ビョー", L"byoo" },
	{ L"ピャー", L"pyaa" },                          { L"ピュー",  L"pyuu" },                         { L"ピョー", L"pyoo" },
	{ L"ディー",  L"dii" },
	{ L"フィェ",  L"fye" }, 
	{ L"ニャー", L"nyaa" },                          { L"ニュー", L"nyuu" },                          { L"ニョー", L"nyoo" },

	// Katakana - Two-character combinations.
	{ L"キェ", L"kye" },
	{ L"クァ", L"kwa" },
	{ L"クヮ", L"kwa" },
	{ L"クィ", L"kwi" },
	{ L"クェ", L"kwe" },
	{ L"クォ", L"kwo" },
	{ L"ギェ", L"gye" },
	{ L"グァ", L"gwa" },
	{ L"グヮ", L"gwa" },
	{ L"グィ", L"gwi" },
	{ L"グェ", L"gwe" },
	{ L"グォ", L"gwo" },
	{ L"スィ", L"si"  },
	{ L"シェ", L"she" },
	{ L"ズィ", L"zi"  },
	{ L"ジェ", L"je"  },
	{ L"ティ", L"ti"  },
	{ L"テゥ", L"tu"  },
	{ L"テュ", L"tyu" },
	{ L"チェ", L"che" },
	{ L"ツァ", L"tsa" },
	{ L"ツィ", L"tsi" },
	{ L"ツェ", L"tse" },
	{ L"ツォ", L"tso" },
	{ L"ディ", L"di"  },
	{ L"ドゥ", L"du"  },
	{ L"デュ", L"dyu" },
	{ L"ニェ", L"nye" },
	{ L"ホゥ", L"hu"  },
	{ L"ヒェ", L"hye" },
	{ L"フョ", L"fyo" },
	{ L"ファ", L"fa"  }, { L"フィ", L"fi"  }, { L"フェ", L"fe"  }, { L"フォ", L"fo"  },
	{ L"フャ", L"fya" }, { L"フュ", L"fyu" },
	{ L"ビェ", L"bye" }, { L"ピェ", L"pye" }, { L"ミェ", L"mye" },
	{ L"イィ", L"yi"  }, { L"イェ", L"ye"  },
	{ L"ラ゜", L"la"  }, { L"リ゜", L"li"  }, { L"ル゜", L"lu"  }, { L"レ゜", L"le"  },	{ L"ロ゜", L"lo"  },
	{ L"リェ", L"rye" },
	{ L"ウァ", L"wa"  }, { L"ウィ", L"wi"  }, { L"ウゥ", L"wu"  }, { L"ウェ", L"we"  }, { L"ウォ", L"wo"  },
	{ L"ウュ", L"wyu" },
	{ L"ヴァ", L"va"  }, { L"ヴィ", L"vi"  }, { L"ヴ",   L"vu"  }, { L"ヴェ", L"ve"  }, { L"ヴォ", L"vo"  },
	{ L"ヴャ", L"vya" }, { L"ヴュ", L"vyu" }, { L"ヴェ", L"vye" }, { L"ヴョ", L"vyo" },

	// Katakana Doubled Vowels
	{ L"アー",     L"aa" }, { L"イー",   L"ii" }, { L"ウー",     L"uu" }, { L"エー",  L"ee" }, { L"オー",     L"oo" },
	{ L"ァー",     L"aa" }, { L"ィー",   L"ii" }, { L"ゥー",     L"uu" }, { L"ェー",  L"ee" }, { L"ォー",     L"oo" },
	{ L"バー",    L"baa" }, { L"ビー",  L"bii" }, { L"ブー",    L"buu" }, { L"ベー", L"bee" }, { L"ボー",    L"boo" },
	{ L"ダー",    L"daa" }, { L"ヂー",  L"jii" }, { L"ヅー",    L"juu" }, { L"デー", L"dee" }, { L"ドー",    L"doo" },
	{ L"ガー",    L"gaa" }, { L"ギー",  L"jii" }, { L"グー",    L"guu" }, { L"ゲー", L"gee" }, { L"ゴー",    L"goo" },
	{ L"ハー",    L"haa" }, { L"ヒー",  L"hii" }, { L"フー",    L"fuu" }, { L"ヘー", L"hee" }, { L"ホー",    L"hoo" },
	{ L"カー",    L"kaa" }, { L"キー",  L"kii" }, { L"クー",    L"kuu" }, { L"ケー", L"kee" }, { L"コー",    L"koo" },
	{ L"マー",    L"maa" }, { L"ミー",  L"mii" }, { L"ムー",    L"muu" }, { L"メー", L"mee" }, { L"モー",    L"moo" },
	{ L"ナー",    L"naa" }, { L"ニー",  L"nii" }, { L"ヌー",    L"nuu" }, { L"ネー", L"nee" }, { L"ノー",    L"noo" },
	{ L"パー",    L"paa" }, { L"ピー",  L"pii" }, { L"プー",    L"puu" }, { L"ペー", L"pee" }, { L"ポー",    L"poo" },
	{ L"ラー",    L"raa" }, { L"リー",  L"rii" }, { L"ルー",    L"ruu" }, { L"レー", L"ree" }, { L"ロー",    L"roo" },
	{ L"サー",    L"saa" }, { L"シー", L"shii" }, { L"スー",    L"suu" }, { L"セー", L"see" }, { L"ソー",    L"soo" },
	{ L"ター",    L"taa" }, { L"チー", L"chii" }, { L"ツー",   L"tsuu" }, { L"テー", L"tee" }, { L"トー",    L"too" },
	{ L"ヤー",    L"yaa" },                       { L"ユー",    L"yuu" },                      { L"ヨー",    L"yoo" },
	{ L"ャー",    L"yaa" },                       { L"ュー",    L"yuu" },                      { L"ョー",    L"yoo" },
	{ L"ワー",    L"waa" },                                                                    { L"ヲー",    L"woo" },
	{ L"ザー",    L"zaa" }, { L"ジー",  L"zii" }, { L"ズー",    L"zuu" }, { L"ゼー", L"zee" }, { L"ゾー",    L"zoo" },
	
	// Katakana Doubled Consonants
	{ L"ッダ", L"dda" }, { L"ッヂ",  L"jji" }, { L"ッヅ",  L"jju" }, { L"ッデ", L"dde" }, { L"ッド", L"ddo" }, 
	{ L"ッガ", L"gga" }, { L"ッギ",  L"ggi" }, { L"ッグ",  L"ggu" }, { L"ッゲ", L"gge" }, { L"ッゴ", L"ggo" }, 
	{ L"ッカ", L"kka" }, { L"ッキ",  L"kki" }, { L"ック",  L"kku" }, { L"ッケ", L"kke" }, { L"ッコ", L"kko" }, 
	{ L"ッサ", L"ssa" }, { L"ッシ", L"sshi" }, { L"ッス",  L"ssu" }, { L"ッセ", L"sse" }, { L"ッソ", L"sso" }, 
	{ L"ッザ", L"zza" }, { L"ッジ",  L"zzi" }, { L"ッズ",  L"zzu" }, { L"ッゼ", L"zze" }, { L"ッゾ", L"zzo" }, 
	{ L"ッタ", L"tta" }, { L"ッチ", L"cchi" }, { L"ッツ", L"ttsu" }, { L"ッテ", L"tte" }, { L"ット", L"tto" }, 
	{ L"ッパ", L"ppa" }, { L"ッピ",  L"ppi" }, { L"ップ",  L"ppu" }, { L"ッペ", L"ppe" }, { L"ッポ", L"ppo" }, 

	// Double Glyph Katakana
	{ L"キャ", L"kya" },{ L"キュ", L"kyu" },{ L"キョ", L"kyo" },
	{ L"シャ", L"sha" },{ L"シュ", L"shu" },{ L"ショ", L"sho" },
	{ L"チャ", L"cha" },{ L"チュ", L"chu" },{ L"チョ", L"cho" },
	{ L"ニャ", L"nya" },{ L"ニュ", L"nyu" },{ L"ニョ", L"nyo" },
	{ L"ヒャ", L"hya" },{ L"ヒュ", L"hyu" },{ L"ヒョ", L"hyo" },
	{ L"ミャ", L"mya" },{ L"ミュ", L"myu" },{ L"ミョ", L"myo" },
	{ L"リャ", L"rya" },{ L"リュ", L"ryu" },{ L"リョ", L"ryo" },
	{ L"ギャ", L"gya" },{ L"ギュ", L"gyu" },{ L"ギョ", L"gyo" },
	{ L"ジャ",  L"ja" },{ L"ジュ",  L"ju" },{ L"ジョ",  L"jo" },
	{ L"ビャ", L"bya" },{ L"ビュ", L"byu" },{ L"ビョ", L"byo" },
	{ L"ピャ", L"pya" },{ L"ピュ", L"pyu" },{ L"ピョ", L"pyo" },
	{ L"フォ",  L"fo" },
	{ L"ティ",  L"ti" },
	{ L"ディ",  L"di" },
	{ L"ウィ",  L"wi" },
	{ L"ヴィ",  L"vi" },
	{ L"イィ",  L"yi" },
	{ L"イェ",  L"ye" },

	// Single Glyph Katakana
	{ L"ア",  L"a" },{ L"イ",   L"i" },{ L"ウ",   L"u" },{ L"エ",  L"e" },{ L"オ",  L"o" },
	{ L"ァ",  L"a" },{ L"ぃ",   L"i" },{ L"ゥ",   L"u" },{ L"ェ",  L"e" },{ L"ォ",  L"o" },
	{ L"カ", L"ka" },{ L"キ",  L"ki" },{ L"ク",  L"ku" },{ L"ケ", L"ke" },{ L"コ", L"ko" },
	{ L"サ", L"sa" },{ L"シ", L"shi" },{ L"ス",  L"su" },{ L"セ", L"se" },{ L"ソ", L"so" },
	{ L"タ", L"ta" },{ L"チ", L"chi" },{ L"ツ", L"tsu" },{ L"テ", L"te" },{ L"ト", L"to" },
	{ L"ナ", L"na" },{ L"ニ",  L"ni" },{ L"ヌ",  L"nu" },{ L"ネ", L"ne" },{ L"ノ", L"no" },
	{ L"ハ", L"ha" },{ L"ヒ",  L"hi" },{ L"フ",  L"fu" },{ L"ヘ", L"he" },{ L"ホ", L"ho" },
	{ L"マ", L"ma" },{ L"ミ",  L"mi" },{ L"ム",  L"mu" },{ L"メ", L"me" },{ L"モ", L"mo" },
	{ L"ヤ", L"ya" },                  { L"ユ",  L"yu" },                 { L"ヨ", L"yo" },
	{ L"ラ", L"ra" },{ L"リ",  L"ri" },{ L"ル",  L"ru" },{ L"レ", L"re" },{ L"ロ", L"ro" },
	{ L"ワ", L"wa" },                                                     { L"ヲ", L"wo" },
	{ L"ガ", L"ga" },{ L"ギ",  L"gi" },{ L"グ",  L"gu" },{ L"ゲ", L"ge" },{ L"ゴ", L"go" },
	{ L"ザ", L"za" },{ L"ジ",  L"ji" },{ L"ズ",  L"zu" },{ L"ゼ", L"ze" },{ L"ゾ", L"zo" },
	{ L"ダ", L"da" },                  { L"ヅ", L"dzu" },{ L"デ", L"de" },{ L"ド", L"do" },
	{ L"バ", L"ba" },{ L"ビ",  L"bi" },{ L"ブ",  L"bu" },{ L"ベ", L"be" },{ L"ボ", L"bo" },
	{ L"パ", L"pa" },{ L"ピ",  L"pi" },{ L"プ",  L"pu" },{ L"ペ", L"pe" },{ L"ポ", L"po" },
	{ L"ン",  L"n" },

	// Numbers
	{ L"０", L"0" }, { L"１", L"1" }, { L"２", L"2" }, { L"３", L"3" }, { L"４", L"4" }, 
	{ L"５", L"5" }, { L"６", L"6" }, { L"７", L"7" }, { L"８", L"8" }, { L"９", L"9" }, 

	// Miscellaneous Punctuation
	{ L"　", L" " },
	{ L"、", L"," },
	{ L"。", L"." },
	{ L"？", L"?" },
	{ L"！", L"!" },
	{ L"（", L"(" }, { L"）", L")" },
	{ L"《", L"(" }, { L"》", L")" },
	{ L"「", L"\"" }, { L"」", L"\"" },
	{ L"『", L"\'" }, { L"』", L"\'" },
} ;

//
// Convert Hiragana to Romaji.
//

static void ConvertToRomaji ( wchar_t*& pKana, unsigned cbKana, wchar_t*& pBuffer, unsigned& cbBuffer ) {
	while ( *pKana && cbKana-- ) {
		unsigned i ;
		for ( i=0; i<_countof(Glyphs); i++ ) {
			if ( !memcmp ( Glyphs[i].Hiragana, pKana, wcslen(Glyphs[i].Hiragana)*sizeof(wchar_t) ) )
				break ;
		} /* endfor */
		if ( i < _countof(Glyphs) ) {
			wchar_t *pGlyph = Glyphs[i].Romaji ;
			unsigned cbGlyph = (unsigned) wcslen ( pGlyph ) ;
			wcscpy_s ( pBuffer, cbBuffer, pGlyph ) ;
			pBuffer += cbGlyph ; cbBuffer -= cbGlyph ;
			size_t cbKana = wcslen ( Glyphs[i].Hiragana ) ;
			pKana += cbKana ;
		} else {
			*pBuffer++ = *pKana++ ;
			cbBuffer-- ;
		} /* endif */
	} /* endwhile */
	*pBuffer = 0 ;
}

inline bool isBreak ( wchar_t c, bool &fBefore, bool &fAfter ) {
	fBefore = fAfter = false ;
	switch ( c ) {
		case L'.': case L'。':
		case L',': case L'、':
		case L';': case L'；': 
		case L':': case L'：': 
		case L'!': case L'！': 
		case L'?': case L'？': 
		case L'…': 
		case L')': case L']': case L'>': case L'」': case L'）': case L'》': case L'〕': case L'』': 
			fAfter = true ;
			return ( true ) ;
		case L'(': case L'[': case L'<': case L'「': case L'（': case L'《': case L'〔': case L'『': 
			fBefore = true ;
			return ( true ) ;
		case L'¶': case L'・': case L' ': case L'　': case 0x0D: case 0x0A: // These are breaks, but no padding is required.
			return ( true ) ;
	} /* endswitch */
	return ( false ) ;
}

inline bool isHiragana ( wchar_t c ) { return ( ( c >= 0x3040 ) && ( c <= 0x309F ) ) ; }
inline bool isKatakana ( wchar_t c ) { return ( ( c >= 0x30A0 ) && ( c <= 0x30FF ) ) ; }
inline bool isKanji ( wchar_t c ) {	return ( ( ( c >= 0x4E00 ) && ( c <= 0x9FBF ) ) || ( c == L'々' ) ) ; }
inline bool isWhite ( wchar_t c ) { return ( ( c == L' ' ) || ( c == L'　' ) || ( c == L'\t' ) || ( c == L'\r' ) || ( c == L'\n' ) ) ; }

const wchar_t* KatakanaSuffix = L"ァィゥェォャュョ";
inline bool isKatakanaSuffix(wchar_t c) { return (wcschr(KatakanaSuffix, c) != NULL); }

const wchar_t* HiraganaSuffix = L"ぁぃぅぇぉゃゅょ";
inline bool isHiraganaSuffix(wchar_t c) { return (wcschr(HiraganaSuffix, c) != NULL); }

typedef std::vector<_IndexEntry2*> SYMBOLS ;
typedef std::vector<_IndexEntry2*> DEFINITIONS ; 

static _IndexEntry2* pParticleDE ;
static _IndexEntry2* pParticleGA ;
static _IndexEntry2* pParticleKA ;
static _IndexEntry2* pParticleMO ;
static _IndexEntry2* pParticleNI ;
static _IndexEntry2* pParticleNO ;
static _IndexEntry2* pParticleTO ;
static _IndexEntry2* pParticleWA ;
static _IndexEntry2* pParticleWO ;
static _IndexEntry2* pHonorificO ;
static _IndexEntry2* pHonorificGO ;

static std::list<std::wstring> GetTypes(const wchar_t* pInfo) {
	std::list<std::wstring> Result;
	const wchar_t* p = pInfo;
	if (*p == '(') {
		bool Done{ false };
		wchar_t Type[20];
		size_t cb = 0;
		++p;
		while (!Done && *p) {
			switch (*p) {
			case L')':
				Type[cb] = 0;
				if (Type[0])
					Result.push_back(Type);
				Done = true;
				break;
			case L',':
				Type[cb] = 0;
				if (Type[0])
					Result.push_back(Type);
				cb = 0;
				break;
			default:
				Type[cb++] = *p;
			}
			++p;
			Type[cb] = 0;
		}
	}
	return Result;
}

static bool IsAdverbTO(_IndexEntry2* pIndexEntry) {
	std::list<std::wstring> Types = GetTypes(pIndexEntry->pszInfo);
	std::list<std::wstring>::iterator Cursor = std::find(Types.begin(), Types.end(), L"adv-to");
	return (Cursor != Types.end());
}

static void FinalizePhrase ( SYMBOLS Symbols, wchar_t*& pKana, unsigned& cbKana, wchar_t*& pRomaji, unsigned& cbRomaji, DEFINITIONS& Definitions ) {
	wchar_t *pKana0 = pKana ;
	SYMBOLS::iterator SymbolsCursor = Symbols.begin ( ) ;
	while ( SymbolsCursor != Symbols.end() ) {
		_IndexEntry2 *pIndexEntry = *SymbolsCursor ;
		if ( pIndexEntry->isExtension() ) {
			Definitions.push_back ( pIndexEntry ) ;
		} else {
			// Scan backwards to first definition for this word.
			_IndexEntry2 *pFirst = pIndexEntry ;
			if ( Index2.Begin() < pFirst ) {
				_IndexEntry2 *pPrevious = pFirst - 1 ;
				while ( !wcscmp ( pPrevious->pszWord, pFirst->pszWord ) ) {
					pFirst = pPrevious ;
					pPrevious = pFirst - 1 ;
				} /* endwhile */
			} /* endif */
			// Stuff every matching word into the definitions list.
			while ( pFirst < Index2.End() ) {
				if ( wcscmp ( pFirst->pszWord, pIndexEntry->pszWord ) ) 
					break ;
				Definitions.push_back ( pFirst ) ;
				pFirst ++ ;
			} /* endwhile */
		} /* endif */
		++ SymbolsCursor ;
		// Insert the sound into the Kana string.
		const wchar_t *pSound = pIndexEntry->pszKana ;
		size_t cbSound = wcslen ( pSound ) ;
		wcscpy_s ( pKana, cbKana, pSound ) ;
		pKana += cbSound ; 
		cbKana = (unsigned) __max ( 0, cbKana - cbSound ) ;
		if ( SymbolsCursor != Symbols.end() ) {
			*pKana++ = L' ' ;
			cbKana = (unsigned) __max ( 0, cbKana - 1 ) ;
		} /* endif */
	} /* endwhile */
	*pKana = 0 ;
	ConvertToRomaji ( pKana0, (unsigned)wcslen(pKana0), pRomaji, cbRomaji ) ;
}

class _Override {
	wchar_t *pszText;
	size_t cbText;
public:
	_Override(wchar_t *psz) : pszText(psz), cbText(wcslen(psz)) { }
	const bool Match(wchar_t *p, unsigned cb) const {
		if (cb < cbText)
			return (false);
		return (!memcmp(p, pszText, cbText));
	}
};

static bool BuildPhrase(SYMBOLS Symbols, wchar_t* p, unsigned cb, wchar_t*& pKana, unsigned& cbKana, wchar_t*& pRomaji, unsigned& cbRomaji, DEFINITIONS& Definitions) {

	// Initialize the result.
	pKana[0] = pRomaji[0] = 0;

	// Special logic: If the current symbol might be a particle, then...
	// If what follows is a complete word,
	//   If the first character plus the complete word is also a word,
	//      Use the combined word.
	//   Use the particle as a word, and save the second word too.
	// End if what followed was a complete word.
	_IndexEntry2* pParticle = NULL;
	switch (p[0]) {
	case L'で': pParticle = pParticleDE; break;
	case L'が': pParticle = pParticleGA; break;
	case L'か': {
		static _Override Overrides[] = {
			_Override(L"かもしれません"),
			_Override(L"かもしれない"),
			_Override(L"かわいそう"),
			_Override(L"かわいい"),
		};
		int i;
		for (i = 0; i < _countof(Overrides); i++) {
			if (Overrides[i].Match(p, cb))
				break;
		} /* endfor */
		if (i >= _countof(Overrides))
			pParticle = pParticleKA;
		break; } /* endcase */
	case L'も': {
		static _Override Overrides[] = {
			_Override(L"もちろん"),
			_Override(L"もらって"),
			_Override(L"もらう"),
		};
		int i;
		for (i = 0; i < _countof(Overrides); i++) {
			if (Overrides[i].Match(p, cb))
				break;
		} /* endfor */
		if (i >= _countof(Overrides))
			pParticle = pParticleMO;
		break; } /* endcase */
	case L'に': pParticle = pParticleNI; break;
	case L'の': pParticle = pParticleNO; break;
	case L'と': {
		static _Override Overrides[] = {
			_Override(L"とんでもない"),
			_Override(L"とりあえず"),
			_Override(L"ともなく"),
			_Override(L"とはいえ"),
		};
		int i;
		for (i = 0; i < _countof(Overrides); i++) {
			if (Overrides[i].Match(p, cb))
				break;
		} /* endfor */
		if (i >= _countof(Overrides))
			pParticle = pParticleTO;
		break; } /* endcase */
	case L'は': pParticle = pParticleWA; break;
	case L'を': pParticle = pParticleWO; break;
	case L'お': pParticle = pHonorificO; break;
	case L'ご': pParticle = pHonorificGO; break;
	} /* endswitch */
	if (pParticle) {
		_IndexEntry2* pIndexEntry1 = Index2.FindLongestMatch(p + 1, cb - 1);
		if (pIndexEntry1) {
			_IndexEntry2* pIndexEntry2 = Index2.FindLongestMatch(p, cb);
			if (pIndexEntry2) {
				unsigned cbWord1 = (unsigned)wcslen(pIndexEntry1->pszWord);
				unsigned cbWord2 = (unsigned)wcslen(pIndexEntry2->pszWord);
				if (cbWord2 == cbWord1 + 1) {
					// Combined word.  Allow old logic to handle it.
				}
				else {
					// Add the particle to the symbol list, adjust where we are and continue.
					Symbols.push_back(pParticle);
					p++; cb--;
				} /* endif */
			} /* endif */
		} /* endif */
	} /* endif */

	// Build the largest words we can first.  We stop searching once we have a match for everything in the sentence.
	for (unsigned i = cb; i > 0; i--) {
		bool DynamicEntry = false;
		_IndexEntry2* pIndexEntry = Index2.FindPreferredEntry(p, i);
		if (pIndexEntry) {
			if ((i < cb) && IsAdverbTO(pIndexEntry)) {
				if (p[i] == L'と') {
					pIndexEntry = new _IndexEntry2(*pIndexEntry, L"と");
					DynamicEntry = true;
					i++;
				} /* endif */
			} /* endif */
			size_t cbEntry = wcslen(pIndexEntry->pszKana);
			if ((cbEntry < cb) && isHiraganaSuffix(p[cbEntry]))
				continue;
			if ((cbEntry < cb) && isKatakanaSuffix(p[cbEntry]))
				continue;
			// Try building a phrase with this word as the start.
			// Pass in the word found and the pointer to continue the search from.
			Symbols.push_back(pIndexEntry);
			if (cb - i) {
				if (BuildPhrase(Symbols, p + i, cb - i, pKana, cbKana, pRomaji, cbRomaji, Definitions))
					return (true);
				else
					DO_NOTHING;
			}
			else {
				FinalizePhrase(Symbols, pKana, cbKana, pRomaji, cbRomaji, Definitions);
				return (true); // We have processed everything.
			} /* endif */
			Symbols.pop_back();
			if (DynamicEntry)
				delete pIndexEntry;
		} /* endif */
	} /* endfor */

	// Found nothing at all?

	// Fallback #1: If the first character is katakana, then gather all the katakana into a "word", store it, and continue.
	if (isKatakana(p[0])) {
		size_t Length = 1; bool fBefore, fAfter;
		while (isKatakana(p[Length]) && !isBreak(p[Length], fBefore, fAfter))
			++Length;
		_IndexEntry2* pIndexEntry = new _IndexEntry2(p, Length, L"???", false);
		Symbols.push_back(pIndexEntry);
		if (cb - Length) {
			if (BuildPhrase(Symbols, p + Length, (unsigned)(cb - Length), pKana, cbKana, pRomaji, cbRomaji, Definitions))
				return (true);
			else
				DO_NOTHING;
		}
		else {
			FinalizePhrase(Symbols, pKana, cbKana, pRomaji, cbRomaji, Definitions);
			return (true); // We have processed everything.
		} /* endif */
		Symbols.pop_back();
	} /* endif */

	// We tried, but we couldn't do it.
	return (false);
}

static void ScanPhrase ( wchar_t *p, unsigned cb, wchar_t*& pKana, unsigned& cbKana, wchar_t*& pRomaji, unsigned& cbRomaji, DEFINITIONS& Definitions ) {

	// Initialize the result.
	pKana[0] = pRomaji[0] = 0 ;

	// Scan the phrase from left to right, looking for the biggest symbol matches as you go.
	wchar_t *pKana0 = pKana ;
	while ( cb ) {
		_IndexEntry2 *pIndexEntry = Index2.FindPreferredEntry ( p, cb ) ;
		if ( pIndexEntry ) {
			// Separate from what went on before, if necessary.
			if ( ( pKana > pKana0 ) && !isWhite ( pKana[-1] ) ) {
				*pKana++ = L' ' ;
				cbKana = (unsigned) __max ( 0, cbKana - 1 ) ;
			} /* endif */
			// Found something in the dictionary?  
			const wchar_t *pWord = pIndexEntry->pszWord ; size_t cbWord = wcslen(pWord) ;
			const wchar_t *pSound = pIndexEntry->pszKana ; size_t cbSound = wcslen(pSound) ;
			if ( ( cbWord > 1 ) || isKanji ( pWord[0] ) ) {
				Definitions.push_back ( pIndexEntry ) ;
				do {
					_IndexEntry2 *pIndexEntry2 = pIndexEntry + 1 ;
					if ( pIndexEntry2 < Index2.End() ) {
						if ( wcscmp ( pIndexEntry->pszWord, pIndexEntry2->pszWord ) )
							break ;
						Definitions.push_back ( pIndexEntry2 ) ;
					} /* endif */
					pIndexEntry = pIndexEntry2 ;
				} while ( pIndexEntry < Index2.End() ) ;
				wcscpy_s ( pKana, cbKana, pSound ) ;
				pKana += cbSound ;
				cbKana = (unsigned) __max ( 0, cbKana - cbSound ) ;
				p += cbWord ; cb -= (unsigned) cbWord ;
				if ( cb ) {
					*pKana++ = L' ' ;
					cbKana = (unsigned) __max ( 0, cbKana - 1 ) ;
				} /* endif */
			} else {
				*pKana++ = *p++ ; cb -- ;
				cbKana = (unsigned) __max ( 0, cbKana - 1 ) ;
			} /* endif */
		} else {
			*pKana++ = *p++ ; cb -- ;
			cbKana = (unsigned) __max ( 0, cbKana - 1 ) ;
		} /* endif */
	} /* endwhile */
	*pKana = 0 ;

	// Convert to Romaji.
	ConvertToRomaji ( pKana0, (unsigned)wcslen(pKana0), pRomaji, cbRomaji ) ;
}

static void WriteDefinitions ( FILE *Destin, DEFINITIONS& Definitions, int Indent=0 ) {

	// Don't do this if the definition list is empty.
	if ( Definitions.size() == 0 )
		return ;

	// Clean up the list of english definitions, removing duplicates.
	DEFINITIONS::iterator Cursor1 = Definitions.begin ( ) ;
	while ( Cursor1 != Definitions.end() ) {
		_IndexEntry2 *pEntry1 = *Cursor1 ;
		bool fRestart = false ;
		DEFINITIONS::iterator Cursor2 = Cursor1 ;
		++ Cursor2 ;
		while ( Cursor2 != Definitions.end() ) {
			_IndexEntry2 *pEntry2 = *Cursor2 ;
			if ( !wcscmp(pEntry1->pszWord,pEntry2->pszWord) && !wcscmp(pEntry1->pszKana,pEntry2->pszKana) && !wcscmp(pEntry1->pszInfo,pEntry2->pszInfo) ) {
				pEntry2->Release ( ) ;
				Definitions.erase ( Cursor2 ) ;
				fRestart = true ;
				break ;
			} /* endif */
			++ Cursor2 ;
		} /* endwhile */
		if ( fRestart )
			Cursor1 = Definitions.begin ( ) ;
		else
			++ Cursor1 ;
	} /* endwhile */

	// Output every definition which applies to this line.
	fwprintf ( Destin, L"%*lsDefinitions:\n", Indent, L"" ) ;
	for (auto& Entry : Definitions) {
		Entry->Dump(Destin, Indent + 2);
		Entry->Release();
	} /* endfor */
}

static bool SeparationMatrix [_Juman::POSC_MAX] [_Juman::POSC_MAX] = { // [From] [To]
	//	SPECIAL	VERB	ADJECTIVE	FINALPARTICLE	AUXILIARY	NOUN	7		ADVERB	PARTICLE	10		11		12		PREFIX	SUFFIX	UNDEFINED
	{	false,	true,	true,		true,			true,		true,	true,	true,	true,		true,	true,	true,	true,	false,	true }, // SPECIAL
	{	false,	true,	true,		true,			true,		true,	true,	true,	true,		true,	true,	true,	true,	false,	true }, // VERB
	{	false,	true,	true,		true,			true,		true,	true,	true,	true,		true,	true,	true,	true,	false,	true }, // ADJECTIVE
	{	false,	true,	true,		true,			true,		true,	true,	true,	true,		true,	true,	true,	true,	false,	true }, // FINALPARTICLE
	{	false,	true,	true,		true,			true,		true,	true,	true,	true,		true,	true,	true,	true,	false,	true }, // AUXILIARY
	{	false,	true,	true,		true,			true,		true,	true,	true,	true,		true,	true,	true,	true,	false,	true }, // NOUN
	{	false,	true,	true,		true,			true,		true,	true,	true,	true,		true,	true,	true,	true,	false,	true }, // 7
	{	false,	true,	true,		true,			true,		true,	true,	true,	true,		true,	true,	true,	true,	false,	true }, // ADVERB
	{	false,	true,	true,		true,			true,		true,	true,	true,	true,		true,	true,	true,	true,	false,	true }, // PARTICLE
	{	false,	true,	true,		true,			true,		true,	true,	true,	true,		true,	true,	true,	true,	false,	true }, // 10
	{	false,	true,	true,		true,			true,		true,	true,	true,	true,		true,	true,	true,	true,	false,	true }, // 11
	{	false,	true,	true,		true,			true,		true,	true,	true,	true,		true,	true,	true,	true,	false,	true }, // 12
	{	false,	false,	false,		false,			false,		false,	false,	false,	false,		false,	false,	false,	false,	false,	false },// PREFIX
	{	false,	true,	true,		true,			true,		true,	true,	true,	true,		true,	true,	true,	true,	false,	true }, // SUFFIX
	{	false,	true,	true,		true,			true,		true,	true,	true,	true,		true,	true,	true,	true,	false,	true }, // UNDEFINED
} ;

//
// Rules for improving Juman analysis with the help of EDICT:
//
// (1) Build the list of parts of speech according to JUMAN.
// (2) Wherever JUMAN fails to decipher a KANJI, look it up in EDICT.
// (3) Where multiple nouns adjoin, check them against EDICT to discover compound nouns.
// (4) During Romaji output, convert particles HA, HE and WO to WA, E and O.
//

class _JumanResult {
private:
	wchar_t szOriginal [100] ;
	wchar_t szKana [100] ;
	_Juman::_PartOfSpeechCode posc ;
public:
	_JumanResult ( wchar_t *pszOriginal, wchar_t *pszKana, _Juman::_PartOfSpeechCode code ) : posc(code) {
		wcscpy_s ( szOriginal, _countof(szOriginal), pszOriginal ) ;
		wcscpy_s ( szKana, _countof(szKana), pszKana ) ;
	} /* endmethod */
	_Juman::_PartOfSpeechCode GetPartOfSpeech ( ) { return ( posc ) ; }
	wchar_t *GetOriginal ( wchar_t *pBuffer, unsigned cbBuffer ) {
		wcscpy_s ( pBuffer, cbBuffer, szOriginal ) ;
		return ( pBuffer ) ;
	} /* endmethod */
	wchar_t *GetKana ( wchar_t *pBuffer, unsigned cbBuffer ) {
		wcscpy_s ( pBuffer, cbBuffer, szKana ) ;
		return ( pBuffer ) ;
	} /* endmethod */
	wchar_t *GetRomaji ( wchar_t *pBuffer, unsigned cbBuffer ) {
		if ( posc == _Juman::POSC_PARTICLE ) {
			if ( !wcscmp ( szKana, L"は" ) )
				return ( L"wa" ) ;
			if ( !wcscmp ( szKana, L"へ" ) )
				return ( L"e" ) ;
			if ( !wcscmp ( szKana, L"を" ) )
				return ( L"o" ) ;
		} /* endif */
		wchar_t *pKana = szKana ;
		wchar_t *pRomaji = pBuffer ; unsigned cbRomaji = cbBuffer ;
		ConvertToRomaji ( pKana, (unsigned)wcslen(szKana), pRomaji, cbRomaji ) ;
		return ( pBuffer ) ;
	} /* endmethod */
} ;

typedef std::list<_JumanResult> JUMAN_RESULTS ;

static void JumanAnalysis ( FILE *Destin, wchar_t *pText, unsigned cbText=0 ) {

	// If no length was given, assume the text to be null-terminated.
	if ( cbText == 0 )
		cbText = (unsigned) wcslen(pText) ;

	// Create an instance of the morphology analyzer.
	_Juman Juman ;

	// Try to analyze the text.  We get three things back:
	//  (1) Original text, grouped into words.
	//  (2) The sound of that text.
	//  (3) The part of speech code.
	//
	// If we get less original text back than we gave it, the analyzer
	//  has seen a character it doesn't know what to do with.  Pass such
	//  characters through to the output and restart the analyzer.
	//
	unsigned cb = cbText ; unsigned cbDone = 0 ;
	wchar_t Parsed [0x400] ; unsigned cbParsed = 0, cbSpaces = 0 ;
	_Juman::_PartOfSpeechCode PreviousPartOfSpeech = _Juman::POSC_MAX ;
	JUMAN_RESULTS JumanResults ;
	while ( cb > cbParsed ) {
		if ( Juman.Analyze ( pText+cbParsed ) ) {
			unsigned Count = Juman.GetResultCount ( ) ;
			for ( unsigned i=0; i<Count; i++ ) {
				wchar_t Buffer [0x400] ;
				if ( Juman.isAlternate ( i, Buffer, _countof(Buffer) ) == false ) {
					wchar_t *pSymbol = Juman.GetSymbol ( i, Buffer, _countof(Buffer) ) ;
					wchar_t *pSound = Juman.GetSound ( i, Buffer, _countof(Buffer) ) ;
					_Juman::_PartOfSpeechCode posc = Juman.GetPartOfSpeech ( i, Buffer, _countof(Buffer) ) ;

					// Did Juman choke?
					if ( ( (int)posc == 0 ) || ( pText[cbParsed] != pSymbol[0] ) ) {
						wchar_t szResult [2] = { pText[cbParsed], 0 } ;
						JumanResults.push_back ( _JumanResult ( szResult, szResult, _Juman::POSC_SPECIAL ) ) ;
						Parsed[cbParsed+cbSpaces] = pText[cbParsed] ; cbParsed ++ ;
						PreviousPartOfSpeech = _Juman::POSC_MAX ;
						break ;
					} /* endif */

					// Do we need to insert a leading space or dash?
					if ( PreviousPartOfSpeech != _Juman::POSC_MAX ) {
						if ( SeparationMatrix[PreviousPartOfSpeech-1][posc-1] ) {
							JumanResults.push_back ( _JumanResult ( L" ", L" ", _Juman::POSC_SPECIAL ) ) ;
							Parsed[cbParsed+cbSpaces] = L' ' ; cbSpaces ++ ;
						} else if ( posc != _Juman::POSC_SPECIAL ) {
							JumanResults.push_back ( _JumanResult ( L"-", L"-", _Juman::POSC_SPECIAL ) ) ;
							Parsed[cbParsed+cbSpaces] = L'-' ; cbSpaces ++ ;
						} else if ( ( pSymbol[0] == L'「' ) || ( pSymbol[0] == L'『' ) ) {
							JumanResults.push_back ( _JumanResult ( L" ", L" ", _Juman::POSC_SPECIAL ) ) ;
							Parsed[cbParsed+cbSpaces] = L' ' ; cbSpaces ++ ;
						} /* endif */
					} /* endif */

					// Store valid Juman result.
					JumanResults.push_back ( _JumanResult ( pSymbol, pSound, posc ) ) ;
					for ( unsigned i=0; pSymbol[i]; i++ ) {
						Parsed[cbParsed+cbSpaces] = pSymbol[i] ; 
						cbParsed ++ ;
					} /* endfor */

					// Insert trailing space if necessary.
					if ( ( pSymbol[0] == L'」' ) || ( pSymbol[0] == L'』' ) ) {
						JumanResults.push_back ( _JumanResult ( L" ", L" ", _Juman::POSC_SPECIAL ) ) ;
						Parsed[cbParsed+cbSpaces] = L' ' ; cbSpaces ++ ;
					} /* endif */

					// Remember the current part of speech.
					PreviousPartOfSpeech = posc ; 

				} else {
					DO_NOTHING ; // Alternate?  What do we want to do here?
				} /* endif */

			} /* endfor */

		// JUMAN choked.
		} else {
			wchar_t szResult [2] = { pText[cbParsed], 0 } ;
			JumanResults.push_back ( _JumanResult ( szResult, szResult, _Juman::POSC_SPECIAL ) ) ;
			Parsed[cbParsed+cbSpaces] = pText[cbParsed] ; cbParsed ++ ;
			PreviousPartOfSpeech = _Juman::POSC_MAX ;

		} /* endif */
	} /* endwhile */

	// Generate the result texts.
	cbParsed = 0 ;
	wchar_t Kana [0x400] ; unsigned cbKana = 0 ;
	wchar_t Romaji [0x400] ; unsigned cbRomaji = 0 ;
	PreviousPartOfSpeech = _Juman::POSC_MAX ;
	for (auto& JumanResult : JumanResults) {
		wchar_t Buffer[0x100];
		wchar_t* pSymbol = JumanResult.GetOriginal(Buffer, _countof(Buffer));
		while (*pSymbol)
			Parsed[cbParsed++] = *pSymbol++;
		wchar_t* pKana = JumanResult.GetKana(Buffer, _countof(Buffer));
		while (*pKana)
			Kana[cbKana++] = *pKana++;
		wchar_t* pRomaji = JumanResult.GetRomaji(Buffer, _countof(Buffer));
		while (*pRomaji)
			Romaji[cbRomaji++] = *pRomaji++;
#if 0 // Annotate parts of speech.
		_Juman::_PartOfSpeechCode posc = JumanResult.GetPartOfSpeech();
		if ((posc != _Juman::POSC_SPECIAL) && (posc != _Juman::POSC_UNDEFINED))
			cbParsed += swprintf(&Parsed[cbParsed], _countof(Parsed) - cbParsed, L"(%ls)", _Juman::GetPartOfSpeechName(posc));
#endif
	} /* endfor */
	Parsed[cbParsed] = 0 ;
	Kana[cbKana] = 0 ;
	Romaji[cbRomaji] = 0 ;

	// Output the results.
	fwprintf ( Destin, L"   JUMAN: %ls\n", Parsed ) ;
	fwprintf ( Destin, L"    Kana: %ls\n", Kana ) ;
	fwprintf ( Destin, L"  Romaji: %ls\n", Romaji ) ;

	// Generate a list of definitions.
	DEFINITIONS Definitions ;
	for (auto& JumanResult : JumanResults) {
		wchar_t Buffer[0x100];
		wchar_t* pSymbol = JumanResult.GetOriginal(Buffer, _countof(Buffer));
		_Juman::_PartOfSpeechCode posc = JumanResult.GetPartOfSpeech();
		switch (posc) {
		case _Juman::POSC_ADJECTIVE:
		case _Juman::POSC_ADVERB:
		case _Juman::POSC_CONJUNCTION:
		case _Juman::POSC_VERB:
		case _Juman::POSC_NOUN: {
			_IndexEntry2* pIndexEntry = Index2.FindFirstEntry(pSymbol, wcslen(pSymbol));
			if (pIndexEntry) {
				Definitions.push_back(pIndexEntry);
				do {
					_IndexEntry2* pIndexEntry2 = pIndexEntry + 1;
					if (pIndexEntry2 < Index2.End()) {
						if (wcscmp(pIndexEntry->pszWord, pIndexEntry2->pszWord))
							break;
						Definitions.push_back(pIndexEntry2);
					} /* endif */
					pIndexEntry = pIndexEntry2;
				} while (pIndexEntry < Index2.End());
			} /* endif */
			break; }
		} /* endswitch */
	} /* endfor */
	WriteDefinitions ( Destin, Definitions, 4 ) ;
} 

static bool ParseSentence ( FILE *Destin, wchar_t *pText, unsigned cbText, wchar_t* pKana, unsigned cbKana, wchar_t* pRomaji, unsigned cbRomaji, DEFINITIONS& Definitions, bool fJUMAN ) {

	// Remove any furigana from the working text.
	wchar_t Buffer [0x200] ; unsigned cbBuffer = 0 ; 
	bool fFurigana = false ; 
	for ( unsigned i=0; i<cbText; i++ ) {
		if ( pText[i] == L'《' ) {
			fFurigana = true ;
		} else if ( pText[i] == L'》' ) {
			fFurigana = false ;
		} else if ( !fFurigana ) {
			Buffer[cbBuffer++] = pText[i] ;
		} /* endif */
	} /* endwhile */
	Buffer[cbBuffer] = 0 ;

	// Trim any leading white-space.
	wchar_t *p = Buffer ;
	while ( ( p < Buffer+cbBuffer ) && isWhite ( *p ) ) 
		p ++ ;

	// Trim any trailing white-space.
	while ( ( cbBuffer > 1 ) && isWhite ( Buffer[cbBuffer-1] ) ) 
		cbBuffer -- ;

	// Reset the results.
	*pKana = 0 ;
	*pRomaji = 0 ;

	// If the line is now empty, exit.
	if ( p >= Buffer+cbBuffer ) 
		return ( false ) ;

	// Output the original text.
	fwprintf ( Destin, L"Original: %0.*ls\n", (unsigned)(cbBuffer-(p-Buffer)), p ) ;

	// Perform a JUMAN analysis.
	if ( fJUMAN )
		JumanAnalysis ( Destin, p, (unsigned)(cbBuffer-(p-Buffer)) ) ;

	// Loop through all text given.
	while ( p < Buffer+cbBuffer ) {

		// First, determine the maximum size of any dictionary lookup right now.
		// NOTE: Should we be considering roman characters and numerals as valid too?
		unsigned cbLeft = unsigned ( cbBuffer - ( p - Buffer ) ) ;
		assert ( cbLeft ) ;
		unsigned cbLookup ;
		for ( cbLookup=0; cbLookup<cbLeft; cbLookup++ ) {
			wchar_t Glyph = p [ cbLookup ] ;
			bool fBefore, fAfter ;
			if ( isBreak ( Glyph, fBefore, fAfter ) )
				break ;
			if ( isKatakana(Glyph) )
				continue ;
			if ( isHiragana(Glyph) )
				continue ;
			if ( isKanji(Glyph) )
				continue ;
			break ;
		} /* endfor */

		// If we got anything, try to build up a list of valid words from it.
		if ( cbLookup ) {
			SYMBOLS Symbols ;
			if ( BuildPhrase ( Symbols, p, cbLookup, pKana, cbKana, pRomaji, cbRomaji, Definitions ) ) {
				DO_NOTHING ;
			} else {
				fwprintf ( Destin, L"            WARNING: Unable to fully parse: %0.*ls\n", cbLookup, p ) ;
				ScanPhrase ( p, cbLookup, pKana, cbKana, pRomaji, cbRomaji, Definitions ) ;
			} /* endif */
		} /* endif */

		// Skip forward to the next block of characters.
		p += cbLookup ;
		while ( p < Buffer+cbBuffer ) {
			if ( isKatakana(*p) && ( *p != L'・' ) ) // The middle dot is technically katakana, but we want to treat it as a break.
				break ;
			if ( isHiragana(*p) )
				break ;
			if ( isKanji(*p) )
				break ;
			bool fBefore, fAfter ;
			bool fBreak = isBreak ( *p, fBefore, fAfter ) ;
			if ( fBreak && fBefore ) {
				*pKana++ = L' ' ; cbKana -- ;
				*pRomaji++ = L' ' ; cbRomaji -- ;
			} /* endif */
			*pKana++ = *p ; cbKana -- ;
			ConvertToRomaji ( p, 1, pRomaji, cbRomaji ) ;
			if ( fBreak && fAfter ) {
				*pKana++ = L' ' ; cbKana -- ;
				*pRomaji++ = L' ' ; cbRomaji -- ;
			} /* endif */
			*pKana = 0 ;
			*pRomaji = 0 ;
		} /* endwhile */

	} /* endwhile */

	// Null terminate the results.
	*pKana = 0 ;
	*pRomaji = 0 ;

	// Done OK.
	return ( true ) ;
}

//
// What indicates the logical end of a sentence within the physical line?
//  (1) The presence of a period, exclamation point, question mark or interrobang (!?).
//

inline int isSentenceBreak ( wchar_t *p ) {
	if ( ( p[0] == L'!' ) && ( p[1] == L'?' ) && ( p[2] == L'」' ) ) {
		if ( p[3] )
			return ( 3 ) ;
		return ( 0 ) ;
	} /* endif */
	if ( ( p[0] == L'？' ) && ( p[1] == L'」' ) ) {
		if ( p[2] )
			return ( 2 ) ;
		return ( 0 ) ;
	} /* endif */
	if ( ( p[0] == L'！' ) && ( p[1] == L'」' ) ) {
		if ( p[2] )
			return ( 2 ) ;
		return ( 0 ) ;
	} /* endif */
	if ( ( p[0] == L'!' ) && ( p[1] == L'?' ) ) {
		if ( p[2] )
			return ( 2 ) ;
		return ( 0 ) ;
	} /* endif */
	if ( ( p[0] == L'。' ) && p[1] )
		return ( 1 ) ;
	if ( ( p[0] == L'！' ) && p[1] )
		return ( 1 ) ;
	if ( ( p[0] == L'？' ) && p[1] )
		return ( 1 ) ;
	return ( 0 ) ;
}

static void WriteLine ( FILE *Destin, wchar_t *Line, size_t& cbLine, unsigned& nLines, bool fJUMAN ) {

	// If the line starts with one blank, followed by non-blank, 
	//  replace that blank with a paragraph start indicator.
	if ( cbLine > 3 ) {
		if ( ( Line[0] == 0x20 ) && ( Line[1] != 0x20 ) ) {
			Line[0] = 0x00B6 ;
		} /* endif */
	} /* endif */

	// Null terminate the line.
	Line[cbLine] = 0 ;

	// Output and parse the text.
	fwprintf ( Destin, L"Line %i:\n", nLines ) ;
	wchar_t Kana [0x1000], Romaji [0x1000] ;
	wchar_t *pLine = Line ; wchar_t *pStart = pLine ;
	while ( *pLine ) {
		int cbBreak = isSentenceBreak ( pLine ) ;
		if (cbBreak) {
			DEFINITIONS Definitions;
			if (ParseSentence(Destin, pStart, unsigned(pLine - pStart + cbBreak), Kana, _countof(Kana), Romaji, _countof(Romaji), Definitions, fJUMAN)) {
				fwprintf(Destin, L"    Kana: %ls\n", Kana);
				fwprintf(Destin, L"  Romaji: %ls\n", Romaji);
				WriteDefinitions(Destin, Definitions, 4);
			} /* endif */
			pLine += cbBreak ;
			pStart = pLine ;
		} else {
			++ pLine ;
		} /* endif */
	} /* endwhile */
	if ( pLine > pStart ) {
		DEFINITIONS Definitions ;
		if ( ParseSentence ( Destin, pStart, unsigned(pLine-pStart), Kana, _countof(Kana), Romaji, _countof(Romaji), Definitions, fJUMAN ) ) {
			fwprintf ( Destin, L"    Kana: %ls\n", Kana ) ;
			fwprintf ( Destin, L"  Romaji: %ls\n", Romaji ) ;
			WriteDefinitions ( Destin, Definitions, 4 ) ;
		} /* endif */
	} /* endif */

	// Output a blank line.
	fwprintf ( Destin, L"\n" ) ;

	// Reset line buffer.
	cbLine = 0 ;

	// Increment line count.
	nLines ++ ;
	printf ( "  %i lines processed.\r", nLines ) ;
}

static wchar_t *CSV_GetFieldValue ( wchar_t *pBuffer, wchar_t* &pNext ) {

	// Trim leading blanks.
	while ( pBuffer[0] == L' ' )
		++ pBuffer ;

	// If the value starts quoted, it will be delimeted by quotes.
	if ( pBuffer[0] == L'\"' ) {
		wchar_t *p = wcschr ( pBuffer+1, L'\"' ) ;
		if ( !p )
			return ( NULL ) ;
		p[0] = 0 ;
		pNext = p + 1 ;
		pNext = pNext + wcsspn ( pNext, L", " ) ;
		return ( pBuffer + 1 ) ;

	// Otherwise, it will delimeted by a comma or a carriage return.
	} else {
		size_t i = wcscspn ( pBuffer, L",\r" ) ;
		pBuffer[i] = 0 ;
		pNext = pBuffer + i ;
		pNext = pNext + wcsspn ( pNext, L", " ) ;
		return ( pBuffer == pNext ? NULL : pBuffer ) ;

	} /* endif */
}

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

#include <dbghelp.h>
#pragma comment ( lib, "dbghelp.lib" )

BOOL CALLBACK MyMiniDumpCallback (
    PVOID                            pParam, 
    const PMINIDUMP_CALLBACK_INPUT   pInput, 
    PMINIDUMP_CALLBACK_OUTPUT        pOutput 
) {
    // Callback implementation 
    return ( TRUE ) ;
}

static void CreateMiniDump ( EXCEPTION_POINTERS* pep ) {
  // Open the file 
  HANDLE hFile = CreateFile ( L"Nihongo.dmp", GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL ) ;
  if ( ( hFile != NULL ) && ( hFile != INVALID_HANDLE_VALUE ) ) {

    // Create the minidump 
    MINIDUMP_EXCEPTION_INFORMATION mdei ;
    mdei.ThreadId           = GetCurrentThreadId ( ) ;
    mdei.ExceptionPointers  = pep ;
    mdei.ClientPointers     = FALSE ;
    MINIDUMP_CALLBACK_INFORMATION mci ; 
    mci.CallbackRoutine     = (MINIDUMP_CALLBACK_ROUTINE) MyMiniDumpCallback ;
    mci.CallbackParam       = 0 ; // this example does not use the context
    MINIDUMP_TYPE mdt       = MiniDumpNormal ;
    BOOL rv = MiniDumpWriteDump ( GetCurrentProcess(), GetCurrentProcessId(),
      hFile, mdt, (pep != 0) ? &mdei : 0, 0, &mci ) ;
    if ( !rv ) 
      printf ( "MiniDumpWriteDump failed. Error: %u \n", GetLastError() ) ;
    else 
      printf ( "Minidump created.\n" ) ;

    // Close the file 
    CloseHandle ( hFile ) ; 

  } else {
     printf ( "CreateFile failed. Error: %u \n", GetLastError() ) ; 
  } /* endif */
}

int _tmain ( int argc, wchar_t* argv[] ) {

    // Set up console interrupt handler.
    SetConsoleCtrlHandler ( handler_routine, TRUE ) ;
    signal ( SIGINT, SignalHandler ) ;
    
	// Parse the command line.
	wchar_t szSource [_MAX_PATH] = { 0 } ;
	wchar_t szDestin [_MAX_PATH] = { 0 } ;
	unsigned StartLine ( 0xFFFFFFFF ) ;
	unsigned EndLine ( 0xFFFFFFFF ) ;
	bool fRegenerateDictionary ( false ) ;
	bool fJUMAN ( false ) ;
	bool fDump ( false ) ;
	while ( --argc ) {
		argv ++ ;
		if ( !wcscmp(argv[0],L"?") || !wcscmp(argv[0],L"/?") || !wcscmp(argv[0],L"-?") ) {
			printf ( "NIHONGO: Japanese Text Parser\n"
				"\n"
				"Syntax: NIHONGO sourcefile destinfile [startline [endline]]\n"
				"\n"
				"Where:\n"
				"  'sourcefile' specifies the file to be parsed.\n"
				"  'destinfile' specifies the file to be output.\n"
				"  startline    specifies the input line at which to start (numbering from zero)\n"
				"  endline      specifies the input line at which to stop.\n"
				"  /REGEN       indicates the dictionary is to be rebuilt.\n"
				"  /JUMAN       adds the JUMAN analysis to the output.\n"
				"  /DUMP        dumps new dictionary and index to text files.\n"
				"\n"
			) ;
			return ( 0 ) ;
		} /* endif */
		if ( !_wcsicmp ( argv[0], L"/regen" ) ) {
			fRegenerateDictionary = true ;
			continue ;
		} else if ( !_wcsicmp ( argv[0], L"/juman" ) ) {
			fJUMAN = true ;
			continue ;
		} else if ( !_wcsicmp ( argv[0], L"/dump") ) {
			fDump = true;
			continue;
		} else if ( szSource[0] == 0 ) {
			WIN32_FILE_ATTRIBUTE_DATA Attributes ;
			if ( !GetFileAttributesEx ( argv[0], GetFileExInfoStandard, &Attributes ) ) {
				fwprintf ( stderr, L"ERROR: File '%ls' not found.\n", argv[0] ) ;
				return ( 1 ) ;
			} /* endif */
			if ( Attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
				fwprintf ( stderr, L"ERROR: File '%ls' is a directory.\n", argv[0] ) ;
				return ( 1 ) ;
			} /* endif */
			wcscpy_s ( szSource, _countof(szSource), argv[0] ) ;
			continue ;
		} else if ( szDestin[0] == 0 ) {
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
			wcscpy_s ( szDestin, _countof(szSource), argv[0] ) ;
			continue ;
		} else if ( StartLine == 0xFFFFFFFF ) {
			if ( swscanf_s ( argv[0], L"%u", &StartLine ) != 1 ) {
				fwprintf ( stderr, L"ERROR: Starting line number '%ls' invalid.\n", argv[0] ) ;
				return ( 1 ) ;
			} /* endif */
			continue ;
		} else if ( EndLine == 0xFFFFFFFF ) {
			if ( swscanf_s ( argv[0], L"%u", &EndLine ) != 1 ) {
				fwprintf ( stderr, L"ERROR: Ending line number '%ls' invalid.\n", argv[0] ) ;
				return ( 1 ) ;
			} /* endif */
			continue ;
		} /* endif */
		fwprintf ( stderr, L"ERROR: Unrecognized parameter '%ls'.\n", argv[0] ) ;
		return ( 1 ) ;
	} /* endwhile */
	if ( fRegenerateDictionary && ( szSource[0] == 0 ) ) {
		; // Do nothing here.
	} else if ( szSource[0] == 0 ) {
		fprintf ( stderr, "ERROR: No source file specified.\n" ) ;
		return ( 1 ) ;
	} else if ( szDestin[0] == 0 ) {
		fprintf ( stderr, "ERROR: No destination file specified.\n" ) ;
		return ( 1 ) ;
	} /* endif */
	if ( ( StartLine == 0xFFFFFFFF ) && ( EndLine = 0xFFFFFFFF ) ) {
		StartLine = 0 ; 
	} else if ( EndLine == 0xFFFFFFFF ) {
		EndLine = StartLine ;
	} else if ( EndLine < StartLine ) {
		fprintf ( stderr, "ERROR: Cannot end before the start...\n" ) ;
		return ( 1 ) ;
	} /* endif */

	// Protect everything below here.
	__try {

		// If we are regenerating the dictionary, or we cannot load an old copy...
		if ( fRegenerateDictionary || !Index2.Restore ( ) ) {

			// Load the dictionary.
			printf ( "Building dictionary file from EDICT.\n" ) ;
			if ( !BuildDictionary ( L"JDICT\\EDICT", true ) ) 
				return ( 1 ) ;
			printf ( "  %zi dictionary entries.\n", Dictionary.size() ) ;

			// Load the dictionary extensions.  File must be UNICODE.
			printf ( "Loading additional words.\n" ) ;
			FILE *AddedWords ; errno_t Error = _wfopen_s ( &AddedWords, L"AddedWords.txt", L"rb" ) ;
			if ( Error ) {
				fprintf ( stderr, "ERROR: Unable to open file 'AddedWords.txt'.\n" ) ;
			} else {
				// Validate the code type tag.  Must be UNICODE.
				unsigned short Tag ;
				if ( ( fread ( &Tag, sizeof(Tag), 1, AddedWords ) < 1 ) || ( Tag != 0xFEFF ) ) {
					fprintf ( stderr, "ERROR: Invalid file 'AddedWords.txt'.  Not unicode-encoded.\n" ) ;
				} else {
					// Now that we've validated the BOM, let's switch to text mode.
					_set_fmode ( _O_TEXT ) ;
					// Load the file now.
					wchar_t Buffer [1024] ; bool fFirst = true ;
					while ( fgetws ( Buffer, _countof(Buffer), AddedWords ) ) {
						if ( fFirst ) {
							fFirst = false ;
						} else {
							wchar_t *pNext ;
							wchar_t *pSymbol = CSV_GetFieldValue ( Buffer, pNext ) ;
							if ( pSymbol ) {
								wchar_t *pSound = CSV_GetFieldValue ( pNext, pNext ) ;
								if ( pSound ) {
									wchar_t *pInfo = CSV_GetFieldValue ( pNext, pNext ) ;
									if ( pInfo ) {
										wchar_t *pConjugate = CSV_GetFieldValue ( pNext, pNext ) ;
										if ( pConjugate && !_wcsicmp ( pConjugate, L"true" ) ) {
											AddWord ( pSymbol, pSound, pInfo, true ) ;
										} else {
											AddWord ( pSymbol, pSound, pInfo ) ;
										} /* endif */
									} /* endif */
								} /* endif */
							} /* endif */
						} /* endif */
					} /* endwhile */
				} /* endif */
				fclose ( AddedWords ) ;
			} /* endif */

			// Dump the dictionary if requested.
			if ( fDump ) {
				printf ( "Dumping dictionary to text file.\n" ) ;
				DumpDictionary ( ) ;
			} /* endif */

			// Build the index.
			printf ( "Building word/phrase index.\n" ) ;
			if ( BuildIndex() == false )
				return ( 1 ) ;
			printf ( "  %zi index entries.\n", Index.size() ) ;

			// Load new dictionary.
			Index2.Load ( Index ) ;

			// If we aren't processing a file, just exit now.
			if ( szSource[0] == 0 )
				return ( 0 ) ;

		} /* endif */

		// Some special lookups.
		pParticleDE = Index2.FindParticle ( L"で", 1 ) ;
		pParticleGA = Index2.FindParticle ( L"が", 1 ) ;
		pParticleKA = Index2.FindParticle ( L"か", 1 ) ;
		pParticleMO = Index2.FindParticle ( L"も", 1 ) ;
		pParticleNI = Index2.FindParticle ( L"に", 1 ) ;
		pParticleNO = Index2.FindParticle ( L"の", 1 ) ;
		pParticleTO = Index2.FindParticle ( L"と", 1 ) ;
		pParticleWA = Index2.FindParticle ( L"は", 1 ) ;
		pParticleWO = Index2.FindParticle ( L"を", 1 ) ;
		pHonorificO = Index2.FindHonorificPrefix ( L"お", 1 ) ;
		pHonorificGO = Index2.FindHonorificPrefix ( L"ご", 1 ) ;

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
		FILE *Destin ; Error = _tfopen_s ( &Destin, szDestin, L"wt,ccs=UNICODE" ) ;
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
			if ( ( cbLine >= 2 ) && ( Line[cbLine-2] == 0x0D ) && ( Line[cbLine-1] == 0x0A ) ) {
				cbLine -= 2 ;

				// If the line is blank, skip it.
				if ( !cbLine || ( ( cbLine == 2 ) && ( Line[0] == 0x0D ) && ( Line[1] == 0x0A ) ) ) {
					cbLine = 0 ;
					++ nLines ;

				// If we're outside the requested range, skip it.
				} else if ( ( nLines < StartLine ) || ( nLines > EndLine ) ) {
					cbLine = 0 ;
					++ nLines ;

				// Otherwise, do it.
				} else {
					WriteLine ( Destin, Line, cbLine, nLines, fJUMAN ) ;

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
			if ( ( Line[cbLine-2] == 0x0D ) && ( Line[cbLine-1] == 0x0A ) )
				cbLine -= 2 ;

			// If the line is blank, skip it.
			if ( !cbLine || ( ( cbLine == 2 ) && ( Line[0] == 0x0D ) && ( Line[1] == 0x0A ) ) ) {
				cbLine = 0 ;
				++ nLines ;

			// If we're outside the requested range, skip it.
			} else if ( ( nLines < StartLine ) || ( nLines > EndLine ) ) {
				cbLine = 0 ;
				++ nLines ;

			// Otherwise, do it.
			} else {
				WriteLine ( Destin, Line, cbLine, nLines, fJUMAN ) ;

			} /* endif */

		} /* endif */

		// Clean up.
		fclose ( Destin ) ;
		fclose ( Source ) ;

		// Done OK.
		printf ( "Done.  %i lines processed.\n", nLines ) ;
	} __except ( CreateMiniDump ( GetExceptionInformation() ), EXCEPTION_EXECUTE_HANDLER ) {
		;
	} 

	// Exit without cleanup.
	ExitProcess ( 0 ) ;
}
