/*
    This file is part of the KDE libraries

    Copyright (C) 1999 Lars Knoll (knoll@kde.org)
    Copyright (C) 2003 Dirk Mueller (mueller@kde.org)
    Copyright (C) 2003 Apple Computer, Inc.
    Copyright (C) 2007 Nick Shaforostoff (shafff@ukr.net)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/
//----------------------------------------------------------------------------
//
// decoder for input stream

#include "kencodingdetector.h"

#undef DECODE_DEBUG
//#define DECODE_DEBUG

#define MAX_BUFFER 16*1024

#include <assert.h>

#include "guess_ja_p.h"

#include <QDebug>
#include <QRegExp>
#include <QTextCodec>

#include "kcharsets.h"
#include <klocalizedstring.h>

#include <ctype.h>

enum MIB
{
    MibLatin1  = 4,
    Mib8859_8  = 85,
    MibUtf8    = 106,
    MibUcs2    = 1000,
    MibUtf16   = 1015,
    MibUtf16BE = 1013,
    MibUtf16LE = 1014
};

static bool is16Bit(QTextCodec* codec)
{
    switch (codec->mibEnum())
    {
    case MibUtf16:
    case MibUtf16BE:
    case MibUtf16LE:
    case MibUcs2:
        return true;
    default:
        return false;
    }
}

class KEncodingDetectorPrivate
{
public:
    QTextCodec *m_codec;
    QTextDecoder *m_decoder; // utf16
    QTextCodec *m_defaultCodec;
    QByteArray  m_storeDecoderName;

    KEncodingDetector::EncodingChoiceSource m_source;
    KEncodingDetector::AutoDetectScript m_autoDetectLanguage;

    bool m_visualRTL : 1;
    bool m_seenBody : 1;
    bool m_writtingHappened : 1;
    bool m_analyzeCalled : 1; //for decode()
    int m_multiByte;

    QByteArray m_bufferForDefferedEncDetection;

    KEncodingDetectorPrivate()
            : m_codec(QTextCodec::codecForMib(MibLatin1))
            , m_decoder(m_codec->makeDecoder())
            , m_defaultCodec(m_codec)
            , m_source(KEncodingDetector::DefaultEncoding)
            , m_autoDetectLanguage(KEncodingDetector::SemiautomaticDetection)
            , m_visualRTL(false)
            , m_seenBody(false)
            , m_writtingHappened(false)
            , m_analyzeCalled(false)
            , m_multiByte(0)
    {
    }

    KEncodingDetectorPrivate(QTextCodec* codec,KEncodingDetector::EncodingChoiceSource source, KEncodingDetector::AutoDetectScript script)
            : m_codec(codec)
            , m_decoder(m_codec->makeDecoder())
            , m_defaultCodec(m_codec)
            , m_source(source)
            , m_autoDetectLanguage(script)
            , m_visualRTL(false)
            , m_seenBody(false)
            , m_writtingHappened(false)
            , m_analyzeCalled(false)
            , m_multiByte(0)
    {
    }

    ~KEncodingDetectorPrivate()
    {
        delete m_decoder;
    }

    // Returns true if the encoding was explicitly specified someplace.
    bool isExplicitlySpecifiedEncoding()
    {
        return m_source != KEncodingDetector::DefaultEncoding && m_source != KEncodingDetector::AutoDetectedEncoding;
    }
};


static QByteArray automaticDetectionForArabic( const unsigned char* ptr, int size )
{
    for ( int i = 0; i < size; ++i ) {
        if ( ( ptr[ i ] >= 0x80 && ptr[ i ] <= 0x9F ) || ptr[ i ] == 0xA1 || ptr[ i ] == 0xA2 || ptr[ i ] == 0xA3
             || ( ptr[ i ] >= 0xA5 && ptr[ i ] <= 0xAB ) || ( ptr[ i ] >= 0xAE && ptr[ i ] <= 0xBA )
             || ptr[ i ] == 0xBC || ptr[ i ] == 0xBD || ptr[ i ] == 0xBE || ptr[ i ] == 0xC0
             || ( ptr[ i ] >= 0xDB && ptr[ i ] <= 0xDF ) || ( ptr[ i ] >= 0xF3 ) ) {
            return "cp1256";
        }
    }

    return "iso-8859-6";
}

static QByteArray automaticDetectionForBaltic( const unsigned char* ptr, int size )
{
    for ( int i = 0; i < size; ++i ) {
        if ( ( ptr[ i ] >= 0x80 && ptr[ i ] <= 0x9E ) )
             return "cp1257";

        if ( ptr[ i ] == 0xA1 || ptr[ i ] == 0xA5 )
            return "iso-8859-13";
    }

    return "iso-8859-13";
}

static QByteArray automaticDetectionForCentralEuropean(const unsigned char* ptr, int size )
{
    QByteArray charset = QByteArray();
    for ( int i = 0; i < size; ++i ) {
        if ( ptr[ i ] >= 0x80 && ptr[ i ] <= 0x9F ) {
            if ( ptr[ i ] == 0x81 || ptr[ i ] == 0x83 || ptr[ i ] == 0x90 || ptr[ i ] == 0x98 )
                return "ibm852";

            if ( i + 1 > size )
                return "cp1250";
            else { // maybe ibm852 ?
                charset = "cp1250";
                continue;
            }
        }
        if ( ptr[ i ] == 0xA5 || ptr[ i ] == 0xAE || ptr[ i ] == 0xBE || ptr[ i ] == 0xC3 || ptr[ i ] == 0xD0 || ptr[ i ] == 0xE3 || ptr[ i ] == 0xF0 ) {
            if ( i + 1 > size )
                return "iso-8859-2";
            else {  // maybe ibm852 ?
                if ( charset.isNull() )
                    charset = "iso-8859-2";
                continue;
            }
        }
    }

    if ( charset.isNull() )
        charset = "iso-8859-3";

    return charset.data();
}

static QByteArray automaticDetectionForCyrillic( const unsigned char* ptr, int size)
{
#ifdef DECODE_DEBUG
        qWarning() << "KEncodingDetector: Cyr heuristics";
#endif

//     if (ptr[0]==0xef && ptr[1]==0xbb && ptr[2]==0xbf)
//         return "utf8";
    int utf8_mark=0;
    int koi_score=0;
    int cp1251_score=0;

    int koi_st=0;
    int cp1251_st=0;

//     int koi_na=0;
//     int cp1251_na=0;

    int koi_o_capital=0;
    int koi_o=0;
    int cp1251_o_capital=0;
    int cp1251_o=0;

    int koi_a_capital=0;
    int koi_a=0;
    int cp1251_a_capital=0;
    int cp1251_a=0;

    int koi_s_capital=0;
    int koi_s=0;
    int cp1251_s_capital=0;
    int cp1251_s=0;

    int koi_i_capital=0;
    int koi_i=0;
    int cp1251_i_capital=0;
    int cp1251_i=0;

    int cp1251_small_range=0;
    int koi_small_range=0;
    int ibm866_small_range=0;

    int i;
    for (i=1; (i<size) && (cp1251_small_range+koi_small_range<1000) ;++i)
    {
        if (ptr[i]>0xdf)
        {
            ++cp1251_small_range;

            if (ptr[i]==0xee)//small o
                ++cp1251_o;
            else if (ptr[i]==0xe0)//small a
                ++cp1251_a;
            else if (ptr[i]==0xe8)//small i
                ++cp1251_i;
            else if (ptr[i]==0xf1)//small s
                ++cp1251_s;
            else if (ptr[i]==0xf2 && ptr[i-1]==0xf1)//small st
                ++cp1251_st;

            else if (ptr[i]==0xef)
                ++koi_o_capital;
            else if (ptr[i]==0xe1)
                ++koi_a_capital;
            else if (ptr[i]==0xe9)
                ++koi_i_capital;
            else if (ptr[i]==0xf3)
                ++koi_s_capital;

        }
        else if (ptr[i]>0xbf)
        {
            ++koi_small_range;

            if (ptr[i]==0xd0||ptr[i]==0xd1)//small o
                ++utf8_mark;
            else if (ptr[i]==0xcf)//small o
                ++koi_o;
            else if (ptr[i]==0xc1)//small a
                ++koi_a;
            else if (ptr[i]==0xc9)//small i
                ++koi_i;
            else if (ptr[i]==0xd3)//small s
                ++koi_s;
            else if (ptr[i]==0xd4 && ptr[i-1]==0xd3)//small st
                ++koi_st;

            else if (ptr[i]==0xce)
                ++cp1251_o_capital;
            else if (ptr[i]==0xc0)
                ++cp1251_a_capital;
            else if (ptr[i]==0xc8)
                ++cp1251_i_capital;
            else if (ptr[i]==0xd1)
                ++cp1251_s_capital;
        }
        else if (ptr[i]>0x9f && ptr[i]<0xb0) //first 16 letterz is 60%
            ++ibm866_small_range;

    }

    //cannot decide?
    if (cp1251_small_range+koi_small_range+ibm866_small_range<8)
    {
        return "";
    }

    if (3*utf8_mark>cp1251_small_range+koi_small_range+ibm866_small_range)
    {
#ifdef DECODE_DEBUG
        qWarning() << "Cyr Enc Detection: UTF8";
#endif
        return "UTF-8";
    }

    if (ibm866_small_range>cp1251_small_range+koi_small_range)
        return "ibm866";

//     QByteArray koi_string = "koi8-u";
//     QByteArray cp1251_string = "cp1251";

    if (cp1251_st==0 && koi_st>1)
        koi_score+=10;
    else if (koi_st==0 && cp1251_st>1)
        cp1251_score+=10;

    if (cp1251_st && koi_st)
    {
        if (cp1251_st/koi_st>2)
            cp1251_score+=20;
        else if (koi_st/cp1251_st>2)
            koi_score+=20;
    }

    if (cp1251_a>koi_a)
        cp1251_score+=10;
    else if (cp1251_a || koi_a)
        koi_score+=10;

    if (cp1251_o>koi_o)
        cp1251_score+=10;
    else if (cp1251_o || koi_o)
        koi_score+=10;

    if (cp1251_i>koi_i)
        cp1251_score+=10;
    else if (cp1251_i || koi_i)
        koi_score+=10;

    if (cp1251_s>koi_s)
        cp1251_score+=10;
    else if (cp1251_s || koi_s)
        koi_score+=10;

    if (cp1251_a_capital>koi_a_capital)
        cp1251_score+=9;
    else if (cp1251_a_capital || koi_a_capital)
        koi_score+=9;

    if (cp1251_o_capital>koi_o_capital)
        cp1251_score+=9;
    else if (cp1251_o_capital || koi_o_capital)
        koi_score+=9;

    if (cp1251_i_capital>koi_i_capital)
        cp1251_score+=9;
    else if (cp1251_i_capital || koi_i_capital)
        koi_score+=9;

    if (cp1251_s_capital>koi_s_capital)
        cp1251_score+=9;
    else if (cp1251_s_capital || koi_s_capital)
        koi_score+=9;
#ifdef DECODE_DEBUG
    qWarning()<<"koi_score " << koi_score << " cp1251_score " << cp1251_score;
#endif
    if (abs(koi_score-cp1251_score)<10)
    {
        //fallback...
        cp1251_score=cp1251_small_range;
        koi_score=koi_small_range;
    }
    if (cp1251_score>koi_score)
        return "cp1251";
    else
        return "koi8-u";


//     if (cp1251_score>koi_score)
//         setEncoding("cp1251",AutoDetectedEncoding);
//     else
//         setEncoding("koi8-u",AutoDetectedEncoding);
//     return true;

}

static QByteArray automaticDetectionForGreek( const unsigned char* ptr, int size )
{
    for ( int i = 0; i < size; ++i ) {
        if ( ptr[ i ] == 0x80 || ( ptr[ i ] >= 0x82 && ptr[ i ] <= 0x87 ) || ptr[ i ] == 0x89 || ptr[ i ] == 0x8B
             || ( ptr[ i ] >= 0x91 && ptr[ i ] <= 0x97 ) || ptr[ i ] == 0x99 || ptr[ i ] == 0x9B || ptr[ i ] == 0xA4
             || ptr[ i ] == 0xA5 || ptr[ i ] == 0xAE ) {
            return "cp1253";
        }
    }

    return "iso-8859-7";
}

static QByteArray automaticDetectionForHebrew( const unsigned char* ptr, int size )
{
    for ( int i = 0; i < size; ++i ) {
        if ( ptr[ i ] == 0x80 || ( ptr[ i ] >= 0x82 && ptr[ i ] <= 0x89 ) || ptr[ i ] == 0x8B
             || ( ptr[ i ] >= 0x91 && ptr[ i ] <= 0x99 ) || ptr[ i ] == 0x9B || ptr[ i ] == 0xA1 || ( ptr[ i ] >= 0xBF && ptr[ i ] <= 0xC9 )
             || ( ptr[ i ] >= 0xCB && ptr[ i ] <= 0xD8 ) ) {
            return "cp1255";
        }

        if ( ptr[ i ] == 0xDF )
            return "iso-8859-8-i";
    }

    return "iso-8859-8-i";
}

static QByteArray automaticDetectionForJapanese( const unsigned char* ptr, int size )
{
    JapaneseCode kc;

    switch ( kc.guess_jp( (const char*)ptr, size ) ) {
    case JapaneseCode::JIS:
        return "jis7";
    case JapaneseCode::EUC:
        return "eucjp";
    case JapaneseCode::SJIS:
        return "sjis";
     case JapaneseCode::UTF8:
        return "utf8";
    default:
        break;
    }

    return "";
}

static QByteArray automaticDetectionForTurkish( const unsigned char* ptr, int size )
{
    for ( int i = 0; i < size; ++i ) {
        if ( ptr[ i ] == 0x80 || ( ptr[ i ] >= 0x82 && ptr[ i ] <= 0x8C ) || ( ptr[ i ] >= 0x91 && ptr[ i ] <= 0x9C ) || ptr[ i ] == 0x9F ) {
            return "cp1254";
        }
    }

    return "iso-8859-9";
}

static QByteArray automaticDetectionForWesternEuropean( const unsigned char* ptr, int size )
{
    --size;
    uint nonansi_count=0;
    for (int i=0; i<size; ++i)
    {
        if (ptr[i]>0x79)
        {
             ++nonansi_count;
            if ( ptr[i]>0xc1 && ptr[i]<0xf0 && ptr[i+1]>0x7f && ptr[i+1]<0xc0)
            {
                return "UTF-8";
            }
            if (ptr[i] >= 0x78 && ptr[i]<=0x9F )
            {
                return "cp1252";
            }
        }

    }

    if (nonansi_count>0)
        return "iso-8859-15";

    return "";
}

// Other browsers allow comments in the head section, so we need to also.
// It's important not to look for tags inside the comments.
static void skipComment(const char *&ptr, const char *pEnd)
{
    const char *p = ptr;
    // Allow <!-->; other browsers do.
    if (*p=='>')
    {
        p++;
    }
    else
    {
        while (p!=pEnd)
        {
            if (*p=='-')
            {
                // This is the real end of comment, "-->".
                if (p[1]=='-' && p[2]=='>')
                {
                    p += 3;
                    break;
                }
                // This is the incorrect end of comment that other browsers allow, "--!>".
                if (p[1] == '-' && p[2] == '!' && p[3] == '>')
                {
                    p += 4;
                    break;
                }
            }
            p++;
        }
    }
    ptr=p;
}

// Returns the position of the encoding string.
static int findXMLEncoding(const QByteArray &str, int &encodingLength)
{
    int len = str.length();
    int pos = str.indexOf("encoding");
    if (pos == -1)
        return -1;
    pos += 8;

    // Skip spaces and stray control characters.
    while (pos<len && str[pos]<=' ')
        ++pos;

    //Bail out if nothing after
    // Skip equals sign.
    if (pos>=len || str[pos] != '=')
        return -1;
    ++pos;

    // Skip spaces and stray control characters.
    while (pos<len && str[pos]<=' ')
        ++pos;

    //Bail out if nothing after
    if (pos >= len)
        return -1;

    // Skip quotation mark.
    char quoteMark = str[pos];
    if (quoteMark != '"' && quoteMark != '\'')
        return -1;
    ++pos;

    // Find the trailing quotation mark.
    int end=pos;
    while (end<len && str[end]!=quoteMark)
        ++end;

    if (end>=len)
        return -1;

    encodingLength = end-pos;
    return pos;
}

bool KEncodingDetector::processNull(char *data, int len)
{
    bool bin=false;
    if(is16Bit(d->m_codec))
    {
        for (int i=1; i < len; i+=2)
        {
            if ((data[i]=='\0') && (data[i-1]=='\0'))
            {
                bin=true;
                data[i]=' ';
            }
        }
        return bin;
    }
    // replace '\0' by spaces, for buggy pages
    int i = len-1;
    while(--i>=0)
    {
        if(data[i]==0)
        {
            bin=true;
            data[i]=' ';
        }
    }
    return bin;
}


bool KEncodingDetector::errorsIfUtf8 (const char* data, int length)
{
    if (d->m_codec->mibEnum()!=MibUtf8)
        return false; //means no errors
// #define highest1Bits (unsigned char)0x80
// #define highest2Bits (unsigned char)0xC0
// #define highest3Bits (unsigned char)0xE0
// #define highest4Bits (unsigned char)0xF0
// #define highest5Bits (unsigned char)0xF8
static const unsigned char highest1Bits = 0x80;
static const unsigned char highest2Bits = 0xC0;
static const unsigned char highest3Bits = 0xE0;
static const unsigned char highest4Bits = 0xF0;
static const unsigned char highest5Bits = 0xF8;

    for (int i=0; i<length; ++i)
    {
        unsigned char c = data[i];

        if (d->m_multiByte>0)
        {
            if ((c & highest2Bits) == 0x80)
            {
                --(d->m_multiByte);
                continue;
            }
#ifdef DECODE_DEBUG
            qWarning() << "EncDetector: Broken UTF8";
#endif
            return true;
        }

        // most significant bit zero, single char
        if ((c & highest1Bits) == 0x00)
            continue;

        // 110xxxxx => init 1 following bytes
        if ((c & highest3Bits) == 0xC0)
        {
            d->m_multiByte = 1;
            continue;
        }

        // 1110xxxx => init 2 following bytes
        if ((c & highest4Bits) == 0xE0)
        {
            d->m_multiByte = 2;
            continue;
        }

        // 11110xxx => init 3 following bytes
        if ((c & highest5Bits) == 0xF0)
        {
            d->m_multiByte = 3;
            continue;
        }
#ifdef DECODE_DEBUG
        qWarning() << "EncDetector:_Broken UTF8";
#endif
        return true;
    }
    return false;
}


KEncodingDetector::KEncodingDetector() : d(new KEncodingDetectorPrivate)
{
}

KEncodingDetector::KEncodingDetector(QTextCodec* codec, EncodingChoiceSource source, AutoDetectScript script) :
    d(new KEncodingDetectorPrivate(codec,source,script))
{
}

KEncodingDetector::~KEncodingDetector()
{
    delete d;
}

void KEncodingDetector::setAutoDetectLanguage( KEncodingDetector::AutoDetectScript lang)
{
    d->m_autoDetectLanguage=lang;
}
KEncodingDetector::AutoDetectScript KEncodingDetector::autoDetectLanguage() const
{
    return d->m_autoDetectLanguage;
}

KEncodingDetector::EncodingChoiceSource KEncodingDetector::encodingChoiceSource() const
{
    return d->m_source;
}

const char* KEncodingDetector::encoding() const
{
    d->m_storeDecoderName = d->m_codec->name();
    return d->m_storeDecoderName.constData();
}

bool KEncodingDetector::visuallyOrdered() const
{
    return d->m_visualRTL;
}

// const QTextCodec* KEncodingDetector::codec() const
// {
//     return d->m_codec;
// }

QTextDecoder* KEncodingDetector::decoder()
{
    return d->m_decoder;
}

void KEncodingDetector::resetDecoder()
{
    assert(d->m_defaultCodec);
    d->m_bufferForDefferedEncDetection.clear();
    d->m_writtingHappened = false;
    d->m_analyzeCalled = false;
    d->m_multiByte = 0;
    delete d->m_decoder;
    if (!d->m_codec)
        d->m_codec = d->m_defaultCodec;
    d->m_decoder = d->m_codec->makeDecoder();
}

bool KEncodingDetector::setEncoding(const char *_encoding, EncodingChoiceSource type)
{
    QTextCodec *codec;
    QByteArray enc(_encoding);
    if(/*enc.isNull() || */enc.isEmpty())
    {
        if (type==DefaultEncoding)
            codec=d->m_defaultCodec;
        else
            return false;
    }
    else
    {
        //QString->QTextCodec

        enc = enc.toLower();
         // hebrew visually ordered
        if(enc=="visual")
            enc="iso8859-8";
        bool b;
        codec = KCharsets::charsets()->codecForName(QLatin1String(enc.data()), b);
        if (!b)
            return false;
    }

    if (d->m_codec->mibEnum()==codec->mibEnum())
    {
        // We already have the codec, but we still want to re-set the type,
        // as we may have overwritten a default with a detected
        d->m_source = type;
        return true;
    }

    if ((type==EncodingFromMetaTag || type==EncodingFromXMLHeader) && is16Bit(codec))
    {
        //Sometimes the codec specified is absurd, i.e. UTF-16 despite
        //us decoding a meta tag as ASCII. In that case, ignore it.
        return false;
    }

    if (codec->mibEnum() == Mib8859_8)
    {
        //We do NOT want to use Qt's QHebrewCodec, since it tries to reorder itself.
        codec = QTextCodec::codecForName("iso8859-8-i");

        // visually ordered unless one of the following
        if(!(enc=="iso-8859-8-i"||enc=="iso_8859-8-i"||enc=="csiso88598i"||enc=="logical"))
            d->m_visualRTL = true;
    }

    d->m_codec = codec;
    d->m_source = type;
    delete d->m_decoder;
    d->m_decoder = d->m_codec->makeDecoder();
