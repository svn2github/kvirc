//=============================================================================
//
//   File : kvi_locale.cpp
//   Creation date : Fri Mar 19 1999 19:08:41 by Szymon Stefanek
//
//   This file is part of the KVirc irc client distribution
//   Copyright (C) 1999-2002 Szymon Stefanek (pragma at kvirc dot net)
//
//   This program is FREE software. You can redistribute it and/or
//   modify it under the terms of the GNU General Public License
//   as published by the Free Software Foundation; either version 2
//   of the License, or (at your opinion) any later version.
//
//   This program is distributed in the HOPE that it will be USEFUL,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//   See the GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program. If not, write to the Free Software Foundation,
//   Inc. ,59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//
//=============================================================================

#define __KVILIB__


//#define _KVI_DEBUG_CHECK_RANGE_
#include "kvi_debug.h"
#include "kvi_malloc.h"
#include "kvi_bswap.h"

#define _KVI_LOCALE_CPP_
#include "kvi_locale.h"

#include <qglobal.h> //for debug()
#include <qasciidict.h>
#include <qtextcodec.h>
#include <qdir.h>

#include "kvi_string.h"
#include "kvi_env.h"
#include "kvi_fileutils.h"


static KviStr                            g_szLang;
static KviTranslator                   * g_pTranslator          = 0;
KVILIB_API KviMessageCatalogue         * g_pMainCatalogue       = 0;
static QAsciiDict<KviMessageCatalogue> * g_pCatalogueDict       = 0;
static QTextCodec                      * g_pUtf8TextCodec       = 0;




// Code point          1st byte    2nd byte    3rd byte    4th byte
// ----------          --------    --------    --------    --------
// U+0000..U+007F      00..7F
// U+0080..U+07FF      C2..DF      80..BF
// U+0800..U+0FFF      E0          A0..BF      80..BF
// U+1000..U+FFFF      E1..EF      80..BF      80..BF
// U+10000..U+3FFFF    F0          90..BF      80..BF      80..BF
// U+40000..U+FFFFF    F1..F3      80..BF      80..BF      80..BF
// U+100000..U+10FFFF  F4          80..8F      80..BF      80..BF
 
// The definition of UTF-8 in Annex D of ISO/IEC 10646-1:2000 also
// allows for the use of five- and six-byte sequences to encode
// characters that are outside the range of the Unicode character
// set; those five- and six-byte sequences are illegal for the use
// of UTF-8 as a transformation of Unicode characters. ISO/IEC 10646
// does not allow mapping of unpaired surrogates, nor U+FFFE and U+FFFF
// (but it does allow other noncharacters).

// At the moment we support only 2 byte unicode and thus 5 and 6 byte
// sequences are treated as NON valid.

static bool may_be_utf8(const unsigned char * p)
{
	while(*p)
	{
		if(*p < 0x80)
		{
			p++;
		} else if((*p & 0xe0) == 0xc0)
		{
			// 2 bytes encoding
			p++;
			if(*p < 0x80)return false; // error
			p++;
		} else if((*p & 0xf0) == 0xe0)
		{
			// 3 bytes encoding
			p++;
			if(*p < 0x80)return false; // error
			p++;
			if(*p < 0x80)return false; // error
			p++;
		} else if((*p & 0xf8) == 0xf0)
		{
			// 4 bytes encoding
			p++;
			if(*p < 0x80)return false; // error
			p++;
			if(*p < 0x80)return false; // error
			p++;
			if(*p < 0x80)return false; // error
			p++;
		} else {
			// treat as non valid
			return false;
		}
	}
	return true;
}

static bool may_be_utf8(const unsigned char * p,int len)
{
	while(*p && (len > 0))
	{
		if(*p < 0x80)
		{
			p++;
			len--;
		} else if((*p & 0xe0) == 0xc0)
		{
			// 2 bytes encoding
			p++; len--;
			if((*p < 0x80) || (!len))return false; // error
			p++; len--;
		} else if((*p & 0xf0) == 0xe0)
		{
			// 3 bytes encoding
			p++; len--;
			if((*p < 0x80) || (!len))return false; // error
			p++; len--;
			if((*p < 0x80) || (!len))return false; // error
			p++; len--;
		} else if((*p & 0xf8) == 0xf0)
		{
			// 4 bytes encoding
			p++; len--;
			if((*p < 0x80) || (!len))return false; // error
			p++; len--;
			if((*p < 0x80) || (!len))return false; // error
			p++; len--;
			if((*p < 0x80) || (!len))return false; // error
			p++; len--;
		} else {
			// treat as non valid
			return false;
		}
	}
	return true;
}



