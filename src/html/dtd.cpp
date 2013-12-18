/**
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 2006-2007 Allan Sandfeld Jensen (kde@carewolf.com)
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
 */

#include "html/dtd.h"

using namespace DOM;

#include <QDebug>

// priority of tags. Closing tags of higher priority close tags of lower
// priority.
// Update this list, whenever you change htmltags.*
//
// 0 elements with forbidden close tag and text. They don't get pushed
//   to the stack.
// 1 formatting elements
// 3 form nobr noscript
// 5 phrasing elements
// 6 TD TH SELECT
// 7 TR
// 8 tbody thead tfoot
// 9 table
// 10 body frameset head noembed noframes
// 11 html

const unsigned short DOM::tagPriorityArray[] = {
    0, // 0
    1, // ID_A == 1
    1, // ID_ABBR
    1, // ID_ACRONYM
    5, // ID_ADDRESS
    1, // ID_APPLET
    0, // ID_AREA
    1, // ID_AUDIO
    1, // ID_B
    0, // ID_BASE
    0, // ID_BASEFONT
    1, // ID_BDO
    1, // ID_BIG
    5, // ID_BLOCKQUOTE
   10, // ID_BODY
    0, // ID_BR
    1, // ID_BUTTON
    1, // ID_CANVAS
    5, // ID_CAPTION
    5, // ID_CENTER
    1, // ID_CITE
    1, // ID_CODE
    0, // ID_COL
    1, // ID_COLGROUP
    5, // ID_DD
    1, // ID_DEL
    1, // ID_DFN
    5, // ID_DIR
    5, // ID_DIV
    5, // ID_DL
    5, // ID_DT
    1, // ID_EM
    0, // ID_EMBED
    5, // ID_FIELDSET
    1, // ID_FONT
    3, // ID_FORM
    0, // ID_FRAME
   10,// ID_FRAMESET
    5, // ID_H1
    5, // ID_H2
    5, // ID_H3
    5, // ID_H4
    5, // ID_H5
    5, // ID_H6
   10,// ID_HEAD
    0, // ID_HR
   11,// ID_HTML
    1, // ID_I
    1, // ID_IFRAME
    1, // ID_ILAYER
    0, // ID_IMAGE
    0, // ID_IMG
    0, // ID_INPUT
    1, // ID_INS
    0, // ID_ISINDEX
    1, // ID_KBD
    0, // ID_KEYGEN
    1, // ID_LABEL
    1, // ID_LAYER
    1, // ID_LEGEND
    5, // ID_LI
    0, // ID_LINK
    5, // ID_LISTING
    1, // ID_MAP
    5, // ID_MARQUEE
    5, // ID_MENU
    0, // ID_META
    5, // ID_NOBR
   10,// ID_NOEMBED
   10,// ID_NOFRAMES
    3, // ID_NOSCRIPT
    1, // ID_NOLAYER
    5, // ID_OBJECT
    5, // ID_OL
    1, // ID_OPTGROUP
    2, // ID_OPTION
    5, // ID_P
    0, // ID_PARAM
    5, // ID_PLAINTEXT
    5, // ID_PRE
    1, // ID_Q
    1, // ID_S
    1, // ID_SAMP
    1, // ID_SCRIPT
    6, // ID_SELECT
    1, // ID_SMALL
    0, // ID_SOURCE
    1, // ID_SPAN
    1, // ID_STRIKE
    1, // ID_STRONG
    1, // ID_STYLE
    1, // ID_SUB
    1, // ID_SUP
    9, // ID_TABLE
    8, // ID_TBODY
    6, // ID_TD
    1, // ID_TEXTAREA
    8, // ID_TFOOT
    6, // ID_TH
    8, // ID_THEAD
    1, // ID_TITLE
    7, // ID_TR
    1, // ID_TT
    1, // ID_U
    5, // ID_UL
    1, // ID_VAR
    1, // ID_VIDEO
    0, // ID_WBR
    5, // ID_XMP
    0, // ID_TEXT
};

