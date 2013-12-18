/*
 * Copyright (C) 2004 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "jsediting.h"
#include "editing/htmlediting_impl.h"
#include "editor.h"

#include "css/cssproperties.h"
#include "css/cssvalues.h"
#include "css/css_valueimpl.h"
#include "xml/dom_selection.h"
#include "xml/dom_docimpl.h"
#include "dom/dom_string.h"

#include "misc/khtml_partaccessor.h"

#include <QHash>
#include <QString>

using khtml::TypingCommandImpl;
using khtml::InsertListCommandImpl;
//
#define KPAC khtml::KHTMLPartAccessor

#define DEBUG_COMMANDS

namespace DOM {

class DocumentImpl;

struct CommandImp {
    bool (*execFn)(KHTMLPart *part, bool userInterface, const DOMString &value);
    bool (*enabledFn)(KHTMLPart *part);
    Editor::TriState (*stateFn)(KHTMLPart *part);
    DOMString (*valueFn)(KHTMLPart *part);
};

typedef QHash<QString,const CommandImp*> CommandDict;
static CommandDict createCommandDictionary();

bool JSEditor::execCommand(const CommandImp *cmd, bool userInterface, const DOMString &value)
{
    if (!cmd || !cmd->enabledFn)
        return false;
    KHTMLPart *part = m_doc->part();
    if (!part)
        return false;
    m_doc->updateLayout();
    return cmd->enabledFn(part) && cmd->execFn(part, userInterface, value);
}

bool JSEditor::queryCommandEnabled(const CommandImp *cmd)
{
    if (!cmd || !cmd->enabledFn)
        return false;
    KHTMLPart *part = m_doc->part();
    if (!part)
        return false;
    m_doc->updateLayout();
    return cmd->enabledFn(part);
}

bool JSEditor::queryCommandIndeterm(const CommandImp *cmd)
{
    if (!cmd || !cmd->enabledFn)
        return false;
    KHTMLPart *part = m_doc->part();
    if (!part)
        return false;
    m_doc->updateLayout();
    return cmd->stateFn(part) == Editor::MixedTriState;
}

bool JSEditor::queryCommandState(const CommandImp *cmd)
{
    if (!cmd || !cmd->enabledFn)
        return false;
    KHTMLPart *part = m_doc->part();
    if (!part)
        return false;
    m_doc->updateLayout();
    return cmd->stateFn(part) != Editor::FalseTriState;
}

bool JSEditor::queryCommandSupported(const CommandImp *cmd)
{
    return cmd != 0;
}

DOMString JSEditor::queryCommandValue(const CommandImp *cmd)
{
    if (!cmd || !cmd->enabledFn)
        return DOMString();
    KHTMLPart *part = m_doc->part();
    if (!part)
        return DOMString();
    m_doc->updateLayout();
    return cmd->valueFn(part);
}

// =============================================================================================

// Private stuff

static bool execStyleChange(KHTMLPart *part, int propertyID, const DOMString &propertyValue)
{
    CSSStyleDeclarationImpl *style = new CSSStyleDeclarationImpl(0);
    style->setProperty(propertyID, propertyValue);
    style->ref();
    part->editor()->applyStyle(style);
    style->deref();
    return true;
}

static bool execStyleChange(KHTMLPart *part, int propertyID, int propertyEnum)
{
    CSSStyleDeclarationImpl *style = new CSSStyleDeclarationImpl(0);
    style->setProperty(propertyID, propertyEnum);
    style->ref();
    part->editor()->applyStyle(style);
    style->deref();
    return true;
}

static bool execStyleChange(KHTMLPart *part, int propertyID, const char *propertyValue)
{
    return execStyleChange(part, propertyID, DOMString(propertyValue));
}

static Editor::TriState stateStyle(KHTMLPart *part, int propertyID, const char *desiredValue)
{
    CSSStyleDeclarationImpl *style = new CSSStyleDeclarationImpl(0);
    style->setProperty(propertyID, desiredValue);
    style->ref();
    Editor::TriState state = part->editor()->selectionHasStyle(style);
    style->deref();
    return state;
}

static bool selectionStartHasStyle(KHTMLPart *part, int propertyID, const char *desiredValue)
{
    CSSStyleDeclarationImpl *style = new CSSStyleDeclarationImpl(0);
    style->setProperty(propertyID, desiredValue);
    style->ref();
    bool hasStyle = part->editor()->selectionStartHasStyle(style);
    style->deref();
    return hasStyle;
}

static DOMString valueStyle(KHTMLPart *part, int propertyID)
{
    return part->editor()->selectionStartStylePropertyValue(propertyID);
}

// =============================================================================================
//
// execCommand implementations
//

static bool execBackColor(KHTMLPart *part, bool /*userInterface*/, const DOMString &value)
{
    return execStyleChange(part, CSS_PROP_BACKGROUND_COLOR, value);
}