class KviSmartTextCodec : public QTextCodec
{
protected:
	KviStr       m_szName;
	QTextCodec * m_pChildCodec;
public:
	KviSmartTextCodec(const char * szName)
	: QTextCodec()
	{
		m_szName = szName;
		KviStr tmp = m_szName;
		tmp.cutToFirst('/');
		if(!g_pUtf8TextCodec)
		{
			g_pUtf8TextCodec = QTextCodec::codecForName("UTF-8");
			if(!g_pUtf8TextCodec)
			{
				debug("Can't find the global utf8 text codec!");
				g_pUtf8TextCodec = QTextCodec::codecForLocale(); // try anything else...
			}
		}
		m_pChildCodec = QTextCodec::codecForName(tmp.ptr());
		if(!m_pChildCodec)
		{
			debug("Can't find the codec for name %s (composite codec creation)",tmp.ptr());
			m_pChildCodec = g_pUtf8TextCodec;
		}
	}
public:
	bool ok(){ return m_pChildCodec && g_pUtf8TextCodec; };
	virtual const char * name () const { return m_szName.ptr(); };
	virtual const char * mimeName () const { return m_pChildCodec->mimeName(); };
	virtual int mibEnum () const { return 0; };
	virtual QTextDecoder * makeDecoder () const { return m_pChildCodec->makeDecoder(); };
	virtual QTextEncoder * makeEncoder () const { return m_pChildCodec->makeEncoder(); };
	QCString fromUnicode ( const QString & uc ) const { return m_pChildCodec->fromUnicode(uc); };
	virtual QCString fromUnicode ( const QString & uc, int & lenInOut ) const { return m_pChildCodec->fromUnicode(uc,lenInOut); };
	QString toUnicode(const char * chars) const
	{
		if(may_be_utf8((const unsigned char *)chars))return g_pUtf8TextCodec->toUnicode(chars);
		return m_pChildCodec->toUnicode(chars);
	};
	virtual QString toUnicode(const char * chars,int len) const
	{
		if(may_be_utf8((const unsigned char *)chars,len))return g_pUtf8TextCodec->toUnicode(chars,len);
		return m_pChildCodec->toUnicode(chars,len);
	};
	QString toUnicode(const QByteArray & a,int len) const
	{
		if(may_be_utf8((const unsigned char *)(a.data()),len))return g_pUtf8TextCodec->toUnicode(a,len);
		return m_pChildCodec->toUnicode(a,len);
	};
	QString toUnicode(const QByteArray & a) const
	{
		if(may_be_utf8((const unsigned char *)(a.data()),a.size()))return g_pUtf8TextCodec->toUnicode(a);
		return m_pChildCodec->toUnicode(a);
	};
	QString toUnicode(const QCString & a,int len) const
	{
		if(may_be_utf8((const unsigned char *)(a.data()),len))return g_pUtf8TextCodec->toUnicode(a,len);
		return m_pChildCodec->toUnicode(a,len);
	};
	QString toUnicode(const QCString & a) const
	{
		if(may_be_utf8((const unsigned char *)(a.data())))return g_pUtf8TextCodec->toUnicode(a);
		return m_pChildCodec->toUnicode(a);
	};
	virtual bool canEncode(QChar ch) const { return m_pChildCodec->canEncode(ch); };
	virtual bool canEncode(const QString &s) const { return m_pChildCodec->canEncode(s); };
	virtual int heuristicContentMatch(const char * chars,int len) const
	{
		int iii = g_pUtf8TextCodec->heuristicContentMatch(chars,len);
		if(iii < 0)return m_pChildCodec->heuristicContentMatch(chars,len);
		return iii;
	}
	virtual int heuristicNameMatch(const char * hint) const { return 0; };
};

static QAsciiDict<KviSmartTextCodec>   * g_pSmartCodecDict      = 0;



/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   The following code was extracted and adapted from gettext.h and gettextP.h
//   from the GNU gettext package.
//   
//   Internal header for GNU gettext internationalization functions.
//   Copyright (C) 1995, 1997 Free Software Foundation, Inc.
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2, or (at your option)
//   any later version.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU Library General Public
//   License along with the GNU C Library; see the file COPYING.LIB.  If not,
//   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
//   Boston, MA 02111-1307, USA. 
//
/////////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>

#if HAVE_LIMITS_H || _LIBC
	#include <limits.h>
#endif

// The magic number of the GNU message catalog format.
#define KVI_LOCALE_MAGIC 0x950412de
#define KVI_LOCALE_MAGIC_SWAPPED 0xde120495

// Revision number of the currently used .mo (binary) file format.
#define MO_REVISION_NUMBER 0