const tagStatus DOM::endTagArray[] = {
    REQUIRED,  // 0
    REQUIRED,  // ID_A == 1
    REQUIRED,  // ID_ABBR
    REQUIRED,  // ID_ACRONYM
    REQUIRED,  // ID_ADDRESS
    REQUIRED,  // ID_APPLET
    FORBIDDEN, // ID_AREA
    REQUIRED,  // ID_AUDIO
    REQUIRED,  // ID_B
    FORBIDDEN, // ID_BASE
    FORBIDDEN, // ID_BASEFONT
    REQUIRED,  // ID_BDO
    REQUIRED,  // ID_BIG
    REQUIRED,  // ID_BLOCKQUOTE
    REQUIRED,  // ID_BODY
    FORBIDDEN, // ID_BR
    REQUIRED,  // ID_BUTTON
    REQUIRED,  // ID_CANVAS
    REQUIRED,  // ID_CAPTION
    REQUIRED,  // ID_CENTER
    REQUIRED,  // ID_CITE
    REQUIRED,  // ID_CODE
    FORBIDDEN, // ID_COL
    OPTIONAL,  // ID_COLGROUP
    OPTIONAL,  // ID_DD
    REQUIRED,  // ID_DEL
    REQUIRED,  // ID_DFN
    REQUIRED,  // ID_DIR
    REQUIRED,  // ID_DIV
    REQUIRED,  // ID_DL
    OPTIONAL,  // ID_DT
    REQUIRED,  // ID_EM
    REQUIRED,  // ID_EMBED
    REQUIRED,  // ID_FIELDSET
    REQUIRED,  // ID_FONT
    REQUIRED,  // ID_FORM
    FORBIDDEN, // ID_FRAME
    REQUIRED,  // ID_FRAMESET
    REQUIRED,  // ID_H1
    REQUIRED,  // ID_H2
    REQUIRED,  // ID_H3
    REQUIRED,  // ID_H4
    REQUIRED,  // ID_H5
    REQUIRED,  // ID_H6
    OPTIONAL,  // ID_HEAD
    FORBIDDEN, // ID_HR
    REQUIRED,  // ID_HTML
    REQUIRED,  // ID_I
    REQUIRED,  // ID_IFRAME
    REQUIRED,  // ID_ILAYER
    FORBIDDEN, // ID_IMAGE
    FORBIDDEN, // ID_IMG
    FORBIDDEN, // ID_INPUT
    REQUIRED,  // ID_INS
    FORBIDDEN, // ID_ISINDEX
    REQUIRED,  // ID_KBD
    REQUIRED,  // ID_KEYGEN
    REQUIRED,  // ID_LABEL
    REQUIRED,  // ID_LAYER
    REQUIRED,  // ID_LEGEND
    OPTIONAL,  // ID_LI
    FORBIDDEN, // ID_LINK
    REQUIRED,  // ID_LISTING
    REQUIRED,  // ID_MAP
    REQUIRED,  // ID_MARQUEE
    REQUIRED,  // ID_MENU
    FORBIDDEN, // ID_META
    REQUIRED,  // ID_NOBR
    REQUIRED,  // ID_NOEMBED
    REQUIRED,  // ID_NOFRAMES
    REQUIRED,  // ID_NOSCRIPT
    REQUIRED,  // ID_NOLAYER
    REQUIRED,  // ID_OBJECT
    REQUIRED,  // ID_OL
    REQUIRED,  // ID_OPTGROUP
    OPTIONAL,  // ID_OPTION
    OPTIONAL,  // ID_P
    FORBIDDEN, // ID_PARAM
    REQUIRED,  // ID_PLAINTEXT
    REQUIRED,  // ID_PRE
    REQUIRED,  // ID_Q
    REQUIRED,  // ID_S
    REQUIRED,  // ID_SAMP
    REQUIRED,  // ID_SCRIPT
    REQUIRED,  // ID_SELECT
    REQUIRED,  // ID_SMALL
    FORBIDDEN, // ID_SOURCE
    REQUIRED,  // ID_SPAN
    REQUIRED,  // ID_STRIKE
    REQUIRED,  // ID_STRONG
    REQUIRED,  // ID_STYLE
    REQUIRED,  // ID_SUB
    REQUIRED,  // ID_SUP
    REQUIRED,  // ID_TABLE
    OPTIONAL,  // ID_TBODY
    OPTIONAL,  // ID_TD
    REQUIRED,  // ID_TEXTAREA
    OPTIONAL,  // ID_TFOOT
    OPTIONAL,  // ID_TH
    OPTIONAL,  // ID_THEAD
    REQUIRED,  // ID_TITLE
    OPTIONAL,  // ID_TR
    REQUIRED,  // ID_TT
    REQUIRED,  // ID_U
    REQUIRED,  // ID_UL
    REQUIRED,  // ID_VAR
    REQUIRED,  // ID_VIDEO
    FORBIDDEN, // ID_WBR
    REQUIRED,  // ID_XMP
    REQUIRED   // ID_TEXT
};

