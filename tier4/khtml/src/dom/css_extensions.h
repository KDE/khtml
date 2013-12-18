/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright 1999 Lars Knoll (knoll@kde.org)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * This file includes excerpts from the Document Object Model (DOM)
 * Level 2 Specification (Candidate Recommendation)
 * http://www.w3.org/TR/2000/CR-DOM-Level-2-20000510/
 * Copyright © 2000 W3C® (MIT, INRIA, Keio), All Rights Reserved.
 *
 */
#ifndef _CSS_css_extensions_h_
#define _CSS_css_extensions_h_

#include <css_value.h>
#include <dom/dom_string.h>

namespace DOM {

/**
 * The \c CSS2Azimuth interface represents the <a
 * href="http://www.w3.org/TR/REC-CSS2/aural.html#propdef-azimuth">
 * azimuth </a> CSS Level 2 property.
 *
 */
class CSS2Azimuth : public CSSValue
{
public:
    CSS2Azimuth();
    CSS2Azimuth(const CSS2Azimuth &other);
    CSS2Azimuth(CSS2AzimuthImpl *impl);
public:

    CSS2Azimuth & operator = (const CSS2Azimuth &other);

    ~CSS2Azimuth();

    /**
     * A code defining the type of the value as defined in
     * \c CSSValue . It would be one of \c CSS_DEG ,
     * \c CSS_RAD , \c CSS_GRAD or
     * \c CSS_IDENT .
     *
     */
    unsigned short azimuthType() const;

    /**
     * If \c azimuthType is \c CSS_IDENT ,
     * \c identifier contains one of left-side, far-left,
     * left, center-left, center, center-right, right, far-right,
     * right-side, leftwards, rightwards. The empty string if none is
     * set.
     *
     */
    DOM::DOMString identifier() const;

    /**
     * \c behind indicates whether the behind identifier
     * has been set.
     *
     */
    bool behind() const;

    /**
     * A method to set the angle value with a specified unit. This
     * method will unset any previously set identifiers values.
     *
     * @param unitType The unitType could only be one of
     * \c CSS_DEG , \c CSS_RAD or \c CSS_GRAD ).
     *
     * @param floatValue The new float value of the angle.
     *
     * @return
     * @exception DOMException
     * INVALID_ACCESS_ERR: Raised if the unit type is invalid.
     *
     *  NO_MODIFICATION_ALLOWED_ERR: Raised if this property is
     * readonly.
     *
     */
    void setAngleValue ( const unsigned short unitType, const float floatValue );

    /**
     * Used to retrieved the float value of the azimuth property.
     *
     * @param unitType The unit type can be only an angle unit type (
     * \c CSS_DEG , \c CSS_RAD or
     * \c CSS_GRAD ).
     *
     * @return The float value.
     *
     * @exception DOMException
     * INVALID_ACCESS_ERR: Raised if the unit type is invalid.
     *
     */
    float getAngleValue ( const unsigned short unitType );

    /**
     * Setting the identifier for the azimuth property will unset any
     * previously set angle value. The value of \c azimuthType
     * is set to \c CSS_IDENT
     *
     * @param identifier The new identifier. If the identifier is
     * "leftwards" or "rightward", the behind attribute is ignored.
     *
     * @param behind The new value for behind.
     *
     * @return
     * @exception DOMException
     * SYNTAX_ERR: Raised if the specified \c identifier
     * has a syntax error and is unparsable.
     *
     *  NO_MODIFICATION_ALLOWED_ERR: Raised if this property is
     * readonly.
     *
     */
    void setIdentifier ( const DOM::DOMString &identifier, const bool behind );
};


class CSS2BackgroundPositionImpl;

/**
 * The \c CSS2BackgroundPosition interface represents the
 * <a
 * href="http://www.w3.org/TR/REC-CSS2/colors.html#propdef-background-position">
 * background-position </a> CSS Level 2 property.
 *
 */
class CSS2BackgroundPosition : public CSSValue
{
public:
    CSS2BackgroundPosition();
    CSS2BackgroundPosition(const CSS2BackgroundPosition &other);
    CSS2BackgroundPosition(CSS2BackgroundPositionImpl *impl);
public:

    CSS2BackgroundPosition & operator = (const CSS2BackgroundPosition &other);

    ~CSS2BackgroundPosition();

    /**
     * A code defining the type of the horizontal value. It would be
     * one \c CSS_PERCENTAGE , \c CSS_EMS ,
     * \c CSS_EXS , \c CSS_PX , \c CSS_CM ,
     * \c CSS_MM , \c CSS_IN ,
     * \c CSS_PT , \c CSS_PC ,
     * \c CSS_IDENT , \c CSS_INHERIT . If one of
     * horizontal or vertical is \c CSS_IDENT or
     * \c CSS_INHERIT , it's guaranteed that the other is the
     * same.
     *
     */
    unsigned short horizontalType() const;

    /**
     * A code defining the type of the horizontal value. The code can
     * be one of the following units : \c CSS_PERCENTAGE ,
     * \c CSS_EMS , \c CSS_EXS , \c CSS_PX
     *  , \c CSS_CM , \c CSS_MM ,
     * \c CSS_IN , \c CSS_PT , \c CSS_PC
     * , \c CSS_IDENT , \c CSS_INHERIT
     * . If one of horizontal or vertical is \c CSS_IDENT
     * or \c CSS_INHERIT , it's guaranteed that the other
     * is the same.
     *
     */
    unsigned short verticalType() const;

    /**
     * If \c horizontalType is \c CSS_IDENT or
     * \c CSS_INHERIT , this attribute contains the string
     * representation of the ident, otherwise it contains an empty
     * string.
     *
     */
    DOM::DOMString horizontalIdentifier() const;

    /**
     * If \c verticalType is \c CSS_IDENT or
     * \c CSS_INHERIT , this attribute contains the string
     * representation of the ident, otherwise it contains an empty
     * string. The value is \c "center" if only the
     * horizontalIdentifier has been set. The value is
     * \c "inherit" if the horizontalIdentifier is
     * \c "inherit" .
     *
     */
    DOM::DOMString verticalIdentifier() const;

    /**
     * This method is used to get the float value in a specified unit
     * if the \c horizontalPosition represents a length or
     * a percentage. If the float doesn't contain a float value or
     * can't be converted into the specified unit, a
     * \c DOMException is raised.
     *
     * @param horizontalType The specified unit.
     *
     * @return The float value.
     *
     * @exception DOMException
     * INVALID_ACCESS_ERR: Raises if the property doesn't contain a
     * float or the value can't be converted.
     *
     */
    float getHorizontalPosition ( const float horizontalType );

    /**
     * This method is used to get the float value in a specified unit
     * if the \c verticalPosition represents a length or a
     * percentage. If the float doesn't contain a float value or can't
     * be converted into the specified unit, a \c DOMException
     * is raised. The value is \c 50% if only the
     * horizontal value has been specified.
     *
     * @param verticalType The specified unit.
     *
     * @return The float value.
     *
     * @exception DOMException
     * INVALID_ACCESS_ERR: Raises if the property doesn't contain a
     * float or the value can't be converted.
     *
     */
    float getVerticalPosition ( const float verticalType );