#ifdef DECODE_DEBUG
    qDebug() << "KEncodingDetector::encoding used is" << d->m_codec->name();
#endif
    return true;
}

QString KEncodingDetector::decode(const char *data, int len)
{
    processNull(const_cast<char *>(data),len);
    if (!d->m_analyzeCalled)
    {
        analyze(data,len);
        d->m_analyzeCalled=true;
    }

    return d->m_decoder->toUnicode(data,len);
}

QString KEncodingDetector::decode(const QByteArray &data)
{
    processNull(const_cast<char *>(data.data()),data.size());
    if (!d->m_analyzeCalled)
    {
        analyze(data.data(),data.size());
        d->m_analyzeCalled=true;
    }

    return d->m_decoder->toUnicode(data);
}

QString KEncodingDetector::decodeWithBuffering(const char *data, int len)
{
#ifdef DECODE_DEBUG
        qWarning() << "KEncodingDetector: decoding "<<len<<" bytes";
#endif
    if (d->m_writtingHappened)
    {
#ifdef DECODE_DEBUG
        qWarning() << "KEncodingDetector: d->m_writtingHappened "<< d->m_codec->name();
#endif
        processNull(const_cast<char *>(data),len);
        return d->m_decoder->toUnicode(data, len);
    }
    else
    {
        if (d->m_bufferForDefferedEncDetection.isEmpty())
        {
            // If encoding detection produced something, and we either got to the body or
            // actually saw the encoding explicitly, we're done.
            if (analyze(data,len) && (d->m_seenBody || d->isExplicitlySpecifiedEncoding()))
            {
#ifdef DECODE_DEBUG
                qWarning() << "KEncodingDetector: m_writtingHappened first time "<< d->m_codec->name();
#endif
                processNull(const_cast<char *>(data),len);
                d->m_writtingHappened=true;
                return d->m_decoder->toUnicode(data, len);
            }
            else
            {
#ifdef DECODE_DEBUG
                qWarning() << "KEncodingDetector: begin deffer";
#endif
                d->m_bufferForDefferedEncDetection=data;
            }
        }
        else
        {
            d->m_bufferForDefferedEncDetection+=data;
            // As above, but also limit the buffer size. We must use the entire buffer here,
            // since the boundaries might split the meta tag, etc.
            bool detected = analyze(d->m_bufferForDefferedEncDetection.constData(), d->m_bufferForDefferedEncDetection.length());
            if ((detected && (d->m_seenBody || d->isExplicitlySpecifiedEncoding())) ||
                 d->m_bufferForDefferedEncDetection.length() > MAX_BUFFER)
            {
                d->m_writtingHappened=true;
                d->m_bufferForDefferedEncDetection.replace('\0',' ');
                QString result(d->m_decoder->toUnicode(d->m_bufferForDefferedEncDetection));
                d->m_bufferForDefferedEncDetection.clear();
#ifdef DECODE_DEBUG
                qWarning() << "KEncodingDetector: m_writtingHappened in the middle " << d->m_codec->name();
#endif
                return result;
            }
        }
    }

    return QString();
}