// This a combination of HTML4.dtd XHTML11.dtd and various extensions
// and deprecated elements
static const ushort tag_list_inline[] = {
    ID_TEXT,
    // %fontstyle
    ID_TT,
    ID_I,
    ID_B,
    ID_BIG,
    ID_SMALL,
    ID_U,           // legacy
    ID_S,           // legacy
    ID_STRIKE,      // legacy
    ID_FONT,        // legacy
    ID_BASEFONT,    // legacy
    // %phrase
    ID_EM,
    ID_STRONG,
    ID_DFN,
    ID_CODE,
    ID_Q,
    ID_SAMP,
    ID_KBD,
    ID_VAR,
    ID_CITE,
    ID_ABBR,
    ID_ACRONYM,
    ID_SUB,
    ID_SUP,
    // %inline.forms
    ID_INPUT,
    ID_SELECT,
    ID_TEXTAREA,
    ID_LABEL,
    ID_BUTTON,
    // %special
    ID_A,
    ID_OBJECT,
    ID_IMAGE,
    ID_IMG,
    ID_APPLET,
    ID_IFRAME,
    ID_EMBED,       // ?
    // %special.pre
    ID_BR,
    ID_SPAN,
    ID_BDO,
    ID_MAP,
    // %misc.inline
    ID_SCRIPT,
    ID_INS,
    ID_DEL,
    // non-standard:
    ID_ILAYER,      // deprecated
    ID_NOBR,        // ?
    ID_WBR,         // ?
    ID_CANVAS,
    ID_AUDIO,
    ID_VIDEO,
    0
};

static const ushort tag_list_quirk_inline[] = {
    ID_NOSCRIPT,    // block, but parsed as inline in Mozilla and MSIE
    0
};

static const ushort tag_list_block[] = {
    ID_TEXT, // white-space is allowed
    ID_P,
    // %headings
    ID_H1,
    ID_H2,
    ID_H3,
    ID_H4,
    ID_H5,
    ID_H6,
    // %lists
    ID_UL,
    ID_OL,
    ID_DL,
    ID_DIR,         // legacy
    ID_MENU,        // legacy
    // %blocktext
    ID_LISTING,
    ID_PRE,
    ID_HR,
    ID_BLOCKQUOTE,
    ID_ADDRESS,
    ID_PLAINTEXT,   // ?
    ID_XMP,         // ?
    ID_CENTER,      // legacy
    // other
    ID_DIV,
    ID_FIELDSET,
    ID_TABLE,
    ID_NOSCRIPT,
    ID_NOFRAMES,
    ID_FORM,
    ID_ISINDEX,     // legacy
    // non-standard:
    ID_LAYER,       // deprecated
    ID_MARQUEE,     // extension
    0
};

// block elements allowed for quirky error recovery
static const ushort tag_list_quirk_block[] = {
    ID_LI,
    ID_DD,
    0
};


static const ushort tag_list_select[] = {
    ID_TEXT,
    ID_OPTGROUP,
    ID_OPTION,
    ID_COMMENT,
    ID_SCRIPT,
    0
};