    /**
     * This method is used to set the horizontal position with a
     * specified unit. If the vertical value is not a percentage or a
     * length, it sets the vertical position to \c 50% .
     *
     * @param horizontalType The specified unit (a length or a
     * percentage).
     *
     * @param value The new value.
     *
     * @return
     * @exception DOMException
     * INVALID_ACCESS_ERR: Raises if the specified unit is not a
     * length or a percentage.
     *
     *  NO_MODIFICATION_ALLOWED_ERR: Raises if this property is
     * readonly.
     *
     */
    void setHorizontalPosition ( const unsigned short horizontalType, const float value );

    /**
     * This method is used to set the vertical position with a
     * specified unit. If the horizontal value is not a percentage or
     * a length, it sets the vertical position to \c 50% .
     *
     * @param verticalType The specified unit (a length or a
     * percentage).
     *
     * @param value The new value.
     *
     * @return
     * @exception DOMException
     * INVALID_ACCESS_ERR: Raises if the specified unit is not a
     * length or a percentage.
     *
     *  NO_MODIFICATION_ALLOWED_ERR: Raises if this property is
     * readonly.
     *
     */
    void setVerticalPosition ( const unsigned short verticalType, const float value );

    /**
     * Sets the identifiers. If the second identifier is the empty
     * string, the vertical identifier is set to his default value (
     * \c "center" ). If the first identfier is
     * \c "inherit , the second identifier is ignored and is set
     * to \c "inherit" .
     *
     * @param horizontalIdentifier The new horizontal identifier.
     *
     * @param verticalIdentifier The new vertical identifier.
     *
     * @return
     * @exception DOMException
     * SYNTAX_ERR: Raises if the identifiers have a syntax error and
     * is unparsable.
     *
     *  NO_MODIFICATION_ALLOWED_ERR: Raises if this property is
     * readonly.
     *
     */
    void setPositionIdentifier ( const DOM::DOMString &horizontalIdentifier, const DOM::DOMString &verticalIdentifier );
};


class CSS2BorderSpacingImpl;

/**
 * The \c CSS2BorderSpacing interface represents the <a
 * href="http://www.w3.org/TR/REC-CSS2/tables.html#propdef-border-spacing">
 * border-spacing </a> CSS Level 2 property.
 *
 */
class CSS2BorderSpacing : public CSSValue
{
public:
    CSS2BorderSpacing();
    CSS2BorderSpacing(const CSS2BorderSpacing &other);
    CSS2BorderSpacing(CSS2BorderSpacingImpl *impl);
public:

    CSS2BorderSpacing & operator = (const CSS2BorderSpacing &other);

    ~CSS2BorderSpacing();

    /**
     * The A code defining the type of the value as defined in
     * \c CSSValue . It would be one of \c CSS_EMS ,
     * \c CSS_EXS , \c CSS_PX , \c CSS_CM
     * , \c CSS_MM , \c CSS_IN ,
     * \c CSS_PT , \c CSS_PC or
     * \c CSS_INHERIT .
     *
     */
    unsigned short horizontalType() const;

    /**
     * The A code defining the type of the value as defined in
     * \c CSSValue . It would be one of \c CSS_EMS ,
     * \c CSS_EXS , \c CSS_PX , \c CSS_CM
     *  , \c CSS_MM , \c CSS_IN ,
     * \c CSS_PT , \c CSS_PC or
     * \c CSS_INHERIT .
     *
     */
    unsigned short verticalType() const;

    /**
     * This method is used to get the float value in a specified unit
     * if the \c horizontalSpacing represents a length. If
     * the float doesn't contain a float value or can't be converted
     * into the specified unit, a \c DOMException is
     * raised.
     *
     * @param horizontalType The specified unit.
     *
     * @return The float value.
     *
     * @exception DOMException
     * INVALID_ACCESS_ERR: Raises if the property doesn't contain a
     * float or the value can't be converted.
     *
     */
    float getHorizontalSpacing ( const float horizontalType );

    /**
     * This method is used to get the float value in a specified unit
     * if the \c verticalSpacing represents a length. If
     * the float doesn't contain a float value or can't be converted
     * into the specified unit, a \c DOMException is
     * raised. The value is \c 0 if only the horizontal
     * value has been specified.
     *
     * @param verticalType The specified unit.
     *
     * @return The float value.
     *
     * @exception DOMException
     * INVALID_ACCESS_ERR: Raises if the property doesn't contain a
     * float or the value can't be converted.
     *
     */
    float getVerticalSpacing ( const float verticalType );

    /**
     * This method is used to set the horizontal spacing with a
     * specified unit. If the vertical value is a length, it sets the
     * vertical spacing to \c 0 .
     *
     * @param horizontalType The specified unit.
     *
     * @param value The new value.
     *
     * @return
     * @exception DOMException
     * INVALID_ACCESS_ERR: Raises if the specified unit is not a
     * length.
     *
     *  NO_MODIFICATION_ALLOWED_ERR: Raises if this property is
     * readonly.
     *
     */
    void setHorizontalSpacing ( const unsigned short horizontalType, const float value );

    /**
     * This method is used to set the vertical spacing with a
     * specified unit. If the horizontal value is not a length, it
     * sets the vertical spacing to \c 0 .
     *
     * @param verticalType The specified unit.
     *
     * @param value The new value.
     *
     * @return
     * @exception DOMException
     * INVALID_ACCESS_ERR: Raises if the specified unit is not a
     * length or a percentage.
     *
     *  NO_MODIFICATION_ALLOWED_ERR: Raises if this property is
     * readonly.
     *
     */
    void setVerticalSpacing ( const unsigned short verticalType, const float value );

    /**
     * Set this property as inherit. \c horizontalType and
     * \c verticalType will be inherited.
     *
     * @return
     */
    void setInherit();
};


class CSS2CounterIncrementImpl;

/**
 * The \c CSS2CounterIncrement interface represents a
 * imple value for the <a
 * href="http://www.w3.org/TR/REC-CSS2/generate.html#propdef-counter-increment">
 * counter-increment </a> CSS Level 2 property.
 *
 */
class CSS2CounterIncrement
{
public:
    CSS2CounterIncrement();
    CSS2CounterIncrement(const CSS2CounterIncrement &other);
    CSS2CounterIncrement(CSS2CounterIncrementImpl *impl);
public:

    CSS2CounterIncrement & operator = (const CSS2CounterIncrement &other);

    ~CSS2CounterIncrement();

    /**
     * The element name.
     *
     */
    DOM::DOMString identifier() const;

    /**
     * see identifier
     * @exception DOMException
     * SYNTAX_ERR: Raised if the specified identifier has a syntax
     * error and is unparsable.
     *
     *  NO_MODIFICATION_ALLOWED_ERR: Raised if this identifier is
     * readonly.
     *
     */
    void setIdentifier( const DOM::DOMString & );

    /**
     * The increment (default value is 1).
     *
     */
    short increment() const;