// Header for binary .mo file format.
struct GnuMoFileHeader
{
	// The magic number.
	Q_UINT32 magic;
	// The revision number of the file format.
	Q_UINT32 revision;
	// The number of strings pairs.
	Q_UINT32 nstrings;
	// Offset of table with start offsets of original strings.
	Q_UINT32 orig_tab_offset;
	// Offset of table with start offsets of translation strings.
	Q_UINT32 trans_tab_offset;
	// Size of hashing table.
	Q_UINT32 hash_tab_size;
	// Offset of first hashing entry.
	Q_UINT32 hash_tab_offset;
};

struct GnuMoStringDescriptor
{
	// Length of addressed string.
	Q_UINT32 length;
	// Offset of string in file.
	Q_UINT32 offset;
};

#define KVI_SWAP_IF_NEEDED(flag,value) (flag ? kvi_swap32(value) : (value))

///////////////////////////////////////////////////////////////////////////////////////////////
//   End of gettext.h & gettextP.h
///////////////////////////////////////////////////////////////////////////////////////////////


// HELPERS

static int somePrimeNumbers[90]=
{
	257 , 521 , 769 , 1031, 1087, 1091, 1103, 1117, 1123, 1151, // Incomplete *.mo files
	1163, 1171, 1181, 1193, 1201, 1213, 1217, 1223, 1229, 1231, // Complete *.mo files
	1237, 1249, 1259, 1277, 1283, 1289, 1291, 1297, 1307, 1319,
	1321, 1327, 1361, 1367, 1373, 1381, 1399, 1409, 1423, 1433,
	1447, 1459, 1471, 1481, 1493, 1511, 1523, 1531, 1543, 1553,
	1567, 1571, 1583, 1597, 1609, 1619, 1627, 1637, 1657, 1667, // Too big for KVIrc *.mo files
	1693, 1709, 1721, 1733, 1741, 1753, 1777, 1789, 1811, 1831,
	1907, 2069, 2111, 2221, 2309, 2441, 2531, 2617, 2731, 2837,
	2903, 3121, 3329, 3331, 3767, 4127, 5051, 6089, 7039, 9973
};

int kvi_getFirstBiggerPrime(int number)
{
	for(int i=0;i<90;i++){
		if(somePrimeNumbers[i] >= number)return somePrimeNumbers[i];
	}
	return 9973; //error!
}


KviMessageCatalogue::KviMessageCatalogue()
{
	//m_uEncoding = 0;
	m_pTextCodec = QTextCodec::codecForLocale();

	m_pMessages = new QAsciiDict<KviTranslationEntry>(1123,true,false); // dictSize, case sensitive , don't copy keys
	m_pMessages->setAutoDelete(true);
}

KviMessageCatalogue::~KviMessageCatalogue()
{
	delete m_pMessages;
}