static const ushort tag_list_frame[] = {
    ID_FRAMESET,
    ID_FRAME,
    ID_NOFRAMES,
    ID_COMMENT,
    0
};

static const ushort tag_list_head[] = {
    ID_SCRIPT,
    ID_STYLE,
    ID_META,
    ID_LINK,
    ID_TITLE,
    ID_ISINDEX,
    ID_BASE,
    ID_COMMENT,
    0
};

// cf. HTML5 8.2.3.2
static const ushort scope_boundary[] = {
    ID_APPLET,
    ID_CAPTION,
    ID_HTML,
    ID_TABLE,
    ID_TD,
    ID_TH,
    ID_BUTTON,
    ID_MARQUEE,
    ID_OBJECT,
//    ID_FOREIGNOBJECT, - only in SVG namespace
    0
};

static bool check_array(ushort child, const ushort *tagList)
{
    int i = 0;
    while(tagList[i] != 0)
    {
        if(tagList[i] == child) return true;
        i++;
    }
    return false;
}


static bool check_block(ushort childID, bool strict)
{
    return check_array(childID, tag_list_block) ||
           (!strict && check_array(childID, tag_list_quirk_block));
}

static bool check_inline(ushort childID, bool strict)
{
    return check_array(childID, tag_list_inline) ||
           (!strict && check_array(childID, tag_list_quirk_inline));
}

static bool check_flow(ushort childID, bool strict)
{
    return check_block(childID, strict) || check_inline(childID, strict);
}

bool DOM::checkIsScopeBoundary(ushort tagID)
{
    return check_array(tagID, scope_boundary);
}