    /**
     * see increment
     * @exception DOMException
     * NO_MODIFICATION_ALLOWED_ERR: Raised if this identifier is
     * readonly.
     *
     */
    void setIncrement( const short  );
};


class CSS2CounterResetImpl;

/**
 * The \c CSS2CounterReset interface represents a simple
 * value for the <a
 * href="http://www.w3.org/TR/REC-CSS2/generate.html#propdef-counter-reset">
 * counter-reset </a> CSS Level 2 property.
 *
 */
class CSS2CounterReset
{
public:
    CSS2CounterReset();
    CSS2CounterReset(const CSS2CounterReset &other);
    CSS2CounterReset(CSS2CounterResetImpl *impl);
public:

    CSS2CounterReset & operator = (const CSS2CounterReset &other);

    ~CSS2CounterReset();

    /**
     * The element name.
     *
     */
    DOM::DOMString identifier() const;

    /**
     * see identifier
     * @exception DOMException
     * SYNTAX_ERR: Raised if the specified identifier has a syntax
     * error and is unparsable.
     *
     *  NO_MODIFICATION_ALLOWED_ERR: Raised if this identifier is
     * readonly.
     *
     */
    void setIdentifier( const DOM::DOMString & );

    /**
     * The reset (default value is 0).
     *
     */
    short reset() const;

    /**
     * see reset
     * @exception DOMException
     * NO_MODIFICATION_ALLOWED_ERR: Raised if this identifier is
     * readonly.
     *
     */
    void setReset( const short  );
};


class CSS2CursorImpl;
class CSSValueList;

/**
 * The \c CSS2Cursor interface represents the <a
 * href="http://www.w3.org/TR/REC-CSS2/ui.html#propdef-cursor"> cursor
 * </a> CSS Level 2 property.
 *
 */
class CSS2Cursor : public CSSValue
{
public:
    CSS2Cursor();
    CSS2Cursor(const CSS2Cursor &other);
    CSS2Cursor(CSS2CursorImpl *impl);
public:

    CSS2Cursor & operator = (const CSS2Cursor &other);

    ~CSS2Cursor();

    /**
     * A code defining the type of the property. It would one of
     * \c CSS_UNKNOWN or \c CSS_INHERIT . If
     * the type is \c CSS_UNKNOWN , then \c uris
     * contains a list of URIs and \c predefinedCursor
     * contains an ident. Setting this attribute from
     * \c CSS_INHERIT to \c CSS_UNKNOWN will set the
     * \c predefinedCursor to \c "auto" .
     *
     */
    unsigned short cursorType() const;

    /**
     * see cursorType
     */
    void setCursorType( const unsigned short  );

    /**
     * \c uris represents the list of URIs (
     * \c CSS_URI ) on the cursor property. The list can be
     * empty.
     *
     */
    CSSValueList uris() const;

    /**
     * This identifier represents a generic cursor name or an empty
     * string.
     *
     */
    DOM::DOMString predefinedCursor() const;

    /**
     * see predefinedCursor
     * @exception DOMException
     * SYNTAX_ERR: Raised if the specified CSS string value has a
     * syntax error and is unparsable.
     *
     *  NO_MODIFICATION_ALLOWED_ERR: Raised if this declaration is
     * readonly.
     *
     */
    void setPredefinedCursor( const DOM::DOMString & );
};


class CSS2FontFaceSrcImpl;
class CSSValueList;

/**
 * The \c CSS2Cursor interface represents the <a
 * href="http://www.w3.org/TR/REC-CSS2/fonts.html#descdef-src"> src
 * </a> CSS Level 2 descriptor.
 *
 */
class CSS2FontFaceSrc
{
public:
    CSS2FontFaceSrc();
    CSS2FontFaceSrc(const CSS2FontFaceSrc &other);
    CSS2FontFaceSrc(CSS2FontFaceSrcImpl *impl);
public:

    CSS2FontFaceSrc & operator = (const CSS2FontFaceSrc &other);

    ~CSS2FontFaceSrc();

    /**
     * Specifies the source of the font, empty string otherwise.
     *
     */
    DOM::DOMString uri() const;

    /**
     * see uri
     * @exception DOMException
     * SYNTAX_ERR: Raised if the specified CSS string value has a
     * syntax error and is unparsable.
     *
     *  NO_MODIFICATION_ALLOWED_ERR: Raised if this declaration is
     * readonly.
     *
     */
    void setUri( const DOM::DOMString & );

    /**
     * This attribute contains a list of strings for the format CSS
     * function.
     *
     */
    CSSValueList format() const;

    /**
     * Specifies the full font name of a locally installed font.
     *
     */
    DOM::DOMString fontFaceName() const;

    /**
     * see fontFaceName
     * @exception DOMException
     * SYNTAX_ERR: Raised if the specified CSS string value has a
     * syntax error and is unparsable.
     *
     *  NO_MODIFICATION_ALLOWED_ERR: Raised if this declaration is
     * readonly.
     *
     */
    void setFontFaceName( const DOM::DOMString & );
};


class CSS2FontFaceWidthsImpl;
class CSSValueList;

/**
 * The \c CSS2Cursor interface represents a simple value
 * for the <a
 * href="http://www.w3.org/TR/REC-CSS2/fonts.html#descdef-widths">
 * widths </a> CSS Level 2 descriptor.
 *
 */
class CSS2FontFaceWidths
{
public:
    CSS2FontFaceWidths();
    CSS2FontFaceWidths(const CSS2FontFaceWidths &other);
    CSS2FontFaceWidths(CSS2FontFaceWidthsImpl *impl);
public:

    CSS2FontFaceWidths & operator = (const CSS2FontFaceWidths &other);

    ~CSS2FontFaceWidths();

    /**
     * The range for the characters.
     *
     */
    DOM::DOMString urange() const;

    /**
     * see urange
     * @exception DOMException
     * SYNTAX_ERR: Raised if the specified CSS string value has a
     * syntax error and is unparsable.
     *
     *  NO_MODIFICATION_ALLOWED_ERR: Raised if this declaration is
     * readonly.
     *
     */
    void setUrange( const DOM::DOMString & );

    /**
     * A list of numbers representing the glyph widths.
     *
     */
    CSSValueList numbers() const;
};


class CSS2PageSizeImpl;

/**
 * The \c CSS2Cursor interface represents the <a
 * href="http://www.w3.org/TR/REC-CSS2/page.html#propdef-size"> size
 * </a> CSS Level 2 descriptor.
 *
 */
class CSS2PageSize : public CSSValue
{
public:
    CSS2PageSize();
    CSS2PageSize(const CSS2PageSize &other);
    CSS2PageSize(CSS2PageSizeImpl *impl);
public:

    CSS2PageSize & operator = (const CSS2PageSize &other);

    ~CSS2PageSize();

    /**
     * A code defining the type of the width of the page. It would be
     * one of \c CSS_EMS , \c CSS_EXS ,
     * \c CSS_PX , \c CSS_CM , \c CSS_MM
     * , \c CSS_IN , \c CSS_PT , \c CSS_PC
     * , \c CSS_IDENT , \c CSS_INHERIT
     * . If one of width or height is \c CSS_IDENT or
     * \c CSS_INHERIT , it's guaranteed that the other is
     * the same.
     *
     */
    unsigned short widthType() const;

