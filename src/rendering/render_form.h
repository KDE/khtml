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

namespace DOM {
    class HTMLInputElementImpl;
    class HTMLSelectElementImpl;
    class HTMLGenericFormElementImpl;
    class HTMLTextAreaElementImpl;
}

namespace khtml {


// -------------------------------------------------------------------------

class RenderFormElement : public khtml::RenderWidget
{
public:
    RenderFormElement(DOM::HTMLGenericFormElementImpl* node);
    virtual ~RenderFormElement();

    virtual const char *renderName() const { return "RenderForm"; }
    virtual void setStyle(RenderStyle *style);

    virtual bool isFormElement() const { return true; }

    // form elements apply the padding themselves, so
    // the rest of the layout should disregard it
    virtual int paddingTop() const;
    virtual int paddingBottom() const;
    virtual int paddingLeft() const;
    virtual int paddingRight() const;

    // some widgets don't apply padding, and thus it is ignored
    // entirely, this function allows them to opt out
    virtual bool includesPadding() const;

    virtual void updateFromElement();

    virtual void layout();
    virtual void calcWidth();
    virtual void calcHeight();
    virtual void calcMinMaxWidth();
    virtual short baselinePosition( bool ) const;
    virtual int calcContentWidth(int w) const;
    virtual int calcContentHeight(int h) const;

    DOM::HTMLGenericFormElementImpl *element() const
    { return static_cast<DOM::HTMLGenericFormElementImpl*>(RenderObject::element()); }

    // this is not a virtual function
    void setQWidget(QWidget *w);

protected:
    virtual bool isRenderButton() const { return false; }
    virtual bool isEditable() const { return false; }
    virtual void setPadding();
    virtual void paintOneBackground(QPainter *p, const QColor& c, const BackgroundLayer* bgLayer, QRect clipr, int _tx, int _ty, int w, int height);

    Qt::Alignment textAlignment() const;
    QProxyStyle* getProxyStyle();
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
    RenderButton(DOM::HTMLGenericFormElementImpl* node);

    virtual const char *renderName() const { return "RenderButton"; }

    virtual void layout();
    virtual short baselinePosition( bool ) const;
    virtual void setStyle(RenderStyle *style);
    virtual bool forceTransparentText() const { return m_hasTextIndentHack; }

    // don't even think about making this method virtual!
    DOM::HTMLInputElementImpl* element() const
    { return static_cast<DOM::HTMLInputElementImpl*>(RenderObject::element()); }

protected:
    virtual bool isRenderButton() const { return true; }

    bool m_hasTextIndentHack;
};

// -------------------------------------------------------------------------

class CheckBoxWidget: public QCheckBox, public KHTMLWidget
{
public:
    CheckBoxWidget(QWidget *p): QCheckBox(p) { m_kwp->setIsRedirected(true); }
};

class RenderCheckBox : public RenderButton
{
    Q_OBJECT
public:
    RenderCheckBox(DOM::HTMLInputElementImpl* node);

    virtual const char *renderName() const { return "RenderCheckBox"; }
    virtual void updateFromElement();
    virtual void calcMinMaxWidth();

    virtual bool handleEvent(const DOM::EventImpl&);

    QCheckBox *widget() const { return static_cast<QCheckBox*>(m_widget); }
protected:
    virtual bool includesPadding() const { return false; }
public Q_SLOTS:
    virtual void slotStateChanged(int state);
private:
    bool m_ignoreStateChanged;
};

// -------------------------------------------------------------------------

class RadioButtonWidget: public QRadioButton, public KHTMLWidget
{
public:
    RadioButtonWidget(QWidget* p): QRadioButton(p) { m_kwp->setIsRedirected(true); }
};

class RenderRadioButton : public RenderButton
{
    Q_OBJECT
public:
    RenderRadioButton(DOM::HTMLInputElementImpl* node);

    virtual const char *renderName() const { return "RenderRadioButton"; }

    virtual void calcMinMaxWidth();
    virtual void updateFromElement();

    virtual bool handleEvent(const DOM::EventImpl&);

    QRadioButton *widget() const { return static_cast<QRadioButton*>(m_widget); }
protected:
    virtual bool includesPadding() const { return false; }
public Q_SLOTS:
    virtual void slotToggled(bool);
private:
    bool m_ignoreToggled;
};


// -------------------------------------------------------------------------

class PushButtonWidget: public QPushButton, public KHTMLWidget
{
public:
    PushButtonWidget(QWidget* p): QPushButton(p) { m_kwp->setIsRedirected(true); }
};

class RenderSubmitButton : public RenderButton
{
public:
    RenderSubmitButton(DOM::HTMLInputElementImpl *element);

    virtual const char *renderName() const { return "RenderSubmitButton"; }