bool DOM::checkChild(ushort tagID, ushort childID, bool strict)
{
    //qDebug() << "checkChild: " << tagID << "/" << childID;

    if (childID == ID_COMMENT) return true;

    // Treat custom elements the same as <span>.
    if (tagID > ID_LAST_TAG)
        tagID = ID_SPAN;
    if (childID > ID_LAST_TAG)
        childID = ID_SPAN;

    switch(tagID)
    {
    case ID_TT:
    case ID_I:
    case ID_B:
    case ID_U:
    case ID_S:
    case ID_STRIKE:
    case ID_BIG:
    case ID_SMALL:
    case ID_EM:
    case ID_STRONG:
    case ID_DFN:
    case ID_CODE:
    case ID_SAMP:
    case ID_KBD:
    case ID_VAR:
    case ID_CITE:
    case ID_ABBR:
    case ID_ACRONYM:
    case ID_SUB:
    case ID_SUP:
    case ID_BDO:
    case ID_FONT:
    case ID_LEGEND:
    case ID_Q:
    case ID_A:
    case ID_NOBR:
    case ID_WBR:
        // %inline *
        return check_inline(childID, strict) || check_block(childID, strict);
    case ID_P:
        // P: %inline *
        return check_inline(childID, strict) ||
               (!strict && childID == ID_TABLE );
    case ID_H1:
    case ID_H2:
    case ID_H3:
    case ID_H4:
    case ID_H5:
    case ID_H6:
        // %inline *
        return check_inline(childID, strict) ||
               (!strict && check_block(childID, true) && (childID < ID_H1 || childID > ID_H6));
    case ID_BASEFONT:
    case ID_BR:
    case ID_AREA:
    case ID_LINK:
    case ID_IMAGE:
    case ID_IMG:
    case ID_PARAM:
    case ID_HR:
    case ID_INPUT:
    case ID_COL:
    case ID_FRAME:
    case ID_ISINDEX:
    case ID_BASE:
    case ID_META:
    case ID_COMMENT:
        // BASEFONT: EMPTY
        return false;
    case ID_BODY:
        // BODY: %block | SCRIPT (but even strict sites expect %flow)
        return check_flow(childID, strict);
    case ID_ADDRESS:
        // ADDRESS: %inline *
        return check_inline(childID, strict) ||
               (!strict && childID == ID_P);
    case ID_DT:
        // DT: %inline *
        return check_inline(childID, strict) ||
              (!strict && check_block(childID, true) && childID != ID_DL);
    case ID_LI:
    case ID_DIV:
    case ID_SPAN:
    case ID_ILAYER:
    case ID_LAYER:
    case ID_CENTER:
    case ID_BLOCKQUOTE:
    case ID_INS:
    case ID_DEL:
    case ID_DD:
    case ID_TH:
    case ID_TD:
    case ID_IFRAME:
    case ID_NOFRAMES:
    case ID_NOSCRIPT:
    case ID_CAPTION:
    case ID_MARQUEE:
    case ID_CANVAS:
        // DIV: %flow *
        return check_flow(childID, strict);
    case ID_MAP:
        // MAP: ( %block | AREA ) (but "prose" in HTML5)
        return check_flow(childID, true) || childID == ID_AREA ||
               (!strict && childID == ID_SCRIPT);
    case ID_OBJECT:
    case ID_EMBED:
    case ID_APPLET:
        // OBJECT: %flow | PARAM *
        return check_flow(childID, true) || childID == ID_PARAM;
    case ID_AUDIO:
    case ID_VIDEO:
        return check_flow(childID, true) || childID == ID_SOURCE;
    case ID_PRE:
    case ID_XMP:
    case ID_PLAINTEXT:
    case ID_LISTING:
        // PRE: %flow * - _5
        return check_flow(childID, true);
    case ID_DL:
        // DL: DT | DD +
        return (childID == ID_DT || childID == ID_DD || check_flow(childID, strict));
    case ID_OL:
    case ID_UL:
    case ID_DIR:
    case ID_MENU:
        // OL: LI +
        // For DIR and MENU, the DTD says - %block, but it contradicts spec language..
        return (childID == ID_LI || check_flow(childID, strict));
    case ID_FORM:
        // FORM: %flow * - FORM
        return check_flow(childID, strict);
    case ID_LABEL:
        // LABEL: %inline * - LABEL
        return check_inline(childID, strict) ||
               (!strict && check_block(childID, true));
    case ID_KEYGEN:
        // KEYGEN does not really allow any children
        // from outside, just need this to be able
        // to add the keylengths ourself
        // Yes, consider it a hack (Dirk)
    case ID_SELECT:
        // SELECT: _7 +
        return check_array(childID, tag_list_select);
    case ID_OPTGROUP:
        // OPTGROUP: OPTION +
        if(childID == ID_OPTION) return true;
        return false;
    case ID_OPTION:
        if(childID == ID_SCRIPT) return true;
        // fallthrough intentional
    case ID_TEXTAREA:
    case ID_TITLE:
    case ID_STYLE:
    case ID_SCRIPT:
        // OPTION: TEXT
        if(childID == ID_TEXT) return true;
        return false;
    case ID_FIELDSET:
        // FIELDSET: ( TEXT , LEGEND , %flow * )
        if(childID == ID_TEXT) return true;
        if(childID == ID_LEGEND) return true;
        return check_flow(childID, strict);
    case ID_BUTTON:
        // BUTTON: %flow * - _8
        return check_flow(childID, strict);
    case ID_TABLE:
        // TABLE: ( CAPTION ? , ( COL * | COLGROUP * ) , THEAD ? , TFOOT ? , TBODY + )
        switch(childID)
        {
        case ID_CAPTION:
        case ID_COL:
        case ID_COLGROUP:
        case ID_THEAD:
        case ID_TFOOT:
        case ID_TBODY:
        case ID_FORM:
        case ID_SCRIPT:
            return true;
        default:
            return false;
        }
    case ID_THEAD:
    case ID_TFOOT:
    case ID_TBODY:
        // THEAD: TR +
        if(childID == ID_TR || childID == ID_SCRIPT) return true;
        return false;
    case ID_COLGROUP:
        // COLGROUP: COL *
        if(childID == ID_COL) return true;
        return false;
    case ID_TR:
        // TR: (TD, TH)
        return (childID == ID_TH || childID == ID_TD || childID == ID_SCRIPT);
    case ID_FRAMESET:
        // FRAMESET: _10
        return check_array(childID, tag_list_frame);
    case ID_HEAD:
        // HEAD: _11
        return check_array(childID, tag_list_head);
    case ID_HTML:
        // HTML: ( HEAD , COMMENT, ( BODY | ( FRAMESET & NOFRAMES ? ) ) )
        switch(childID)
        {
        case ID_HEAD:
        case ID_BODY:
        case ID_FRAMESET:
        case ID_NOFRAMES:
        case ID_SCRIPT:
            return true;
        default:
            return false;
        }
    default:
        // qDebug() << "unhandled tag in dtd.cpp:checkChild(): tagID=" << tagID << "!";
        return false;
    }
}