static bool execBold(KHTMLPart *part, bool /*userInterface*/, const DOMString &/*value*/)
{
    bool isBold = selectionStartHasStyle(part, CSS_PROP_FONT_WEIGHT, "bold");
    return execStyleChange(part, CSS_PROP_FONT_WEIGHT, isBold ? "normal" : "bold");
}

static bool execCopy(KHTMLPart *part, bool /*userInterface*/, const DOMString &/*value*/)
{
    part->editor()->copy();
    return true;
}

static bool execCut(KHTMLPart *part, bool /*userInterface*/, const DOMString &/*value*/)
{
    part->editor()->cut();
    return true;
}

static bool execDelete(KHTMLPart *part, bool /*userInterface*/, const DOMString &/*value*/)
{
    TypingCommandImpl::deleteKeyPressed0(KPAC::xmlDocImpl(part));
    return true;
}

static bool execFontName(KHTMLPart *part, bool /*userInterface*/, const DOMString &value)
{
    return execStyleChange(part, CSS_PROP_FONT_FAMILY, value);
}

static bool execFontSize(KHTMLPart *part, bool /*userInterface*/, const DOMString &value)
{
    // This should handle sizes 1-7 like <font> does. Who the heck designed this interface? (Rhetorical question)
    bool ok;
    int val = value.string().toInt(&ok);
    if (ok && val >= 1 && val <= 7) {
        int size;
        switch (val) {
        case 1: size = CSS_VAL_XX_SMALL; break;
        case 2: size = CSS_VAL_SMALL; break;
        case 3: size = CSS_VAL_MEDIUM; break;
        case 4: size = CSS_VAL_LARGE; break;
        case 5: size = CSS_VAL_X_LARGE;  break;
        case 6: size = CSS_VAL_XX_LARGE; break;
        default: size = CSS_VAL__KHTML_XXX_LARGE;
        }
        return execStyleChange(part, CSS_PROP_FONT_SIZE, size);
    }
    
    return execStyleChange(part, CSS_PROP_FONT_SIZE, value);
}

static bool execForeColor(KHTMLPart *part, bool /*userInterface*/, const DOMString &value)
{
    return execStyleChange(part, CSS_PROP_COLOR, value);
}

static bool execIndent(KHTMLPart *part, bool /*userInterface*/, const DOMString &/*value*/)
{
    part->editor()->indent();
    return true;
}

static bool execInsertNewline(KHTMLPart *part, bool /*userInterface*/, const DOMString &/*value*/)
{
    TypingCommandImpl::insertNewline0(KPAC::xmlDocImpl(part));
    return true;
}

static bool execInsertParagraph(KHTMLPart * /*part*/, bool /*userInterface*/, const DOMString &/*value*/)
{
    // FIXME: Implement.
    return false;
}

static bool execInsertText(KHTMLPart *part, bool /*userInterface*/, const DOMString &value)
{
    TypingCommandImpl::insertText0(KPAC::xmlDocImpl(part), value);
    return true;
}

static bool execInsertOrderedList(KHTMLPart *part, bool /*userInterface*/, const DOMString &/*value*/)
{
    InsertListCommandImpl::insertList(KPAC::xmlDocImpl(part), InsertListCommandImpl::OrderedList);
    return true;
}

static bool execInsertUnorderedList(KHTMLPart *part, bool /*userInterface*/, const DOMString &/*value*/)
{
    InsertListCommandImpl::insertList(KPAC::xmlDocImpl(part), InsertListCommandImpl::UnorderedList);
    return true;
}

static bool execItalic(KHTMLPart *part, bool /*userInterface*/, const DOMString &/*value*/)
{
    bool isItalic = selectionStartHasStyle(part, CSS_PROP_FONT_STYLE, "italic");
    return execStyleChange(part, CSS_PROP_FONT_STYLE, isItalic ? "normal" : "italic");
}