bool KviMessageCatalogue::load(const QString& name)
{
	QString szCatalogueFile(name);

	// Try to load the header
	QFile f(szCatalogueFile);
	if(!f.open(IO_ReadOnly))
	{
		debug("[KviLocale]: Failed to open the messages file %s: probably doesn't exist",szCatalogueFile.utf8().data());
		return false;
	}

	GnuMoFileHeader hdr;

	if(f.readBlock((char *)&hdr,sizeof(GnuMoFileHeader)) < (int)sizeof(GnuMoFileHeader))
	{
		debug("KviLocale: Failed to read header of %s",szCatalogueFile.utf8().data());
		f.close();
		return false;
	}

	bool bMustSwap = false;

	if(hdr.magic != KVI_LOCALE_MAGIC)
	{
		if(hdr.magic == KVI_LOCALE_MAGIC_SWAPPED)
		{
			debug("KviLocale: Swapped magic for file %s: swapping data too",szCatalogueFile.utf8().data());
			bMustSwap = true;
		} else {
			debug("KviLocale: Bad locale magic for file %s: not a *.mo file ?",szCatalogueFile.utf8().data());
			f.close();
			return false;
		}
	}

	if(KVI_SWAP_IF_NEEDED(bMustSwap,hdr.revision) != MO_REVISION_NUMBER)
	{
		debug("KviLocale: Invalid *.mo file revision number for file %s",szCatalogueFile.utf8().data());
		f.close();
		return false;
	}

	int numberOfStrings = KVI_SWAP_IF_NEEDED(bMustSwap,hdr.nstrings);

	if(numberOfStrings <= 0)
	{
		debug("KviLocale: No translated messages found in file %s",szCatalogueFile.utf8().data());
		f.close();
		return false;
	}

	if(numberOfStrings >= 9972)
	{
		debug("Number of strings too big...sure that it is a KVIrc catalog file ?");
		numberOfStrings = 9972;
	}

	// return back
	f.at(0);

	unsigned int fSize = f.size();
	char * buffer = (char *)kvi_malloc(fSize);

	// FIXME: maybe read it in blocks eh ?
	if(f.readBlock(buffer,fSize) < (int)fSize)
	{
		debug("KviLocale: Error while reading the translation file %s",szCatalogueFile.utf8().data());
		kvi_free(buffer);
		f.close();
		return false;
	}

	// Check for broken *.mo files
	if(fSize < (24 + (sizeof(GnuMoStringDescriptor) * numberOfStrings)))
	{
		debug("KviLocale: Broken translation file %s (too small for all descriptors)",szCatalogueFile.utf8().data());
		kvi_free(buffer);
		f.close();
		return false;
	}

	GnuMoStringDescriptor * origDescriptor  = (GnuMoStringDescriptor *)(buffer + KVI_SWAP_IF_NEEDED(bMustSwap,hdr.orig_tab_offset));
	GnuMoStringDescriptor * transDescriptor = (GnuMoStringDescriptor *)(buffer + KVI_SWAP_IF_NEEDED(bMustSwap,hdr.trans_tab_offset));

	// Check again for broken *.mo files
	int expectedFileSize = KVI_SWAP_IF_NEEDED(bMustSwap,transDescriptor[numberOfStrings - 1].offset) +
							KVI_SWAP_IF_NEEDED(bMustSwap,transDescriptor[numberOfStrings - 1].length);

	if(fSize < (unsigned int)expectedFileSize)
	{
		debug("KviLocale: Broken translation file %s (too small for all the message strings)",szCatalogueFile.utf8().data());
		kvi_free(buffer);
		f.close();
		return false;
	}

	// Ok...we can run now

	int dictSize = kvi_getFirstBiggerPrime(numberOfStrings);
	m_pMessages->resize(dictSize);

	KviStr szHeader;

	for(int i=0;i < numberOfStrings;i++)
	{
		// FIXME: "Check for NULL inside strings here ?"
		//debug("original seems to be at %u and %u byttes long",KVI_SWAP_IF_NEEDED(bMustSwap,origDescriptor[i].offset),
		//	KVI_SWAP_IF_NEEDED(bMustSwap,origDescriptor[i].length));
		//debug("translated seems to be at %u and %u byttes long",KVI_SWAP_IF_NEEDED(bMustSwap,transDescriptor[i].offset),
		//	KVI_SWAP_IF_NEEDED(bMustSwap,transDescriptor[i].length));

		KviTranslationEntry * e = new KviTranslationEntry(
			(char *)(buffer + KVI_SWAP_IF_NEEDED(bMustSwap,origDescriptor[i].offset)),
			KVI_SWAP_IF_NEEDED(bMustSwap,origDescriptor[i].length),
			(char *)(buffer + KVI_SWAP_IF_NEEDED(bMustSwap,transDescriptor[i].offset)),
			KVI_SWAP_IF_NEEDED(bMustSwap,transDescriptor[i].length));

		// In some (or all?) *.mo files the first string
		// is zero bytes long and the translated one contains
		// informations about the translation
		if(e->m_szKey.len() == 0)
		{
			szHeader = e->m_szEncodedTranslation;
			delete e;
			continue;
		}

		m_pMessages->insert(e->m_szKey.ptr(),e);
	}

	kvi_free(buffer);
	f.close();

	m_pTextCodec = 0;

	// find out the text encoding , if possible
	if(szHeader.hasData())
	{
		// find "charset=*\n"
		int idx = szHeader.findFirstIdx("charset=");
		if(idx != -1)
		{
			szHeader.cutLeft(idx + 8);
			szHeader.cutFromFirst('\n');
			szHeader.stripWhiteSpace();
			m_pTextCodec = KviLocale::codecForName(szHeader.ptr());
			if(!m_pTextCodec)
			{
				debug("Can't find the codec for charset=%s",szHeader.ptr());
				debug("Falling back to codecForLocale()");
				m_pTextCodec = QTextCodec::codecForLocale();
			}
		}
	}

	if(!m_pTextCodec)
	{
		debug("The message catalogue does not have a \"charset\" header");
		debug("Assuming utf8"); // FIXME: or codecForLocale() ?
		m_pTextCodec = QTextCodec::codecForName("UTF-8");
	}

	return true;
}

const char * KviMessageCatalogue::translate(const char *text)
{
	KviTranslationEntry * aux = m_pMessages->find(text);
	if(aux)return aux->m_szEncodedTranslation.ptr();
	return text;
}

