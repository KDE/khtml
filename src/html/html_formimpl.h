/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 *           (C) 2004, 2005, 2006 Apple Computer, Inc.
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
#ifndef HTML_FORMIMPL_H
#define HTML_FORMIMPL_H

#include "html/html_elementimpl.h"
#include "html/html_imageimpl.h"
#include "dom/html_element.h"
#include <wtf/HashSet.h>
#include <wtf/Vector.h>

class QTextCodec;

namespace khtml
{
class RenderFormElement;
class RenderTextArea;
class RenderSelect;
class RenderLineEdit;
class RenderRadioButton;
class RenderFileButton;

typedef QList<QByteArray> encodingList;
}

namespace KWallet
{
class Wallet;
}

namespace DOM
{

class HTMLFormElement;
class DOMString;
class HTMLGenericFormElementImpl;
class HTMLOptionElementImpl;
class HTMLCollectionImpl;

// -------------------------------------------------------------------------

class HTMLFormElementImpl : public HTMLElementImpl
{
public:
    HTMLFormElementImpl(DocumentImpl *doc, bool implicit);
    virtual ~HTMLFormElementImpl();

    Id id() const override;

    void insertedIntoDocument() override;
    void removedFromDocument() override;
    void addId(const DOMString &id) override;
    void removeId(const DOMString &id) override;

    // See "past names map" in HTML5, 4.10.3, "The form element"
    HTMLGenericFormElementImpl *lookupByPastName(const DOMString &id);
    void bindPastName(HTMLGenericFormElementImpl *element);

    long length() const;

    QByteArray formData(bool &ok);

    DOMString enctype() const
    {
        return m_enctype;
    }
    void setEnctype(const DOMString &);

    DOMString target() const;
    DOMString action() const;
    HTMLCollectionImpl *elements();

    bool autoComplete() const
    {
        return m_autocomplete;
    }
    void doAutoFill();
    void walletOpened(KWallet::Wallet *w);

    void parseAttribute(AttributeImpl *attr) override;

    void uncheckOtherRadioButtonsInGroup(HTMLGenericFormElementImpl *caller, bool setDefaultChecked = false);

    void registerFormElement(HTMLGenericFormElementImpl *);
    void removeFormElement(HTMLGenericFormElementImpl *);
    void registerImgElement(HTMLImageElementImpl *);
    void removeImgElement(HTMLImageElementImpl *);

    void submitFromKeyboard();
    bool prepareSubmit();
    void submit();
    void reset();

    void setMalformed(bool malformed)
    {
        m_malformed = malformed;
    }
    bool isMalformed() const
    {
        return m_malformed;
    }

    friend class HTMLFormElement;
    friend class HTMLFormCollectionImpl;

private:
    // Collects nodes that are inside the toGather set in tree order
    WTF::Vector<HTMLGenericFormElementImpl *> gatherInTreeOrder(NodeImpl *root,
            const WTF::HashSet<NodeImpl *> &toGather);

    void gatherWalletData();
    QList<HTMLGenericFormElementImpl *> formElements;
    QList<HTMLImageElementImpl *> imgElements;
    DOMString m_target;
    DOMString m_enctype;
    QString m_boundary;
    DOMString m_acceptcharset;
    bool m_post : 1;
    bool m_multipart : 1;
    bool m_autocomplete : 1;
    bool m_insubmit : 1;
    bool m_doingsubmit : 1;
    bool m_inreset : 1;
    bool m_malformed : 1;
    bool m_haveTextarea : 1; // for wallet storage
    bool m_havePassword : 1; // for wallet storage
    DOMString m_name;        // our name
    QMap<QString, QString> m_walletMap; // for wallet storage

    QHash<DOMString, HTMLGenericFormElementImpl *> m_pastNamesMap;
};

// -------------------------------------------------------------------------

class HTMLGenericFormElementImpl : public HTMLElementImpl
{
    friend class HTMLFormElementImpl;
    friend class khtml::RenderFormElement;

public:
    HTMLGenericFormElementImpl(DocumentImpl *doc, HTMLFormElementImpl *f = nullptr);
    virtual ~HTMLGenericFormElementImpl();

    HTMLFormElementImpl *form() const
    {
        return m_form;
    }

    void parseAttribute(AttributeImpl *attr) override;
    void attach() override;
    virtual void reset() {}

    void insertedIntoDocument() override;
    void removedFromDocument() override;

    void onSelect();
    void onChange();

    bool disabled() const
    {
        return m_disabled;
    }
    void setDisabled(bool _disabled);

    bool isFocusableImpl(FocusType ft) const override;
    virtual bool isEnumerable() const
    {
        return false;
    }
    virtual bool isDefault() const
    {
        return false;
    }

