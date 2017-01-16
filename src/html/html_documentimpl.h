/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
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

#ifndef HTML_DOCUMENTIMPL_H
#define HTML_DOCUMENTIMPL_H

#include "xml/dom_docimpl.h"
#include "html/html_miscimpl.h"

#include <QtCore/QMap>

class KHTMLView;
class QString;

namespace DOM
{

class Element;
class HTMLElement;
class HTMLElementImpl;
class DOMString;
class HTMLMapElementImpl;

class HTMLDocumentImpl : public DOM::DocumentImpl
{
    Q_OBJECT
public:
    HTMLDocumentImpl(KHTMLView *v = nullptr);
    ~HTMLDocumentImpl();

    bool isHTMLDocument() const Q_DECL_OVERRIDE
    {
        return true;
    }

    DOMString referrer() const;
    DOMString lastModified() const;
    DOMString cookie() const;
    void setCookie(const DOMString &);
    NodeListImpl *getElementsByName(const DOMString &elementName);

    HTMLCollectionImpl *images();
    HTMLCollectionImpl *applets();
    HTMLCollectionImpl *links();
    HTMLCollectionImpl *forms();
    HTMLCollectionImpl *layers();
    HTMLCollectionImpl *anchors();
    HTMLCollectionImpl *all();
    HTMLCollectionImpl *scripts();

    void setBody(HTMLElementImpl *_body, int &exceptioncode);

    khtml::Tokenizer *createTokenizer() Q_DECL_OVERRIDE;

    bool childAllowed(NodeImpl *newChild) Q_DECL_OVERRIDE;

    ElementImpl *createElement(const DOMString &tagName, int *pExceptioncode) Q_DECL_OVERRIDE;

    // HTML5
    ElementImpl *activeElement() const;

    HTMLMapElementImpl *getMap(const DOMString &url_);

    void determineParseMode() Q_DECL_OVERRIDE;
    void close() Q_DECL_OVERRIDE;
    void contentLoaded() Q_DECL_OVERRIDE;

    void setAutoFill()
    {
        m_doAutoFill = true;
    }

    // If true, HTML was requested by mimetype (e.g. HTTP Content-Type). Otherwise XHTML was requested.
    // This is independent of the actual doctype, of course. (#86446)
    void setHTMLRequested(bool html)
    {
        m_htmlRequested = html;
    }

    // Change parse and html modes
    void changeModes(ParseMode newPMode, HTMLMode newHMode);

protected:
    HTMLElementImpl *htmlElement;
    friend class HTMLMapElementImpl;
    friend class HTMLImageElementImpl;
    QMap<QString, HTMLMapElementImpl *> mapMap;
    bool m_doAutoFill;
    bool m_htmlRequested;
    bool m_determineParseMode;

protected Q_SLOTS:
    /**
     * Repaints, so that all links get the proper color
     */
    void slotHistoryChanged();
};

// Used to display text in iframes
class HTMLTextDocumentImpl: public HTMLDocumentImpl
{
public:
    HTMLTextDocumentImpl(KHTMLView *v = nullptr);
    khtml::Tokenizer *createTokenizer() Q_DECL_OVERRIDE;
};

} //namespace

#endif
