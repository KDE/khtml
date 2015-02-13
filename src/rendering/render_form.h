/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000-2003 Dirk Mueller (mueller@kde.org)
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
#ifndef RENDER_FORM_H
#define RENDER_FORM_H

#include "rendering/render_replaced.h"
#include "rendering/render_image.h"
#include "rendering/render_flow.h"
#include "rendering/render_style.h"
#include "khtmlview.h"
#include "html/html_formimpl.h"

class QWidget;

#include <ktextedit.h>
#include <kurlrequester.h>
#include <klineedit.h>
#include <QCheckBox>
#include <QRadioButton>
#include <QPushButton>
#include <QListWidget>
#include <QProxyStyle>
#include <kcombobox.h>
#include "dom/dom_misc.h"

class QAction;
class KUrlRequester;

namespace DOM
{
class HTMLInputElementImpl;
class HTMLSelectElementImpl;
class HTMLGenericFormElementImpl;
class HTMLTextAreaElementImpl;
}

namespace khtml
{

// -------------------------------------------------------------------------

class RenderFormElement : public khtml::RenderWidget
{
public:
    RenderFormElement(DOM::HTMLGenericFormElementImpl *node);
    virtual ~RenderFormElement();

    const char *renderName() const Q_DECL_OVERRIDE
    {
        return "RenderForm";
    }
    void setStyle(RenderStyle *style) Q_DECL_OVERRIDE;

    bool isFormElement() const Q_DECL_OVERRIDE
    {
        return true;
    }

    // form elements apply the padding themselves, so
    // the rest of the layout should disregard it
    int paddingTop() const Q_DECL_OVERRIDE;
    int paddingBottom() const Q_DECL_OVERRIDE;
    int paddingLeft() const Q_DECL_OVERRIDE;
    int paddingRight() const Q_DECL_OVERRIDE;

    // some widgets don't apply padding, and thus it is ignored
    // entirely, this function allows them to opt out
    bool includesPadding() const Q_DECL_OVERRIDE;

    void updateFromElement() Q_DECL_OVERRIDE;

    void layout() Q_DECL_OVERRIDE;
    void calcWidth() Q_DECL_OVERRIDE;
    void calcHeight() Q_DECL_OVERRIDE;
    void calcMinMaxWidth() Q_DECL_OVERRIDE;
    short baselinePosition(bool) const Q_DECL_OVERRIDE;
    int calcContentWidth(int w) const Q_DECL_OVERRIDE;
    int calcContentHeight(int h) const Q_DECL_OVERRIDE;

    DOM::HTMLGenericFormElementImpl *element() const
    {
        return static_cast<DOM::HTMLGenericFormElementImpl *>(RenderObject::element());
    }

    // this is not a virtual function
    void setQWidget(QWidget *w);

protected:
    virtual bool isRenderButton() const
    {
        return false;
    }
    bool isEditable() const Q_DECL_OVERRIDE
    {
        return false;
    }
    virtual void setPadding();
    void paintOneBackground(QPainter *p, const QColor &c, const BackgroundLayer *bgLayer, QRect clipr, int _tx, int _ty, int w, int height) Q_DECL_OVERRIDE;

    Qt::Alignment textAlignment() const;
    QProxyStyle *getProxyStyle();
    QProxyStyle *m_proxyStyle;
    bool m_exposeInternalPadding;
    bool m_isOxygenStyle;
};

// -------------------------------------------------------------------------

// generic class for all buttons
class RenderButton : public RenderFormElement
{
    Q_OBJECT
public:
    RenderButton(DOM::HTMLGenericFormElementImpl *node);

    const char *renderName() const Q_DECL_OVERRIDE
    {
        return "RenderButton";
    }

    void layout() Q_DECL_OVERRIDE;
    short baselinePosition(bool) const Q_DECL_OVERRIDE;
    void setStyle(RenderStyle *style) Q_DECL_OVERRIDE;
    bool forceTransparentText() const Q_DECL_OVERRIDE
    {
        return m_hasTextIndentHack;
    }

    // don't even think about making this method virtual!
    DOM::HTMLInputElementImpl *element() const
    {
        return static_cast<DOM::HTMLInputElementImpl *>(RenderObject::element());
    }

protected:
    bool isRenderButton() const Q_DECL_OVERRIDE
    {
        return true;
    }

    bool m_hasTextIndentHack;
};

// -------------------------------------------------------------------------

class CheckBoxWidget: public QCheckBox, public KHTMLWidget
{
public:
    CheckBoxWidget(QWidget *p): QCheckBox(p)
    {
        m_kwp->setIsRedirected(true);
    }
};

class RenderCheckBox : public RenderButton
{
    Q_OBJECT
public:
    RenderCheckBox(DOM::HTMLInputElementImpl *node);