const QString & KviMessageCatalogue::translateToQString(const char *text)
{
	KviTranslationEntry * aux = m_pMessages->find(text);
	if(aux)
	{
		if(aux->m_pQTranslation)return *(aux->m_pQTranslation);
		aux->m_pQTranslation = new QString(m_pTextCodec->toUnicode(aux->m_szEncodedTranslation.ptr()));
		return *(aux->m_pQTranslation);
	}
	// no translation is available: let's avoid continous string decoding
	aux = new KviTranslationEntry(text);
	m_pMessages->insert(aux->m_szKey.ptr(),aux);
	aux->m_pQTranslation = new QString(m_pTextCodec->toUnicode(aux->m_szEncodedTranslation.ptr()));
	return *(aux->m_pQTranslation);
}




namespace KviLocale
{
#ifndef QT_NO_BIG_CODECS
	#define NUM_ENCODINGS 73
#else
	#define NUM_ENCODINGS 57
#endif



	static EncodingDescription supported_encodings[]=
	{
		{ "UTF-8"                , 0 , "8-bit Unicode" },
		{ "ISO-8859-1"           , 0 , "Western, Latin-1" },
		{ "ISO-8859-2"           , 0 , "Central European 1" },
		{ "ISO-8859-3"           , 0 , "Central European 2" },
		{ "ISO-8859-4"           , 0 , "Baltic, Standard" },
		{ "ISO-8859-5"           , 0 , "Cyrillic, ISO" },
		{ "ISO-8859-6"           , 0 , "Arabic, Standard" },
		{ "ISO-8859-7"           , 0 , "Greek" },
		{ "ISO-8859-8"           , 0 , "Hebrew, visually ordered" },
		{ "ISO-8859-8-i"         , 0 , "Hebrew, logically ordered" },
		{ "ISO-8859-9"           , 0 , "Turkish, Latin-5" },
		{ "ISO-8859-15"          , 0 , "Western, Latin-1 + Euro" },
		{ "KOI8-R"               , 0 , "Cyrillic, KOI" },
		{ "KOI8-U"               , 0 , "Ukrainian" },
		{ "CP-1250"              , 0 , "Central European 3" },
		{ "CP-1251"              , 0 , "Cyrillic, Windows" },
		{ "CP-1252"              , 0 , "Western, CP" },
		{ "CP-1253"              , 0 , "Greek, CP" },
		{ "CP-1256"              , 0 , "Arabic, CP" },
		{ "CP-1257"              , 0 , "Baltic, CP" },
		{ "CP-1255"              , 0 , "Hebrew, CP" },
		{ "CP-1254"              , 0 , "Turkish, CP" },
		{ "TIS-620"              , 0 , "Thai" },
#ifndef QT_NO_BIG_CODECS
		{ "Big5"                 , 0 , "Chinese Traditional" },
		{ "Big5-HKSCS"           , 0 , "Chinese Traditional, Hong Kong" },
		{ "GB18030"              , 0 , "Chinese Simplified" },
		{ "JIS7"                 , 0 , "Japanese (JIS7)" },
		{ "Shift-JIS"            , 0 , "Japanese (Shift-JIS)" },
		{ "EUC-JP"               , 0 , "Japanese (EUC-JP)" },
		{ "EUC-KR"               , 0 , "Korean" },
		{ "TSCII"                , 0 , "Tamil" },
#endif
		{ "ISO-8859-10"          , 0 , "ISO-8859-10" },
		{ "ISO-8859-13"          , 0 , "ISO-8859-13" },
		{ "ISO-8859-14"          , 0 , "ISO-8859-14" },
		{ "IBM-850"              , 0 , "IBM-850" },
		{ "IBM-866"              , 0 , "IBM-866" },
		{ "CP874"                , 0 , "CP874" },

		{ "UTF-8/ISO-8859-1"     , 1 , "UTF-8? -> Western, Latin-1" },
		{ "UTF-8/ISO-8859-2"     , 1 , "UTF-8? -> Central European 1" },
		{ "UTF-8/ISO-8859-3"     , 1 , "UTF-8? -> Central European 2" },
		{ "UTF-8/ISO-8859-4"     , 1 , "UTF-8? -> Baltic, Standard" },
		{ "UTF-8/ISO-8859-5"     , 1 , "UTF-8? -> Cyrillic, ISO" },
		{ "UTF-8/ISO-8859-6"     , 1 , "UTF-8? -> Arabic, Standard" },
		{ "UTF-8/ISO-8859-7"     , 1 , "UTF-8? -> Greek" },
		{ "UTF-8/ISO-8859-8"     , 1 , "UTF-8? -> Hebrew, visually ordered" },
		{ "UTF-8/ISO-8859-8-i"   , 1 , "UTF-8? -> Hebrew, logically ordered" },
		{ "UTF-8/ISO-8859-9"     , 1 , "UTF-8? -> Turkish, Latin-5" },
		{ "UTF-8/ISO-8859-15"    , 1 , "UTF-8? -> Western, Latin-1 + Euro" },
		{ "UTF-8/KOI8-R"         , 1 , "UTF-8? -> Cyrillic, KOI" },
		{ "UTF-8/KOI8-U"         , 1 , "UTF-8? -> Ukrainian" },
		{ "UTF-8/CP-1250"        , 1 , "UTF-8? -> Central European 3" },
		{ "UTF-8/CP-1251"        , 1 , "UTF-8? -> Cyrillic, Windows" },
		{ "UTF-8/CP-1252"        , 1 , "UTF-8? -> Western, CP" },
		{ "UTF-8/CP-1253"        , 1 , "UTF-8? -> Greek, CP" },
		{ "UTF-8/CP-1256"        , 1 , "UTF-8? -> Arabic, CP" },
		{ "UTF-8/CP-1257"        , 1 , "UTF-8? -> Baltic, CP" },
		{ "UTF-8/CP-1255"        , 1 , "UTF-8? -> Hebrew, CP" },
		{ "UTF-8/CP-1254"        , 1 , "UTF-8? -> Turkish, CP" },
		{ "UTF-8/TIS-620"        , 1 , "UTF-8? -> Thai" },
#ifndef QT_NO_BIG_CODECS
		{ "UTF-8/Big5"           , 1 , "UTF-8? -> Chinese Traditional" },
		{ "UTF-8/Big5-HKSCS"     , 1 , "UTF-8? -> Chinese Traditional, Hong Kong" },
		{ "UTF-8/GB18030"        , 1 , "UTF-8? -> Chinese Simplified" },
		{ "UTF-8/JIS7"           , 1 , "UTF-8? -> Japanese (JIS7)" },
		{ "UTF-8/Shift-JIS"      , 1 , "UTF-8? -> Japanese (Shift-JIS)" },
		{ "UTF-8/EUC-JP"         , 1 , "UTF-8? -> Japanese (EUC-JP)" },
		{ "UTF-8/EUC-KR"         , 1 , "UTF-8? -> Korean" },
		{ "UTF-8/TSCII"          , 1 , "UTF-8? -> Tamil" },
#endif
		{ "UTF-8/ISO-8859-10"    , 1 , "UTF-8? -> ISO-8859-10" },
		{ "UTF-8/ISO-8859-13"    , 1 , "UTF-8? -> ISO-8859-13" },
		{ "UTF-8/ISO-8859-14"    , 1 , "UTF-8? -> ISO-8859-14" },
		{ "UTF-8/IBM-850"        , 1 , "UTF-8? -> IBM-850" },
		{ "UTF-8/IBM-866"        , 1 , "UTF-8? -> IBM-866" },
		{ "UTF-8/CP874"          , 1 , "UTF-8? -> CP874" },
		{ 0                      , 0 , 0 }
	};
	