static bool execJustifyCenter(KHTMLPart *part, bool /*userInterface*/, const DOMString &/*value*/)
{
    return execStyleChange(part, CSS_PROP_TEXT_ALIGN, "center");
}

static bool execJustifyFull(KHTMLPart *part, bool /*userInterface*/, const DOMString &/*value*/)
{
    return execStyleChange(part, CSS_PROP_TEXT_ALIGN, "justify");
}

static bool execJustifyLeft(KHTMLPart *part, bool /*userInterface*/, const DOMString &/*value*/)
{
    return execStyleChange(part, CSS_PROP_TEXT_ALIGN, "left");
}

static bool execJustifyRight(KHTMLPart *part, bool /*userInterface*/, const DOMString &/*value*/)
{
    return execStyleChange(part, CSS_PROP_TEXT_ALIGN, "right");
}

static bool execOutdent(KHTMLPart *part, bool /*userInterface*/, const DOMString &/*value*/)
{
    part->editor()->outdent();
    return true;
}

#ifndef NO_SUPPORT_PASTE

static bool execPaste(KHTMLPart *part, bool /*userInterface*/, const DOMString &/*value*/)
{
    part->editor()->paste();
    return true;
}

#endif

static bool execPrint(KHTMLPart *part, bool /*userInterface*/, const DOMString &/*value*/)
{
    part->editor()->print();
    return true;
}

static bool execRedo(KHTMLPart *part, bool /*userInterface*/, const DOMString &/*value*/)
{
    part->editor()->redo();
    return true;
}

static bool execSelectAll(KHTMLPart *part, bool /*userInterface*/, const DOMString &/*value*/)
{
    part->selectAll();
    return true;
}

static bool execStrikeThrough(KHTMLPart *part, bool /*userInterface*/, const DOMString &/*value*/)
{
    bool isStriked = selectionStartHasStyle(part, CSS_PROP_TEXT_DECORATION, "line-through");
    return execStyleChange(part, CSS_PROP_TEXT_DECORATION, isStriked ? "none" : "line-through");
}

static bool execSubscript(KHTMLPart *part, bool /*userInterface*/, const DOMString &/*value*/)
{
    return execStyleChange(part, CSS_PROP_VERTICAL_ALIGN, "sub");
}

static bool execSuperscript(KHTMLPart *part, bool /*userInterface*/, const DOMString &/*value*/)
{
    return execStyleChange(part, CSS_PROP_VERTICAL_ALIGN, "super");
}

static bool execUndo(KHTMLPart *part, bool /*userInterface*/, const DOMString &/*value*/)
{
    part->editor()->undo();
    return true;
}

static bool execUnderline(KHTMLPart *part, bool /*userInterface*/, const DOMString &/*value*/)
{
    bool isUnderline = selectionStartHasStyle(part, CSS_PROP_TEXT_DECORATION, "underline");
    return execStyleChange(part, CSS_PROP_TEXT_DECORATION, isUnderline ? "none" : "underline");
}

static bool execUnselect(KHTMLPart *part, bool /*userInterface*/, const DOMString &/*value*/)
{
    KPAC::clearSelection(part);
    return true;
}

// =============================================================================================
//
// queryCommandEnabled implementations
//
// It's a bit difficult to get a clear notion of the difference between
// "supported" and "enabled" from reading the Microsoft documentation, but
// what little I could glean from that seems to make some sense.
//     Supported = The command is supported by this object.
//     Enabled =   The command is available and enabled.

static bool enabled(KHTMLPart * /*part*/)
{
    return true;
}

static bool enabledAnySelection(KHTMLPart *part)
{
    return KPAC::caret(part).notEmpty();
}

#ifndef NO_SUPPORT_PASTE

static bool enabledPaste(KHTMLPart *part)
{
    return part->editor()->canPaste();
}

#endif

static bool enabledRangeSelection(KHTMLPart *part)
{
    return KPAC::caret(part).state() == Selection::RANGE;
}

static bool enabledRedo(KHTMLPart *part)
{
    return part->editor()->canRedo();
}

static bool enabledUndo(KHTMLPart *part)
{
    return part->editor()->canUndo();
}