    const char *renderName() const Q_DECL_OVERRIDE
    {
        return "RenderCheckBox";
    }
    void updateFromElement() Q_DECL_OVERRIDE;
    void calcMinMaxWidth() Q_DECL_OVERRIDE;

    bool handleEvent(const DOM::EventImpl &) Q_DECL_OVERRIDE;

    QCheckBox *widget() const
    {
        return static_cast<QCheckBox *>(m_widget);
    }
protected:
    bool includesPadding() const Q_DECL_OVERRIDE
    {
        return false;
    }
public Q_SLOTS:
    virtual void slotStateChanged(int state);
private:
    bool m_ignoreStateChanged;
};

// -------------------------------------------------------------------------

class RadioButtonWidget: public QRadioButton, public KHTMLWidget
{
public:
    RadioButtonWidget(QWidget *p): QRadioButton(p)
    {
        m_kwp->setIsRedirected(true);
    }
};

class RenderRadioButton : public RenderButton
{
    Q_OBJECT
public:
    RenderRadioButton(DOM::HTMLInputElementImpl *node);

    const char *renderName() const Q_DECL_OVERRIDE
    {
        return "RenderRadioButton";
    }

    void calcMinMaxWidth() Q_DECL_OVERRIDE;
    void updateFromElement() Q_DECL_OVERRIDE;

    bool handleEvent(const DOM::EventImpl &) Q_DECL_OVERRIDE;

    QRadioButton *widget() const
    {
        return static_cast<QRadioButton *>(m_widget);
    }
protected:
    bool includesPadding() const Q_DECL_OVERRIDE
    {
        return false;
    }
public Q_SLOTS:
    virtual void slotToggled(bool);
private:
    bool m_ignoreToggled;
};

// -------------------------------------------------------------------------

class PushButtonWidget: public QPushButton, public KHTMLWidget
{
public:
    PushButtonWidget(QWidget *p): QPushButton(p)
    {
        m_kwp->setIsRedirected(true);
    }
};

class RenderSubmitButton : public RenderButton
{
public:
    RenderSubmitButton(DOM::HTMLInputElementImpl *element);

    const char *renderName() const Q_DECL_OVERRIDE
    {
        return "RenderSubmitButton";
    }

    void calcMinMaxWidth() Q_DECL_OVERRIDE;
    void updateFromElement() Q_DECL_OVERRIDE;
    short baselinePosition(bool) const Q_DECL_OVERRIDE;
protected:
    void setPadding() Q_DECL_OVERRIDE;
    void setStyle(RenderStyle *style) Q_DECL_OVERRIDE;
    bool canHaveBorder() const Q_DECL_OVERRIDE;
private:
    QString rawText();
};

// -------------------------------------------------------------------------

class RenderImageButton : public RenderImage
{
public:
    RenderImageButton(DOM::HTMLInputElementImpl *element)
        : RenderImage(element) {}

    const char *renderName() const Q_DECL_OVERRIDE
    {
        return "RenderImageButton";
    }
};

// -------------------------------------------------------------------------

class RenderResetButton : public RenderSubmitButton
{
public:
    RenderResetButton(DOM::HTMLInputElementImpl *element);

    const char *renderName() const Q_DECL_OVERRIDE
    {
        return "RenderResetButton";
    }

};

// -------------------------------------------------------------------------

class RenderPushButton : public RenderSubmitButton
{
public:
    RenderPushButton(DOM::HTMLInputElementImpl *element)
        : RenderSubmitButton(element) {}

};

// -------------------------------------------------------------------------

class RenderLineEdit : public RenderFormElement
{
    Q_OBJECT
public:
    RenderLineEdit(DOM::HTMLInputElementImpl *element);

    void calcMinMaxWidth() Q_DECL_OVERRIDE;

    const char *renderName() const Q_DECL_OVERRIDE
    {
        return "RenderLineEdit";
    }
    void updateFromElement() Q_DECL_OVERRIDE;
    void setStyle(RenderStyle *style) Q_DECL_OVERRIDE;
    short baselinePosition(bool) const Q_DECL_OVERRIDE;
    void handleFocusOut() Q_DECL_OVERRIDE;

    void select();

    KLineEdit *widget() const
    {
        return static_cast<KLineEdit *>(m_widget);
    }
    DOM::HTMLInputElementImpl *element() const
    {
        return static_cast<DOM::HTMLInputElementImpl *>(RenderObject::element());
    }
    void highLightWord(unsigned int length, unsigned int pos);

