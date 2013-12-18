/* This file is part of the KDE project
 *
 * Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>
 * Copyright (C) 2007 David Faure <faure@kde.org>
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
 */

#include "khtml_global.h"
#include "khtml_part.h"
#include "khtml_settings.h"
#include "../khtml_version.h"

#include "css/cssstyleselector.h"
#include "css/css_mediaquery.h"
#include "html/html_imageimpl.h"
#include "rendering/render_style.h"
#include "rendering/break_lines.h"
#include "misc/htmlnames.h"
#include "misc/loader.h"
#include "misc/arena.h"
#include "misc/paintbuffer.h"

#include <QtCore/QLinkedList>

#include <kiconloader.h>
#include <kaboutdata.h>
#include <klocalizedstring.h>

#include <assert.h>

#include <QDebug>

// SVG
#include "svg/SVGNames.h"

KHTMLGlobal *KHTMLGlobal::s_self = 0;
unsigned long int KHTMLGlobal::s_refcnt = 0;
KIconLoader *KHTMLGlobal::s_iconLoader = 0;
KAboutData *KHTMLGlobal::s_about = 0;
KHTMLSettings *KHTMLGlobal::s_settings = 0;

static QLinkedList<KHTMLPart*> *s_parts = 0;
static QLinkedList<DOM::DocumentImpl*> *s_docs = 0;

KHTMLGlobal::KHTMLGlobal()
{
    assert(!s_self);
    s_self = this;
    ref();

    khtml::Cache::init();

    khtml::NamespaceFactory::initIdTable();
    khtml::PrefixFactory::initIdTable();
    khtml::LocalNameFactory::initIdTable();
    DOM::emptyLocalName = DOM::LocalName::fromId(0);
    DOM::emptyPrefixName = DOM::PrefixName::fromId(0);
    DOM::emptyNamespaceName = DOM::NamespaceName::fromId(DOM::emptyNamespace);
    WebCore::SVGNames::init();
}

KHTMLGlobal::~KHTMLGlobal()
{
    //qDebug() << this;
    if ( s_self == this )
    {
        finalCheck();
        delete s_iconLoader;
        delete s_about;
        delete s_settings;
        delete KHTMLSettings::avFamilies;
        if (s_parts) {
            assert(s_parts->isEmpty());
            delete s_parts;
        }
        if (s_docs) {
            assert(s_docs->isEmpty());
            delete s_docs;
        }

        s_iconLoader = 0;
        s_about = 0;
        s_settings = 0;
        s_parts = 0;
        s_docs = 0;
        KHTMLSettings::avFamilies = 0;

        // clean up static data
        khtml::CSSStyleSelector::clear();
        khtml::RenderStyle::cleanup();
        khtml::RenderObject::cleanup();
        khtml::PaintBuffer::cleanup();
        khtml::MediaQueryEvaluator::cleanup();
        khtml::Cache::clear();
        khtml::cleanup_thaibreaks();
        khtml::ArenaFinish();
    }
    else
        deref();
}

void KHTMLGlobal::ref()
{
    if ( !s_refcnt && !s_self )
    {
        //qDebug() << "Creating KHTMLGlobal instance";
        // we can't use a staticdeleter here, because that would mean
        // that the KHTMLGlobal instance gets deleted from within a qPostRoutine, called
        // from the QApplication destructor. That however is too late, because
        // we want to destruct a KComponentData object, which involves destructing
        // a KConfig object, which might call KGlobal::dirs() (in sync()) which
        // probably is not going to work ;-)
        // well, perhaps I'm wrong here, but as I'm unsure I try to stay on the
        // safe side ;-) -> let's use a simple reference counting scheme
        // (Simon)
        new KHTMLGlobal; // does initial ref()
    } else {
        ++s_refcnt;
    }
    //qDebug() << "s_refcnt=" << s_refcnt;
}

void KHTMLGlobal::deref()
{
    //qDebug() << "s_refcnt=" << s_refcnt - 1;
    if ( !--s_refcnt && s_self )
    {
        delete s_self;
        s_self = 0;
    }
}

void KHTMLGlobal::registerPart( KHTMLPart *part )
{
    //qDebug() << part;
    if ( !s_parts )
        s_parts = new QLinkedList<KHTMLPart*>;

    if ( !s_parts->contains( part ) ) {
        s_parts->append( part );
        ref();
    }
}

void KHTMLGlobal::deregisterPart( KHTMLPart *part )
{
    //qDebug() << part;
    assert( s_parts );

    if ( s_parts->removeAll( part ) ) {
        if ( s_parts->isEmpty() ) {
            delete s_parts;
            s_parts = 0;
        }
        deref();
    }
}

void KHTMLGlobal::registerDocumentImpl( DOM::DocumentImpl *doc )
{
    //qDebug() << doc;
    if ( !s_docs )
        s_docs = new QLinkedList<DOM::DocumentImpl*>;

    if ( !s_docs->contains( doc ) ) {
        s_docs->append( doc );
        ref();
    }
}

void KHTMLGlobal::deregisterDocumentImpl( DOM::DocumentImpl *doc )
{
    //qDebug() << doc;
    assert( s_docs );

    if ( s_docs->removeAll( doc ) ) {
        if ( s_docs->isEmpty() ) {
            delete s_docs;
            s_docs = 0;
        }
        deref();
    }
}

const KAboutData &KHTMLGlobal::aboutData()
{
  assert( s_self );

  if ( !s_about )
  {
    s_about = new KAboutData( "khtml", 0, i18n("KHTML"), QStringLiteral(KHTML_VERSION_STRING),
                              i18n( "Embeddable HTML component"),
                              KAboutData::License_LGPL );
    s_about->addAuthor(QStringLiteral("Lars Knoll"), QString(), "knoll@kde.org");
    s_about->addAuthor(QStringLiteral("Antti Koivisto"), QString(), "koivisto@kde.org");
    s_about->addAuthor(QStringLiteral("Waldo Bastian"), QString(), "bastian@kde.org");
    s_about->addAuthor(QStringLiteral("Dirk Mueller"), QString(), "mueller@kde.org");
    s_about->addAuthor(QStringLiteral("Peter Kelly"), QString(), "pmk@kde.org");
    s_about->addAuthor(QStringLiteral("Torben Weis"), QString(), "weis@kde.org");
    s_about->addAuthor(QStringLiteral("Martin Jones"), QString(), "mjones@kde.org");
    s_about->addAuthor(QStringLiteral("Simon Hausmann"), QString(), "hausmann@kde.org");
    s_about->addAuthor(QStringLiteral("Tobias Anton"), QString(), "anton@stud.fbi.fh-darmstadt.de");

  }

  return *s_about;
}

KIconLoader *KHTMLGlobal::iconLoader()
{
  if ( !s_iconLoader )
  {
    s_iconLoader = new KIconLoader(aboutData().componentName());
  }

  return s_iconLoader;
}

KHTMLSettings *KHTMLGlobal::defaultHTMLSettings()
{
  assert( s_self );
  if ( !s_settings )
    s_settings = new KHTMLSettings();

  return s_settings;
}

void KHTMLGlobal::finalCheck()
{
#ifndef NDEBUG
    if (s_refcnt) {
        if (s_parts && !s_parts->isEmpty()) {
            Q_FOREACH(KHTMLPart *part, *s_parts) {
                qWarning() << "Part" << part->url() << "was not deleted";
            }
        }
        if (s_docs && !s_docs->isEmpty()) {
            Q_FOREACH(DOM::DocumentImpl *doc, *s_docs) {
                qWarning() << "Document" << doc->URL() << "was not deleted";
            }
        }
    }
#endif
}