void DOM::addForbidden(int tagId, ushort *forbiddenTags)
{
    switch(tagId)
    {
    case ID_A:
        // we allow nested anchors. The innermost one wil be taken...
        //forbiddenTags[ID_A]++;
        break;
    case ID_NOBR:
        forbiddenTags[ID_PRE]++;
        forbiddenTags[ID_LISTING]++;
        // fall through
    case ID_PRE:
    case ID_LISTING:
    case ID_PLAINTEXT:
    case ID_XMP:
        //forbiddenTags[ID_IMAGE]++;
        //forbiddenTags[ID_IMG]++;
        forbiddenTags[ID_OBJECT]++;
        forbiddenTags[ID_EMBED]++;
        forbiddenTags[ID_APPLET]++;
        // why forbid them. We can deal with them in PRE
        //forbiddenTags[ID_BIG]++;
        //forbiddenTags[ID_SMALL]++;
        //forbiddenTags[ID_SUB]++;
        //forbiddenTags[ID_SUP]++;
        forbiddenTags[ID_BASEFONT]++;
        break;
    case ID_LABEL:
        forbiddenTags[ID_LABEL]++;
        break;
    case ID_BUTTON:
        forbiddenTags[ID_A]++;
        forbiddenTags[ID_INPUT]++;
        forbiddenTags[ID_SELECT]++;
        forbiddenTags[ID_TEXTAREA]++;
        forbiddenTags[ID_LABEL]++;
        forbiddenTags[ID_BUTTON]++;
        forbiddenTags[ID_FORM]++;
        forbiddenTags[ID_ISINDEX]++;
        forbiddenTags[ID_FIELDSET]++;
        forbiddenTags[ID_IFRAME]++;
        break;
    default:
        break;
    }
}

void DOM::removeForbidden(int tagId, ushort *forbiddenTags)
{
    switch(tagId)
    {
    case ID_A:
        //forbiddenTags[ID_A]--;
        break;
    case ID_NOBR:
        forbiddenTags[ID_PRE]--;
        forbiddenTags[ID_LISTING]--;
        // fall through
    case ID_PRE:
    case ID_LISTING:
    case ID_XMP:
    case ID_PLAINTEXT:
        //forbiddenTags[ID_IMAGE]--;
        //forbiddenTags[ID_IMG]--;
        forbiddenTags[ID_OBJECT]--;
        forbiddenTags[ID_EMBED]--;
        forbiddenTags[ID_APPLET]--;
        //forbiddenTags[ID_BIG]--;
        //forbiddenTags[ID_SMALL]--;
        //forbiddenTags[ID_SUB]--;
        //forbiddenTags[ID_SUP]--;
        forbiddenTags[ID_BASEFONT]--;
        break;
    case ID_LABEL:
        forbiddenTags[ID_LABEL]--;
        break;
    case ID_BUTTON:
        forbiddenTags[ID_A]--;
        forbiddenTags[ID_INPUT]--;
        forbiddenTags[ID_SELECT]--;
        forbiddenTags[ID_TEXTAREA]--;
        forbiddenTags[ID_LABEL]--;
        forbiddenTags[ID_BUTTON]--;
        forbiddenTags[ID_FORM]--;
        forbiddenTags[ID_ISINDEX]--;
        forbiddenTags[ID_FIELDSET]--;
        forbiddenTags[ID_IFRAME]--;
        break;
    default:
        break;
    }
}