bool KEncodingDetector::decodedInvalidCharacters() const
{
    return d->m_decoder ? d->m_decoder->hasFailure() : false;
}

QString KEncodingDetector::flush()
{
    if (d->m_bufferForDefferedEncDetection.isEmpty())
        return QString();

    d->m_bufferForDefferedEncDetection.replace('\0',' ');
    QString result(d->m_decoder->toUnicode(d->m_bufferForDefferedEncDetection));
    d->m_bufferForDefferedEncDetection.clear();
#ifdef DECODE_DEBUG
    qWarning() << "KEncodingDetector:flush() "<< d->m_bufferForDefferedEncDetection.length()<<" bytes "<< d->m_codec->name();
#endif
    return result;
}

bool KEncodingDetector::analyze(const char *data, int len)
{
    // Check for UTF-16 or UTF-8 BOM mark at the beginning, which is a sure sign of a Unicode encoding.
    // maximumBOMLength = 10
    // Even if the user has chosen utf16 we still need to auto-detect the endianness
    if (len >= 10 && ((d->m_source != UserChosenEncoding) || is16Bit(d->m_codec)))
    {
        // Extract the first three bytes.
        const uchar *udata = (const uchar *)data;
        uchar c1 = *udata++;
        uchar c2 = *udata++;
        uchar c3 = *udata++;

        // Check for the BOM
        const char *autoDetectedEncoding;
        if ((c1 == 0xFE && c2 == 0xFF) || (c1 == 0xFF && c2 == 0xFE))
        {
            autoDetectedEncoding = "UTF-16";
        }
        else if (c1 == 0xEF && c2 == 0xBB && c3 == 0xBF)
        {
            autoDetectedEncoding = "UTF-8";
        }
        else if (c1 == 0x00 || c2 == 0x00)
        {
            uchar c4 = *udata++;
            uchar c5 = *udata++;
            uchar c6 = *udata++;
            uchar c7 = *udata++;
            uchar c8 = *udata++;
            uchar c9 = *udata++;
            uchar c10 = *udata++;

            int nul_count_even = (c2 != 0) + (c4 != 0) + (c6 != 0) + (c8 != 0) + (c10 != 0);
            int nul_count_odd = (c1 != 0) + (c3 != 0) + (c5 != 0) + (c7 != 0) + (c9 != 0);
            if ((nul_count_even==0 && nul_count_odd==5) || (nul_count_even==5 && nul_count_odd==0))
                autoDetectedEncoding = "UTF-16";
            else
                autoDetectedEncoding = 0;
        }
        else
        {
            autoDetectedEncoding = 0;
        }

        // If we found a BOM, use the encoding it implies.
        if (autoDetectedEncoding != 0)
        {
            d->m_source = BOM;
            d->m_codec = QTextCodec::codecForName(autoDetectedEncoding);
            assert(d->m_codec);
            //enc = d->m_codec->name();
            delete d->m_decoder;
            d->m_decoder = d->m_codec->makeDecoder();
#ifdef DECODE_DEBUG
            qWarning() << "Detection by BOM";
#endif
            if (is16Bit(d->m_codec) && c2==0x00)
            {
                // utf16LE, we need to put the decoder in LE mode
                char reverseUtf16[3] = {(char)0xFF, (char)0xFE, 0x00};
                d->m_decoder->toUnicode(reverseUtf16, 2);
            }
            return true;
        }
    }

    //exit from routine in case it was called to only detect byte order for utf-16
    if (d->m_source==UserChosenEncoding)
    {
#ifdef DECODE_DEBUG
        qWarning() << "KEncodingDetector: UserChosenEncoding exit ";
#endif

        if (errorsIfUtf8(data, len))
            setEncoding("",DefaultEncoding);
        return true;
    }

    // HTTP header takes precedence over meta-type stuff
    if (d->m_source==EncodingFromHTTPHeader)
        return true;

    if (!d->m_seenBody)
    {
        // we still don't have an encoding, and are in the head
        // the following tags are allowed in <head>:
        // SCRIPT|STYLE|META|LINK|OBJECT|TITLE|BASE
        const char *ptr = data;
        const char *pEnd = data+len;

        while(ptr != pEnd)
        {
            if(*ptr!='<')
            {
                ++ptr;
                continue;
            }
            ++ptr;
            // Handle comments.
            if (ptr[0] == '!' && ptr[1] == '-' && ptr[2] == '-')
            {
                ptr += 3;
                skipComment(ptr, pEnd);
                continue;
            }

            // Handle XML header, which can have encoding in it.
            if (ptr[0]=='?' && ptr[1]=='x' && ptr[2]=='m' && ptr[3]=='l')
            {
                const char *end = ptr;
                while (*end != '>' && end < pEnd)
                    end++;
                if (*end == '\0' || end == pEnd)
                    break;
                QByteArray str(ptr, end - ptr); // qbytearray provides the \0 terminator
                int length;
                int pos = findXMLEncoding(str, length);
                // also handles the case when specified encoding aint correct
                if (pos!=-1 && setEncoding(str.mid(pos, length).data(), EncodingFromXMLHeader))
                {
                    return true;
                }
            }

            //look for <meta>, stop if we reach <body>
            while (
                        !(((*ptr >= 'a') && (*ptr <= 'z')) ||
                        ((*ptr >= 'A') && (*ptr <= 'Z')))
                        && ptr < pEnd
                )
                ++ptr;

            char tmp[5];
            int length=0;
            const char* max=ptr+4;
            if (pEnd<max)
                max=pEnd;
            while (
                        (((*ptr >= 'a') && (*ptr <= 'z')) ||
                        ((*ptr >= 'A') && (*ptr <= 'Z')) ||
                        ((*ptr >= '0') && (*ptr <= '9')))
                        && ptr < max
                )
            {
                tmp[length] = tolower( *ptr );
                ++ptr;
                ++length;
            }
            tmp[length] = 0;
            if (tmp[0]=='m'&&tmp[1]=='e'&&tmp[2]=='t'&&tmp[3]=='a')
            {
                // found a meta tag...
                const char* end = ptr;
                while(*end != '>' && *end != '\0' && end<pEnd)
                    end++;
                //if ( *end == '\0' ) break;
                QByteArray str( ptr, (end-ptr)+1);
                str = str.toLower();
                int pos=0;
                        //if( (pos = str.find("http-equiv", pos)) == -1) break;
                        //if( (pos = str.find("content-type", pos)) == -1) break;
                if( (pos = str.indexOf("charset")) == -1)
                    continue;
                pos+=6;
                // skip to '='
                if( (pos = str.indexOf("=", pos)) == -1)
                    continue;

                // skip '='
                ++pos;

                // skip whitespace before encoding itself
                while (pos < (int)str.length() && str[pos] <= ' ')
                    ++pos;

                // there may also be an opening quote, if this is a charset= and not
                // a http-equiv.
                if (pos < (int)str.length() && str[pos] == '"')
                    ++pos;

                if ( pos == (int)str.length())
                    continue;

                int endpos = pos;
                while( endpos < str.length() &&
                        (str[endpos] != ' ' && str[endpos] != '"' && str[endpos] != '\''
                                    && str[endpos] != ';' && str[endpos] != '>') )
                    ++endpos;
    #ifdef DECODE_DEBUG
                qDebug() << "KEncodingDetector: found charset in <meta>: " << str.mid(pos,endpos-pos).data();
    #endif
                if (setEncoding(str.mid(pos,endpos-pos).data(), EncodingFromMetaTag))
                    return true;
            }
            else if (tmp[0]=='b'&&tmp[1]=='o'&&tmp[2]=='d'&&tmp[3]=='y')
            {
                d->m_seenBody=true;
                break;
            }
        }
    }

    if (len<20)
        return false;

#ifdef DECODE_DEBUG
    qDebug() << "KEncodingDetector: using heuristics (" << strlen(data) << ")";
#endif

    switch ( d->m_autoDetectLanguage)
    {
        case KEncodingDetector::Arabic:
            return setEncoding(automaticDetectionForArabic( (const unsigned char*) data, len ).data(), AutoDetectedEncoding);
//             break;
        case KEncodingDetector::Baltic:
            return setEncoding(automaticDetectionForBaltic( (const unsigned char*) data, len ).data(), AutoDetectedEncoding);
//             break;
        case KEncodingDetector::CentralEuropean:
            return setEncoding(automaticDetectionForCentralEuropean( (const unsigned char*) data, len ).data(), AutoDetectedEncoding);
//            break;
        case KEncodingDetector::Cyrillic:
            return setEncoding(automaticDetectionForCyrillic( (const unsigned char*) data, len).data(), AutoDetectedEncoding);
//             break;
        case KEncodingDetector::Greek:
            return setEncoding(automaticDetectionForGreek( (const unsigned char*) data, len ).data(), AutoDetectedEncoding);
//             break;
        case KEncodingDetector::Hebrew:
            return setEncoding(automaticDetectionForHebrew( (const unsigned char*) data, len ).data(), AutoDetectedEncoding);
//             break;
        case KEncodingDetector::Japanese:
            return setEncoding(automaticDetectionForJapanese( (const unsigned char*) data, len ).data(), AutoDetectedEncoding);
//             break;
        case KEncodingDetector::Turkish:
            return setEncoding(automaticDetectionForTurkish( (const unsigned char*) data, len ).data(), AutoDetectedEncoding);
//             break;
        case KEncodingDetector::WesternEuropean:
            if (setEncoding(automaticDetectionForWesternEuropean( (const unsigned char*) data, len ).data(), AutoDetectedEncoding))
                return true;
            else if (d->m_defaultCodec->mibEnum()==MibLatin1) //detection for khtml
            {
                return setEncoding("iso-8859-15",AutoDetectedEncoding);
            }
            else //use default provided by eg katepart
            {
                return setEncoding("",DefaultEncoding);
            }
//             break;
        case KEncodingDetector::SemiautomaticDetection:
        case KEncodingDetector::ChineseSimplified:
        case KEncodingDetector::ChineseTraditional:
        case KEncodingDetector::Korean:
        case KEncodingDetector::Thai:
        case KEncodingDetector::Unicode:
        case KEncodingDetector::NorthernSaami:
        case KEncodingDetector::SouthEasternEurope:
        case KEncodingDetector::None:
            // huh. somethings broken in this code ### FIXME
            //enc = 0; //Reset invalid codec we tried, so we get back to latin1 fallback.
            break;
    }

    return true;
}