    bool readOnly() const
    {
        return m_readOnly;
    }
    void setReadOnly(bool _readOnly)
    {
        m_readOnly = _readOnly;
    }

    DOMString name() const;
    void setName(const DOMString &name);

    bool isGenericFormElement() const override
    {
        return true;
    }
    virtual bool isHiddenInput() const
    {
        return false;
    }

    bool hasPastNames() const
    {
        return m_hasPastNames;
    }
    void setHasPastNames()
    {
        m_hasPastNames = true;
    }

    /*
     * override in derived classes to get the encoded name=value pair
     * for submitting
     * return true for a successful control (see HTML4-17.13.2)
     */
    virtual bool encoding(const QTextCodec *, khtml::encodingList &, bool)
    {
        return false;
    }

    void defaultEventHandler(EventImpl *evt) override;
    virtual bool isEditable();

    virtual bool unsubmittedFormChanges() const
    {
        return false;
    }

protected:
    HTMLFormElementImpl *getForm() const;

    DOMStringImpl *m_name;
    HTMLFormElementImpl *m_form;
    bool m_disabled, m_readOnly, m_hasPastNames;
};

// -------------------------------------------------------------------------

class HTMLButtonElementImpl : public HTMLGenericFormElementImpl
{
public:
    HTMLButtonElementImpl(DocumentImpl *doc, HTMLFormElementImpl *f = nullptr);

    virtual ~HTMLButtonElementImpl();

    enum typeEnum {
        SUBMIT,
        RESET,
        BUTTON
    };

    Id id() const override;
    bool isEnumerable() const override
    {
        return true;
    }

    DOMString type() const;
    typeEnum buttonType() const
    {
        return KDE_CAST_BF_ENUM(typeEnum, m_type);
    }
    void parseAttribute(AttributeImpl *attr) override;
    void defaultEventHandler(EventImpl *evt) override;
    bool encoding(const QTextCodec *, khtml::encodingList &, bool) override;
    void activate();
    void attach() override;
    void click();

protected:
    DOMString             m_value;
    QString               m_currValue;
    KDE_BF_ENUM(typeEnum) m_type : 2;
    bool                  m_dirty : 1;
    bool                  m_clicked : 1;
    bool                  m_activeSubmit : 1;
};

// -------------------------------------------------------------------------

class HTMLFieldSetElementImpl : public HTMLGenericFormElementImpl
{
public:
    HTMLFieldSetElementImpl(DocumentImpl *doc, HTMLFormElementImpl *f = nullptr);

    virtual ~HTMLFieldSetElementImpl();

    Id id() const override;
    void attach() override;
    void parseAttribute(AttributeImpl *attr) override;

};

// -------------------------------------------------------------------------

class HTMLInputElementImpl : public HTMLGenericFormElementImpl
{
    friend class khtml::RenderLineEdit;
    friend class khtml::RenderRadioButton;
    friend class khtml::RenderFileButton;

public:
    // do not change the order!
    enum typeEnum {
        TEXT,
        PASSWORD,
        ISINDEX,
        CHECKBOX,
        RADIO,
        SUBMIT,
        RESET,
        FILE,
        HIDDEN,
        IMAGE,
        BUTTON
    };

    HTMLInputElementImpl(DocumentImpl *doc, HTMLFormElementImpl *f = nullptr);
    virtual ~HTMLInputElementImpl();

    Id id() const override;

    bool isEnumerable() const override
    {
        return inputType() != IMAGE;
    }
    bool isDefault() const override
    {
        return m_defaultChecked;
    }
    bool isHiddenInput() const override
    {
        return inputType() == HIDDEN;
    }

    bool autoComplete() const
    {
        return m_autocomplete;
    }

    bool defaultChecked() const
    {
        return m_defaultChecked;
    }
    bool checked() const
    {
        return m_useDefaultChecked ? m_defaultChecked : m_checked;
    }
    void setChecked(bool, bool setDefaultChecked = false);
    bool indeterminate() const
    {
        return m_indeterminate;
    }
    void setIndeterminate(bool);
    long maxLength() const
    {
        return m_maxLen;
    }
    int size() const
    {
        return m_size;
    }
    DOMString type() const;
    void setType(const DOMString &t);

    DOMString value() const;
    void setValue(DOMString val);

    DOMString valueWithDefault() const;

    bool maintainsState() override
    {
        return true;
    }
    QString state() override;
    void restoreState(const QString &state) override;

    void select();
    void click();

    void parseAttribute(AttributeImpl *attr) override;

    void copyNonAttributeProperties(const ElementImpl *source) override;

    void attach() override;
    bool encoding(const QTextCodec *, khtml::encodingList &, bool) override;

    typeEnum inputType() const
    {
        return KDE_CAST_BF_ENUM(typeEnum, m_type);
    }
    void reset() override;