    virtual void calcMinMaxWidth();
    virtual void updateFromElement();
    virtual short baselinePosition( bool ) const;
protected:
    virtual void setPadding();
    virtual void setStyle(RenderStyle *style);
    virtual bool canHaveBorder() const;
private:
    QString rawText();
};

// -------------------------------------------------------------------------

class RenderImageButton : public RenderImage
{
public:
    RenderImageButton(DOM::HTMLInputElementImpl *element)
        : RenderImage(element) {}

    virtual const char *renderName() const { return "RenderImageButton"; }
};


// -------------------------------------------------------------------------

class RenderResetButton : public RenderSubmitButton
{
public:
    RenderResetButton(DOM::HTMLInputElementImpl *element);

    virtual const char *renderName() const { return "RenderResetButton"; }

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

    virtual void calcMinMaxWidth();

    virtual const char *renderName() const { return "RenderLineEdit"; }
    virtual void updateFromElement();
    virtual void setStyle(RenderStyle *style);
    virtual short baselinePosition( bool ) const;
    virtual void handleFocusOut();

    void select();

    KLineEdit *widget() const { return static_cast<KLineEdit*>(m_widget); }
    DOM::HTMLInputElementImpl* element() const
    { return static_cast<DOM::HTMLInputElementImpl*>(RenderObject::element()); }
    void highLightWord( unsigned int length, unsigned int pos );

    long selectionStart();
    long selectionEnd();
    void setSelectionStart(long pos);
    void setSelectionEnd(long pos);
    void setSelectionRange(long start, long end);
public Q_SLOTS:
    void slotReturnPressed();
    void slotTextChanged(const QString &string);
protected:
    virtual bool canHaveBorder() const { return true; }
private:
    virtual bool isEditable() const { return true; }
    bool m_blockElementUpdates;
};

// -------------------------------------------------------------------------

class LineEditWidget : public KLineEdit, public KHTMLWidget
{
    Q_OBJECT
public:
    LineEditWidget(DOM::HTMLInputElementImpl* input,
                   KHTMLView* view, QWidget* parent);
    ~LineEditWidget();
    void setFocus();
    void highLightWord( unsigned int length, unsigned int pos );

protected:
    virtual bool event( QEvent *e );
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void contextMenuEvent(QContextMenuEvent *e);
private Q_SLOTS:
    void clearHistoryActivated();
    void slotCheckSpelling();
    void slotSpellCheckDone( const QString &s );
    void slotCreateWebShortcut();
    void spellCheckerMisspelling( const QString &text, int pos);
    void spellCheckerCorrected( const QString &, int, const QString &);
    void spellCheckerFinished();

private:
    enum LineEditMenuID {
        ClearHistory
    };
    DOM::HTMLInputElementImpl* m_input;
    KHTMLView* m_view;
    QAction *m_spellAction;
};

// -------------------------------------------------------------------------

class RenderFieldset : public RenderBlock
{
public:
    RenderFieldset(DOM::HTMLGenericFormElementImpl *element);

    virtual void calcMinMaxWidth();
    virtual const char *renderName() const { return "RenderFieldSet"; }
    virtual RenderObject* layoutLegend(bool relayoutChildren);
    virtual void setStyle(RenderStyle* _style);

protected:
    virtual void paintBoxDecorations(PaintInfo& pI, int _tx, int _ty);
    void paintBorderMinusLegend(QPainter *p, int _tx, int _ty, int w,
                                  int h, const RenderStyle *style, int lx, int lw, int lb);
    RenderObject* findLegend() const;
    virtual short intrinsicWidth() const { return m_intrinsicWidth; }
    int m_intrinsicWidth;
};

// -------------------------------------------------------------------------

class FileButtonWidget: public KUrlRequester, public KHTMLWidget
{
public:
    FileButtonWidget(QWidget* p): KUrlRequester(p) { m_kwp->setIsRedirected(true); }
};

class RenderFileButton : public RenderFormElement
{
    Q_OBJECT
public:
    RenderFileButton(DOM::HTMLInputElementImpl *element);

    virtual const char *renderName() const { return "RenderFileButton"; }
    virtual void calcMinMaxWidth();
    virtual void updateFromElement();
    virtual short baselinePosition( bool ) const;
    void select();

    KUrlRequester *widget() const { return static_cast<KUrlRequester*>(m_widget); }

    DOM::HTMLInputElementImpl *element() const
    { return static_cast<DOM::HTMLInputElementImpl*>(RenderObject::element()); }

public Q_SLOTS:
    void slotReturnPressed();
    void slotTextChanged(const QString &string);
    void slotUrlSelected();

protected:
    virtual void handleFocusOut();

    virtual bool isEditable() const { return true; }
    virtual bool acceptsSyntheticEvents() const { return false; }

    virtual bool includesPadding() const { return false; }