// =============================================================================================
//
// queryCommandIndeterm/State implementations
//
// It's a bit difficult to get a clear notion of what these methods are supposed
// to do from reading the Microsoft documentation, but my current guess is this:
//
//     queryCommandState and queryCommandIndeterm work in concert to return
//     the two bits of information that are needed to tell, for instance,
//     if the text of a selection is bold. The answer can be "yes", "no", or
//     "partially".
//
// If this is so, then queryCommandState should return "yes" in the case where
// all the text is bold and "no" for non-bold or partially-bold text.
// Then, queryCommandIndeterm should return "no" in the case where
// all the text is either all bold or not-bold and and "yes" for partially-bold text.

static Editor::TriState stateNone(KHTMLPart * /*part*/)
{
    return Editor::FalseTriState;
}

static Editor::TriState stateBold(KHTMLPart *part)
{
    return stateStyle(part, CSS_PROP_FONT_WEIGHT, "bold");
}

static Editor::TriState stateItalic(KHTMLPart *part)
{
    return stateStyle(part, CSS_PROP_FONT_STYLE, "italic");
}

static Editor::TriState stateStrike(KHTMLPart *part)
{
    return stateStyle(part, CSS_PROP_TEXT_DECORATION, "line-through");
}

static Editor::TriState stateSubscript(KHTMLPart *part)
{
    return stateStyle(part, CSS_PROP_VERTICAL_ALIGN, "sub");
}

static Editor::TriState stateSuperscript(KHTMLPart *part)
{
    return stateStyle(part, CSS_PROP_VERTICAL_ALIGN, "super");
}

static Editor::TriState stateUnderline(KHTMLPart *part)
{
    return stateStyle(part, CSS_PROP_TEXT_DECORATION, "underline");
}

// =============================================================================================
//
// queryCommandValue implementations
//

static DOMString valueNull(KHTMLPart * /*part*/)
{
    return DOMString();
}

static DOMString valueBackColor(KHTMLPart *part)
{
    return valueStyle(part, CSS_PROP_BACKGROUND_COLOR);
}

static DOMString valueFontName(KHTMLPart *part)
{
    return valueStyle(part, CSS_PROP_FONT_FAMILY);
}

static DOMString valueFontSize(KHTMLPart *part)
{
    return valueStyle(part, CSS_PROP_FONT_SIZE);
}

static DOMString valueForeColor(KHTMLPart *part)
{
    return valueStyle(part, CSS_PROP_COLOR);
}

// =============================================================================================

struct EditorCommandInfo { const char *name; CommandImp imp; };