KEncodingDetector::AutoDetectScript KEncodingDetector::scriptForName(const QString& lang)
{
    if (lang.isEmpty())
        return KEncodingDetector::None;
    else if (lang==i18nc("@item Text character set", "Unicode"))
        return KEncodingDetector::Unicode;
    else if (lang==i18nc("@item Text character set", "Cyrillic"))
        return KEncodingDetector::Cyrillic;
    else if (lang==i18nc("@item Text character set", "Western European"))
        return KEncodingDetector::WesternEuropean;
    else if (lang==i18nc("@item Text character set", "Central European"))
        return KEncodingDetector::CentralEuropean;
    else if (lang==i18nc("@item Text character set", "Greek"))
        return KEncodingDetector::Greek;
    else if (lang==i18nc("@item Text character set", "Hebrew"))
        return KEncodingDetector::Hebrew;
    else if (lang==i18nc("@item Text character set", "Turkish"))
        return KEncodingDetector::Turkish;
    else if (lang==i18nc("@item Text character set", "Japanese"))
        return KEncodingDetector::Japanese;
    else if (lang==i18nc("@item Text character set", "Baltic"))
        return KEncodingDetector::Baltic;
    else if (lang==i18nc("@item Text character set", "Arabic"))
        return KEncodingDetector::Arabic;

    return KEncodingDetector::None;
}

