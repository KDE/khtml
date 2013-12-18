/* This file is part of the KDE project
 *
 * Copyright (C) 2004 Leo Savernik <l.savernik@aon.at>
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
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef EDITOR_COMMAND_H
#define EDITOR_COMMAND_H

namespace DOM {

/**
 * List of all supported built-in editor commands.
 */
enum EditorCommand {
    BackColorCommand,
    BoldCommand,
    CopyCommand,
    CutCommand,
    DeleteCommand,
    FontNameCommand,
    FontSizeCommand,
    ForeColorCommand,
    IndentCommand,
    InsertNewlineCommand,
    InsertParagraphCommand,
    InsertTextCommand,
    ItalicCommand,
    JustifyCenterCommand,
    JustifyFullCommand,
    JustifyLeftCommand,
    JustifyNoneCommand,
    JustifyRightCommand,
    OutdentCommand,
    PasteCommand,
    PrintCommand,
    RedoCommand,
    SelectAllCommand,
    SubscriptCommand,
    SuperscriptCommand,
    UndoCommand,
    UnselectCommand
  
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

}/*namespace DOM*/


#endif