	EncodingDescription * encodingDescription(int iIdx)
	{
		if(iIdx > NUM_ENCODINGS)return &(supported_encodings[NUM_ENCODINGS]);
		return &(supported_encodings[iIdx]);
	}

	QTextCodec * codecForName(const char * szName)
	{
		if(kvi_strEqualCIN("UTF-8/",szName,6))
		{
			// composite codec
			KviSmartTextCodec * c = g_pSmartCodecDict->find(szName);
			if(c)return c;
			KviStr tmp = szName;
			tmp.cutToFirst('/');
			c = new KviSmartTextCodec(tmp.ptr());
			if(c->ok())
			{
				g_pSmartCodecDict->replace(szName,c);
				return c;
			} else {
				delete c;
			}
		}
		return QTextCodec::codecForName(szName);
	}

	const KviStr & localeName()
	{
		return g_szLang;
	}

	bool loadCatalogue(const QString& name,const QString& szLocaleDir)
	{
		//debug("Looking up catalogue %s",name);
		if(g_pCatalogueDict->find(name))return true; // already loaded

		QString szBuffer;

		if(findCatalogue(szBuffer,name,szLocaleDir))
		{
			KviMessageCatalogue * c = new KviMessageCatalogue();
			if(c->load(szBuffer))
			{
				//debug("KviLocale: loaded catalogue %s",name);
				g_pCatalogueDict->insert(name,c);
				return true;
			}
		}
		return false;
	}

	bool unloadCatalogue(const QString& name)
	{
		//debug("Unloading catalogue : %s",name);
		return g_pCatalogueDict->remove(name);
	}