    long selectionStart();
    long selectionEnd();
    void setSelectionStart(long pos);
    void setSelectionEnd(long pos);
    void setSelectionRange(long start, long end);
public Q_SLOTS:
    void slotReturnPressed();
    void slotTextChanged(const QString &string);
protected:
    bool canHaveBorder() const Q_DECL_OVERRIDE
    {
        return true;
    }
private:
    bool isEditable() const Q_DECL_OVERRIDE
    {
        return true;
    }
    bool m_blockElementUpdates;
};

// -------------------------------------------------------------------------

class LineEditWidget : public KLineEdit, public KHTMLWidget
{
    Q_OBJECT
public:
    LineEditWidget(DOM::HTMLInputElementImpl *input,
                   KHTMLView *view, QWidget *parent);
    ~LineEditWidget();
    void setFocus();
    void highLightWord(unsigned int length, unsigned int pos);

protected:
    bool event(QEvent *e) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void contextMenuEvent(QContextMenuEvent *e) Q_DECL_OVERRIDE;
private Q_SLOTS:
    void clearHistoryActivated();
    void slotCheckSpelling();
    void slotSpellCheckDone(const QString &s);
    void slotCreateWebShortcut();
    void spellCheckerMisspelling(const QString &text, int pos);
    void spellCheckerCorrected(const QString &, int, const QString &);
    void spellCheckerFinished();

private:
    enum LineEditMenuID {
        ClearHistory
    };
    DOM::HTMLInputElementImpl *m_input;
    KHTMLView *m_view;
    QAction *m_spellAction;
};

// -------------------------------------------------------------------------

class RenderFieldset : public RenderBlock
{
public:
    RenderFieldset(DOM::HTMLGenericFormElementImpl *element);

    void calcMinMaxWidth() Q_DECL_OVERRIDE;
    const char *renderName() const Q_DECL_OVERRIDE
    {
        return "RenderFieldSet";
    }
    RenderObject *layoutLegend(bool relayoutChildren) Q_DECL_OVERRIDE;
    void setStyle(RenderStyle *_style) Q_DECL_OVERRIDE;

protected:
    void paintBoxDecorations(PaintInfo &pI, int _tx, int _ty) Q_DECL_OVERRIDE;
    void paintBorderMinusLegend(QPainter *p, int _tx, int _ty, int w,
                                int h, const RenderStyle *style, int lx, int lw, int lb);
    RenderObject *findLegend() const;
    short intrinsicWidth() const Q_DECL_OVERRIDE
    {
        return m_intrinsicWidth;
    }
    int m_intrinsicWidth;
};

// -------------------------------------------------------------------------

class FileButtonWidget: public KUrlRequester, public KHTMLWidget
{
public:
    FileButtonWidget(QWidget *p): KUrlRequester(p)
    {
        m_kwp->setIsRedirected(true);
    }
};

class RenderFileButton : public RenderFormElement
{
    Q_OBJECT
public:
    RenderFileButton(DOM::HTMLInputElementImpl *element);

    const char *renderName() const Q_DECL_OVERRIDE
    {
        return "RenderFileButton";
    }
    void calcMinMaxWidth() Q_DECL_OVERRIDE;
    void updateFromElement() Q_DECL_OVERRIDE;
    short baselinePosition(bool) const Q_DECL_OVERRIDE;
    void select();

    KUrlRequester *widget() const
    {
        return static_cast<KUrlRequester *>(m_widget);
    }

    DOM::HTMLInputElementImpl *element() const
    {
        return static_cast<DOM::HTMLInputElementImpl *>(RenderObject::element());
    }

public Q_SLOTS:
    void slotReturnPressed();
    void slotTextChanged(const QString &string);
    void slotUrlSelected();

protected:
    void handleFocusOut() Q_DECL_OVERRIDE;

    bool isEditable() const Q_DECL_OVERRIDE
    {
        return true;
    }
    bool acceptsSyntheticEvents() const Q_DECL_OVERRIDE
    {
        return false;
    }

    bool includesPadding() const Q_DECL_OVERRIDE
    {
        return false;
    }

    bool m_clicked;
    bool m_haveFocus;
};

// -------------------------------------------------------------------------

class RenderLabel : public RenderFormElement
{
public:
    RenderLabel(DOM::HTMLGenericFormElementImpl *element);

    const char *renderName() const Q_DECL_OVERRIDE
    {
        return "RenderLabel";
    }

protected:
    bool canHaveBorder() const Q_DECL_OVERRIDE
    {
        return true;
    }
};

// -------------------------------------------------------------------------

class RenderLegend : public RenderBlock
{
public:
    RenderLegend(DOM::HTMLGenericFormElementImpl *element);

    const char *renderName() const Q_DECL_OVERRIDE
    {
        return "RenderLegend";
    }
};

// -------------------------------------------------------------------------

class ComboBoxWidget : public KComboBox, public KHTMLWidget
{
public:
    ComboBoxWidget(QWidget *parent);

protected:
    bool event(QEvent *) Q_DECL_OVERRIDE;
    bool eventFilter(QObject *dest, QEvent *e) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *e) Q_DECL_OVERRIDE;
    void showPopup() Q_DECL_OVERRIDE;
    void hidePopup() Q_DECL_OVERRIDE;
};