    bool m_clicked;
    bool m_haveFocus;
};


// -------------------------------------------------------------------------

class RenderLabel : public RenderFormElement
{
public:
    RenderLabel(DOM::HTMLGenericFormElementImpl *element);

    virtual const char *renderName() const { return "RenderLabel"; }

protected:
    virtual bool canHaveBorder() const { return true; }
};


// -------------------------------------------------------------------------

class RenderLegend : public RenderBlock
{
public:
    RenderLegend(DOM::HTMLGenericFormElementImpl *element);

    virtual const char *renderName() const { return "RenderLegend"; }
};

// -------------------------------------------------------------------------

class ComboBoxWidget : public KComboBox, public KHTMLWidget
{
public:
    ComboBoxWidget(QWidget *parent);

protected:
    virtual bool event(QEvent *);
    virtual bool eventFilter(QObject *dest, QEvent *e);
    virtual void keyPressEvent(QKeyEvent *e);
    virtual void showPopup();
    virtual void hidePopup();
};

// -------------------------------------------------------------------------

class ListBoxWidget: public QListWidget, public KHTMLWidget
{
public:
    ListBoxWidget(QWidget* p): QListWidget(p) { m_kwp->setIsRedirected(true); }
protected:
    void scrollContentsBy(int, int)
    {
        viewport()->update();
    }
    virtual bool event( QEvent * event );
};

class RenderSelect : public RenderFormElement
{
    Q_OBJECT
public:
    RenderSelect(DOM::HTMLSelectElementImpl *element);

    virtual const char *renderName() const { return "RenderSelect"; }

    virtual void calcMinMaxWidth();
    virtual void layout();

    void setOptionsChanged(bool _optionsChanged);

    bool selectionChanged() { return m_selectionChanged; }
    void setSelectionChanged(bool _selectionChanged) { m_selectionChanged = _selectionChanged; }
    virtual void setStyle(RenderStyle* _style);
    virtual void updateFromElement();
    virtual short baselinePosition( bool ) const;

    void updateSelection();

    DOM::HTMLSelectElementImpl *element() const
    { return static_cast<DOM::HTMLSelectElementImpl*>(RenderObject::element()); }

protected:
    void setPadding();
    ListBoxWidget *createListBox();
    ComboBoxWidget *createComboBox();

    unsigned  m_size;
    bool m_multiple;
    bool m_useListBox;
    bool m_selectionChanged;
    bool m_ignoreSelectEvents;
    bool m_optionsChanged;

    void clearItemFlags(int index, Qt::ItemFlags flags);
    virtual bool canHaveBorder() const { return true; }

protected Q_SLOTS:
    void slotSelected(int index);
    void slotSelectionChanged();
};

// -------------------------------------------------------------------------
class TextAreaWidget : public KTextEdit, public KHTMLWidget
{
    Q_OBJECT
public:
    TextAreaWidget(int wrap, QWidget* parent);
    virtual ~TextAreaWidget();

protected:
    virtual bool event (QEvent *e );
    virtual void keyPressEvent(QKeyEvent *e);
    virtual void scrollContentsBy(int dx, int dy);

};


// -------------------------------------------------------------------------

class RenderTextArea : public RenderFormElement
{
    Q_OBJECT
public:
    RenderTextArea(DOM::HTMLTextAreaElementImpl *element);
    ~RenderTextArea();

    virtual const char *renderName() const { return "RenderTextArea"; }
    virtual void calcMinMaxWidth();
    virtual void setStyle(RenderStyle *style);

    virtual short scrollWidth() const;
    virtual int scrollHeight() const;

    virtual void updateFromElement();

    // don't even think about making this method virtual!
    TextAreaWidget *widget() const { return static_cast<TextAreaWidget*>(m_widget); }
    DOM::HTMLTextAreaElementImpl* element() const
    { return static_cast<DOM::HTMLTextAreaElementImpl*>(RenderObject::element()); }

    QString text();
    void setText(const QString& text);

    void highLightWord( unsigned int length, unsigned int pos );

    void select();

    long selectionStart();
    long selectionEnd();
    void setSelectionStart(long pos);
    void setSelectionEnd(long pos);
    void setSelectionRange(long start, long end);
protected Q_SLOTS:
    void slotTextChanged();

protected:
    virtual void handleFocusOut();

    virtual bool isEditable() const { return true; }
    virtual bool canHaveBorder() const { return true; }

    Qt::Alignment m_textAlignment;
};

// -------------------------------------------------------------------------

class ScrollBarWidget: public QScrollBar, public KHTMLWidget
{
public:
    ScrollBarWidget( QWidget * parent = 0 ): QScrollBar(parent) { m_kwp->setIsRedirected( true ); }
    ScrollBarWidget( Qt::Orientation orientation, QWidget * parent = 0 ): QScrollBar(orientation, parent) { m_kwp->setIsRedirected( true ); }
};

} //namespace

#endif