bool KEncodingDetector::hasAutoDetectionForScript(KEncodingDetector::AutoDetectScript script)
{
    switch (script)
    {
        case KEncodingDetector::Arabic:
            return true;
        case KEncodingDetector::Baltic:
            return true;
        case KEncodingDetector::CentralEuropean:
            return true;
        case KEncodingDetector::Cyrillic:
            return true;
        case KEncodingDetector::Greek:
            return true;
        case KEncodingDetector::Hebrew:
            return true;
        case KEncodingDetector::Japanese:
            return true;
        case KEncodingDetector::Turkish:
            return true;
        case KEncodingDetector::WesternEuropean:
            return true;
        case KEncodingDetector::ChineseTraditional:
            return true;
        case KEncodingDetector::ChineseSimplified:
            return true;
        case KEncodingDetector::Unicode:
            return true;
            break;
        default:
            return false;
    }
}

QString KEncodingDetector::nameForScript(KEncodingDetector::AutoDetectScript script)
{
    switch (script)
    {
        case KEncodingDetector::Arabic:
            return i18nc("@item Text character set", "Arabic");
            break;
        case KEncodingDetector::Baltic:
            return i18nc("@item Text character set", "Baltic");
            break;
        case KEncodingDetector::CentralEuropean:
            return i18nc("@item Text character set", "Central European");
            break;
        case KEncodingDetector::Cyrillic:
            return i18nc("@item Text character set", "Cyrillic");
            break;
        case KEncodingDetector::Greek:
            return i18nc("@item Text character set", "Greek");
            break;
        case KEncodingDetector::Hebrew:
            return i18nc("@item Text character set", "Hebrew");
            break;
        case KEncodingDetector::Japanese:
            return i18nc("@item Text character set", "Japanese");
            break;
        case KEncodingDetector::Turkish:
            return i18nc("@item Text character set", "Turkish");
            break;
        case KEncodingDetector::WesternEuropean:
            return i18nc("@item Text character set", "Western European");
            break;
        case KEncodingDetector::ChineseTraditional:
            return i18nc("@item Text character set", "Chinese Traditional");
            break;
        case KEncodingDetector::ChineseSimplified:
            return i18nc("@item Text character set", "Chinese Simplified");
            break;
        case KEncodingDetector::Korean:
            return i18nc("@item Text character set", "Korean");
            break;
        case KEncodingDetector::Thai:
            return i18nc("@item Text character set", "Thai");
            break;
        case KEncodingDetector::Unicode:
            return i18nc("@item Text character set", "Unicode");
            break;
        //case KEncodingDetector::SemiautomaticDetection:
        default:
            return QString();

        }
}

#undef DECODE_DEBUG