    // used in case input type=image was clicked.
    int clickX() const
    {
        return xPos;
    }
    int clickY() const
    {
        return yPos;
    }

    void defaultEventHandler(EventImpl *evt) override;
    bool isEditable() override;

    DOMString altText() const;
    void activate();

    void setUnsubmittedFormChange(bool unsubmitted)
    {
        m_unsubmittedFormChange = unsubmitted;
    }
    bool unsubmittedFormChanges() const override
    {
        return m_unsubmittedFormChange;
    }

    //Mozilla extensions.
    long selectionStart();
    long selectionEnd();
    void setSelectionStart(long pos);
    void setSelectionEnd(long pos);
    void setSelectionRange(long start, long end);

    // HTML5
    void setPlaceholder(const DOMString &p);
    DOMString placeholder() const;

protected:
    void parseType(const DOMString &t);

    DOMString m_value;
    int       xPos;
    short     m_maxLen;
    short     m_size;
    short     yPos;

    KDE_BF_ENUM(typeEnum) m_type : 4;
    bool m_clicked : 1;
    bool m_checked : 1;
    bool m_defaultChecked : 1; // could do without by checking ATTR_CHECKED
    bool m_useDefaultChecked : 1;
    bool m_indeterminate : 1;
    bool m_haveType : 1;
    bool m_activeSubmit : 1;
    bool m_autocomplete : 1;
    bool m_inited : 1;
    bool m_unsubmittedFormChange : 1;
};

// -------------------------------------------------------------------------

class HTMLLabelElementImpl : public HTMLGenericFormElementImpl
{
public:
    HTMLLabelElementImpl(DocumentImpl *doc);
    virtual ~HTMLLabelElementImpl();

    Id id() const override;
    void attach() override;
    void defaultEventHandler(EventImpl *evt) override;
    bool isFocusableImpl(FocusType ft) const override;
    NodeImpl *getFormElement();

private:
    DOMString m_formElementID;
};

// -------------------------------------------------------------------------

class HTMLLegendElementImpl : public HTMLGenericFormElementImpl
{
public:
    HTMLLegendElementImpl(DocumentImpl *doc, HTMLFormElementImpl *f = nullptr);
    virtual ~HTMLLegendElementImpl();

    Id id() const override;
    void attach() override;
    void parseAttribute(AttributeImpl *attr) override;
};

// -------------------------------------------------------------------------

class HTMLSelectElementImpl : public HTMLGenericFormElementImpl
{
    friend class khtml::RenderSelect;

public:
    HTMLSelectElementImpl(DocumentImpl *doc, HTMLFormElementImpl *f = nullptr);
    ~HTMLSelectElementImpl();

    Id id() const override;

    DOMString type() const;

    HTMLCollectionImpl *options();

    long selectedIndex() const;
    void setSelectedIndex(long index);

    bool isEnumerable() const override
    {
        return true;
    }

    long length() const;

    long minWidth() const
    {
        return m_minwidth;
    }

    long size() const
    {
        return m_size;
    }

    bool multiple() const
    {
        return m_multiple;
    }

    void add(HTMLElementImpl *element, HTMLElementImpl *before, int &exceptioncode);
    using DOM::NodeImpl::remove;
    void remove(long index);

    DOMString value() const;
    void setValue(DOMStringImpl *value);

    bool maintainsState() override
    {
        return true;
    }
    QString state() override;
    void restoreState(const QString &state) override;

    NodeImpl *insertBefore(NodeImpl *newChild, NodeImpl *refChild, int &exceptioncode) override;
    void      replaceChild(NodeImpl *newChild, NodeImpl *oldChild, int &exceptioncode) override;
    void      removeChild(NodeImpl *oldChild, int &exceptioncode) override;
    void      removeChildren() override;
    NodeImpl *appendChild(NodeImpl *newChild, int &exceptioncode) override;
    NodeImpl *addChild(NodeImpl *newChild) override;

    void childrenChanged() override;

    void parseAttribute(AttributeImpl *attr) override;

    void attach() override;
    bool encoding(const QTextCodec *, khtml::encodingList &, bool) override;

    // get the actual listbox index of the optionIndexth option
    int optionToListIndex(int optionIndex) const;
    // reverse of optionToListIndex - get optionIndex from listboxIndex
    int listToOptionIndex(int listIndex) const;

    void setRecalcListItems();