// -------------------------------------------------------------------------

class ListBoxWidget: public QListWidget, public KHTMLWidget
{
public:
    ListBoxWidget(QWidget *p): QListWidget(p)
    {
        m_kwp->setIsRedirected(true);
    }
protected:
    void scrollContentsBy(int, int) Q_DECL_OVERRIDE
    {
        viewport()->update();
    }
    bool event(QEvent *event) Q_DECL_OVERRIDE;
};

class RenderSelect : public RenderFormElement
{
    Q_OBJECT
public:
    RenderSelect(DOM::HTMLSelectElementImpl *element);

    const char *renderName() const Q_DECL_OVERRIDE
    {
        return "RenderSelect";
    }

    void calcMinMaxWidth() Q_DECL_OVERRIDE;
    void layout() Q_DECL_OVERRIDE;

    void setOptionsChanged(bool _optionsChanged);

    bool selectionChanged()
    {
        return m_selectionChanged;
    }
    void setSelectionChanged(bool _selectionChanged)
    {
        m_selectionChanged = _selectionChanged;
    }
    void setStyle(RenderStyle *_style) Q_DECL_OVERRIDE;
    void updateFromElement() Q_DECL_OVERRIDE;
    short baselinePosition(bool) const Q_DECL_OVERRIDE;

    void updateSelection();

    DOM::HTMLSelectElementImpl *element() const
    {
        return static_cast<DOM::HTMLSelectElementImpl *>(RenderObject::element());
    }

protected:
    void setPadding() Q_DECL_OVERRIDE;
    ListBoxWidget *createListBox();
    ComboBoxWidget *createComboBox();

    unsigned  m_size;
    bool m_multiple;
    bool m_useListBox;
    bool m_selectionChanged;
    bool m_ignoreSelectEvents;
    bool m_optionsChanged;

    void clearItemFlags(int index, Qt::ItemFlags flags);
    bool canHaveBorder() const Q_DECL_OVERRIDE
    {
        return true;
    }

protected Q_SLOTS:
    void slotSelected(int index);
    void slotSelectionChanged();
};

// -------------------------------------------------------------------------
class TextAreaWidget : public KTextEdit, public KHTMLWidget
{
    Q_OBJECT
public:
    TextAreaWidget(int wrap, QWidget *parent);
    virtual ~TextAreaWidget();

protected:
    bool event(QEvent *e) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *e) Q_DECL_OVERRIDE;
    void scrollContentsBy(int dx, int dy) Q_DECL_OVERRIDE;

};

// -------------------------------------------------------------------------

class RenderTextArea : public RenderFormElement
{
    Q_OBJECT
public:
    RenderTextArea(DOM::HTMLTextAreaElementImpl *element);
    ~RenderTextArea();

    const char *renderName() const Q_DECL_OVERRIDE
    {
        return "RenderTextArea";
    }
    void calcMinMaxWidth() Q_DECL_OVERRIDE;
    void setStyle(RenderStyle *style) Q_DECL_OVERRIDE;

    short scrollWidth() const Q_DECL_OVERRIDE;
    int scrollHeight() const Q_DECL_OVERRIDE;

    void updateFromElement() Q_DECL_OVERRIDE;

    // don't even think about making this method virtual!
    TextAreaWidget *widget() const
    {
        return static_cast<TextAreaWidget *>(m_widget);
    }
    DOM::HTMLTextAreaElementImpl *element() const
    {
        return static_cast<DOM::HTMLTextAreaElementImpl *>(RenderObject::element());
    }

    QString text();
    void setText(const QString &text);

    void highLightWord(unsigned int length, unsigned int pos);

    void select();

    long selectionStart();
    long selectionEnd();
    void setSelectionStart(long pos);
    void setSelectionEnd(long pos);
    void setSelectionRange(long start, long end);
protected Q_SLOTS:
    void slotTextChanged();

protected:
    void handleFocusOut() Q_DECL_OVERRIDE;

    bool isEditable() const Q_DECL_OVERRIDE
    {
        return true;
    }
    bool canHaveBorder() const Q_DECL_OVERRIDE
    {
        return true;
    }

    Qt::Alignment m_textAlignment;
};

// -------------------------------------------------------------------------

class ScrollBarWidget: public QScrollBar, public KHTMLWidget
{
public:
    ScrollBarWidget(QWidget *parent = 0): QScrollBar(parent)
    {
        m_kwp->setIsRedirected(true);
    }
    ScrollBarWidget(Qt::Orientation orientation, QWidget *parent = 0): QScrollBar(orientation, parent)
    {
        m_kwp->setIsRedirected(true);
    }
};

} //namespace

#endif