	bool findCatalogue(QString &szBuffer,const QString& name,const QString& szLocaleDir)
	{
		KviStr szLocale = g_szLang;

		QString szLocDir = szLocaleDir;
		KviQString::ensureLastCharIs(szLocDir,KVI_PATH_SEPARATOR_CHAR);

		szBuffer = szLocDir+name+"_"+szLocale.ptr()+".mo";
//		debug("Lookup %s",szBuffer.utf8().data());
		if(KviFileUtils::fileExists(szBuffer))return true;

		if(szLocale.findFirstIdx('.') != -1)
		{
			// things like en_GB.utf8
			// kill them
			szLocale.cutFromFirst('.');
			szBuffer = szLocDir+name+"_"+szLocale.ptr()+".mo";
			if(KviFileUtils::fileExists(szBuffer))return true;
		}

		if(szLocale.findFirstIdx('@') != -1)
		{
			// things like @euro ?
			// kill them
			szLocale.cutFromFirst('@');
			szBuffer = szLocDir+name+"_"+szLocale.ptr()+".mo";
			if(KviFileUtils::fileExists(szBuffer))return true;
		}

		if(szLocale.findFirstIdx('_') != -1)
		{
			// things like en_GB
			// kill them
			szLocale.cutFromFirst('_');
			szBuffer = szLocDir+name+"_"+szLocale.ptr()+".mo";
			if(KviFileUtils::fileExists(szBuffer))return true;
		}

		// try the lower case version too
		szLocale.toLower();
		szBuffer = szLocDir+name+"_"+szLocale.ptr()+".mo";
		if(KviFileUtils::fileExists(szBuffer))return true;

		return false;
	}

	//
	// This function attempts to determine the current locale
	// and then load the corresponding translation file
	// from the KVIrc locale directory
	// Returns true if the locale was correctly set
	// i.e. the locale is C or POSIX (no translation needed)
	//     or the locale is correctly defined and the
	//     translation map was sucesfully loaded
	//

	void init(QApplication * app,const QString& localeDir)
	{
		// first of all try to find out the current locale
		g_szLang="";
		QString szLangFile=QString("%1/.kvirc_force_locale").arg(QDir::homeDirPath());
		if(KviFileUtils::fileExists(szLangFile))
		{
			QString szTmp;
			KviFileUtils::readFile(szLangFile,szTmp);
			g_szLang=szTmp;
		}
		if(g_szLang.isEmpty())g_szLang = kvi_getenv("KVIRC_LANG");
		if(g_szLang.isEmpty())g_szLang = QTextCodec::locale();
		if(g_szLang.isEmpty())g_szLang = kvi_getenv("LC_MESSAGES");
		if(g_szLang.isEmpty())g_szLang = kvi_getenv("LANG");
		if(g_szLang.isEmpty())g_szLang = "en";
		g_szLang.stripWhiteSpace();

		// the main catalogue is supposed to be kvirc_<language>.mo
		g_pMainCatalogue = new KviMessageCatalogue();
		// the catalogue dict
		g_pCatalogueDict = new QAsciiDict<KviMessageCatalogue>;
		g_pCatalogueDict->setAutoDelete(true);

		// the smart codec dict
		g_pSmartCodecDict = new QAsciiDict<KviSmartTextCodec>;
		// the Qt docs explicitly state that we shouldn't delete
		// the codecs by ourselves...
		g_pSmartCodecDict->setAutoDelete(false);

		if(g_szLang.hasData())
		{
			QString szBuffer;
			if(findCatalogue(szBuffer,"kvirc",localeDir))
			{
				g_pMainCatalogue->load(szBuffer);
				g_pTranslator = new KviTranslator(app,"kvirc_translator");
				app->installTranslator(g_pTranslator);
			} else {
				KviStr szTmp = g_szLang;
				szTmp.cutFromFirst('.');
				szTmp.cutFromFirst('_');
				szTmp.cutFromFirst('@');
				szTmp.toLower();
				if(!(kvi_strEqualCI(szTmp.ptr(),"en") ||
					kvi_strEqualCI(szTmp.ptr(),"c") ||
					kvi_strEqualCI(szTmp.ptr(),"us") ||
					kvi_strEqualCI(szTmp.ptr(),"gb") ||
					kvi_strEqualCI(szTmp.ptr(),"posix")))
				{
					// FIXME: THIS IS NO LONGER VALID!!!
					debug("Can't find the catalogue for locale \"%s\" (%s)",g_szLang.ptr(),szTmp.ptr());
					debug("There is no such translation or the $LANG variable was incorrectly set");
					debug("You can use $KVIRC_LANG to override the catalogue name");
					debug("For example you can set KVIRC_LANG to it_IT to force usage of the it.mo catalogue");
				}
			}
		}

		//g_pTextCodec = QTextCodec::codecForLocale();
		//if(!g_pTextCodec)g_pTextCodec = QTextCodec::codecForLocale();
	}