    QVector<HTMLGenericFormElementImpl *> listItems() const
    {
        if (m_recalcListItems) {
            const_cast<HTMLSelectElementImpl *>(this)->recalcListItems();
        }
        return m_listItems;
    }
    void reset() override;
    void notifyOptionSelected(HTMLOptionElementImpl *selectedOption, bool selected);

private:
    void recalcListItems() const;
    HTMLOptionElementImpl *firstSelectedItem() const;

protected:
    mutable QVector<HTMLGenericFormElementImpl *> m_listItems;
    short m_minwidth;
    signed short m_size : 15;
    bool m_multiple : 1;
    mutable bool m_recalcListItems : 1;
    mutable unsigned int   m_length: 31;
};

// -------------------------------------------------------------------------

class HTMLKeygenElementImpl : public HTMLSelectElementImpl
{
public:
    HTMLKeygenElementImpl(DocumentImpl *doc, HTMLFormElementImpl *f = nullptr);

    Id id() const override;

    DOMString type() const;

    long selectedIndex() const;
    void setSelectedIndex(long index);

    // ### this is just a rough guess
    bool isEnumerable() const override
    {
        return false;
    }

    void parseAttribute(AttributeImpl *attr) override;
    bool encoding(const QTextCodec *, khtml::encodingList &, bool) override;

};

// -------------------------------------------------------------------------

class HTMLOptGroupElementImpl : public HTMLGenericFormElementImpl
{
public:
    HTMLOptGroupElementImpl(DocumentImpl *doc, HTMLFormElementImpl *f = nullptr)
        : HTMLGenericFormElementImpl(doc, f) {}

    Id id() const override;
};

// ---------------------------------------------------------------------------

class HTMLOptionElementImpl : public HTMLGenericFormElementImpl
{
    friend class khtml::RenderSelect;
    friend class DOM::HTMLSelectElementImpl;

public:
    HTMLOptionElementImpl(DocumentImpl *doc, HTMLFormElementImpl *f = nullptr);

    Id id() const override;

    DOMString text() const;

    long index() const;
    void setIndex(long);
    void parseAttribute(AttributeImpl *attr) override;
    DOMString value() const;
    void setValue(DOMStringImpl *value);

    bool isDefault() const override
    {
        return m_defaultSelected;
    }

    // For internal use --- just returns the bit
    bool selectedBit() const
    {
        return m_selected;
    }

    // DOM use --- may recompute information of a parent <select>
    bool selected() const;
    void setSelected(bool _selected);
    void setDefaultSelected(bool _defaultSelected);

    HTMLSelectElementImpl *getSelect() const;

protected:
    DOMString m_value;
    bool m_selected;
    bool m_defaultSelected;
};

// -------------------------------------------------------------------------

class HTMLTextAreaElementImpl : public HTMLGenericFormElementImpl
{
    friend class khtml::RenderTextArea;

public:
    enum WrapMethod {
        ta_NoWrap,
        ta_Virtual,
        ta_Physical
    };

    HTMLTextAreaElementImpl(DocumentImpl *doc, HTMLFormElementImpl *f = nullptr);
    ~HTMLTextAreaElementImpl();

    Id id() const override;
    void childrenChanged() override;

    long cols() const
    {
        return m_cols;
    }

    long rows() const
    {
        return m_rows;
    }

    WrapMethod wrap() const
    {
        return m_wrap;
    }

    bool isEnumerable() const override
    {
        return true;
    }

    DOMString type() const;

    bool maintainsState() override
    {
        return true;
    }
    QString state() override;
    void restoreState(const QString &state) override;

    void select();

    void parseAttribute(AttributeImpl *attr) override;
    void attach() override;
    bool encoding(const QTextCodec *, khtml::encodingList &, bool) override;
    void reset() override;
    DOMString value();
    void setValue(DOMString _value);
    DOMString defaultValue();
    void setDefaultValue(DOMString _defaultValue);

    bool isEditable() override;
    void setUnsubmittedFormChange(bool unsubmitted)
    {
        m_unsubmittedFormChange = unsubmitted;
    }
    bool unsubmittedFormChanges() const override
    {
        return m_unsubmittedFormChange;
    }

    //Mozilla extensions.
    long selectionStart();
    long selectionEnd();
    void setSelectionStart(long pos);
    void setSelectionEnd(long pos);
    void setSelectionRange(long start, long end);
    long textLength();

    // HTML5
    void setPlaceholder(const DOMString &p);
    DOMString placeholder() const;

protected:
    int m_rows;
    int m_cols;
    WrapMethod m_wrap;
    QString m_value;
    bool m_changed: 1;    //States whether the contents has been editted
    bool m_unsubmittedFormChange: 1;
    bool m_initialized: 1;
};

// -------------------------------------------------------------------------

class HTMLIsIndexElementImpl : public HTMLInputElementImpl
{
public:
    HTMLIsIndexElementImpl(DocumentImpl *doc, HTMLFormElementImpl *f = nullptr);
    ~HTMLIsIndexElementImpl();

    Id id() const override;
    void parseAttribute(AttributeImpl *attr) override;

    DOMString prompt() const;
    void setPrompt(const DOMString &_value);
};

} //namespace

#endif