    /**
     * A code defining the type of the height of the page. It would be
     * one of \c CSS_EMS , \c CSS_EXS ,
     * \c CSS_PX , \c CSS_CM , \c CSS_MM
     * , \c CSS_IN , \c CSS_PT , \c CSS_PC
     * , \c CSS_IDENT , \c CSS_INHERIT
     * . If one of width or height is \c CSS_IDENT or
     * \c CSS_INHERIT , it's guaranteed that the other is
     * the same.
     *
     */
    unsigned short heightType() const;

    /**
     * If \c width is \c CSS_IDENT or
     * \c CSS_INHERIT , this attribute contains the string
     * representation of the ident, otherwise it contains an empty
     * string.
     *
     */
    DOM::DOMString identifier() const;

    /**
     * This method is used to get the float value in a specified unit
     * if the \c widthType represents a length. If the
     * float doesn't contain a float value or can't be converted into
     * the specified unit, a \c DOMException is raised.
     *
     * @param widthType The specified unit.
     *
     * @return The float value.
     *
     * @exception DOMException
     * INVALID_ACCESS_ERR: Raises if the property doesn't contain a
     * float or the value can't be converted.
     *
     */
    float getWidth ( const float widthType );

    /**
     * This method is used to get the float value in a specified unit
     * if the \c heightType represents a length. If the
     * float doesn't contain a float value or can't be converted into
     * the specified unit, a \c DOMException is raised. If
     * only the width value has been specified, the height value is
     * the same.
     *
     * @param heightType The specified unit.
     *
     * @return The float value.
     *
     * @exception DOMException
     * INVALID_ACCESS_ERR: Raises if the property doesn't contain a
     * float or the value can't be converted.
     *
     */
    float getHeightSize ( const float heightType );

    /**
     * This method is used to set the width position with a specified
     * unit. If the \c heightType is not a length, it sets
     * the height position to the same value.
     *
     * @param widthType The specified unit.
     *
     * @param value The new value.
     *
     * @return
     * @exception DOMException
     * INVALID_ACCESS_ERR: Raises if the specified unit is not a
     * length or a percentage.
     *
     *  NO_MODIFICATION_ALLOWED_ERR: Raises if this property is
     * readonly.
     *
     */
    void setWidthSize ( const unsigned short widthType, const float value );

    /**
     * This method is used to set the height position with a specified
     * unit. If the \c widthType is not a length, it sets
     * the width position to the same value.
     *
     * @param heightType The specified unit.
     *
     * @param value The new value.
     *
     * @return
     * @exception DOMException
     * INVALID_ACCESS_ERR: Raises if the specified unit is not a
     * length or a percentage.
     *
     *  NO_MODIFICATION_ALLOWED_ERR: Raises if this property is
     * readonly.
     *
     */
    void setHeightSize ( const unsigned short heightType, const float value );

    /**
     * Sets the identifier.
     *
     * @param identifier The new identifier.
     *
     * @return
     * @exception DOMException
     * SYNTAX_ERR: Raises if the identifier has a syntax error and is
     * unparsable.
     *
     *  NO_MODIFICATION_ALLOWED_ERR: Raises if this property is
     * readonly.
     *
     */
    void setIdentifier ( const DOM::DOMString &identifier );
};


class CSS2PlayDuringImpl;

/**
 * The \c CSS2PlayDuring interface represents the <a
 * href="http://www.w3.org/TR/REC-CSS2/aural.html#propdef-play-during">
 * play-during </a> CSS Level 2 property.
 *
 */
class CSS2PlayDuring : public CSSValue
{
public:
    CSS2PlayDuring();
    CSS2PlayDuring(const CSS2PlayDuring &other);
    CSS2PlayDuring(CSS2PlayDuringImpl *impl);
public:

    CSS2PlayDuring & operator = (const CSS2PlayDuring &other);

    ~CSS2PlayDuring();

    /**
     * A code defining the type of the value as define in
     * \c CSSvalue . It would be one of \c CSS_UNKNOWN
     * , \c CSS_INHERIT , \c CSS_IDENT
     *
     */
    unsigned short playDuringType() const;

    /**
     * One of \c "inherit" , \c "auto" ,
     * \c "none" or the empty string if the
     * \c playDuringType is \c CSS_UNKNOWN . On
     * setting, it will set the \c uri to the empty string
     * and \c mix and \c repeat to
     * \c false .
     *
     */
    DOM::DOMString playDuringIdentifier() const;

    /**
     * see playDuringIdentifier
     * @exception DOMException
     * SYNTAX_ERR: Raised if the specified CSS string value has a
     * syntax error and is unparsable.
     *
     *  NO_MODIFICATION_ALLOWED_ERR: Raised if this declaration is
     * readonly.
     *
     */
    void setPlayDuringIdentifier( const DOM::DOMString & );

    /**
     * The sound specified by the \c uri . It will set the
     * \c playDuringType attribute to \c CSS_UNKNOWN .
     *
     */
    DOM::DOMString uri() const;

    /**
     * see uri
     * @exception DOMException
     * SYNTAX_ERR: Raised if the specified CSS string value has a
     * syntax error and is unparsable.
     *
     *  NO_MODIFICATION_ALLOWED_ERR: Raised if this declaration is
     * readonly.
     *
     */
    void setUri( const DOM::DOMString & );

    /**
     * \c true if the sound should be mixed. It will be
     * ignored if the attribute doesn't contain a \c uri .
     *
     */
    bool mix() const;

    /**
     * see mix
     * @exception DOMException
     * NO_MODIFICATION_ALLOWED_ERR: Raised if this declaration is
     * readonly.
     *
     */
    void setMix( const bool  );

    /**
     * \c true if the sound should be repeated. It will be
     * ignored if the attribute doesn't contain a \c uri .
     *
     */
    bool repeat() const;

