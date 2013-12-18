/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2007, 2008 Maks Orlovich (maksim@kde.org)
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
#ifndef HTML_OBJECTIMPL_H
#define HTML_OBJECTIMPL_H

#include "html_elementimpl.h"
#include "xml/dom_stringimpl.h"
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QStringList>
#include <QWidget>


// -------------------------------------------------------------------------
class KHTMLPart;

namespace DOM {

class HTMLFormElementImpl;
class HTMLEmbedElementImpl;

// Base class of all objects that are displayed as KParts:
// frames, objects, applets, etc.
class HTMLPartContainerElementImpl : public QObject, public HTMLElementImpl
{
    Q_OBJECT
public:

    enum DOMChildFrameEvents { DOMCFResizeEvent = 0x3030 };

    HTMLPartContainerElementImpl(DocumentImpl *doc);
    ~HTMLPartContainerElementImpl();

    void computeContentIfNeeded();
    void setNeedComputeContent();

    virtual void recalcStyle(StyleChange ch);
    virtual void close();

    // These methods will be called to notify the element of
    // any progress in loading of the document: setWidgetNotify if the
    // KPart was created, and partLoadingErrorNotify when
    // there was a problem with creating the part or loading the data
    // (hence setWidgetNotify may be followed by partLoadingErrorNotify).
    // This class take care of all the memory management, and during
    // the setWidgetNotify call, both old (if any) and new widget are alive
    // Note: setWidgetNotify may be called with 0...
    virtual void setWidgetNotify(QWidget *widget) = 0;
    virtual void partLoadingErrorNotify();

    // This is called when a mimetype is discovered, and should return true
    // if KHTMLPart should not make a kpart for it, but rather let it be handled directly.
    virtual bool mimetypeHandledInternally(const QString& mime);

    virtual bool event(QEvent *e);

    // IMPORTANT: you should call this when requesting a URL, to make sure
    // that we don't get stale references to iframes or such.
    void clearChildWidget();
    QWidget* childWidget() const { return m_childWidget; }

    void postResizeEvent();
    static void sendPostedResizeEvents();
public Q_SLOTS:
    void slotEmitLoadEvent();
private:
    friend class ::KHTMLPart;
    // This is called by KHTMLPart to notify us of the new widget.
    void setWidget(QWidget* widget);
private:
    virtual void computeContent() = 0;
    bool m_needToComputeContent; // This flag is set to true when
                                 // we may have to load a new KPart, due to
                                 // source changing, etc.
    QPointer<QWidget> m_childWidget; // may be deleted by global child widget cleanup on us..
};

class HTMLObjectBaseElementImpl : public HTMLPartContainerElementImpl
{
    Q_OBJECT
public:
    HTMLObjectBaseElementImpl(DocumentImpl *doc);

    virtual void parseAttribute(AttributeImpl *attr);
    virtual void attach();
    virtual void defaultEventHandler(EventImpl *e);

    void setServiceType(const QString &);

    QString url;
    QString classId;
    QString serviceType;

    bool m_rerender; // This is set to true if a reattach is pending,
                     // due to a change in how we need to display this...

    bool m_renderAlternative;
    bool m_imageLike;

    virtual void insertedIntoDocument();
    virtual void removedFromDocument();
    virtual void addId(const DOMString& id);
    virtual void removeId(const DOMString& id);

    HTMLEmbedElementImpl* relevantEmbed();

    virtual void setWidgetNotify( QWidget *widget );
    virtual void partLoadingErrorNotify();
    virtual bool mimetypeHandledInternally(const QString& mime);

    // This method figures out what to render -- perhaps KPart, perhaps an image, perhaps
    // alternative content, and forces a reattach if need be.
    virtual void computeContent();

    // Ask for a reattach, since we may need a different renderer..
    void requestRerender();

    void renderAlternative();
protected Q_SLOTS:
    void slotRerender();
    void slotPartLoadingErrorNotify();
protected:
    DOMString     m_name;
};

// -------------------------------------------------------------------------

class HTMLAppletElementImpl : public HTMLObjectBaseElementImpl
{
public:
    HTMLAppletElementImpl(DocumentImpl *doc);

    ~HTMLAppletElementImpl();

    virtual Id id() const;

    virtual void parseAttribute(AttributeImpl *token);
    virtual void computeContent();
protected:
    khtml::VAlign valign;
};

// -------------------------------------------------------------------------

class HTMLEmbedElementImpl : public HTMLObjectBaseElementImpl
{
public:
    HTMLEmbedElementImpl(DocumentImpl *doc);
    ~HTMLEmbedElementImpl();

    virtual Id id() const;

    virtual void parseAttribute(AttributeImpl *attr);
    virtual void attach();
    virtual void computeContent();

    virtual HTMLEmbedElementImpl* relevantEmbed();

    QString pluginPage;
    bool hidden;
};

// -------------------------------------------------------------------------

class HTMLObjectElementImpl : public HTMLObjectBaseElementImpl
{
public:
    HTMLObjectElementImpl(DocumentImpl *doc);

    ~HTMLObjectElementImpl();

    virtual Id id() const;

    HTMLFormElementImpl *form() const;

    virtual void parseAttribute(AttributeImpl *token);

    virtual void attach();

    DocumentImpl* contentDocument() const;
};

// -------------------------------------------------------------------------

class HTMLParamElementImpl : public HTMLElementImpl
{
    friend class HTMLAppletElementImpl;
public:
    HTMLParamElementImpl(DocumentImpl* _doc) : HTMLElementImpl(_doc) {}

    virtual Id id() const;

    virtual void parseAttribute(AttributeImpl *token);

    QString name() const { return m_name; }
    QString value() const { return m_value; }

 protected:
    QString m_name;
    QString m_value;
};

} // namespace
#endif
