/*
    This file is part of the KDE libraries

    Copyright (C) 1999 Lars Knoll (knoll@mpi-hd.mpg.de)
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
#ifndef KENCODINGDETECTOR_H
#define KENCODINGDETECTOR_H

#include <QtCore/QString>

class QTextCodec;
class QTextDecoder;
class KEncodingDetectorPrivate;

/**
 * @short Provides encoding detection capabilities.
 *
 * Searches for encoding declaration inside raw data -- meta and xml tags. 
 * In the case it can't find it, uses heuristics for specified language.
 *
 * If it finds unicode BOM marks, it changes encoding regardless of what the user has told
 *
 * Intended lifetime of the object: one instance per document.
 *
 * Typical use:
 * \code
 * QByteArray data;
 * ...
 * KEncodingDetector detector;
 * detector.setAutoDetectLanguage(KEncodingDetector::Cyrillic);
 * QString out=detector.decode(data);
 * \endcode
 *
 *
 * Do not mix decode() with decodeWithBuffering()
 *
 * @short Guess encoding of char array
 *
 */
class KEncodingDetector
{
public:
    enum EncodingChoiceSource
    {
        DefaultEncoding,
        AutoDetectedEncoding,
        BOM,
        EncodingFromXMLHeader,
        EncodingFromMetaTag,
        EncodingFromHTTPHeader,
        UserChosenEncoding
    };

    enum AutoDetectScript
    {
        None,
        SemiautomaticDetection,
        Arabic,
        Baltic,
        CentralEuropean,
        ChineseSimplified,
        ChineseTraditional,
        Cyrillic,
        Greek,
        Hebrew,
        Japanese,
        Korean,
        NorthernSaami,
        SouthEasternEurope,
        Thai,
        Turkish,
        Unicode,
        WesternEuropean
    };

    /**
     * Default codec is latin1 (as html spec says), EncodingChoiceSource is default, AutoDetectScript=Semiautomatic
     */
    KEncodingDetector();

    /**
     * Allows to set Default codec, EncodingChoiceSource, AutoDetectScript
     */
    KEncodingDetector(QTextCodec* codec, EncodingChoiceSource source, AutoDetectScript script=None);
    ~KEncodingDetector();

    //const QTextCodec* codec() const;

    /**
    * @returns true if specified encoding was recognized
    */
    bool setEncoding(const char *encoding, EncodingChoiceSource type);

    /**
    * Convenience method.
    * @returns mime name of detected encoding
    */
    const char* encoding() const;

    bool visuallyOrdered() const;

//     void setAutoDetectLanguage( const QString& );
//     const QString& autoDetectLanguage() const;

    void setAutoDetectLanguage( AutoDetectScript );
    AutoDetectScript autoDetectLanguage() const;

    EncodingChoiceSource encodingChoiceSource() const;

    /**
    * The main class method
    *
    * Calls protected analyze() only the first time of the whole object life
    *
    * Replaces all null chars with spaces.
    */
    QString decode(const char *data, int len);
    QString decode(const QByteArray &data);

    //* You don't need to call analyze() if you use this method.
    /**
    * Convenience method that uses buffering. It waits for full html head to be buffered
    * (i.e. calls analyze every time until it returns true).
    *
    * Replaces all null chars with spaces.
    *
    * @returns Decoded data, or empty string, if there was not enough data for accurate detection
    * @see flush()
    */
    QString decodeWithBuffering(const char *data, int len);

    /**
     * This method checks whether invalid characters were found
     * during a decoding operation.
     *
     * Note that this bit is never reset once invalid characters have been found.
     * To force a reset, either change the encoding using setEncoding() or call
     * resetDecoder()
     * 
     * @returns a boolean reflecting said state.
     * @since 4.3
     * @see resetDecoder() setEncoding()
     */    
    bool decodedInvalidCharacters() const;

    /**
     * Resets the decoder. Any stateful decoding information (such as resulting from previous calls
     * to decodeWithBuffering()) will be lost.
     * Will Reset the state of decodedInvalidCharacters() as a side effect.
     *
     * @since 4.3
     * @see decodeWithBuffering() decodedInvalidCharacters()
     *
     */ 
    void resetDecoder();

    /**
    * Convenience method to be used with decodeForHtml. Flushes buffer.
    * @see decodeForHtml()
    */
    QString flush();

    /**
     * Takes lang name _after_ it were i18n()'ed
     */
    static AutoDetectScript scriptForName(const QString& lang);
    static QString nameForScript(AutoDetectScript);
    static bool hasAutoDetectionForScript(AutoDetectScript);

protected:
    /**
     * This nice method will kill all 0 bytes (or double bytes)
     * and remember if this was a binary or not ;)
     */
    bool processNull(char* data,int length);

    /**
     * Check if we are really utf8. Taken from kate
     *
     * @returns true if current encoding is utf8 and the text cannot be in this encoding
     *
     * Please somebody read http://de.wikipedia.org/wiki/UTF-8 and check this code...
     */
    bool errorsIfUtf8 (const char* data, int length);

    /**
    * Analyze text data.
    * @returns true if there was enough data for accurate detection
    */
    bool analyze (const char *data, int len);

    /**
    * @returns QTextDecoder for detected encoding
    */
    QTextDecoder* decoder();

private:
    KEncodingDetectorPrivate* const d;
};

#endif