    /**
     * see repeat
     * @exception DOMException
     * NO_MODIFICATION_ALLOWED_ERR: Raised if this declaration is
     * readonly.
     *
     */
    void setRepeat( const bool );
};


class CSS2PropertiesImpl;

/**
 * The \c CSS2Properties interface represents a
 * convenience mechanism for retrieving and setting properties within
 * a \c CSSStyleDeclaration . The attributes of this
 * interface correspond to all the <a
 * href="http://www.w3.org/TR/REC-CSS2/propidx.html"> properties
 * specified in CSS2 </a> . Getting an attribute of this interface is
 * equivalent to calling the \c getPropertyValue method of
 * the \c CSSStyleDeclaration interface. Setting an
 * attribute of this interface is equivalent to calling the
 * \c setProperty method of the \c CSSStyleDeclaration
 * interface.
 *
 *  A compliant implementation is not required to implement the
 * \c CSS2Properties interface. If an implementation does
 * implement this interface, the expectation is that language-specific
 * methods can be used to cast from an instance of the
 * \c CSSStyleDeclaration interface to the \c CSS2Properties
 * interface.
 *
 *  If an implementation does implement this interface, it is expected
 * to understand the specific syntax of the shorthand properties, and
 * apply their semantics; when the \c margin property is
 * set, for example, the \c marginTop , \c marginRight
 * , \c marginBottom and \c marginLeft
 * properties are actually being set by the underlying implementation.
 *
 *  When dealing with CSS "shorthand" properties, the shorthand
 * properties should be decomposed into their component longhand
 * properties as appropriate, and when querying for their value, the
 * form returned should be the shortest form exactly equivalent to the
 * declarations made in the ruleset. However, if there is no shorthand
 * declaration that could be added to the ruleset without changing in
 * any way the rules already declared in the ruleset (i.e., by adding
 * longhand rules that were previously not declared in the ruleset),
 * then the empty string should be returned for the shorthand
 * property.
 *
 *  For example, querying for the \c font property should
 * not return "normal normal normal 14pt/normal Arial, sans-serif",
 * when "14pt Arial, sans-serif" suffices (the normals are initial
 * values, and are implied by use of the longhand property).
 *
 *  If the values for all the longhand properties that compose a
 * particular string are the initial values, then a string consisting
 * of all the initial values should be returned (e.g. a
 * \c border-width value of "medium" should be returned as such,
 * not as "").
 *
 *  For some shorthand properties that take missing values from other
 * sides, such as the \c margin , \c padding ,
 * and \c border-[width|style|color] properties, the
 * minimum number of sides possible should be used, i.e., "0px 10px"
 * will be returned instead of "0px 10px 0px 10px".
 *
 *  If the value of a shorthand property can not be decomposed into
 * its component longhand properties, as is the case for the
 * \c font property with a value of "menu", querying for the
 * values of the component longhand properties should return the empty
 * string.
 *
 */
class CSS2Properties
{
public:
    CSS2Properties();
    CSS2Properties(const CSS2Properties &other);
    CSS2Properties(CSS2PropertiesImpl *impl);
public:

    CSS2Properties & operator = (const CSS2Properties &other);

    ~CSS2Properties();

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/aural.html#propdef-azimuth">
     * azimuth property definition </a> in CSS2.
     *
     */
    DOM::DOMString azimuth() const;