// NOTE: strictly keep in sync with EditorCommand in editor_command.h
static const EditorCommandInfo commands[] = {

    { "backColor", { execBackColor, enabled, stateNone, valueBackColor } },
    { "bold", { execBold, enabledAnySelection, stateBold, valueNull } },
    { "copy", { execCopy, enabledRangeSelection, stateNone, valueNull } },
    { "cut", { execCut, enabledRangeSelection, stateNone, valueNull } },
    { "delete", { execDelete, enabledAnySelection, stateNone, valueNull } },
    { "fontName", { execFontName, enabledAnySelection, stateNone, valueFontName } },
    { "fontSize", { execFontSize, enabledAnySelection, stateNone, valueFontSize } },
    { "foreColor", { execForeColor, enabledAnySelection, stateNone, valueForeColor } },
    { "indent", { execIndent, enabledAnySelection, stateNone, valueNull } },
    { "insertNewline", { execInsertNewline, enabledAnySelection, stateNone, valueNull } },
    { "insertOrderedList", { execInsertOrderedList, enabledAnySelection, stateNone, valueNull } },
    { "insertParagraph", { execInsertParagraph, enabledAnySelection, stateNone, valueNull } },
    { "insertText", { execInsertText, enabledAnySelection, stateNone, valueNull } },
    { "insertUnorderedList", { execInsertUnorderedList, enabledAnySelection, stateNone, valueNull } },
    { "italic", { execItalic, enabledAnySelection, stateItalic, valueNull } },
    { "justifyCenter", { execJustifyCenter, enabledAnySelection, stateNone, valueNull } },
    { "justifyFull", { execJustifyFull, enabledAnySelection, stateNone, valueNull } },
    { "justifyLeft", { execJustifyLeft, enabledAnySelection, stateNone, valueNull } },
    { "justifyNone", { execJustifyLeft, enabledAnySelection, stateNone, valueNull } },
    { "justifyRight", { execJustifyRight, enabledAnySelection, stateNone, valueNull } },
    { "outdent", { execOutdent, enabledAnySelection, stateNone, valueNull } },
#ifndef NO_SUPPORT_PASTE
    { "paste", { execPaste, enabledPaste, stateNone, valueNull } },
#else
    { 0, { 0, 0, 0, 0 } },
#endif
    { "print", { execPrint, enabled, stateNone, valueNull } },
    { "redo", { execRedo, enabledRedo, stateNone, valueNull } },
    { "selectAll", { execSelectAll, enabled, stateNone, valueNull } },
    { "StrikeThrough", {execStrikeThrough, enabled, stateStrike, valueNull } },
    { "subscript", { execSubscript, enabledAnySelection, stateSubscript, valueNull } },
    { "superscript", { execSuperscript, enabledAnySelection, stateSuperscript, valueNull } },
    { "underline", { execUnderline, enabledAnySelection, stateUnderline, valueNull } },
    { "undo", { execUndo, enabledUndo, stateNone, valueNull } },
    { "unselect", { execUnselect, enabledAnySelection, stateNone, valueNull } }

    //
    // The "unsupported" commands are listed here since they appear in the Microsoft
    // documentation used as the basis for the list.
    //

    // 2d-position (not supported)
    // absolutePosition (not supported)
    // blockDirLTR (not supported)
    // blockDirRTL (not supported)
    // browseMode (not supported)
    // clearAuthenticationCache (not supported)
    // createBookmark (not supported)
    // createLink (not supported)
    // dirLTR (not supported)
    // dirRTL (not supported)
    // editMode (not supported)
    // formatBlock (not supported)
    // inlineDirLTR (not supported)
    // inlineDirRTL (not supported)
    // insertButton (not supported)
    // insertFieldSet (not supported)
    // insertHorizontalRule (not supported)
    // insertIFrame (not supported)
    // insertImage (not supported)
    // insertInputButton (not supported)
    // insertInputCheckbox (not supported)
    // insertInputFileUpload (not supported)
    // insertInputHidden (not supported)
    // insertInputImage (not supported)
    // insertInputPassword (not supported)
    // insertInputRadio (not supported)
    // insertInputReset (not supported)
    // insertInputSubmit (not supported)
    // insertInputText (not supported)
    // insertMarquee (not supported)
    // insertOrderedList (not supported)
    // insertSelectDropDown (not supported)
    // insertSelectListBox (not supported)
    // insertTextArea (not supported)
    // insertUnorderedList (not supported)
    // liveResize (not supported)
    // multipleSelection (not supported)
    // open (not supported)
    // overwrite (not supported)
    // playImage (not supported)
    // refresh (not supported)
    // removeFormat (not supported)
    // removeParaFormat (not supported)
    // saveAs (not supported)
    // sizeToControl (not supported)
    // sizeToControlHeight (not supported)
    // sizeToControlWidth (not supported)
    // stop (not supported)
    // stopimage (not supported)
    // strikethrough (not supported)
    // unbookmark (not supported)
    // underline (not supported)
    // unlink (not supported)
};

static CommandDict createCommandDictionary()
{
    const int numCommands = sizeof(commands) / sizeof(commands[0]);
    CommandDict commandDictionary; // case-insensitive dictionary
    for (int i = 0; i < numCommands; ++i) {
        if (commands[i].name)
            commandDictionary.insert(QString(commands[i].name).toLower(), &commands[i].imp);
    }
    return commandDictionary;
}

const CommandImp *JSEditor::commandImp(const DOMString &command)
{
    static CommandDict commandDictionary = createCommandDictionary();
    const CommandImp *result = commandDictionary.value( command.string().toLower() );
#ifdef DEBUG_COMMANDS
    if (!result)
        qDebug() << "[Command is not supported yet]" << command << endl;
#endif
    return result;
}

const CommandImp *JSEditor::commandImp(int command)
{
    if (command < 0 || command >= int(sizeof commands / sizeof commands[0]) )
        return 0;
    return &commands[command].imp;
}



} // namespace DOM

#undef KPAC