	void done(QApplication * app)
	{
		delete g_pMainCatalogue;
		delete g_pCatalogueDict;
		delete g_pSmartCodecDict;
		g_pMainCatalogue = 0;
		g_pCatalogueDict = 0;
		g_pSmartCodecDict = 0;
		if(g_pTranslator)
		{
			app->removeTranslator(g_pTranslator);
			delete g_pTranslator;
            g_pTranslator = 0;
		}
	}

	KviMessageCatalogue * getLoadedCatalogue(const QString& name)
	{
		return g_pCatalogueDict->find(name);
	}


	const char * translate(const char * text,const char * context)
	{
		if(context)
		{
			KviMessageCatalogue * c = g_pCatalogueDict->find(context);
			if(!c)
			{
				// FIXME: Should really try to load the catalogue here!
				c = new KviMessageCatalogue();
				g_pCatalogueDict->insert(context,c);
			}
			return c->translate(text);
		}
		return g_pMainCatalogue->translate(text);
	}

	const QString & translateToQString(const char * text,const char * context)
	{
		if(context)
		{
			KviMessageCatalogue * c = g_pCatalogueDict->find(context);
			if(!c)
			{
				// FIXME: Should really try to load the catalogue here!
				c = new KviMessageCatalogue();
				g_pCatalogueDict->insert(context,c);
			}
			return c->translateToQString(text);
		}
		return g_pMainCatalogue->translateToQString(text);
	}
};

KviTranslator::KviTranslator(QObject * par,const char * nam)
: QTranslator(par,nam)
{
}

KviTranslator::~KviTranslator()
{
}

QString KviTranslator::find(const char *context,const char * message) const
{
	// we ignore contexts for qt translations
	return g_pMainCatalogue->translateToQString(message);
}

QTranslatorMessage KviTranslator::findMessage(const char * context,const char * sourceText,const char * comment) const
{
	// we ignore contexts for qt translations
	return QTranslatorMessage(context,sourceText,comment,g_pMainCatalogue->translateToQString(sourceText));
}

#if 0

// a fake table that will force these translations
// to be included in the *.pot file

static QString fake_translations_table[]=
{
	// global
	__tr2qs("OK"),
	__tr2qs("Cancel"),
	// color dialog
	__tr2qs("Select color"),
	__tr2qs("&Basic colors"),
	__tr2qs("&Custom colors"),
	__tr2qs("&Red"),
	__tr2qs("&Green"),
	__tr2qs("Bl&ue"),
	__tr2qs("&Define Custom Colors >>"),
	__tr2qs("&Add to Custom Colors"),
	// font dialog
	__tr2qs("Select Font"),
	__tr2qs("&Font"),
	__tr2qs("Font st&yle"),
	__tr2qs("&Size"),
	__tr2qs("Sample"),
	__tr2qs("Effects"),
	__tr2qs("Stri&keout"),
	__tr2qs("&Underline"),
	__tr2qs("Scr&ipt"),
	//File selector
	__tr2qs("Parent Directory"),
	__tr2qs("Back"),
	__tr2qs("Forward"),
	__tr2qs("Reload"),
	__tr2qs("New Directory"),
	__tr2qs("Bookmarks"),
	__tr2qs("Add Bookmark"),
	__tr2qs("&Edit Bookmarks"),
	__tr2qs("New Bookmark Folder..."),
	__tr2qs("Configure"),
	__tr2qs("Sorting"),
	__tr2qs("By Name"),
	__tr2qs("By Date"),
	__tr2qs("By Size"),
	__tr2qs("Reverse"),
	__tr2qs("Directories First"),
	__tr2qs("Case Insensitive"),
	__tr2qs("Short View"),
	__tr2qs("Detailed View"),
	__tr2qs("Show Hidden Files"),
	__tr2qs("Show Quick Access Navigation Panel"),
	__tr2qs("Show Preview"),
	__tr2qs("Separate Directories"),
	__tr2qs("Often used directories"),
	__tr2qs("Desktop"),
	__tr2qs("Home Directory"),
	__tr2qs("Floppy"),
	__tr2qs("Temporary Files"),
	__tr2qs("Network"),
	__tr2qs("New Directory..."),
	__tr2qs("Delete"),
	__tr2qs("Thumbnail Previews"),
	__tr2qs("Large Icons"),
	__tr2qs("Small Icons"),
	__tr2qs("Properties..."),
	__tr2qs("&Automatic Preview"),
	__tr2qs("&Preview"),
	__tr2qs("&Location:"),
	__tr2qs("&Filter:"),
	__tr2qs("All Files"),
	__tr2qs("&OK"),
	__tr2qs("&Cancel")

}

#endif