    /**
     * see azimuth
     */
    void setAzimuth( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/colors.html#propdef-background">
     * background property definition </a> in CSS2.
     *
     */
    DOM::DOMString background() const;

    /**
     * see background
     */
    void setBackground( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/colors.html#propdef-background-attachment">
     * background-attachment property definition </a> in CSS2.
     *
     */
    DOM::DOMString backgroundAttachment() const;

    /**
     * see backgroundAttachment
     */
    void setBackgroundAttachment( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/colors.html#propdef-background-color">
     * background-color property definition </a> in CSS2.
     *
     */
    DOM::DOMString backgroundColor() const;

    /**
     * see backgroundColor
     */
    void setBackgroundColor( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/colors.html#propdef-background-image">
     * background-image property definition </a> in CSS2.
     *
     */
    DOM::DOMString backgroundImage() const;

    /**
     * see backgroundImage
     */
    void setBackgroundImage( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/colors.html#propdef-background-position">
     * background-position property definition </a> in CSS2.
     *
     */
    DOM::DOMString backgroundPosition() const;

    /**
     * see backgroundPosition
     */
    void setBackgroundPosition( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/colors.html#propdef-background-repeat">
     * background-repeat property definition </a> in CSS2.
     *
     */
    DOM::DOMString backgroundRepeat() const;

    /**
     * see backgroundRepeat
     */
    void setBackgroundRepeat( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/box.html#propdef-border">
     * border property definition </a> in CSS2.
     *
     */
    DOM::DOMString border() const;

    /**
     * see border
     */
    void setBorder( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/tables.html#propdef-border-collapse">
     * border-collapse property definition </a> in CSS2.
     *
     */
    DOM::DOMString borderCollapse() const;

    /**
     * see borderCollapse
     */
    void setBorderCollapse( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/box.html#propdef-border-color">
     * border-color property definition </a> in CSS2.
     *
     */
    DOM::DOMString borderColor() const;

    /**
     * see borderColor
     */
    void setBorderColor( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/tables.html#propdef-border-spacing">
     * border-spacing property definition </a> in CSS2.
     *
     */
    DOM::DOMString borderSpacing() const;

    /**
     * see borderSpacing
     */
    void setBorderSpacing( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/box.html#propdef-border-style">
     * border-style property definition </a> in CSS2.
     *
     */
    DOM::DOMString borderStyle() const;

    /**
     * see borderStyle
     */
    void setBorderStyle( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/box.html#propdef-border-top">
     * border-top property definition </a> in CSS2.
     *
     */
    DOM::DOMString borderTop() const;

    /**
     * see borderTop
     */
    void setBorderTop( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/box.html#propdef-border-right">
     * border-right property definition </a> in CSS2.
     *
     */
    DOM::DOMString borderRight() const;

    /**
     * see borderRight
     */
    void setBorderRight( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/box.html#propdef-border-bottom">
     * border-bottom property definition </a> in CSS2.
     *
     */
    DOM::DOMString borderBottom() const;

    /**
     * see borderBottom
     */
    void setBorderBottom( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/box.html#propdef-border-left">
     * border-left property definition </a> in CSS2.
     *
     */
    DOM::DOMString borderLeft() const;

    /**
     * see borderLeft
     */
    void setBorderLeft( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/box.html#propdef-border-top-color">
     * border-top-color property definition </a> in CSS2.
     *
     */
    DOM::DOMString borderTopColor() const;

    /**
     * see borderTopColor
     */
    void setBorderTopColor( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/box.html#propdef-border-right-color">
     * border-right-color property definition </a> in CSS2.
     *
     */
    DOM::DOMString borderRightColor() const;

    /**
     * see borderRightColor
     */
    void setBorderRightColor( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/#propdef-border-bottom-color">
     * border-bottom-color property definition </a> in CSS2.
     *
     */
    DOM::DOMString borderBottomColor() const;

    /**
     * see borderBottomColor
     */
    void setBorderBottomColor( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/box.html#propdef-border-left-color">
     * border-left-color property definition </a> in CSS2.
     *
     */
    DOM::DOMString borderLeftColor() const;

    /**
     * see borderLeftColor
     */
    void setBorderLeftColor( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/box.html#propdef-border-top-style">
     * border-top-style property definition </a> in CSS2.
     *
     */
    DOM::DOMString borderTopStyle() const;

    /**
     * see borderTopStyle
     */
    void setBorderTopStyle( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/box.html#propdef-border-right-style">
     * border-right-style property definition </a> in CSS2.
     *
     */
    DOM::DOMString borderRightStyle() const;

    /**
     * see borderRightStyle
     */
    void setBorderRightStyle( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/box.html#propdef-border-bottom-style">
     * border-bottom-style property definition </a> in CSS2.
     *
     */
    DOM::DOMString borderBottomStyle() const;

    /**
     * see borderBottomStyle
     */
    void setBorderBottomStyle( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/box.html#propdef-border-left-style">
     * border-left-style property definition </a> in CSS2.
     *
     */
    DOM::DOMString borderLeftStyle() const;

    /**
     * see borderLeftStyle
     */
    void setBorderLeftStyle( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/box.html#propdef-border-top-width">
     * border-top-width property definition </a> in CSS2.
     *
     */
    DOM::DOMString borderTopWidth() const;

    /**
     * see borderTopWidth
     */
    void setBorderTopWidth( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/box.html#propdef-border-right-width">
     * border-right-width property definition </a> in CSS2.
     *
     */
    DOM::DOMString borderRightWidth() const;

    /**
     * see borderRightWidth
     */
    void setBorderRightWidth( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/box.html#propdef-border-bottom-width">
     * border-bottom-width property definition </a> in CSS2.
     *
     */
    DOM::DOMString borderBottomWidth() const;

    /**
     * see borderBottomWidth
     */
    void setBorderBottomWidth( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/box.html#propdef-border-left-width">
     * border-left-width property definition </a> in CSS2.
     *
     */
    DOM::DOMString borderLeftWidth() const;

    /**
     * see borderLeftWidth
     */
    void setBorderLeftWidth( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/box.html#propdef-border-width">
     * border-width property definition </a> in CSS2.
     *
     */
    DOM::DOMString borderWidth() const;

    /**
     * see borderWidth
     */
    void setBorderWidth( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/visuren.html#propdef-bottom">
     * bottom property definition </a> in CSS2.
     *
     */
    DOM::DOMString bottom() const;

    /**
     * see bottom
     */
    void setBottom( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/tables.html#propdef-caption-side">
     * caption-side property definition </a> in CSS2.
     *
     */
    DOM::DOMString captionSide() const;

    /**
     * see captionSide
     */
    void setCaptionSide( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/visuren.html#propdef-clear">
     * clear property definition </a> in CSS2.
     *
     */
    DOM::DOMString clear() const;

    /**
     * see clear
     */
    void setClear( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/visufx#propdef-clip"> clip
     * property definition </a> in CSS2.
     *
     */
    DOM::DOMString clip() const;

    /**
     * see clip
     */
    void setClip( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/colors.html#propdef-color">
     * color property definition </a> in CSS2.
     *
     */
    DOM::DOMString color() const;

    /**
     * see color
     */
    void setColor( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/generate.html#propdef-content">
     * content property definition </a> in CSS2.
     *
     */
    DOM::DOMString content() const;

    /**
     * see content
     */
    void setContent( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/generate.html#propdef-counter-increment">
     * counter-increment property definition </a> in CSS2.
     *
     */
    DOM::DOMString counterIncrement() const;

    /**
     * see counterIncrement
     */
    void setCounterIncrement( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/generate.html#propdef-counter-reset">
     * counter-reset property definition </a> in CSS2.
     *
     */
    DOM::DOMString counterReset() const;

    /**
     * see counterReset
     */
    void setCounterReset( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/aural.html#propdef-cue">
     * cue property definition </a> in CSS2.
     *
     */
    DOM::DOMString cue() const;

    /**
     * see cue
     */
    void setCue( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/aural.html#propdef-cue-fter">
     * cue-after property definition </a> in CSS2.
     *
     */
    DOM::DOMString cueAfter() const;

    /**
     * see cueAfter
     */
    void setCueAfter( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/aural.html#propdef-cue-before">
     * cue-before property definition </a> in CSS2.
     *
     */
    DOM::DOMString cueBefore() const;

    /**
     * see cueBefore
     */
    void setCueBefore( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/ui.html#propdef-cursor">
     * cursor property definition </a> in CSS2.
     *
     */
    DOM::DOMString cursor() const;

    /**
     * see cursor
     */
    void setCursor( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/visuren.html#propdef-direction">
     * direction property definition </a> in CSS2.
     *
     */
    DOM::DOMString direction() const;

    /**
     * see direction
     */
    void setDirection( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/visuren.html#propdef-display">
     * display property definition </a> in CSS2.
     *
     */
    DOM::DOMString display() const;

    /**
     * see display
     */
    void setDisplay( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/aural.html#propdef-elevation">
     * elevation property definition </a> in CSS2.
     *
     */
    DOM::DOMString elevation() const;

    /**
     * see elevation
     */
    void setElevation( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/tables.html#propdef-empty-cells">
     * empty-cells property definition </a> in CSS2.
     *
     */
    DOM::DOMString emptyCells() const;

    /**
     * see emptyCells
     */
    void setEmptyCells( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/visuren.html#propdef-float">
     * float property definition </a> in CSS2.
     *
     */
    DOM::DOMString cssFloat() const;

    /**
     * see cssFloat
     */
    void setCssFloat( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/fonts.html#propdef-font">
     * font property definition </a> in CSS2.
     *
     */
    DOM::DOMString font() const;

    /**
     * see font
     */
    void setFont( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/fonts.html#propdef-font-family">
     * font-family property definition </a> in CSS2.
     *
     */
    DOM::DOMString fontFamily() const;

    /**
     * see fontFamily
     */
    void setFontFamily( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/fonts.html#propdef-font-size">
     * font-size property definition </a> in CSS2.
     *
     */
    DOM::DOMString fontSize() const;

    /**
     * see fontSize
     */
    void setFontSize( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/fonts.html#propdef-font-size-adjust">
     * font-size-adjust property definition </a> in CSS2.
     *
     */
    DOM::DOMString fontSizeAdjust() const;

    /**
     * see fontSizeAdjust
     */
    void setFontSizeAdjust( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/fonts.html#propdef-font-stretch">
     * font-stretch property definition </a> in CSS2.
     *
     */
    DOM::DOMString fontStretch() const;

    /**
     * see fontStretch
     */
    void setFontStretch( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/fonts.html#propdef-font-style">
     * font-style property definition </a> in CSS2.
     *
     */
    DOM::DOMString fontStyle() const;

    /**
     * see fontStyle
     */
    void setFontStyle( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/fonts.html#propdef-font-variant">
     * font-variant property definition </a> in CSS2.
     *
     */
    DOM::DOMString fontVariant() const;

    /**
     * see fontVariant
     */
    void setFontVariant( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/fonts.html#propdef-font-weight">
     * font-weight property definition </a> in CSS2.
     *
     */
    DOM::DOMString fontWeight() const;

    /**
     * see fontWeight
     */
    void setFontWeight( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/visudet.html#propdef-height">
     * height property definition </a> in CSS2.
     *
     */
    DOM::DOMString height() const;

    /**
     * see height
     */
    void setHeight( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/visuren.html#propdef-left">
     * left property definition </a> in CSS2.
     *
     */
    DOM::DOMString left() const;

    /**
     * see left
     */
    void setLeft( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/text.html#propdef-letter-spacing">
     * letter-spacing property definition </a> in CSS2.
     *
     */
    DOM::DOMString letterSpacing() const;

    /**
     * see letterSpacing
     */
    void setLetterSpacing( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/visudet.html#propdef-line-height">
     * line-height property definition </a> in CSS2.
     *
     */
    DOM::DOMString lineHeight() const;

    /**
     * see lineHeight
     */
    void setLineHeight( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/generate.html#propdef-list-style">
     * list-style property definition </a> in CSS2.
     *
     */
    DOM::DOMString listStyle() const;

    /**
     * see listStyle
     */
    void setListStyle( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/generate.html#propdef-list-style-image">
     * list-style-image property definition </a> in CSS2.
     *
     */
    DOM::DOMString listStyleImage() const;

    /**
     * see listStyleImage
     */
    void setListStyleImage( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/generate.html#propdef-list-style-position">
     * list-style-position property definition </a> in CSS2.
     *
     */
    DOM::DOMString listStylePosition() const;

    /**
     * see listStylePosition
     */
    void setListStylePosition( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/generate.html#propdef-list-style-type">
     * list-style-type property definition </a> in CSS2.
     *
     */
    DOM::DOMString listStyleType() const;

    /**
     * see listStyleType
     */
    void setListStyleType( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/box.html#propdef-margin">
     * margin property definition </a> in CSS2.
     *
     */
    DOM::DOMString margin() const;

    /**
     * see margin
     */
    void setMargin( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/box.html#propdef-margin-top">
     * margin-top property definition </a> in CSS2.
     *
     */
    DOM::DOMString marginTop() const;

    /**
     * see marginTop
     */
    void setMarginTop( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/box.html#propdef-margin-right">
     * margin-right property definition </a> in CSS2.
     *
     */
    DOM::DOMString marginRight() const;

    /**
     * see marginRight
     */
    void setMarginRight( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/box.html#propdef-margin-bottom">
     * margin-bottom property definition </a> in CSS2.
     *
     */
    DOM::DOMString marginBottom() const;

    /**
     * see marginBottom
     */
    void setMarginBottom( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/box.html#propdef-margin-left">
     * margin-left property definition </a> in CSS2.
     *
     */
    DOM::DOMString marginLeft() const;

    /**
     * see marginLeft
     */
    void setMarginLeft( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/generate.html#propdef-marker-offset">
     * marker-offset property definition </a> in CSS2.
     *
     */
    DOM::DOMString markerOffset() const;

    /**
     * see markerOffset
     */
    void setMarkerOffset( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/page.html#propdef-marks">
     * marks property definition </a> in CSS2.
     *
     */
    DOM::DOMString marks() const;

    /**
     * see marks
     */
    void setMarks( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/visudet.html#propdef-max-height">
     * max-height property definition </a> in CSS2.
     *
     */
    DOM::DOMString maxHeight() const;

    /**
     * see maxHeight
     */
    void setMaxHeight( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/visudet.html#propdef-max-width">
     * max-width property definition </a> in CSS2.
     *
     */
    DOM::DOMString maxWidth() const;

    /**
     * see maxWidth
     */
    void setMaxWidth( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/visudet.html#propdef-min-height">
     * min-height property definition </a> in CSS2.
     *
     */
    DOM::DOMString minHeight() const;

    /**
     * see minHeight
     */
    void setMinHeight( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/visudet.html#propdef-min-width">
     * min-width property definition </a> in CSS2.
     *
     */
    DOM::DOMString minWidth() const;

    /**
     * see minWidth
     */
    void setMinWidth( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/page.html#propdef-orphans">
     * orphans property definition </a> in CSS2.
     *
     */
    DOM::DOMString orphans() const;

    /**
     * see orphans
     */
    void setOrphans( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/ui.html#propdef-outline">
     * outline property definition </a> in CSS2.
     *
     */
    DOM::DOMString outline() const;

    /**
     * see outline
     */
    void setOutline( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/ui.html#propdef-outline-color">
     * outline-color property definition </a> in CSS2.
     *
     */
    DOM::DOMString outlineColor() const;

    /**
     * see outlineColor
     */
    void setOutlineColor( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/ui.html#propdef-outline-style">
     * outline-style property definition </a> in CSS2.
     *
     */
    DOM::DOMString outlineStyle() const;

    /**
     * see outlineStyle
     */
    void setOutlineStyle( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/ui.html#propdef-outline-width">
     * outline-width property definition </a> in CSS2.
     *
     */
    DOM::DOMString outlineWidth() const;

    /**
     * see outlineWidth
     */
    void setOutlineWidth( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/visufx.html#propdef-overflow">
     * overflow property definition </a> in CSS2.
     *
     */
    DOM::DOMString overflow() const;

    /**
     * see overflow
     */
    void setOverflow( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/box.html#propdef-padding">
     * padding property definition </a> in CSS2.
     *
     */
    DOM::DOMString padding() const;

    /**
     * see padding
     */
    void setPadding( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/box.html#propdef-padding-top">
     * padding-top property definition </a> in CSS2.
     *
     */
    DOM::DOMString paddingTop() const;

    /**
     * see paddingTop
     */
    void setPaddingTop( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/box.html#propdef-padding-right">
     * padding-right property definition </a> in CSS2.
     *
     */
    DOM::DOMString paddingRight() const;

    /**
     * see paddingRight
     */
    void setPaddingRight( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/box.html#propdef-padding-bottom">
     * padding-bottom property definition </a> in CSS2.
     *
     */
    DOM::DOMString paddingBottom() const;

    /**
     * see paddingBottom
     */
    void setPaddingBottom( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/box.html#propdef-padding-left">
     * padding-left property definition </a> in CSS2.
     *
     */
    DOM::DOMString paddingLeft() const;

    /**
     * see paddingLeft
     */
    void setPaddingLeft( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/page.html#propdef-page">
     * page property definition </a> in CSS2.
     *
     */
    DOM::DOMString page() const;

    /**
     * see page
     */
    void setPage( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/page.html#propdef-page-break-after">
     * page-break-after property definition </a> in CSS2.
     *
     */
    DOM::DOMString pageBreakAfter() const;

    /**
     * see pageBreakAfter
     */
    void setPageBreakAfter( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/page.html#propdef-page-break-before">
     * page-break-before property definition </a> in CSS2.
     *
     */
    DOM::DOMString pageBreakBefore() const;

    /**
     * see pageBreakBefore
     */
    void setPageBreakBefore( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/page.html#propdef-page-break-inside">
     * page-break-inside property definition </a> in CSS2.
     *
     */
    DOM::DOMString pageBreakInside() const;

    /**
     * see pageBreakInside
     */
    void setPageBreakInside( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/aural.html#propdef-pause">
     * pause property definition </a> in CSS2.
     *
     */
    DOM::DOMString pause() const;

    /**
     * see pause
     */
    void setPause( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/aural.html#propdef-pause-after">
     * pause-after property definition </a> in CSS2.
     *
     */
    DOM::DOMString pauseAfter() const;

    /**
     * see pauseAfter
     */
    void setPauseAfter( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/aural.html#propdef-pause-before">
     * pause-before property definition </a> in CSS2.
     *
     */
    DOM::DOMString pauseBefore() const;

    /**
     * see pauseBefore
     */
    void setPauseBefore( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/aural.html#propdef-pitch">
     * pitch property definition </a> in CSS2.
     *
     */
    DOM::DOMString pitch() const;

    /**
     * see pitch
     */
    void setPitch( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/aural.html#propdef-pitch-range">
     * pitch-range property definition </a> in CSS2.
     *
     */
    DOM::DOMString pitchRange() const;

    /**
     * see pitchRange
     */
    void setPitchRange( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/aural.html#propdef-play-during">
     * play-during property definition </a> in CSS2.
     *
     */
    DOM::DOMString playDuring() const;

    /**
     * see playDuring
     */
    void setPlayDuring( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/visuren.html#propdef-position">
     * position property definition </a> in CSS2.
     *
     */
    DOM::DOMString position() const;

    /**
     * see position
     */
    void setPosition( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/generate.html#propdef-quotes">
     * quotes property definition </a> in CSS2.
     *
     */
    DOM::DOMString quotes() const;

    /**
     * see quotes
     */
    void setQuotes( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/aural.html#propdef-richness">
     * richness property definition </a> in CSS2.
     *
     */
    DOM::DOMString richness() const;

    /**
     * see richness
     */
    void setRichness( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/visuren.html#propdef-right">
     * right property definition </a> in CSS2.
     *
     */
    DOM::DOMString right() const;

    /**
     * see right
     */
    void setRight( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/page.html#propdef-size">
     * size property definition </a> in CSS2.
     *
     */
    DOM::DOMString size() const;

    /**
     * see size
     */
    void setSize( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/aural.html#propdef-speak">
     * speak property definition </a> in CSS2.
     *
     */
    DOM::DOMString speak() const;

    /**
     * see speak
     */
    void setSpeak( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/tables.html#propdef-speak-header">
     * speak-header property definition </a> in CSS2.
     *
     */
    DOM::DOMString speakHeader() const;

    /**
     * see speakHeader
     */
    void setSpeakHeader( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/aural.html#propdef-speak-numeral">
     * speak-numeral property definition </a> in CSS2.
     *
     */
    DOM::DOMString speakNumeral() const;

    /**
     * see speakNumeral
     */
    void setSpeakNumeral( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/aural.html#propdef-speak-punctuation">
     * speak-punctuation property definition </a> in CSS2.
     *
     */
    DOM::DOMString speakPunctuation() const;

    /**
     * see speakPunctuation
     */
    void setSpeakPunctuation( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/aural.html#propdef-speech-rate">
     * speech-rate property definition </a> in CSS2.
     *
     */
    DOM::DOMString speechRate() const;

    /**
     * see speechRate
     */
    void setSpeechRate( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/aural.html#propdef-stress">
     * stress property definition </a> in CSS2.
     *
     */
    DOM::DOMString stress() const;

    /**
     * see stress
     */
    void setStress( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/tables.html#propdef-table-layout">
     * table-layout property definition </a> in CSS2.
     *
     */
    DOM::DOMString tableLayout() const;

    /**
     * see tableLayout
     */
    void setTableLayout( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/text.html#propdef-text-align">
     * text-align property definition </a> in CSS2.
     *
     */
    DOM::DOMString textAlign() const;

    /**
     * see textAlign
     */
    void setTextAlign( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/text.html#propdef-text-decoration">
     * text-decoration property definition </a> in CSS2.
     *
     */
    DOM::DOMString textDecoration() const;

    /**
     * see textDecoration
     */
    void setTextDecoration( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/text.html#propdef-text-indent">
     * text-indent property definition </a> in CSS2.
     *
     */
    DOM::DOMString textIndent() const;

    /**
     * see textIndent
     */
    void setTextIndent( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/text.html#propdef-text-shadow">
     * text-shadow property definition </a> in CSS2.
     *
     */
    DOM::DOMString textShadow() const;

    /**
     * see textShadow
     */
    void setTextShadow( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/text.html#propdef-text-transform">
     * text-transform property definition </a> in CSS2.
     *
     */
    DOM::DOMString textTransform() const;

    /**
     * see textTransform
     */
    void setTextTransform( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/visuren.html#propdef-top">
     * top property definition </a> in CSS2.
     *
     */
    DOM::DOMString top() const;

    /**
     * see top
     */
    void setTop( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/visuren.html#propdef-unicode-bidi">
     * unicode-bidi property definition </a> in CSS2.
     *
     */
    DOM::DOMString unicodeBidi() const;

    /**
     * see unicodeBidi
     */
    void setUnicodeBidi( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/visudet.html#propdef-vertical-align">
     * vertical-align property definition </a> in CSS2.
     *
     */
    DOM::DOMString verticalAlign() const;

    /**
     * see verticalAlign
     */
    void setVerticalAlign( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/visufx.html#propdef-visibility">
     * visibility property definition </a> in CSS2.
     *
     */
    DOM::DOMString visibility() const;

    /**
     * see visibility
     */
    void setVisibility( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/aural.html#propdef-voice-family">
     * voice-family property definition </a> in CSS2.
     *
     */
    DOM::DOMString voiceFamily() const;

    /**
     * see voiceFamily
     */
    void setVoiceFamily( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/aural.html#propdef-volume">
     * volume property definition </a> in CSS2.
     *
     */
    DOM::DOMString volume() const;

    /**
     * see volume
     */
    void setVolume( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/text.html#propdef-white-space">
     * white-space property definition </a> in CSS2.
     *
     */
    DOM::DOMString whiteSpace() const;

    /**
     * see whiteSpace
     */
    void setWhiteSpace( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/page.html#propdef-widows">
     * widows property definition </a> in CSS2.
     *
     */
    DOM::DOMString widows() const;

    /**
     * see widows
     */
    void setWidows( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/visudet.html#propdef-width">
     * width property definition </a> in CSS2.
     *
     */
    DOM::DOMString width() const;

    /**
     * see width
     */
    void setWidth( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/text.html#propdef-word-spacing">
     * word-spacing property definition </a> in CSS2.
     *
     */
    DOM::DOMString wordSpacing() const;

    /**
     * see wordSpacing
     */
    void setWordSpacing( const DOM::DOMString & );

    /**
     * See the <a
     * href="http://www.w3.org/TR/REC-CSS2/visufx.html#propdef-z-index">
     * z-index property definition </a> in CSS2.
     *
     */
    DOM::DOMString zIndex() const;

    /**
     * see zIndex
     */
    void setZIndex( const DOM::DOMString & );
};


class CSS2TextShadowImpl;
class CSSValue;

/**
 * The \c CSS2TextShadow interface represents a simple
 * value for the <a
 * href="http://www.w3.org/TR/REC-CSS2/text.html#propdef-text-shadow">
 * text-shadow </a> CSS Level 2 property.
 *
 */
class CSS2TextShadow
{
public:
    CSS2TextShadow();
    CSS2TextShadow(const CSS2TextShadow &other);
    CSS2TextShadow(CSS2TextShadowImpl *impl);
public:

    CSS2TextShadow & operator = (const CSS2TextShadow &other);

    ~CSS2TextShadow();

    /**
     * Specified the color of the text shadow. The CSS Value can
     * contain an empty string if no color has been specified.
     *
     */
    CSSValue color() const;

    /**
     * The horizontal position of the text shadow. \c 0 if
     * no length has been specified.
     *
     */
    CSSValue horizontal() const;

    /**
     * The vertical position of the text shadow. \c 0 if
     * no length has been specified.
     *
     */
    CSSValue vertical() const;

    /**
     * The blur radius of the text shadow. \c 0 if no
     * length has been specified.
     *
     */
    CSSValue blur() const;
};


}; // namespace

#endif
