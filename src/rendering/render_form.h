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

    const char *renderName() const override
    {
        return "RenderForm";
    }
    void setStyle(RenderStyle *style) override;

    bool isFormElement() const override
    {
        return true;
    }

    // form elements apply the padding themselves, so
    // the rest of the layout should disregard it
    int paddingTop() const override;
    int paddingBottom() const override;
    int paddingLeft() const override;
    int paddingRight() const override;

    // some widgets don't apply padding, and thus it is ignored
    // entirely, this function allows them to opt out
    bool includesPadding() const override;

    void updateFromElement() override;

    void layout() override;
    void calcWidth() override;
    void calcHeight() override;
    void calcMinMaxWidth() override;
    short baselinePosition(bool) const override;
    int calcContentWidth(int w) const override;
    int calcContentHeight(int h) const override;

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
    bool isEditable() const override
    {
        return false;
    }
    virtual void setPadding();
    void paintOneBackground(QPainter *p, const QColor &c, const BackgroundLayer *bgLayer, QRect clipr, int _tx, int _ty, int w, int height) override;

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

    const char *renderName() const override
    {
        return "RenderButton";
    }

    void layout() override;
    short baselinePosition(bool) const override;
    void setStyle(RenderStyle *style) override;
    bool forceTransparentText() const override
    {
        return m_hasTextIndentHack;
    }

    // don't even think about making this method virtual!
    DOM::HTMLInputElementImpl *element() const
    {
        return static_cast<DOM::HTMLInputElementImpl *>(RenderObject::element());
    }

protected:
    bool isRenderButton() const override
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

    const char *renderName() const override
    {
        return "RenderCheckBox";
    }
    void updateFromElement() override;
    void calcMinMaxWidth() override;

    bool handleEvent(const DOM::EventImpl &) override;

    QCheckBox *widget() const
    {
        return static_cast<QCheckBox *>(m_widget);
    }
protected:
    bool includesPadding() const override
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

    const char *renderName() const override
    {
        return "RenderRadioButton";
    }

    void calcMinMaxWidth() override;
    void updateFromElement() override;

    bool handleEvent(const DOM::EventImpl &) override;

    QRadioButton *widget() const
    {
        return static_cast<QRadioButton *>(m_widget);
    }
protected:
    bool includesPadding() const override
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

    const char *renderName() const override
    {
        return "RenderSubmitButton";
    }

    void calcMinMaxWidth() override;
    void updateFromElement() override;
    short baselinePosition(bool) const override;
protected:
    void setPadding() override;
    void setStyle(RenderStyle *style) override;
    bool canHaveBorder() const override;
private:
    QString rawText();
};

// -------------------------------------------------------------------------

class RenderImageButton : public RenderImage
{
public:
    RenderImageButton(DOM::HTMLInputElementImpl *element)
        : RenderImage(element) {}

    const char *renderName() const override
    {
        return "RenderImageButton";
    }
};

// -------------------------------------------------------------------------

class RenderResetButton : public RenderSubmitButton
{
public:
    RenderResetButton(DOM::HTMLInputElementImpl *element);

    const char *renderName() const override
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

    void calcMinMaxWidth() override;

    const char *renderName() const override
    {
        return "RenderLineEdit";
    }
    void updateFromElement() override;
    void setStyle(RenderStyle *style) override;
    short baselinePosition(bool) const override;
    void handleFocusOut() override;

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
    bool canHaveBorder() const override
    {
        return true;
    }
private:
    bool isEditable() const override
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
    bool event(QEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *e) override;
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

    void calcMinMaxWidth() override;
    const char *renderName() const override
    {
        return "RenderFieldSet";
    }
    RenderObject *layoutLegend(bool relayoutChildren) override;
    void setStyle(RenderStyle *_style) override;

protected:
    void paintBoxDecorations(PaintInfo &pI, int _tx, int _ty) override;
    void paintBorderMinusLegend(QPainter *p, int _tx, int _ty, int w,
                                int h, const RenderStyle *style, int lx, int lw, int lb);
    RenderObject *findLegend() const;
    short intrinsicWidth() const override
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

    const char *renderName() const override
    {
        return "RenderFileButton";
    }
    void calcMinMaxWidth() override;
    void updateFromElement() override;
    short baselinePosition(bool) const override;
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
    void handleFocusOut() override;

    bool isEditable() const override
    {
        return true;
    }
    bool acceptsSyntheticEvents() const override
    {
        return false;
    }

    bool includesPadding() const override
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

    const char *renderName() const override
    {
        return "RenderLabel";
    }

protected:
    bool canHaveBorder() const override
    {
        return true;
    }
};

// -------------------------------------------------------------------------

class RenderLegend : public RenderBlock
{
public:
    RenderLegend(DOM::HTMLGenericFormElementImpl *element);

    const char *renderName() const override
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
    bool event(QEvent *) override;
    bool eventFilter(QObject *dest, QEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void showPopup() override;
    void hidePopup() override;
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
    void scrollContentsBy(int, int) override
    {
        viewport()->update();
    }
    bool event(QEvent *event) override;
};

class RenderSelect : public RenderFormElement
{
    Q_OBJECT
public:
    RenderSelect(DOM::HTMLSelectElementImpl *element);

    const char *renderName() const override
    {
        return "RenderSelect";
    }

    void calcMinMaxWidth() override;
    void layout() override;

    void setOptionsChanged(bool _optionsChanged);

    bool selectionChanged()
    {
        return m_selectionChanged;
    }
    void setSelectionChanged(bool _selectionChanged)
    {
        m_selectionChanged = _selectionChanged;
    }
    void setStyle(RenderStyle *_style) override;
    void updateFromElement() override;
    short baselinePosition(bool) const override;

    void updateSelection();

    DOM::HTMLSelectElementImpl *element() const
    {
        return static_cast<DOM::HTMLSelectElementImpl *>(RenderObject::element());
    }

protected:
    void setPadding() override;
    ListBoxWidget *createListBox();
    ComboBoxWidget *createComboBox();

    unsigned  m_size;
    bool m_multiple;
    bool m_useListBox;
    bool m_selectionChanged;
    bool m_ignoreSelectEvents;
    bool m_optionsChanged;

    void clearItemFlags(int index, Qt::ItemFlags flags);
    bool canHaveBorder() const override
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
    bool event(QEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void scrollContentsBy(int dx, int dy) override;

};

// -------------------------------------------------------------------------

class RenderTextArea : public RenderFormElement
{
    Q_OBJECT
public:
    RenderTextArea(DOM::HTMLTextAreaElementImpl *element);
    ~RenderTextArea();

    const char *renderName() const override
    {
        return "RenderTextArea";
    }
    void calcMinMaxWidth() override;
    void setStyle(RenderStyle *style) override;

    short scrollWidth() const override;
    int scrollHeight() const override;

    void updateFromElement() override;

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
    void handleFocusOut() override;

    bool isEditable() const override
    {
        return true;
    }
    bool canHaveBorder() const override
    {
        return true;
    }

    Qt::Alignment m_textAlignment;
};

// -------------------------------------------------------------------------

class ScrollBarWidget: public QScrollBar, public KHTMLWidget
{
public:
    ScrollBarWidget(QWidget *parent = nullptr): QScrollBar(parent)
    {
        m_kwp->setIsRedirected(true);
    }
    ScrollBarWidget(Qt::Orientation orientation, QWidget *parent = nullptr): QScrollBar(orientation, parent)
    {
        m_kwp->setIsRedirected(true);
    }
};

} //namespace

#endif
