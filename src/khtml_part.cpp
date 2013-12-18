/* This file is part of the KDE project
 *
 * Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
 *                     1999 Lars Knoll <knoll@kde.org>
 *                     1999 Antti Koivisto <koivisto@kde.org>
 *                     2000 Simon Hausmann <hausmann@kde.org>
 *                     2000 Stefan Schimanski <1Stein@gmx.de>
 *                     2001-2005 George Staikos <staikos@kde.org>
 *                     2001-2003 Dirk Mueller <mueller@kde.org>
 *                     2000-2005 David Faure <faure@kde.org>
 *                     2002 Apple Computer, Inc.
 *                     2010 Maksim Orlovich (maksim@kde.org)
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

//#define SPEED_DEBUG
#include "khtml_part.h"

#include "ui_htmlpageinfo.h"

#include "khtmlviewbar.h"
#include "khtml_pagecache.h"

#include "dom/dom_string.h"
#include "dom/dom_element.h"
#include "dom/dom_exception.h"
#include "dom/html_document.h"
#include "dom/dom2_range.h"
#include "editing/editor.h"
#include "html/html_documentimpl.h"
#include "html/html_baseimpl.h"
#include "html/html_objectimpl.h"
#include "html/html_miscimpl.h"
#include "html/html_imageimpl.h"
#include "imload/imagemanager.h"
#include "rendering/render_text.h"
#include "rendering/render_frames.h"
#include "rendering/render_layer.h"
#include "rendering/render_position.h"
#include "misc/loader.h"
#include "misc/khtml_partaccessor.h"
#include "xml/dom2_eventsimpl.h"
#include "xml/dom2_rangeimpl.h"
#include "xml/xml_tokenizer.h"
#include "css/cssstyleselector.h"
#include "css/csshelper.h"
using namespace DOM;

#include "khtmlview.h"
#include <kparts/partmanager.h>
#include <kparts/browseropenorsavequestion.h>
#include <kacceleratormanager.h>
#include "ecma/kjs_proxy.h"
#include "ecma/kjs_window.h"
#include "ecma/kjs_events.h"
#include "khtml_settings.h"
#include "kjserrordlg.h"

#include <kjs/function.h>
#include <kjs/interpreter.h>

#include <sys/types.h>
#include <assert.h>
#include <unistd.h>

#include <kstringhandler.h>
#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <kio/global.h>
#include <kio/pixmaploader.h>
#include <kio/hostinfo.h>
#include <kprotocolmanager.h>
#include <QDebug>
#include <kjobwidgets.h>
#include <kmessagebox.h>
#include <kstandardaction.h>
#include <kstandardguiitem.h>
#include <kactioncollection.h>
#include <kmimetypetrader.h>
#include <qtemporaryfile.h>
#include <ktoolinvocation.h>
#include <kurlauthorized.h>
#include <kparts/browserinterface.h>
#include <kparts/scriptableextension.h>
#include <kactionmenu.h>
#include <ktoggleaction.h>
#include <kcodecaction.h>
#include <kselectaction.h>

#include <QDBusConnection>
#include <ksslinfodialog.h>
#include <ksslsettings.h>

#include <QDBusInterface>
#include <QMimeData>
#include <kfileitem.h>
#include <kurifilter.h>
#include <kurllabel.h>
#include <kurlmimedata.h>

#include <QClipboard>
#include <QLocale>
#include <QMenu>
#include <QToolTip>
#include <QDrag>
#include <QtCore/QFile>
#include <QtCore/QMetaEnum>
#include <QTextDocument>
#include <QtCore/QDate>
#include <QtNetwork/QSslCertificate>
#include <QStatusBar>
#include <QStyle>
#include <qmimedatabase.h>
#include <qplatformdefs.h>
#include <QFileInfo>

#include "khtmlpart_p.h"
#include "khtml_iface.h"

#include "kpassivepopup.h"
#include "rendering/render_form.h"
#include <kwindowsystem.h>
#include <kconfiggroup.h>

#ifdef KJS_DEBUGGER
#include "ecma/debugger/debugwindow.h"
#endif

// SVG
#include <svg/SVGDocument.h>
#include <qstandardpaths.h>


bool KHTMLPartPrivate::s_dnsInitialised = false;

// DNS prefetch settings
static const int sMaxDNSPrefetchPerPage = 42;
static const int sDNSPrefetchTimerDelay = 200;
static const int sDNSTTLSeconds = 400;
static const int sDNSCacheSize = 500;


namespace khtml {

    class PartStyleSheetLoader : public CachedObjectClient
    {
    public:
        PartStyleSheetLoader(KHTMLPart *part, DOM::DOMString url, DocLoader* dl)
        {
            m_part = part;
            m_cachedSheet = dl->requestStyleSheet(url, QString(), "text/css",
                                                  true /* "user sheet" */);
            if (m_cachedSheet)
                m_cachedSheet->ref( this );
        }
        virtual ~PartStyleSheetLoader()
        {
            if ( m_cachedSheet ) m_cachedSheet->deref(this);
        }
        virtual void setStyleSheet(const DOM::DOMString&, const DOM::DOMString &sheet, const DOM::DOMString &, const DOM::DOMString &/*mimetype*/)
        {
          if ( m_part )
            m_part->setUserStyleSheet( sheet.string() );

            delete this;
        }
        virtual void error( int, const QString& ) {
          delete this;
        }
        QPointer<KHTMLPart> m_part;
        khtml::CachedCSSStyleSheet *m_cachedSheet;
    };
}

KHTMLPart::KHTMLPart( QWidget *parentWidget, QObject *parent, GUIProfile prof )
: KParts::ReadOnlyPart( parent )
{
    d = 0;
    KHTMLGlobal::registerPart( this );
    setComponentData( KHTMLGlobal::aboutData(), false );
    init( new KHTMLView( this, parentWidget ), prof );
}

KHTMLPart::KHTMLPart( KHTMLView *view, QObject *parent, GUIProfile prof )
: KParts::ReadOnlyPart( parent )
{
    d = 0;
    KHTMLGlobal::registerPart( this );
    setComponentData( KHTMLGlobal::aboutData(), false );
    assert( view );
    if (!view->part())
        view->setPart( this );
    init( view, prof );
}

void KHTMLPart::init( KHTMLView *view, GUIProfile prof )
{
  if ( prof == DefaultGUI )
    setXMLFile( "khtml.rc" );
  else if ( prof == BrowserViewGUI )
    setXMLFile( "khtml_browser.rc" );

  d = new KHTMLPartPrivate(this, parent());

  d->m_view = view;

  if (!parentPart()) {
      QWidget *widget = new QWidget( view->parentWidget() );
      widget->setObjectName("khtml_part_widget");
      QVBoxLayout *layout = new QVBoxLayout( widget );
      layout->setContentsMargins( 0, 0, 0, 0 );
      layout->setSpacing( 0 );
      widget->setLayout( layout );

      d->m_topViewBar = new KHTMLViewBar( KHTMLViewBar::Top, d->m_view, widget );
      d->m_bottomViewBar = new KHTMLViewBar( KHTMLViewBar::Bottom, d->m_view, widget );

      layout->addWidget( d->m_topViewBar );
      layout->addWidget( d->m_view );
      layout->addWidget( d->m_bottomViewBar );
      setWidget( widget );
      widget->setFocusProxy( d->m_view );
  } else {
      setWidget( view );
  }

  d->m_guiProfile = prof;
  d->m_extension = new KHTMLPartBrowserExtension( this );
  d->m_extension->setObjectName( "KHTMLBrowserExtension" );
  d->m_hostExtension = new KHTMLPartBrowserHostExtension( this );
  d->m_statusBarExtension = new KParts::StatusBarExtension( this );
  d->m_scriptableExtension = new KJS::KHTMLPartScriptable( this );
  new KHTMLTextExtension( this );
  new KHTMLHtmlExtension( this );
  d->m_statusBarPopupLabel = 0L;
  d->m_openableSuppressedPopups = 0;

  d->m_paLoadImages = 0;
  d->m_paDebugScript = 0;
  d->m_bMousePressed = false;
  d->m_bRightMousePressed = false;
  d->m_bCleared = false;

  if ( prof == BrowserViewGUI ) {
    d->m_paViewDocument = new QAction( i18n( "View Do&cument Source" ), this );
    actionCollection()->addAction( "viewDocumentSource", d->m_paViewDocument );
    connect( d->m_paViewDocument, SIGNAL(triggered(bool)), this, SLOT(slotViewDocumentSource()) );
    if (!parentPart()) {
        d->m_paViewDocument->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_U) );
    }

    d->m_paViewFrame = new QAction( i18n( "View Frame Source" ), this );
    actionCollection()->addAction( "viewFrameSource", d->m_paViewFrame );
    connect( d->m_paViewFrame, SIGNAL(triggered(bool)), this, SLOT(slotViewFrameSource()) );
    if (!parentPart()) {
        d->m_paViewFrame->setShortcut( QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_U) );
    }

    d->m_paViewInfo = new QAction( i18n( "View Document Information" ), this );
    actionCollection()->addAction( "viewPageInfo", d->m_paViewInfo );
    if (!parentPart()) {
        d->m_paViewInfo->setShortcut( QKeySequence(Qt::CTRL+Qt::Key_I) );
    }
    connect( d->m_paViewInfo, SIGNAL(triggered(bool)), this, SLOT(slotViewPageInfo()) );

    d->m_paSaveBackground = new QAction( i18n( "Save &Background Image As..." ), this );
    actionCollection()->addAction( "saveBackground", d->m_paSaveBackground );
    connect( d->m_paSaveBackground, SIGNAL(triggered(bool)), this, SLOT(slotSaveBackground()) );

    d->m_paSaveDocument = actionCollection()->addAction( KStandardAction::SaveAs, "saveDocument",
                                                       this, SLOT(slotSaveDocument()) );
    if ( parentPart() )
        d->m_paSaveDocument->setShortcuts( QList<QKeySequence>() ); // avoid clashes

    d->m_paSaveFrame = new QAction( i18n( "Save &Frame As..." ), this );
    actionCollection()->addAction( "saveFrame", d->m_paSaveFrame );
    connect( d->m_paSaveFrame, SIGNAL(triggered(bool)), this, SLOT(slotSaveFrame()) );
  } else {
    d->m_paViewDocument = 0;
    d->m_paViewFrame = 0;
    d->m_paViewInfo = 0;
    d->m_paSaveBackground = 0;
    d->m_paSaveDocument = 0;
    d->m_paSaveFrame = 0;
  }

  d->m_paSecurity = new QAction( i18n( "SSL" ), this );
  actionCollection()->addAction( "security", d->m_paSecurity );
  connect( d->m_paSecurity, SIGNAL(triggered(bool)), this, SLOT(slotSecurity()) );

  d->m_paDebugRenderTree = new QAction( i18n( "Print Rendering Tree to STDOUT" ), this );
  actionCollection()->addAction( "debugRenderTree", d->m_paDebugRenderTree );
  connect( d->m_paDebugRenderTree, SIGNAL(triggered(bool)), this, SLOT(slotDebugRenderTree()) );

  d->m_paDebugDOMTree = new QAction( i18n( "Print DOM Tree to STDOUT" ), this );
  actionCollection()->addAction( "debugDOMTree", d->m_paDebugDOMTree );
  connect( d->m_paDebugDOMTree, SIGNAL(triggered(bool)), this, SLOT(slotDebugDOMTree()) );

  QAction* paDebugFrameTree = new QAction( i18n( "Print frame tree to STDOUT" ), this );
  actionCollection()->addAction( "debugFrameTree", paDebugFrameTree );
  connect( paDebugFrameTree, SIGNAL(triggered(bool)), this, SLOT(slotDebugFrameTree()) );

  d->m_paStopAnimations = new QAction( i18n( "Stop Animated Images" ), this );
  actionCollection()->addAction( "stopAnimations", d->m_paStopAnimations );
  connect( d->m_paStopAnimations, SIGNAL(triggered(bool)), this, SLOT(slotStopAnimations()) );

  d->m_paSetEncoding = new KCodecAction( QIcon::fromTheme("character-set"), i18n( "Set &Encoding" ), this, true );
  actionCollection()->addAction( "setEncoding", d->m_paSetEncoding );
//   d->m_paSetEncoding->setDelayed( false );

  connect( d->m_paSetEncoding, SIGNAL(triggered(QString)), this, SLOT(slotSetEncoding(QString)));
  connect( d->m_paSetEncoding, SIGNAL(triggered(KEncodingProber::ProberType)), this, SLOT(slotAutomaticDetectionLanguage(KEncodingProber::ProberType)));

  if ( KSharedConfig::openConfig()->hasGroup( "HTML Settings" ) ) {
    KConfigGroup config( KSharedConfig::openConfig(), "HTML Settings" );

    d->m_autoDetectLanguage = static_cast<KEncodingProber::ProberType>(config.readEntry( "AutomaticDetectionLanguage", /*static_cast<int>(language) */0));
    if (d->m_autoDetectLanguage==KEncodingProber::None) {
      const QByteArray name = QTextCodec::codecForLocale()->name().toLower();
//       qWarning() << "00000000 ";
      if (name.endsWith("1251")||name.startsWith("koi")||name=="iso-8859-5")
        d->m_autoDetectLanguage=KEncodingProber::Cyrillic;
      else if (name.endsWith("1256")||name=="iso-8859-6")
        d->m_autoDetectLanguage=KEncodingProber::Arabic;
      else if (name.endsWith("1257")||name=="iso-8859-13"||name=="iso-8859-4")
        d->m_autoDetectLanguage=KEncodingProber::Baltic;
      else if (name.endsWith("1250")|| name=="ibm852" || name=="iso-8859-2" || name=="iso-8859-3" )
        d->m_autoDetectLanguage=KEncodingProber::CentralEuropean;
      else if (name.endsWith("1253")|| name=="iso-8859-7" )
        d->m_autoDetectLanguage=KEncodingProber::Greek;
      else if (name.endsWith("1255")|| name=="iso-8859-8" || name=="iso-8859-8-i" )
        d->m_autoDetectLanguage=KEncodingProber::Hebrew;
      else if (name=="jis7" || name=="eucjp" || name=="sjis"  )
        d->m_autoDetectLanguage=KEncodingProber::Japanese;
      else if (name=="gb2312" || name=="gbk" || name=="gb18030"  )
        d->m_autoDetectLanguage=KEncodingProber::ChineseSimplified;
      else if (name=="big5"  )
        d->m_autoDetectLanguage=KEncodingProber::ChineseTraditional;
      else if (name=="euc-kr"  )
        d->m_autoDetectLanguage=KEncodingProber::Korean;
      else if (name.endsWith("1254")|| name=="iso-8859-9" )
        d->m_autoDetectLanguage=KEncodingProber::Turkish;
      else if (name.endsWith("1252")|| name=="iso-8859-1" || name=="iso-8859-15" )
        d->m_autoDetectLanguage=KEncodingProber::WesternEuropean;
      else
        d->m_autoDetectLanguage=KEncodingProber::Universal;
//         qWarning() << "0000000end " << d->m_autoDetectLanguage << " " << QTextCodec::codecForLocale()->mibEnum();
    }
    d->m_paSetEncoding->setCurrentProberType(d->m_autoDetectLanguage);
  }

  d->m_paUseStylesheet = new KSelectAction( i18n( "Use S&tylesheet"), this );
  actionCollection()->addAction( "useStylesheet", d->m_paUseStylesheet );
  connect( d->m_paUseStylesheet, SIGNAL(triggered(int)), this, SLOT(slotUseStylesheet()) );

  if ( prof == BrowserViewGUI ) {
      d->m_paIncZoomFactor = new KHTMLZoomFactorAction( this, true, "format-font-size-more", i18n( "Enlarge Font" ), this );
      actionCollection()->addAction( "incFontSizes", d->m_paIncZoomFactor );
      connect(d->m_paIncZoomFactor, SIGNAL(triggered(bool)), SLOT(slotIncFontSizeFast()));
      d->m_paIncZoomFactor->setWhatsThis( i18n( "<qt>Enlarge Font<br /><br />"
                                                "Make the font in this window bigger. "
                            "Click and hold down the mouse button for a menu with all available font sizes.</qt>" ) );

      d->m_paDecZoomFactor = new KHTMLZoomFactorAction( this, false, "format-font-size-less", i18n( "Shrink Font" ), this );
      actionCollection()->addAction( "decFontSizes", d->m_paDecZoomFactor );
      connect(d->m_paDecZoomFactor, SIGNAL(triggered(bool)), SLOT(slotDecFontSizeFast()));
      d->m_paDecZoomFactor->setWhatsThis( i18n( "<qt>Shrink Font<br /><br />"
                                                "Make the font in this window smaller. "
                            "Click and hold down the mouse button for a menu with all available font sizes.</qt>" ) );
      if (!parentPart()) {
          // For framesets, this action also affects frames, so only
          // the frameset needs to define a shortcut for the action.

          // TODO: Why also CTRL+=?  Because of http://trolltech.com/developer/knowledgebase/524/?
          // Nobody else does it...
          d->m_paIncZoomFactor->setShortcut( QKeySequence("CTRL++; CTRL+=") );
          d->m_paDecZoomFactor->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_Minus) );
      }
  }

  d->m_paFind = actionCollection()->addAction( KStandardAction::Find, "find", this, SLOT(slotFind()) );
  d->m_paFind->setWhatsThis( i18n( "<qt>Find text<br /><br />"
                                   "Shows a dialog that allows you to find text on the displayed page.</qt>" ) );

  d->m_paFindNext = actionCollection()->addAction( KStandardAction::FindNext, "findNext", this, SLOT(slotFindNext()) );
  d->m_paFindNext->setWhatsThis( i18n( "<qt>Find next<br /><br />"
                                       "Find the next occurrence of the text that you "
                                       "have found using the <b>Find Text</b> function.</qt>" ) );

  d->m_paFindPrev = actionCollection()->addAction( KStandardAction::FindPrev, "findPrevious",
                                                   this, SLOT(slotFindPrev()) );
  d->m_paFindPrev->setWhatsThis( i18n( "<qt>Find previous<br /><br />"
                                       "Find the previous occurrence of the text that you "
                                       "have found using the <b>Find Text</b> function.</qt>" ) );

  // These two actions aren't visible in the menus, but exist for the (configurable) shortcut
  d->m_paFindAheadText = new QAction( i18n("Find Text as You Type"), this );
  actionCollection()->addAction( "findAheadText", d->m_paFindAheadText );
  d->m_paFindAheadText->setShortcut( QKeySequence("/") );
  d->m_paFindAheadText->setToolTip(i18n("This shortcut shows the find bar, for finding text in the displayed page. It cancels the effect of \"Find Links as You Type\", which sets the \"Find links only\" option."));
  d->m_paFindAheadText->setStatusTip(d->m_paFindAheadText->toolTip());
  connect( d->m_paFindAheadText, SIGNAL(triggered(bool)), this, SLOT(slotFindAheadText()) );

  d->m_paFindAheadLinks = new QAction( i18n("Find Links as You Type"), this );
  actionCollection()->addAction( "findAheadLink", d->m_paFindAheadLinks );
  // The issue is that it sets the (sticky) option FindLinksOnly, so
  // if you trigger this shortcut once by mistake, Esc and Ctrl+F will still have the option set.
  // Better let advanced users configure a shortcut for this advanced option
  //d->m_paFindAheadLinks->setShortcut( QKeySequence("\'") );
  d->m_paFindAheadLinks->setToolTip(i18n("This shortcut shows the find bar, and sets the option \"Find links only\"."));
  d->m_paFindAheadLinks->setStatusTip(d->m_paFindAheadLinks->toolTip());
  connect( d->m_paFindAheadLinks, SIGNAL(triggered(bool)), this, SLOT(slotFindAheadLink()) );

  if ( parentPart() )
  {
      d->m_paFind->setShortcuts( QList<QKeySequence>() ); // avoid clashes
      d->m_paFindNext->setShortcuts( QList<QKeySequence>() ); // avoid clashes
      d->m_paFindPrev->setShortcuts( QList<QKeySequence>() ); // avoid clashes
      d->m_paFindAheadText->setShortcuts( QList<QKeySequence>());
      d->m_paFindAheadLinks->setShortcuts( QList<QKeySequence>());
  }

  d->m_paPrintFrame = new QAction( i18n( "Print Frame..." ), this );
  actionCollection()->addAction( "printFrame", d->m_paPrintFrame );
  d->m_paPrintFrame->setIcon( QIcon::fromTheme( "document-print-frame" ) );
  connect( d->m_paPrintFrame, SIGNAL(triggered(bool)), this, SLOT(slotPrintFrame()) );
  d->m_paPrintFrame->setWhatsThis( i18n( "<qt>Print Frame<br /><br />"
                                         "Some pages have several frames. To print only a single frame, click "
                                         "on it and then use this function.</qt>" ) );

  // Warning: The name selectAll is used hardcoded by some 3rd parties to remove the
  // shortcut for selectAll so they do not get ambigous shortcuts. Renaming it
  // will either crash or render useless that workaround. It would be better
  // to use the name KStandardAction::name(KStandardAction::SelectAll) but we
  // can't for the same reason.
  d->m_paSelectAll = actionCollection()->addAction( KStandardAction::SelectAll, "selectAll",
                                                    this, SLOT(slotSelectAll()) );
  if ( parentPart() ) // Only the frameset has the shortcut, but the slot uses the current frame.
      d->m_paSelectAll->setShortcuts( QList<QKeySequence>() ); // avoid clashes

  d->m_paToggleCaretMode = new KToggleAction(i18n("Toggle Caret Mode"), this );
  actionCollection()->addAction( "caretMode", d->m_paToggleCaretMode );
  d->m_paToggleCaretMode->setShortcut( QKeySequence(Qt::Key_F7) );
  connect( d->m_paToggleCaretMode, SIGNAL(triggered(bool)), this, SLOT(slotToggleCaretMode()) );
  d->m_paToggleCaretMode->setChecked(isCaretMode());
  if (parentPart())
      d->m_paToggleCaretMode->setShortcuts(QList<QKeySequence>()); // avoid clashes

  // set the default java(script) flags according to the current host.
  d->m_bOpenMiddleClick = d->m_settings->isOpenMiddleClickEnabled();
  d->m_bJScriptEnabled = d->m_settings->isJavaScriptEnabled();
  setDebugScript( d->m_settings->isJavaScriptDebugEnabled() );
  d->m_bJavaEnabled = d->m_settings->isJavaEnabled();
  d->m_bPluginsEnabled = d->m_settings->isPluginsEnabled();

  // Set the meta-refresh flag...
  d->m_metaRefreshEnabled = d->m_settings->isAutoDelayedActionsEnabled ();

  KHTMLSettings::KSmoothScrollingMode ssm = d->m_settings->smoothScrolling();
  if (ssm == KHTMLSettings::KSmoothScrollingDisabled)
      d->m_view->setSmoothScrollingModeDefault(KHTMLView::SSMDisabled);
  else if (ssm == KHTMLSettings::KSmoothScrollingWhenEfficient)
      d->m_view->setSmoothScrollingModeDefault(KHTMLView::SSMWhenEfficient);
  else
      d->m_view->setSmoothScrollingModeDefault(KHTMLView::SSMEnabled);

  if (d->m_bDNSPrefetchIsDefault && !onlyLocalReferences()) {
      KHTMLSettings::KDNSPrefetch dpm = d->m_settings->dnsPrefetch();
      if (dpm == KHTMLSettings::KDNSPrefetchDisabled)
          d->m_bDNSPrefetch = DNSPrefetchDisabled;
      else if (dpm == KHTMLSettings::KDNSPrefetchOnlyWWWAndSLD)
          d->m_bDNSPrefetch = DNSPrefetchOnlyWWWAndSLD;
      else
          d->m_bDNSPrefetch = DNSPrefetchEnabled;
  }

  if (!KHTMLPartPrivate::s_dnsInitialised && d->m_bDNSPrefetch != DNSPrefetchDisabled) {
      KIO::HostInfo::setCacheSize( sDNSCacheSize );
      KIO::HostInfo::setTTL( sDNSTTLSeconds );
      KHTMLPartPrivate::s_dnsInitialised = true;
  }

  // all shortcuts should only be active, when this part has focus
  foreach ( QAction *action, actionCollection ()->actions () ) {
      action->setShortcutContext ( Qt::WidgetWithChildrenShortcut );
  }
  actionCollection()->associateWidget(view);

  connect( view, SIGNAL(zoomView(int)), SLOT(slotZoomView(int)) );

  connect( this, SIGNAL(completed()),
           this, SLOT(updateActions()) );
  connect( this, SIGNAL(completed(bool)),
           this, SLOT(updateActions()) );
  connect( this, SIGNAL(started(KIO::Job*)),
           this, SLOT(updateActions()) );

  // #### FIXME: the process wide loader is going to signal every part about every loaded object.
  //      That's quite inefficient. Should be per-document-tree somehow. Even signaling to
  //      child parts that a request from an ancestor has loaded is inefficent..
  connect( khtml::Cache::loader(), SIGNAL(requestStarted(khtml::DocLoader*,khtml::CachedObject*)),
           this, SLOT(slotLoaderRequestStarted(khtml::DocLoader*,khtml::CachedObject*)) );
  connect( khtml::Cache::loader(), SIGNAL(requestDone(khtml::DocLoader*,khtml::CachedObject*)),
           this, SLOT(slotLoaderRequestDone(khtml::DocLoader*,khtml::CachedObject*)) );
  connect( khtml::Cache::loader(), SIGNAL(requestFailed(khtml::DocLoader*,khtml::CachedObject*)),
           this, SLOT(slotLoaderRequestDone(khtml::DocLoader*,khtml::CachedObject*)) );

  connect ( &d->m_progressUpdateTimer, SIGNAL(timeout()), this, SLOT(slotProgressUpdate()) );

  findTextBegin(); //reset find variables

  connect( &d->m_redirectionTimer, SIGNAL(timeout()),
           this, SLOT(slotRedirect()) );

  if (QDBusConnection::sessionBus().isConnected()) {
      new KHTMLPartIface(this); // our "adaptor"
    for (int i = 1; ; ++i)
      if (QDBusConnection::sessionBus().registerObject(QString("/KHTML/%1/widget").arg(i), this))
        break;
      else if (i == 0xffff)
        qFatal("Something is very wrong in KHTMLPart!");
  }

  if (prof == BrowserViewGUI && !parentPart())
      loadPlugins();
}

KHTMLPart::~KHTMLPart()
{
  // qDebug() << this;
  KConfigGroup config( KSharedConfig::openConfig(), "HTML Settings" );
  config.writeEntry( "AutomaticDetectionLanguage", int(d->m_autoDetectLanguage) );

  if (d->m_manager) { // the PartManager for this part's children
    d->m_manager->removePart(this);
  }

  slotWalletClosed();
  if (!parentPart()) { // only delete it if the top khtml_part closes
    removeJSErrorExtension();
  }

  stopAutoScroll();
  d->m_redirectionTimer.stop();

  if (!d->m_bComplete)
    closeUrl();

  disconnect( khtml::Cache::loader(), SIGNAL(requestStarted(khtml::DocLoader*,khtml::CachedObject*)),
           this, SLOT(slotLoaderRequestStarted(khtml::DocLoader*,khtml::CachedObject*)) );
  disconnect( khtml::Cache::loader(), SIGNAL(requestDone(khtml::DocLoader*,khtml::CachedObject*)),
           this, SLOT(slotLoaderRequestDone(khtml::DocLoader*,khtml::CachedObject*)) );
  disconnect( khtml::Cache::loader(), SIGNAL(requestFailed(khtml::DocLoader*,khtml::CachedObject*)),
           this, SLOT(slotLoaderRequestDone(khtml::DocLoader*,khtml::CachedObject*)) );

  clear();
  hide();

  if ( d->m_view )
  {
    d->m_view->m_part = 0;
  }

  // Have to delete this here since we forward declare it in khtmlpart_p and
  // at least some compilers won't call the destructor in this case.
  delete d->m_jsedlg;
  d->m_jsedlg = 0;

  if (!parentPart()) // only delete d->m_frame if the top khtml_part closes
      delete d->m_frame;
  else if (d->m_frame && d->m_frame->m_run) // for kids, they may get detached while
      d->m_frame->m_run.data()->abort();  //  resolving mimetype; cancel that if needed
  delete d; d = 0;
  KHTMLGlobal::deregisterPart( this );
}

bool KHTMLPart::restoreURL( const QUrl &url )
{
  // qDebug() << url;

  d->m_redirectionTimer.stop();

  /*
   * That's not a good idea as it will call closeUrl() on all
   * child frames, preventing them from further loading. This
   * method gets called from restoreState() in case of a full frameset
   * restoral, and restoreState() calls closeUrl() before restoring
   * anyway.
  // qDebug() << "closing old URL";
  closeUrl();
  */

  d->m_bComplete = false;
  d->m_bLoadEventEmitted = false;
  d->m_workingURL = url;

  // set the java(script) flags according to the current host.
  d->m_bJScriptEnabled = KHTMLGlobal::defaultHTMLSettings()->isJavaScriptEnabled(url.host());
  setDebugScript( KHTMLGlobal::defaultHTMLSettings()->isJavaScriptDebugEnabled() );
  d->m_bJavaEnabled = KHTMLGlobal::defaultHTMLSettings()->isJavaEnabled(url.host());
  d->m_bPluginsEnabled = KHTMLGlobal::defaultHTMLSettings()->isPluginsEnabled(url.host());

  setUrl(url);

  d->m_restoreScrollPosition = true;
  disconnect(d->m_view, SIGNAL(finishedLayout()), this, SLOT(restoreScrollPosition()));
  connect(d->m_view, SIGNAL(finishedLayout()), this, SLOT(restoreScrollPosition()));

  KHTMLPageCache::self()->fetchData( d->m_cacheId, this, SLOT(slotRestoreData(QByteArray)));

  emit started( 0L );

  return true;
}

static bool areUrlsForSamePage(const QUrl& url1, const QUrl& url2)
{
    QUrl u1 = url1.adjusted(QUrl::StripTrailingSlash);
    u1.setFragment(QString());
    if (u1.path() == QLatin1String("/")) {
        u1.setPath(QString());
    }
    QUrl u2 = url2.adjusted(QUrl::StripTrailingSlash);
    u2.setFragment(QString());
    if (u2.path() == QLatin1String("/")) {
        u2.setPath(QString());
    }
    return u1 == u2;
}

bool KHTMLPartPrivate::isLocalAnchorJump( const QUrl& url )
{
    // kio_help actually uses fragments to identify different pages, so
    // always reload with it.
    if (url.scheme() == QLatin1String("help"))
        return false;

    return url.hasFragment() && areUrlsForSamePage(url, q->url());
}

void KHTMLPartPrivate::executeAnchorJump( const QUrl& url, bool lockHistory )
{
    // Note: we want to emit openUrlNotify first thing, to make the history capture the old state.
    if (!lockHistory)
        emit m_extension->openUrlNotify();

    DOM::HashChangeEventImpl *hashChangeEvImpl = 0;
    const QString &oldRef = q->url().fragment();
    const QString &newRef = url.fragment();
    if ((oldRef != newRef) || (oldRef.isNull() && newRef.isEmpty())) {
        hashChangeEvImpl = new DOM::HashChangeEventImpl();
        hashChangeEvImpl->initHashChangeEvent("hashchange",
                                    true, //bubble
                                    false, //cancelable
                                    q->url().toString(), //oldURL
                                    url.toString() //newURL
                                    );
    }

    if ( !q->gotoAnchor( url.fragment(QUrl::FullyEncoded)) )
        q->gotoAnchor( url.fragment() );

    q->setUrl(url);
    emit m_extension->setLocationBarUrl( url.toDisplayString() );

    if (hashChangeEvImpl) {
        m_doc->dispatchWindowEvent(hashChangeEvImpl);
    }
}

bool KHTMLPart::openUrl(const QUrl &_url)
{
  QUrl url(_url);
  // qDebug() << this << "opening" << url;

#ifndef KHTML_NO_WALLET
  // Wallet forms are per page, so clear it when loading a different page if we
  // are not an iframe (because we store walletforms only on the topmost part).
  if(!parentPart())
    d->m_walletForms.clear();
#endif
  d->m_redirectionTimer.stop();

  // check to see if this is an "error://" URL. This is caused when an error
  // occurs before this part was loaded (e.g. KonqRun), and is passed to
  // khtmlpart so that it can display the error.
  if ( url.scheme() == "error" ) {
    closeUrl();

    if(  d->m_bJScriptEnabled ) {
      d->m_statusBarText[BarOverrideText].clear();
      d->m_statusBarText[BarDefaultText].clear();
    }

    /**
     * The format of the error url is that two variables are passed in the query:
     * error = int kio error code, errText = QString error text from kio
     * and the URL where the error happened is passed as a sub URL.
     */
    const QUrl mainURL(url.fragment());
    //qDebug() << "Handling error URL. URL count:" << urls.count();

    if ( mainURL.isValid() ) {
      QString query = url.query(QUrl::FullyDecoded);
      QRegularExpression pattern("error=(\\d+)&errText=(.*)");
      QRegularExpressionMatch match = pattern.match(query);
      int error = match.captured(1).toInt();
      // error=0 isn't a valid error code, so 0 means it's missing from the URL
      if ( error == 0 ) error = KIO::ERR_UNKNOWN;
      const QString errorText = match.captured(2);
      d->m_workingURL = mainURL;
      //qDebug() << "Emitting fixed URL " << d->m_workingURL;
      emit d->m_extension->setLocationBarUrl( d->m_workingURL.toDisplayString() );
      htmlError( error, errorText, d->m_workingURL );
      return true;
    }
  }

  if (!parentPart()) { // only do it for toplevel part
    QString host = url.isLocalFile() ? "localhost" : url.host();
    QString userAgent = KProtocolManager::userAgentForHost(host);
    if (userAgent != KProtocolManager::userAgentForHost(QString())) {
      if (!d->m_statusBarUALabel) {
        d->m_statusBarUALabel = new KUrlLabel(d->m_statusBarExtension->statusBar());
        d->m_statusBarUALabel->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum));
        d->m_statusBarUALabel->setUseCursor(false);
        d->m_statusBarExtension->addStatusBarItem(d->m_statusBarUALabel, 0, false);
        d->m_statusBarUALabel->setPixmap(SmallIcon("preferences-web-browser-identification"));
      }
      d->m_statusBarUALabel->setToolTip(i18n("The fake user-agent '%1' is in use.", userAgent));
    } else if (d->m_statusBarUALabel) {
      d->m_statusBarExtension->removeStatusBarItem(d->m_statusBarUALabel);
      delete d->m_statusBarUALabel;
      d->m_statusBarUALabel = 0L;
    }
  }

  KParts::BrowserArguments browserArgs( d->m_extension->browserArguments() );
  KParts::OpenUrlArguments args( arguments() );

  // in case
  // a) we have no frameset (don't test m_frames.count(), iframes get in there)
  // b) the url is identical with the currently displayed one (except for the htmlref!)
  // c) the url request is not a POST operation and
  // d) the caller did not request to reload the page
  // e) there was no HTTP redirection meanwhile (testcase: webmin's software/tree.cgi)
  // => we don't reload the whole document and
  // we just jump to the requested html anchor
  bool isFrameSet = false;
  if ( d->m_doc && d->m_doc->isHTMLDocument() ) {
      HTMLDocumentImpl* htmlDoc = static_cast<HTMLDocumentImpl*>(d->m_doc);
      isFrameSet = htmlDoc->body() && (htmlDoc->body()->id() == ID_FRAMESET);
  }

  if (isFrameSet && d->isLocalAnchorJump(url) && browserArgs.softReload)
  {
    QList<khtml::ChildFrame*>::Iterator it = d->m_frames.begin();
    const QList<khtml::ChildFrame*>::Iterator end = d->m_frames.end();
    for (; it != end; ++it) {
      KHTMLPart* const part = qobject_cast<KHTMLPart *>( (*it)->m_part.data() );
      if (part)
      {
        // We are reloading frames to make them jump into offsets.
        KParts::OpenUrlArguments partargs( part->arguments() );
        partargs.setReload( true );
        part->setArguments( partargs );

        part->openUrl( part->url() );
      }
    }/*next it*/
    return true;
  }

  if ( url.hasFragment() && !isFrameSet )
  {
    bool noReloadForced = !args.reload() && !browserArgs.redirectedRequest() && !browserArgs.doPost();
    if ( noReloadForced &&  d->isLocalAnchorJump(url) )
    {
        // qDebug() << "jumping to anchor. m_url = " << url;
        setUrl(url);
        emit started( 0 );

        if ( !gotoAnchor( url.fragment(QUrl::FullyEncoded)) )
          gotoAnchor( url.fragment() );

        d->m_bComplete = true;
        if (d->m_doc)
            d->m_doc->setParsing(false);

        // qDebug() << "completed...";
        emit completed();
        return true;
    }
  }

  // Save offset of viewport when page is reloaded to be compliant
  // to every other capable browser out there.
  if (args.reload()) {
    args.setXOffset( d->m_view->contentsX() );
    args.setYOffset( d->m_view->contentsY() );
    setArguments(args);
  }

  if (!d->m_restored)
    closeUrl();

  d->m_restoreScrollPosition = d->m_restored;
  disconnect(d->m_view, SIGNAL(finishedLayout()), this, SLOT(restoreScrollPosition()));
  connect(d->m_view, SIGNAL(finishedLayout()), this, SLOT(restoreScrollPosition()));

  // Classify the mimetype. Some, like images and plugins are handled
  // by wrapping things up in tags, so we want to plain output the HTML,
  // and not start the job and all that (since we would want the
  // KPart or whatever to load it).
  // This is also the only place we need to do this, as it's for
  // internal iframe use, not any other clients.
  MimeType type = d->classifyMimeType(args.mimeType());

  if (type == MimeImage || type == MimeOther) {
      begin(url, args.xOffset(), args.yOffset());
      write(QString::fromLatin1("<html><head></head><body>"));
      if (type == MimeImage)
          write(QString::fromLatin1("<img "));
      else
          write(QString::fromLatin1("<embed "));
      write(QString::fromLatin1("src=\""));

      assert(url.toString().indexOf('"') == -1);
      write(url.toString());

      write(QString::fromLatin1("\">"));
      end();
      return true;
  }


  // initializing m_url to the new url breaks relative links when opening such a link after this call and _before_ begin() is called (when the first
  // data arrives) (Simon)
  d->m_workingURL = url;
  if(url.scheme().startsWith( "http" ) && !url.host().isEmpty() &&
     url.path().isEmpty()) {
    d->m_workingURL.setPath("/");
    emit d->m_extension->setLocationBarUrl( d->m_workingURL.toDisplayString() );
  }
  setUrl(d->m_workingURL);

  QMap<QString,QString>& metaData = args.metaData();
  metaData.insert("main_frame_request", parentPart() == 0 ? "TRUE" : "FALSE" );
  metaData.insert("ssl_parent_ip", d->m_ssl_parent_ip);
  metaData.insert("ssl_parent_cert", d->m_ssl_parent_cert);
  metaData.insert("PropagateHttpHeader", "true");
  metaData.insert("ssl_was_in_use", d->m_ssl_in_use ? "TRUE" : "FALSE" );
  metaData.insert("ssl_activate_warnings", "TRUE" );
  metaData.insert("cross-domain", toplevelURL().toString());

  if (d->m_restored)
  {
     metaData.insert("referrer", d->m_pageReferrer);
     d->m_cachePolicy = KIO::CC_Cache;
  }
  else if (args.reload() && !browserArgs.softReload)
     d->m_cachePolicy = KIO::CC_Reload;
  else
     d->m_cachePolicy = KProtocolManager::cacheControl();

  if ( browserArgs.doPost() && (url.scheme().startsWith("http")) )
  {
      d->m_job = KIO::http_post( url, browserArgs.postData, KIO::HideProgressInfo );
      d->m_job->addMetaData("content-type", browserArgs.contentType() );
  }
  else
  {
      d->m_job = KIO::get( url, KIO::NoReload, KIO::HideProgressInfo );
      d->m_job->addMetaData("cache", KIO::getCacheControlString(d->m_cachePolicy));
  }

  if (widget())
     KJobWidgets::setWindow(d->m_job, widget()->topLevelWidget());
  d->m_job->addMetaData(metaData);

  connect( d->m_job, SIGNAL(result(KJob*)),
           SLOT(slotFinished(KJob*)) );
  connect( d->m_job, SIGNAL(data(KIO::Job*,QByteArray)),
           SLOT(slotData(KIO::Job*,QByteArray)) );
  connect ( d->m_job, SIGNAL(infoMessage(KJob*,QString,QString)),
           SLOT(slotInfoMessage(KJob*,QString)) );
  connect( d->m_job, SIGNAL(redirection(KIO::Job*,QUrl)),
           SLOT(slotRedirection(KIO::Job*,QUrl)) );

  d->m_bComplete = false;
  d->m_bLoadEventEmitted = false;

  // delete old status bar msg's from kjs (if it _was_ activated on last URL)
  if( d->m_bJScriptEnabled ) {
    d->m_statusBarText[BarOverrideText].clear();
    d->m_statusBarText[BarDefaultText].clear();
  }

  // set the javascript flags according to the current url
  d->m_bJScriptEnabled = KHTMLGlobal::defaultHTMLSettings()->isJavaScriptEnabled(url.host());
  setDebugScript( KHTMLGlobal::defaultHTMLSettings()->isJavaScriptDebugEnabled() );
  d->m_bJavaEnabled = KHTMLGlobal::defaultHTMLSettings()->isJavaEnabled(url.host());
  d->m_bPluginsEnabled = KHTMLGlobal::defaultHTMLSettings()->isPluginsEnabled(url.host());


  connect( d->m_job, SIGNAL(speed(KJob*,ulong)),
           this, SLOT(slotJobSpeed(KJob*,ulong)) );

  connect( d->m_job, SIGNAL(percent(KJob*,ulong)),
           this, SLOT(slotJobPercent(KJob*,ulong)) );

  connect( d->m_job, SIGNAL(result(KJob*)),
           this, SLOT(slotJobDone(KJob*)) );

  d->m_jobspeed = 0;

  // If this was an explicit reload and the user style sheet should be used,
  // do a stat to see whether the stylesheet was changed in the meanwhile.
  if ( args.reload() && !settings()->userStyleSheet().isEmpty() ) {
    QUrl url( settings()->userStyleSheet() );
    KIO::StatJob *job = KIO::stat( url, KIO::HideProgressInfo );
    connect( job, SIGNAL(result(KJob*)),
             this, SLOT(slotUserSheetStatDone(KJob*)) );
  }
  startingJob( d->m_job );
  emit started( 0L );

  return true;
}

bool KHTMLPart::closeUrl()
{
  if ( d->m_job )
  {
    KHTMLPageCache::self()->cancelEntry(d->m_cacheId);
    d->m_job->kill();
    d->m_job = 0;
  }

  if ( d->m_doc && d->m_doc->isHTMLDocument() ) {
    HTMLDocumentImpl* hdoc = static_cast<HTMLDocumentImpl*>( d->m_doc );

    if ( hdoc->body() && d->m_bLoadEventEmitted ) {
      hdoc->body()->dispatchWindowEvent( EventImpl::UNLOAD_EVENT, false, false );
      if ( d->m_doc )
        d->m_doc->updateRendering();
      d->m_bLoadEventEmitted = false;
    }
  }

  d->m_bComplete = true; // to avoid emitting completed() in slotFinishedParsing() (David)
  d->m_bLoadEventEmitted = true; // don't want that one either
  d->m_cachePolicy = KProtocolManager::cacheControl(); // reset cache policy

  disconnect(d->m_view, SIGNAL(finishedLayout()), this, SLOT(restoreScrollPosition()));

  KHTMLPageCache::self()->cancelFetch(this);
  if ( d->m_doc && d->m_doc->parsing() )
  {
    // qDebug() << " was still parsing... calling end ";
    slotFinishedParsing();
    d->m_doc->setParsing(false);
  }

  if ( !d->m_workingURL.isEmpty() )
  {
    // Aborted before starting to render
    // qDebug() << "Aborted before starting to render, reverting location bar to " << url();
    emit d->m_extension->setLocationBarUrl( url().toDisplayString() );
  }

  d->m_workingURL = QUrl();

  if ( d->m_doc && d->m_doc->docLoader() )
    khtml::Cache::loader()->cancelRequests( d->m_doc->docLoader() );

  // tell all subframes to stop as well
  {
    ConstFrameIt it = d->m_frames.constBegin();
    const ConstFrameIt end = d->m_frames.constEnd();
    for (; it != end; ++it )
    {
      if ( (*it)->m_run )
        (*it)->m_run.data()->abort();
      if ( !( *it )->m_part.isNull() )
        ( *it )->m_part.data()->closeUrl();
    }
  }
  // tell all objects to stop as well
  {
    ConstFrameIt it = d->m_objects.constBegin();
    const ConstFrameIt end = d->m_objects.constEnd();
    for (; it != end; ++it)
    {
      if ( !( *it )->m_part.isNull() )
        ( *it )->m_part.data()->closeUrl();
    }
  }
  // Stop any started redirections as well!! (DA)
  if ( d && d->m_redirectionTimer.isActive() )
    d->m_redirectionTimer.stop();

  // null node activated.
  emit nodeActivated(Node());

  // make sure before clear() runs, we pop out of a dialog's message loop
  if ( d->m_view )
    d->m_view->closeChildDialogs();

  return true;
}

DOM::HTMLDocument KHTMLPart::htmlDocument() const
{
  if (d->m_doc && d->m_doc->isHTMLDocument())
    return static_cast<HTMLDocumentImpl*>(d->m_doc);
  else
    return static_cast<HTMLDocumentImpl*>(0);
}

DOM::Document KHTMLPart::document() const
{
    return d->m_doc;
}

QString KHTMLPart::documentSource() const
{
  QString sourceStr;
  if ( !( url().isLocalFile() ) && KHTMLPageCache::self()->isComplete( d->m_cacheId ) )
  {
     QByteArray sourceArray;
     QDataStream dataStream( &sourceArray, QIODevice::WriteOnly );
     KHTMLPageCache::self()->saveData( d->m_cacheId, &dataStream );
     QTextStream stream( sourceArray, QIODevice::ReadOnly );
     stream.setCodec( QTextCodec::codecForName( encoding().toLatin1().constData() ) );
     sourceStr = stream.readAll();
  } else
  {
    QTemporaryFile tmpFile;
    if ( !tmpFile.open() ) {
        return sourceStr;
    }

    KIO::FileCopyJob *job = KIO::file_copy(url(), QUrl::fromLocalFile(tmpFile.fileName()), KIO::Overwrite);
    if( job->exec() )
    {
      QTextStream stream( &tmpFile );
      stream.setCodec( QTextCodec::codecForName( encoding().toLatin1().constData() ) );
      sourceStr = stream.readAll();
    }
  }

  return sourceStr;
}


KParts::BrowserExtension *KHTMLPart::browserExtension() const
{
  return d->m_extension;
}

KParts::BrowserHostExtension *KHTMLPart::browserHostExtension() const
{
  return d->m_hostExtension;
}

KHTMLView *KHTMLPart::view() const
{
  return d->m_view;
}

KHTMLViewBar *KHTMLPart::pTopViewBar() const
{
  if (const_cast<KHTMLPart*>(this)->parentPart())
      return const_cast<KHTMLPart*>(this)->parentPart()->pTopViewBar();
  return d->m_topViewBar;
}

KHTMLViewBar *KHTMLPart::pBottomViewBar() const
{
  if (const_cast<KHTMLPart*>(this)->parentPart())
      return const_cast<KHTMLPart*>(this)->parentPart()->pBottomViewBar();
  return d->m_bottomViewBar;
}

void KHTMLPart::setStatusMessagesEnabled( bool enable )
{
  d->m_statusMessagesEnabled = enable;
}

KJS::Interpreter *KHTMLPart::jScriptInterpreter()
{
  KJSProxy *proxy = jScript();
  if (!proxy || proxy->paused())
    return 0;

  return proxy->interpreter();
}

bool KHTMLPart::statusMessagesEnabled() const
{
  return d->m_statusMessagesEnabled;
}

void KHTMLPart::setJScriptEnabled( bool enable )
{
  if ( !enable && jScriptEnabled() && d->m_frame && d->m_frame->m_jscript ) {
    d->m_frame->m_jscript->clear();
  }
  d->m_bJScriptForce = enable;
  d->m_bJScriptOverride = true;
}

bool KHTMLPart::jScriptEnabled() const
{
  if(onlyLocalReferences()) return false;

  if ( d->m_bJScriptOverride )
      return d->m_bJScriptForce;
  return d->m_bJScriptEnabled;
}

void KHTMLPart::setDNSPrefetch( DNSPrefetch pmode )
{
  d->m_bDNSPrefetch = pmode;
  d->m_bDNSPrefetchIsDefault = false;
}

KHTMLPart::DNSPrefetch KHTMLPart::dnsPrefetch() const
{
  if (onlyLocalReferences())
      return DNSPrefetchDisabled;
  return d->m_bDNSPrefetch;
}

void KHTMLPart::setMetaRefreshEnabled( bool enable )
{
  d->m_metaRefreshEnabled = enable;
}

bool KHTMLPart::metaRefreshEnabled() const
{
  return d->m_metaRefreshEnabled;
}

KJSProxy *KHTMLPart::jScript()
{
  if (!jScriptEnabled()) return 0;

  if ( !d->m_frame ) {
      KHTMLPart * p = parentPart();
      if (!p) {
          d->m_frame = new khtml::ChildFrame;
          d->m_frame->m_part = this;
      } else {
          ConstFrameIt it = p->d->m_frames.constBegin();
          const ConstFrameIt end = p->d->m_frames.constEnd();
          for (; it != end; ++it)
              if ((*it)->m_part.data() == this) {
                  d->m_frame = *it;
                  break;
              }
      }
      if ( !d->m_frame )
        return 0;
  }
  if ( !d->m_frame->m_jscript )
     d->m_frame->m_jscript = new KJSProxy(d->m_frame);
  d->m_frame->m_jscript->setDebugEnabled(d->m_bJScriptDebugEnabled);

  return d->m_frame->m_jscript;
}

QVariant KHTMLPart::crossFrameExecuteScript(const QString& target,  const QString& script)
{
  KHTMLPart* destpart = this;

  QString trg = target.toLower();

  if (target == "_top") {
    while (destpart->parentPart())
      destpart = destpart->parentPart();
  }
  else if (target == "_parent") {
    if (parentPart())
      destpart = parentPart();
  }
  else if (target == "_self" || target == "_blank")  {
    // we always allow these
  }
  else {
    destpart = findFrame(target);
    if (!destpart)
       destpart = this;
  }

  // easy way out?
  if (destpart == this)
    return executeScript(DOM::Node(), script);

  // now compare the domains
  if (destpart->checkFrameAccess(this))
    return destpart->executeScript(DOM::Node(), script);

  // eww, something went wrong. better execute it in our frame
  return executeScript(DOM::Node(), script);
}

//Enable this to see all JS scripts being executed
//#define KJS_VERBOSE

KJSErrorDlg *KHTMLPart::jsErrorExtension() {
  if (!d->m_settings->jsErrorsEnabled()) {
    return 0L;
  }

  if (parentPart()) {
    return parentPart()->jsErrorExtension();
  }

  if (!d->m_statusBarJSErrorLabel) {
    d->m_statusBarJSErrorLabel = new KUrlLabel(d->m_statusBarExtension->statusBar());
    d->m_statusBarJSErrorLabel->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum));
    d->m_statusBarJSErrorLabel->setUseCursor(false);
    d->m_statusBarExtension->addStatusBarItem(d->m_statusBarJSErrorLabel, 0, false);
    d->m_statusBarJSErrorLabel->setToolTip(i18n("This web page contains coding errors."));
    d->m_statusBarJSErrorLabel->setPixmap(SmallIcon("script-error"));
    connect(d->m_statusBarJSErrorLabel, SIGNAL(leftClickedUrl()), SLOT(launchJSErrorDialog()));
    connect(d->m_statusBarJSErrorLabel, SIGNAL(rightClickedUrl()), SLOT(jsErrorDialogContextMenu()));
  }
  if (!d->m_jsedlg) {
    d->m_jsedlg = new KJSErrorDlg;
    d->m_jsedlg->setURL(url().toDisplayString());
    if (widget()->style()->styleHint(QStyle::SH_DialogButtonBox_ButtonsHaveIcons, 0, widget())) {
      d->m_jsedlg->_clear->setIcon(QIcon::fromTheme("edit-clear-locationbar-ltr"));
      d->m_jsedlg->_close->setIcon(QIcon::fromTheme("window-close"));
    }
  }
  return d->m_jsedlg;
}

void KHTMLPart::removeJSErrorExtension() {
  if (parentPart()) {
    parentPart()->removeJSErrorExtension();
    return;
  }
  if (d->m_statusBarJSErrorLabel != 0) {
    d->m_statusBarExtension->removeStatusBarItem( d->m_statusBarJSErrorLabel );
    delete d->m_statusBarJSErrorLabel;
    d->m_statusBarJSErrorLabel = 0;
  }
  delete d->m_jsedlg;
  d->m_jsedlg = 0;
}

void KHTMLPart::disableJSErrorExtension() {
  removeJSErrorExtension();
  // These two lines are really kind of hacky, and it sucks to do this inside
  // KHTML but I don't know of anything that's reasonably easy as an alternative
  // right now.  It makes me wonder if there should be a more clean way to
  // contact all running "KHTML" instance as opposed to Konqueror instances too.
  d->m_settings->setJSErrorsEnabled(false);
  emit configurationChanged();
}

void KHTMLPart::jsErrorDialogContextMenu() {
  QMenu *m = new QMenu(0L);
  m->addAction(i18n("&Hide Errors"), this, SLOT(removeJSErrorExtension()));
  m->addAction(i18n("&Disable Error Reporting"), this, SLOT(disableJSErrorExtension()));
  m->popup(QCursor::pos());
}

void KHTMLPart::launchJSErrorDialog() {
  KJSErrorDlg *dlg = jsErrorExtension();
  if (dlg) {
    dlg->show();
    dlg->raise();
  }
}

void KHTMLPart::launchJSConfigDialog() {
  QStringList args;
  args << "khtml_java_js";
  KToolInvocation::kdeinitExec( "kcmshell4", args );
}

QVariant KHTMLPart::executeScript(const QString& filename, int baseLine, const DOM::Node& n, const QString& script)
{
#ifdef KJS_VERBOSE
  // The script is now printed by KJS's Parser::parse
  qDebug() << "executeScript: caller='" << objectName() << "' filename=" << filename << " baseLine=" << baseLine /*<< " script=" << script*/;
#endif
  KJSProxy *proxy = jScript();

  if (!proxy || proxy->paused())
    return QVariant();

  //Make sure to initialize the interpreter before creating Completion
  (void)proxy->interpreter();

  KJS::Completion comp;

  QVariant ret = proxy->evaluate(filename, baseLine, script, n, &comp);

  /*
   *  Error handling
   */
  if (comp.complType() == KJS::Throw && comp.value()) {
    KJSErrorDlg *dlg = jsErrorExtension();
    if (dlg) {
      QString msg = KJS::exceptionToString(
                              proxy->interpreter()->globalExec(), comp.value());
      dlg->addError(i18n("<qt><b>Error</b>: %1: %2</qt>",
                         Qt::escape(filename), Qt::escape(msg)));
    }
  }

  // Handle immediate redirects now (e.g. location='foo')
  if ( !d->m_redirectURL.isEmpty() && d->m_delayRedirect == -1 )
  {
    // qDebug() << "executeScript done, handling immediate redirection NOW";
    // Must abort tokenizer, no further script must execute.
    khtml::Tokenizer* t = d->m_doc->tokenizer();
    if(t)
      t->abort();
    d->m_redirectionTimer.setSingleShot( true );
    d->m_redirectionTimer.start( 0 );
  }

  return ret;
}

QVariant KHTMLPart::executeScript( const QString &script )
{
    return executeScript( DOM::Node(), script );
}

QVariant KHTMLPart::executeScript( const DOM::Node &n, const QString &script )
{
#ifdef KJS_VERBOSE
  qDebug() << "caller=" << objectName() << "node=" << n.nodeName().string().toLatin1().constData() << "(" << (n.isNull() ? 0 : n.nodeType()) << ") " /* << script */;
#endif
  KJSProxy *proxy = jScript();

  if (!proxy || proxy->paused())
    return QVariant();
  (void)proxy->interpreter();//Make sure stuff is initialized

  ++(d->m_runningScripts);
  KJS::Completion comp;
  const QVariant ret = proxy->evaluate( QString(), 1, script, n, &comp );
  --(d->m_runningScripts);

  /*
   *  Error handling
   */
  if (comp.complType() == KJS::Throw && comp.value()) {
    KJSErrorDlg *dlg = jsErrorExtension();
    if (dlg) {
      QString msg = KJS::exceptionToString(
                              proxy->interpreter()->globalExec(), comp.value());
      dlg->addError(i18n("<qt><b>Error</b>: node %1: %2</qt>",
                         n.nodeName().string(), Qt::escape(msg)));
    }
  }

  if (!d->m_runningScripts && d->m_doc && !d->m_doc->parsing() && d->m_submitForm )
      submitFormAgain();

#ifdef KJS_VERBOSE
  qDebug() << "done";
#endif
  return ret;
}

void KHTMLPart::setJavaEnabled( bool enable )
{
  d->m_bJavaForce = enable;
  d->m_bJavaOverride = true;
}

bool KHTMLPart::javaEnabled() const
{
  if (onlyLocalReferences()) return false;

  if( d->m_bJavaOverride )
      return d->m_bJavaForce;
  return d->m_bJavaEnabled;
}

void KHTMLPart::setPluginsEnabled( bool enable )
{
  d->m_bPluginsForce = enable;
  d->m_bPluginsOverride = true;
}

bool KHTMLPart::pluginsEnabled() const
{
  if (onlyLocalReferences()) return false;

  if ( d->m_bPluginsOverride )
      return d->m_bPluginsForce;
  return d->m_bPluginsEnabled;
}

static int s_DOMTreeIndentLevel = 0;

void KHTMLPart::slotDebugDOMTree()
{
  if ( d->m_doc )
    qDebug("%s", d->m_doc->toString().string().toLatin1().constData());

  // Now print the contents of the frames that contain HTML

  const int indentLevel = s_DOMTreeIndentLevel++;

  ConstFrameIt it = d->m_frames.constBegin();
  const ConstFrameIt end = d->m_frames.constEnd();
  for (; it != end; ++it )
    if ( !( *it )->m_part.isNull() && (*it)->m_part.data()->inherits( "KHTMLPart" ) ) {
      KParts::ReadOnlyPart* const p = ( *it )->m_part.data();
      // qDebug() << QString().leftJustified(s_DOMTreeIndentLevel*4,' ') << "FRAME " << p->objectName() << " ";
      static_cast<KHTMLPart*>( p )->slotDebugDOMTree();
    }
  s_DOMTreeIndentLevel = indentLevel;
}

void KHTMLPart::slotDebugScript()
{
  if (jScript())
    jScript()->showDebugWindow();
}

void KHTMLPart::slotDebugRenderTree()
{
#ifndef NDEBUG
  if ( d->m_doc ) {
    d->m_doc->renderer()->printTree();
    // dump out the contents of the rendering & DOM trees
//    QString dumps;
//    QTextStream outputStream(&dumps,QIODevice::WriteOnly);
//    d->m_doc->renderer()->layer()->dump( outputStream );
//    qDebug() << "dump output:" << "\n" + dumps;
//    d->m_doc->renderer()->printLineBoxTree();
  }
#endif
}

void KHTMLPart::slotDebugFrameTree()
{
    khtml::ChildFrame::dumpFrameTree(this);
}

void KHTMLPart::slotStopAnimations()
{
  stopAnimations();
}

void KHTMLPart::setAutoloadImages( bool enable )
{
  if ( d->m_doc && d->m_doc->docLoader()->autoloadImages() == enable )
    return;

  if ( d->m_doc )
    d->m_doc->docLoader()->setAutoloadImages( enable );

  unplugActionList( "loadImages" );

  if ( enable ) {
    delete d->m_paLoadImages;
    d->m_paLoadImages = 0;
  }
  else if ( !d->m_paLoadImages ) {
    d->m_paLoadImages = new QAction( i18n( "Display Images on Page" ), this );
    actionCollection()->addAction( "loadImages", d->m_paLoadImages );
    d->m_paLoadImages->setIcon( QIcon::fromTheme( "image-loading" ) );
    connect( d->m_paLoadImages, SIGNAL(triggered(bool)), this, SLOT(slotLoadImages()) );
  }

  if ( d->m_paLoadImages ) {
    QList<QAction*> lst;
    lst.append( d->m_paLoadImages );
    plugActionList( "loadImages", lst );
  }
}

bool KHTMLPart::autoloadImages() const
{
  if ( d->m_doc )
    return d->m_doc->docLoader()->autoloadImages();

  return true;
}

void KHTMLPart::clear()
{
  if ( d->m_bCleared )
    return;

  d->m_bCleared = true;

  d->m_bClearing = true;

  {
    ConstFrameIt it = d->m_frames.constBegin();
    const ConstFrameIt end = d->m_frames.constEnd();
    for(; it != end; ++it )
    {
      // Stop HTMLRun jobs for frames
      if ( (*it)->m_run )
        (*it)->m_run.data()->abort();
    }
  }

  {
    ConstFrameIt it = d->m_objects.constBegin();
    const ConstFrameIt end = d->m_objects.constEnd();
    for(; it != end; ++it )
    {
      // Stop HTMLRun jobs for objects
      if ( (*it)->m_run )
        (*it)->m_run.data()->abort();
    }
  }


  findTextBegin(); // resets d->m_findNode and d->m_findPos
  d->m_mousePressNode = DOM::Node();


  if ( d->m_doc )
  {
    if (d->m_doc->attached()) //the view may have detached it already
        d->m_doc->detach();
  }

  // Moving past doc so that onUnload works.
  if ( d->m_frame && d->m_frame->m_jscript )
    d->m_frame->m_jscript->clear();

  // stopping marquees
  if (d->m_doc && d->m_doc->renderer() && d->m_doc->renderer()->layer())
      d->m_doc->renderer()->layer()->suspendMarquees();

  if ( d->m_view )
    d->m_view->clear();

  // do not dereference the document before the jscript and view are cleared, as some destructors
  // might still try to access the document.
  if ( d->m_doc ) {
    d->m_doc->deref();
  }
  d->m_doc = 0;

  delete d->m_decoder;
  d->m_decoder = 0;

  // We don't want to change between parts if we are going to delete all of them anyway
  if (partManager()) {
        disconnect( partManager(), SIGNAL(activePartChanged(KParts::Part*)),
                    this, SLOT(slotActiveFrameChanged(KParts::Part*)) );
  }

  if (d->m_frames.count())
  {
    const KHTMLFrameList frames = d->m_frames;
    d->m_frames.clear();
    ConstFrameIt it = frames.begin();
    const ConstFrameIt end = frames.end();
    for(; it != end; ++it )
    {
      if ( (*it)->m_part )
      {
        partManager()->removePart( (*it)->m_part.data() );
        delete (*it)->m_part.data();
      }
      delete *it;
    }
  }
  d->m_suppressedPopupOriginParts.clear();

  if (d->m_objects.count())
  {
    KHTMLFrameList objects = d->m_objects;
    d->m_objects.clear();
    ConstFrameIt oi = objects.constBegin();
    const ConstFrameIt oiEnd = objects.constEnd();

    for (; oi != oiEnd; ++oi )
    {
      delete (*oi)->m_part.data();
      delete *oi;
    }
  }

  // Listen to part changes again
  if (partManager()) {
        connect( partManager(), SIGNAL(activePartChanged(KParts::Part*)),
                 this, SLOT(slotActiveFrameChanged(KParts::Part*)) );
  }

  d->clearRedirection();
  d->m_redirectLockHistory = true;
  d->m_bClearing = false;
  d->m_frameNameId = 1;
  d->m_bFirstData = true;

  d->m_bMousePressed = false;

  if (d->editor_context.m_caretBlinkTimer >= 0)
      killTimer(d->editor_context.m_caretBlinkTimer);
  d->editor_context.reset();
#ifndef QT_NO_CLIPBOARD
  connect( qApp->clipboard(), SIGNAL(selectionChanged()), SLOT(slotClearSelection()));
#endif

  d->m_jobPercent = 0;

  if ( !d->m_haveEncoding )
    d->m_encoding.clear();

  d->m_DNSPrefetchQueue.clear();
  if (d->m_DNSPrefetchTimer > 0)
      killTimer(d->m_DNSPrefetchTimer);
  d->m_DNSPrefetchTimer = -1;
  d->m_lookedupHosts.clear();
  if (d->m_DNSTTLTimer > 0)
      killTimer(d->m_DNSTTLTimer);
  d->m_DNSTTLTimer = -1;
  d->m_numDNSPrefetchedNames = 0;

#ifdef SPEED_DEBUG
  d->m_parsetime.restart();
#endif
}

bool KHTMLPart::openFile()
{
  return true;
}

DOM::HTMLDocumentImpl *KHTMLPart::docImpl() const
{
    if ( d && d->m_doc && d->m_doc->isHTMLDocument() )
        return static_cast<HTMLDocumentImpl*>(d->m_doc);
    return 0;
}

DOM::DocumentImpl *KHTMLPart::xmlDocImpl() const
{
    if ( d )
        return d->m_doc;
    return 0;
}

void KHTMLPart::slotInfoMessage(KJob* kio_job, const QString& msg)
{
  assert(d->m_job == kio_job);
  Q_ASSERT(kio_job);
  Q_UNUSED(kio_job);

  if (!parentPart())
    setStatusBarText(msg, BarDefaultText);
}

void KHTMLPart::setPageSecurity( PageSecurity sec )
{
  emit d->m_extension->setPageSecurity( sec );
}

void KHTMLPart::slotData( KIO::Job* kio_job, const QByteArray &data )
{
  assert ( d->m_job == kio_job );
  Q_ASSERT(kio_job);
  Q_UNUSED(kio_job);

  //qDebug() << "slotData: " << data.size();
  // The first data ?
  if ( !d->m_workingURL.isEmpty() )
  {
      //qDebug() << "begin!";

    // We must suspend KIO while we're inside begin() because it can cause
    // crashes if a window (such as kjsdebugger) goes back into the event loop,
    // more data arrives, and begin() gets called again (re-entered).
    d->m_job->suspend();
    begin( d->m_workingURL, arguments().xOffset(), arguments().yOffset() );
    d->m_job->resume();

    // CC_Refresh means : always send the server an If-Modified-Since conditional request.
    //                    This is the default cache setting and correspond to the KCM's "Keep cache in sync".
    // CC_Verify means :  only send a conditional request if the cache expiry date is passed.
    //                    It doesn't have a KCM setter.
    // We override the first to the second, except when doing a soft-reload.
    if (d->m_cachePolicy == KIO::CC_Refresh && !d->m_extension->browserArguments().softReload)
        d->m_doc->docLoader()->setCachePolicy(KIO::CC_Verify);
    else
        d->m_doc->docLoader()->setCachePolicy(d->m_cachePolicy);

    d->m_workingURL = QUrl();

    d->m_cacheId = KHTMLPageCache::self()->createCacheEntry();

    // When the first data arrives, the metadata has just been made available
    d->m_httpHeaders = d->m_job->queryMetaData("HTTP-Headers");
    QDateTime cacheCreationDate =  QDateTime::fromTime_t(d->m_job->queryMetaData("cache-creation-date").toLong());
    d->m_doc->docLoader()->setCacheCreationDate(cacheCreationDate);

    d->m_pageServices = d->m_job->queryMetaData("PageServices");
    d->m_pageReferrer = d->m_job->queryMetaData("referrer");
    d->m_ssl_in_use = (d->m_job->queryMetaData("ssl_in_use") == "TRUE");

    {
    KHTMLPart *p = parentPart();
    if (p && p->d->m_ssl_in_use != d->m_ssl_in_use) {
        while (p->parentPart()) p = p->parentPart();

        p->setPageSecurity( NotCrypted );
    }
    }

    setPageSecurity( d->m_ssl_in_use ? Encrypted : NotCrypted );

    // Shouldn't all of this be done only if ssl_in_use == true ? (DF)
    d->m_ssl_parent_ip = d->m_job->queryMetaData("ssl_parent_ip");
    d->m_ssl_parent_cert = d->m_job->queryMetaData("ssl_parent_cert");
    d->m_ssl_peer_chain = d->m_job->queryMetaData("ssl_peer_chain");
    d->m_ssl_peer_ip = d->m_job->queryMetaData("ssl_peer_ip");
    d->m_ssl_cipher = d->m_job->queryMetaData("ssl_cipher");
    d->m_ssl_protocol_version = d->m_job->queryMetaData("ssl_protocol_version");
    d->m_ssl_cipher_used_bits = d->m_job->queryMetaData("ssl_cipher_used_bits");
    d->m_ssl_cipher_bits = d->m_job->queryMetaData("ssl_cipher_bits");
    d->m_ssl_cert_errors = d->m_job->queryMetaData("ssl_cert_errors");

    // Check for charset meta-data
    QString qData = d->m_job->queryMetaData("charset");
    if ( !qData.isEmpty() && !d->m_haveEncoding ) // only use information if the user didn't override the settings
       d->m_encoding = qData;


    // Support for http-refresh
    qData = d->m_job->queryMetaData("http-refresh");
    if( !qData.isEmpty())
      d->m_doc->processHttpEquiv("refresh", qData);

    // DISABLED: Support Content-Location per section 14.14 of RFC 2616.
    // See BR# 51185,BR# 82747
    /*
    QString baseURL = d->m_job->queryMetaData ("content-location");
    if (!baseURL.isEmpty())
      d->m_doc->setBaseURL(QUrl( d->m_doc->completeURL(baseURL) ));
    */

    // Support for Content-Language
    QString language = d->m_job->queryMetaData("content-language");
    if (!language.isEmpty())
      d->m_doc->setContentLanguage(language);

    if ( !url().isLocalFile() )
    {
      // Support for http last-modified
      d->m_lastModified = d->m_job->queryMetaData("modified");
    }
    else
      d->m_lastModified.clear(); // done on-demand by lastModified()
  }

  KHTMLPageCache::self()->addData(d->m_cacheId, data);
  write( data.data(), data.size() );
}

void KHTMLPart::slotRestoreData(const QByteArray &data )
{
  // The first data ?
  if ( !d->m_workingURL.isEmpty() )
  {
     long saveCacheId = d->m_cacheId;
     QString savePageReferrer = d->m_pageReferrer;
     QString saveEncoding     = d->m_encoding;
     begin( d->m_workingURL, arguments().xOffset(), arguments().yOffset() );
     d->m_encoding     = saveEncoding;
     d->m_pageReferrer = savePageReferrer;
     d->m_cacheId = saveCacheId;
     d->m_workingURL = QUrl();
  }

  //qDebug() << data.size();
  write( data.data(), data.size() );

  if (data.size() == 0)
  {
      //qDebug() << "<<end of data>>";
     // End of data.
    if (d->m_doc && d->m_doc->parsing())
        end(); //will emit completed()
  }
}

void KHTMLPart::showError( KJob* job )
{
  // qDebug() << "d->m_bParsing=" << (d->m_doc && d->m_doc->parsing()) << " d->m_bComplete=" << d->m_bComplete
  //              << " d->m_bCleared=" << d->m_bCleared;

  if (job->error() == KIO::ERR_NO_CONTENT)
        return;

  if ( (d->m_doc && d->m_doc->parsing()) || d->m_workingURL.isEmpty() ) // if we got any data already
    job->uiDelegate()->showErrorMessage();
  else
  {
    htmlError( job->error(), job->errorText(), d->m_workingURL );
  }
}

// This is a protected method, placed here because of it's relevance to showError
void KHTMLPart::htmlError( int errorCode, const QString& text, const QUrl& reqUrl )
{
  // qDebug() << "errorCode" << errorCode << "text" << text;
  // make sure we're not executing any embedded JS
  bool bJSFO = d->m_bJScriptForce;
  bool bJSOO = d->m_bJScriptOverride;
  d->m_bJScriptForce = false;
  d->m_bJScriptOverride = true;
  begin();

  QString errorName, techName, description;
  QStringList causes, solutions;

  QByteArray raw = KIO::rawErrorDetail( errorCode, text, &reqUrl );
  QDataStream stream(raw);

  stream >> errorName >> techName >> description >> causes >> solutions;

  QString url, protocol, datetime;

  // This is somewhat confusing, but we have to escape the externally-
  // controlled URL twice: once for i18n, and once for HTML.
  url = Qt::escape( Qt::escape( reqUrl.toDisplayString() ) );
  protocol = reqUrl.scheme();
  datetime = QDateTime::currentDateTime().toString(Qt::DefaultLocaleLongDate);

  QString filename( QStandardPaths::locate(QStandardPaths::GenericDataLocation, "khtml/error.html" ) );
  QFile file( filename );
  bool isOpened = file.open( QIODevice::ReadOnly );
  if ( !isOpened )
    qWarning() << "Could not open error html template:" << filename;

  QString html = QString( QLatin1String( file.readAll() ) );

  html.replace( QLatin1String( "TITLE" ), i18n( "Error: %1 - %2", errorName, url ) );
  html.replace( QLatin1String( "DIRECTION" ), QApplication::isRightToLeft() ? "rtl" : "ltr" );
  html.replace( QLatin1String( "ICON_PATH" ), KIconLoader::global()->iconPath( "dialog-warning", -KIconLoader::SizeHuge ) );

  QString doc = QLatin1String( "<h1>" );
  doc += i18n( "The requested operation could not be completed" );
  doc += QLatin1String( "</h1><h2>" );
  doc += errorName;
  doc += QLatin1String( "</h2>" );
  if ( !techName.isNull() ) {
    doc += QLatin1String( "<h2>" );
    doc += i18n( "Technical Reason: " );
    doc += techName;
    doc += QLatin1String( "</h2>" );
  }
  doc += QLatin1String( "<br clear=\"all\">" );
  doc += QLatin1String( "<h3>" );
  doc += i18n( "Details of the Request:" );
  doc += QLatin1String( "</h3><ul><li>" );
  doc += i18n( "URL: %1" ,  url );
  doc += QLatin1String( "</li><li>" );
  if ( !protocol.isNull() ) {
    doc += i18n( "Protocol: %1", protocol );
    doc += QLatin1String( "</li><li>" );
  }
  doc += i18n( "Date and Time: %1" ,  datetime );
  doc += QLatin1String( "</li><li>" );
  doc += i18n( "Additional Information: %1" ,  text );
  doc += QLatin1String( "</li></ul><h3>" );
  doc += i18n( "Description:" );
  doc += QLatin1String( "</h3><p>" );
  doc += description;
  doc += QLatin1String( "</p>" );
  if ( causes.count() ) {
    doc += QLatin1String( "<h3>" );
    doc += i18n( "Possible Causes:" );
    doc += QLatin1String( "</h3><ul><li>" );
    doc += causes.join( "</li><li>" );
    doc += QLatin1String( "</li></ul>" );
  }
  if ( solutions.count() ) {
    doc += QLatin1String( "<h3>" );
    doc += i18n( "Possible Solutions:" );
    doc += QLatin1String( "</h3><ul><li>" );
    doc += solutions.join( "</li><li>" );
    doc += QLatin1String( "</li></ul>" );
  }

  html.replace( QLatin1String("TEXT"), doc );

  write( html );
  end();

  d->m_bJScriptForce = bJSFO;
  d->m_bJScriptOverride = bJSOO;

  // make the working url the current url, so that reload works and
  // emit the progress signals to advance one step in the history
  // (so that 'back' works)
  setUrl(reqUrl); // same as d->m_workingURL
  d->m_workingURL = QUrl();
  emit started( 0 );
  emit completed();
}

void KHTMLPart::slotFinished( KJob * job )
{
  d->m_job = 0L;
  d->m_jobspeed = 0L;

  if (job->error())
  {
    KHTMLPageCache::self()->cancelEntry(d->m_cacheId);

    // The following catches errors that occur as a result of HTTP
    // to FTP redirections where the FTP URL is a directory. Since
    // KIO cannot change a redirection request from GET to LISTDIR,
    // we have to take care of it here once we know for sure it is
    // a directory...
    if (job->error() == KIO::ERR_IS_DIRECTORY)
    {
      emit canceled( job->errorString() );
      emit d->m_extension->openUrlRequest( d->m_workingURL );
    }
    else
    {
      emit canceled( job->errorString() );
      // TODO: what else ?
      checkCompleted();
      showError( job );
    }

    return;
  }
  KIO::TransferJob *tjob = ::qobject_cast<KIO::TransferJob*>(job);
  if (tjob && tjob->isErrorPage()) {
    HTMLPartContainerElementImpl *elt = d->m_frame ?
                                           d->m_frame->m_partContainerElement.data() : 0;

    if (!elt)
      return;

    elt->partLoadingErrorNotify();
    checkCompleted();
    if (d->m_bComplete) return;
  }

  //qDebug() << "slotFinished";

  KHTMLPageCache::self()->endData(d->m_cacheId);

  if ( d->m_doc && d->m_doc->docLoader()->expireDate().isValid() && url().scheme().startsWith("http"))
      KIO::http_update_cache(url(), false, d->m_doc->docLoader()->expireDate());

  d->m_workingURL = QUrl();

  if ( d->m_doc && d->m_doc->parsing())
    end(); //will emit completed()
}

MimeType KHTMLPartPrivate::classifyMimeType(const QString& mimeStr)
{
  // See HTML5's "5.5.1 Navigating across documents" section.
  if (mimeStr == "application/xhtml+xml")
      return MimeXHTML;
  if (mimeStr == "image/svg+xml")
      return MimeSVG;
  if (mimeStr == "text/html" || mimeStr.isEmpty())
      return MimeHTML;

  QMimeDatabase db;
  QMimeType mime = db.mimeTypeForName(mimeStr);
  if (mime.inherits("text/xml") || mimeStr.endsWith("+xml"))
      return MimeXML;

  if (mime.inherits("text/plain"))
      return MimeText;

  if (khtmlImLoad::ImageManager::loaderDatabase()->supportedMimeTypes().contains(mimeStr))
      return MimeImage;

  // Sometimes our subclasses like to handle custom mimetypes. In that case,
  // we want to handle them as HTML. We do that in the following cases:
  // 1) We're at top-level, so we were forced to open something
  // 2) We're an object --- this again means we were forced to open something,
  //    as an iframe-generating-an-embed case would have us as an iframe
  if (!q->parentPart() || (m_frame && m_frame->m_type == khtml::ChildFrame::Object))
      return MimeHTML;

  return MimeOther;
}

void KHTMLPart::begin( const QUrl &url, int xOffset, int yOffset )
{
  if ( d->m_view->underMouse() )
    QToolTip::hideText();  // in case a previous tooltip is still shown

  // No need to show this for a new page until an error is triggered
  if (!parentPart()) {
    removeJSErrorExtension();
    setSuppressedPopupIndicator( false );
    d->m_openableSuppressedPopups = 0;
    foreach ( KHTMLPart* part, d->m_suppressedPopupOriginParts ) {
      if (part) {
        KJS::Window *w = KJS::Window::retrieveWindow( part );
        if (w)
          w->forgetSuppressedWindows();
      }
    }
  }

  d->m_bCleared = false;
  d->m_cacheId = 0;
  d->m_bComplete = false;
  d->m_bLoadEventEmitted = false;
  clear();
  d->m_bCleared = false;

  if(url.isValid()) {
      QString urlString = url.toString();
      KHTMLGlobal::vLinks()->insert( urlString );
      QString urlString2 = url.toDisplayString();
      if ( urlString != urlString2 ) {
          KHTMLGlobal::vLinks()->insert( urlString2 );
      }
  }

  // ###
  //stopParser();

  KParts::OpenUrlArguments args = arguments();
  args.setXOffset(xOffset);
  args.setYOffset(yOffset);
  setArguments(args);

  d->m_pageReferrer.clear();

  d->m_referrer = url.scheme().startsWith("http") ? url.toString() : "";

  setUrl(url);

  // Note: by now, any special mimetype besides plaintext would have been
  // handled specially inside openURL, so we handle their cases the same
  // as HTML.
  MimeType type = d->classifyMimeType(args.mimeType());
  switch (type) {
  case MimeSVG:
      d->m_doc = DOMImplementationImpl::createSVGDocument( d->m_view );
      break;
  case MimeXML: // any XML derivative, except XHTML or SVG
      // ### not sure if XHTML documents served as text/xml should use DocumentImpl or HTMLDocumentImpl
     d->m_doc = DOMImplementationImpl::createXMLDocument( d->m_view );
     break;
  case MimeText:
     d->m_doc = new HTMLTextDocumentImpl( d->m_view );
     break;
  case MimeXHTML:
  case MimeHTML:
  default:
      d->m_doc = DOMImplementationImpl::createHTMLDocument( d->m_view );
      // HTML or XHTML? (#86446)
      static_cast<HTMLDocumentImpl *>(d->m_doc)->setHTMLRequested( type != MimeXHTML );
  }

  d->m_doc->ref();
  d->m_doc->setURL( url.toString() );
  d->m_doc->open( );
  if (!d->m_doc->attached())
    d->m_doc->attach( );
  d->m_doc->setBaseURL( QUrl() );
  d->m_doc->docLoader()->setShowAnimations( KHTMLGlobal::defaultHTMLSettings()->showAnimations() );
  emit docCreated();

  d->m_paUseStylesheet->setItems(QStringList());
  d->m_paUseStylesheet->setEnabled( false );

  setAutoloadImages( KHTMLGlobal::defaultHTMLSettings()->autoLoadImages() );
  QString userStyleSheet = KHTMLGlobal::defaultHTMLSettings()->userStyleSheet();
  if ( !userStyleSheet.isEmpty() )
    setUserStyleSheet( QUrl( userStyleSheet ) );

  d->m_doc->setRestoreState(d->m_extension->browserArguments().docState);
  connect(d->m_doc,SIGNAL(finishedParsing()),this,SLOT(slotFinishedParsing()));

  emit d->m_extension->enableAction( "print", true );

  d->m_doc->setParsing(true);
}

void KHTMLPart::write( const char *data, int len )
{
  if ( !d->m_decoder )
    d->m_decoder = createDecoder();

  if ( len == -1 )
    len = strlen( data );

  if ( len == 0 )
    return;

  QString decoded=d->m_decoder->decodeWithBuffering(data,len);

  if(decoded.isEmpty())
      return;

  if(d->m_bFirstData)
      onFirstData();

  khtml::Tokenizer* t = d->m_doc->tokenizer();
  if(t)
    t->write( decoded, true );
}

// ### KDE5: remove
void KHTMLPart::setAlwaysHonourDoctype( bool b )
{
    d->m_bStrictModeQuirk = !b;
}

void KHTMLPart::write( const QString &str )
{
    if ( str.isNull() )
        return;

    if(d->m_bFirstData) {
            // determine the parse mode
        if (d->m_bStrictModeQuirk) {
            d->m_doc->setParseMode( DocumentImpl::Strict );
            d->m_bFirstData = false;
        } else {
            onFirstData();
        }
    }
    khtml::Tokenizer* t = d->m_doc->tokenizer();
    if(t)
        t->write( str, true );
}

void KHTMLPart::end()
{
    if (d->m_doc) {
        if (d->m_decoder)
        {
            QString decoded=d->m_decoder->flush();
            if (d->m_bFirstData)
                onFirstData();
            if (!decoded.isEmpty())
                write(decoded);
        }
        d->m_doc->finishParsing();
    }
}

void KHTMLPart::onFirstData()
{
      assert( d->m_bFirstData );

      // determine the parse mode
      d->m_doc->determineParseMode();
      d->m_bFirstData = false;

      // ### this is still quite hacky, but should work a lot better than the old solution
      // Note: decoder may be null if only write(QString) is used.
      if (d->m_decoder && d->m_decoder->visuallyOrdered())
          d->m_doc->setVisuallyOrdered();
      // ensure part and view shares zoom-level before styling
      updateZoomFactor();
      d->m_doc->recalcStyle( NodeImpl::Force );
}

bool KHTMLPart::doOpenStream( const QString& mimeType )
{
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForName(mimeType);
    if ( mime.inherits( "text/html" ) || mime.inherits( "text/xml" ) )
    {
        begin( url() );
        return true;
    }
    return false;
}

bool KHTMLPart::doWriteStream( const QByteArray& data )
{
    write( data.data(), data.size() );
    return true;
}

bool KHTMLPart::doCloseStream()
{
    end();
    return true;
}


void KHTMLPart::paint(QPainter *p, const QRect &rc, int yOff, bool *more)
{
    if (!d->m_view) return;
    d->m_view->paint(p, rc, yOff, more);
}

void KHTMLPart::stopAnimations()
{
  if ( d->m_doc )
    d->m_doc->docLoader()->setShowAnimations( KHTMLSettings::KAnimationDisabled );

  ConstFrameIt it = d->m_frames.constBegin();
  const ConstFrameIt end = d->m_frames.constEnd();
  for (; it != end; ++it ) {
    if ( KHTMLPart* p = qobject_cast<KHTMLPart*>((*it)->m_part.data()) )
      p->stopAnimations();
  }
}

void KHTMLPart::resetFromScript()
{
    closeUrl();
    d->m_bComplete = false;
    d->m_bLoadEventEmitted = false;
    disconnect(d->m_doc,SIGNAL(finishedParsing()),this,SLOT(slotFinishedParsing()));
    connect(d->m_doc,SIGNAL(finishedParsing()),this,SLOT(slotFinishedParsing()));
    d->m_doc->setParsing(true);

    emit started( 0L );
}

void KHTMLPart::slotFinishedParsing()
{
  d->m_doc->setParsing(false);
  d->m_doc->dispatchHTMLEvent(EventImpl::KHTML_CONTENTLOADED_EVENT, true, false);
  checkEmitLoadEvent();
  disconnect(d->m_doc,SIGNAL(finishedParsing()),this,SLOT(slotFinishedParsing()));

  if (!d->m_view)
    return; // We are probably being destructed.

  checkCompleted();
}

void KHTMLPart::slotLoaderRequestStarted( khtml::DocLoader* dl, khtml::CachedObject *obj )
{
  if ( obj && obj->type() == khtml::CachedObject::Image && d->m_doc && d->m_doc->docLoader() == dl ) {
    KHTMLPart* p = this;
    while ( p ) {
      KHTMLPart* const op = p;
      ++(p->d->m_totalObjectCount);
      p = p->parentPart();
      if ( !p && op->d->m_loadedObjects <= op->d->m_totalObjectCount
        && !op->d->m_progressUpdateTimer.isActive()) {
        op->d->m_progressUpdateTimer.setSingleShot( true );
        op->d->m_progressUpdateTimer.start( 200 );
      }
    }
  }
}

static bool isAncestorOrSamePart(KHTMLPart* p1, KHTMLPart* p2)
{
    KHTMLPart* p = p2;
    do {
        if (p == p1)
            return true;
    } while ((p = p->parentPart()));
    return false;
}

void KHTMLPart::slotLoaderRequestDone( khtml::DocLoader* dl, khtml::CachedObject *obj )
{
  if ( obj && obj->type() == khtml::CachedObject::Image && d->m_doc && d->m_doc->docLoader() == dl ) {
    KHTMLPart* p = this;
    while ( p ) {
      KHTMLPart* const op = p;
      ++(p->d->m_loadedObjects);
      p = p->parentPart();
      if ( !p && op->d->m_loadedObjects <= op->d->m_totalObjectCount && op->d->m_jobPercent <= 100
        && !op->d->m_progressUpdateTimer.isActive()) {
        op->d->m_progressUpdateTimer.setSingleShot( true );
        op->d->m_progressUpdateTimer.start( 200 );
      }
    }
  }
  /// if we have no document, or the object is not a request of one of our children,
  //  then our loading state can't possibly be affected : don't waste time checking for completion.
  if (!d->m_doc || !dl->doc()->part() || !isAncestorOrSamePart(this, dl->doc()->part()))
      return;
  checkCompleted();
}

void KHTMLPart::slotProgressUpdate()
{
  int percent;
  if ( d->m_loadedObjects < d->m_totalObjectCount )
    percent = d->m_jobPercent / 4 + ( d->m_loadedObjects*300 ) / ( 4*d->m_totalObjectCount );
  else
    percent = d->m_jobPercent;

  if( d->m_bComplete )
    percent = 100;

  if (d->m_statusMessagesEnabled) {
    if( d->m_bComplete )
      emit d->m_extension->infoMessage( i18n( "Page loaded." ));
    else if ( d->m_loadedObjects < d->m_totalObjectCount && percent >= 75 )
      emit d->m_extension->infoMessage( i18np( "%1 Image of %2 loaded.", "%1 Images of %2 loaded.", d->m_loadedObjects, d->m_totalObjectCount) );
  }

  emit d->m_extension->loadingProgress( percent );
}

void KHTMLPart::slotJobSpeed( KJob* /*job*/, unsigned long speed )
{
  d->m_jobspeed = speed;
  if (!parentPart())
    setStatusBarText(jsStatusBarText(), BarOverrideText);
}

void KHTMLPart::slotJobPercent( KJob* /*job*/, unsigned long percent )
{
  d->m_jobPercent = percent;

  if ( !parentPart() ) {
    d->m_progressUpdateTimer.setSingleShot( true );
    d->m_progressUpdateTimer.start( 0 );
  }
}

void KHTMLPart::slotJobDone( KJob* /*job*/ )
{
  d->m_jobPercent = 100;

  if ( !parentPart() ) {
    d->m_progressUpdateTimer.setSingleShot( true );
    d->m_progressUpdateTimer.start( 0 );
  }
}

void KHTMLPart::slotUserSheetStatDone( KJob *_job )
{
  using namespace KIO;

  if ( _job->error() ) {
    showError( _job );
    return;
  }

  const UDSEntry entry = dynamic_cast<KIO::StatJob *>( _job )->statResult();
  const QDateTime lastModified = QDateTime::fromTime_t(entry.numberValue(KIO::UDSEntry::UDS_MODIFICATION_TIME, -1));

  // If the filesystem supports modification times, only reload the
  // user-defined stylesheet if necessary - otherwise always reload.
  if (lastModified.isValid()) {
    if (d->m_userStyleSheetLastModified >= lastModified) {
      return;
    }
    d->m_userStyleSheetLastModified = lastModified;
  }

  setUserStyleSheet( QUrl( settings()->userStyleSheet() ) );
}

bool KHTMLPartPrivate::isFullyLoaded(bool* pendingRedirections) const
{
  *pendingRedirections = false;

  // Any frame that hasn't completed yet ?
  ConstFrameIt it = m_frames.constBegin();
  const ConstFrameIt end = m_frames.constEnd();
  for (; it != end; ++it ) {
    if ( !(*it)->m_bCompleted || (*it)->m_run )
    {
      //qDebug() << this << " is waiting for " << (*it)->m_part;
      return false;
    }
    // Check for frames with pending redirections
    if ( (*it)->m_bPendingRedirection )
      *pendingRedirections = true;
  }

  // Any object that hasn't completed yet ?
  {
    ConstFrameIt oi = m_objects.constBegin();
    const ConstFrameIt oiEnd = m_objects.constEnd();

    for (; oi != oiEnd; ++oi )
      if ( !(*oi)->m_bCompleted )
        return false;
  }

  // Are we still parsing
  if ( m_doc && m_doc->parsing() )
    return false;

  // Still waiting for images/scripts from the loader ?
  int requests = 0;
  if ( m_doc && m_doc->docLoader() )
    requests = khtml::Cache::loader()->numRequests( m_doc->docLoader() );

  if ( requests > 0 )
  {
    //qDebug() << "still waiting for images/scripts from the loader - requests:" << requests;
    return false;
  }

  return true;
}

void KHTMLPart::checkCompleted()
{
//   qDebug() << this;
//   qDebug() << "   parsing: " << (d->m_doc && d->m_doc->parsing());
//   qDebug() << "   complete: " << d->m_bComplete;

  // restore the cursor position
  if (d->m_doc && !d->m_doc->parsing() && !d->m_focusNodeRestored)
  {
      if (d->m_focusNodeNumber >= 0)
          d->m_doc->setFocusNode(d->m_doc->nodeWithAbsIndex(d->m_focusNodeNumber));

      d->m_focusNodeRestored = true;
  }

  bool fullyLoaded, pendingChildRedirections;
  fullyLoaded = d->isFullyLoaded(&pendingChildRedirections);

  // Are we still loading, or already have done the relevant work?
  if (!fullyLoaded || d->m_bComplete)
    return;

  // OK, completed.
  // Now do what should be done when we are really completed.
  d->m_bComplete = true;
  d->m_cachePolicy = KProtocolManager::cacheControl(); // reset cache policy
  d->m_totalObjectCount = 0;
  d->m_loadedObjects = 0;

  KHTMLPart* p = this;
  while ( p ) {
    KHTMLPart* op = p;
    p = p->parentPart();
    if ( !p && !op->d->m_progressUpdateTimer.isActive()) {
      op->d->m_progressUpdateTimer.setSingleShot( true );
      op->d->m_progressUpdateTimer.start( 0 );
    }
  }

  checkEmitLoadEvent(); // if we didn't do it before

  bool pendingAction = false;

  if ( !d->m_redirectURL.isEmpty() )
  {
    // DA: Do not start redirection for frames here! That action is
    // deferred until the parent emits a completed signal.
    if ( parentPart() == 0 ) {
      //qDebug() << this << " starting redirection timer";
      d->m_redirectionTimer.setSingleShot( true );
      d->m_redirectionTimer.start( qMax(0, 1000 * d->m_delayRedirect) );
    } else {
      //qDebug() << this << " not toplevel -> not starting redirection timer. Waiting for slotParentCompleted.";
    }

    pendingAction = true;
  }
  else if ( pendingChildRedirections )
  {
    pendingAction = true;
  }

  // the view will emit completed on our behalf,
  // either now or at next repaint if one is pending

  //qDebug() << this << " asks the view to emit completed. pendingAction=" << pendingAction;
  d->m_view->complete( pendingAction );

  // find the alternate stylesheets
  QStringList sheets;
  if (d->m_doc)
     sheets = d->m_doc->availableStyleSheets();
  sheets.prepend( i18n( "Automatic Detection" ) );
  d->m_paUseStylesheet->setItems( sheets );

  d->m_paUseStylesheet->setEnabled( sheets.count() > 2);
  if (sheets.count() > 2)
  {
    d->m_paUseStylesheet->setCurrentItem(qMax(sheets.indexOf(d->m_sheetUsed), 0));
    slotUseStylesheet();
  }

  setJSDefaultStatusBarText(QString());

#ifdef SPEED_DEBUG
  if (!parentPart())
      qDebug() << "DONE:" <<d->m_parsetime.elapsed();
#endif
}

void KHTMLPart::checkEmitLoadEvent()
{
  bool fullyLoaded, pendingChildRedirections;
  fullyLoaded = d->isFullyLoaded(&pendingChildRedirections);

  // ### might want to wait on pendingChildRedirections here, too
  if ( d->m_bLoadEventEmitted || !d->m_doc || !fullyLoaded ) return;

  d->m_bLoadEventEmitted = true;
  if (d->m_doc)
    d->m_doc->close();
}

const KHTMLSettings *KHTMLPart::settings() const
{
  return d->m_settings;
}

#ifndef KDE_NO_COMPAT // KDE5: remove this ifndef, keep the method (renamed to baseUrl)
QUrl KHTMLPart::baseURL() const
{
  if ( !d->m_doc ) return QUrl();

  return d->m_doc->baseURL();
}
#endif

QUrl KHTMLPart::completeURL( const QString &url )
{
  if ( !d->m_doc ) return QUrl( url );

#if 0
  if (d->m_decoder)
    return QUrl(d->m_doc->completeURL(url), d->m_decoder->codec()->mibEnum());
#endif

  return QUrl( d->m_doc->completeURL( url ) );
}

QString KHTMLPartPrivate::codeForJavaScriptURL(const QString &u)
{
    return QUrl::fromPercentEncoding( u.right( u.length() - 11 ).toUtf8() );
}

void KHTMLPartPrivate::executeJavascriptURL(const QString &u)
{
    QString script = codeForJavaScriptURL(u);
    // qDebug() << "script=" << script;
    QVariant res = q->executeScript( DOM::Node(), script );
    if ( res.type() == QVariant::String ) {
      q->begin( q->url() );
      q->setAlwaysHonourDoctype(); // Disable public API compat; it messes with doctype
      q->write( res.toString() );
      q->end();
    }
    emit q->completed();
}

bool KHTMLPartPrivate::isJavaScriptURL(const QString& url)
{
    return url.indexOf( QLatin1String( "javascript:" ), 0, Qt::CaseInsensitive ) == 0;
}

// Called by ecma/kjs_window in case of redirections from Javascript,
// and by xml/dom_docimpl.cpp in case of http-equiv meta refresh.
void KHTMLPart::scheduleRedirection( int delay, const QString &url, bool doLockHistory )
{
  // qDebug() << "delay=" << delay << " url=" << url << " from=" << this->url() << "parent=" << parentPart();
  // qDebug() << "current redirectURL=" << d->m_redirectURL << " with delay " << d->m_delayRedirect;

  // In case of JS redirections, some, such as jump to anchors, and javascript:
  // evaluation should actually be handled immediately, and not waiting until
  // the end of the script. (Besides, we don't want to abort the tokenizer for those)
  if ( delay == -1 && d->isInPageURL(url) ) {
    d->executeInPageURL(url, doLockHistory);
    return;
  }

  if( delay < 24*60*60 &&
      ( d->m_redirectURL.isEmpty() || delay <= d->m_delayRedirect) ) {
    d->m_delayRedirect = delay;
    d->m_redirectURL = url;
    d->m_redirectLockHistory = doLockHistory;
    // qDebug() << " d->m_bComplete=" << d->m_bComplete;

    if ( d->m_bComplete ) {
      d->m_redirectionTimer.stop();
      d->m_redirectionTimer.setSingleShot( true );
      d->m_redirectionTimer.start( qMax(0, 1000 * d->m_delayRedirect) );
    }
  }
}

void KHTMLPartPrivate::clearRedirection()
{
  m_delayRedirect = 0;
  m_redirectURL.clear();
  m_redirectionTimer.stop();
}

void KHTMLPart::slotRedirect()
{
  // qDebug() << this;
  QString u = d->m_redirectURL;
  QUrl url( u );
  d->clearRedirection();

  if ( d->isInPageURL(u) )
  {
    d->executeInPageURL(u, d->m_redirectLockHistory);
    return;
  }

  KParts::OpenUrlArguments args;
  QUrl cUrl( this->url() );

  // handle windows opened by JS
  if ( openedByJS() && d->m_opener )
      cUrl = d->m_opener->url();

  if (!KUrlAuthorized::authorizeUrlAction("redirect", cUrl, url))
  {
    qWarning() << "KHTMLPart::scheduleRedirection: Redirection from " << cUrl << " to " << url << " REJECTED!";
    emit completed();
    return;
  }

  if (areUrlsForSamePage(url, this->url())) {
    args.metaData().insert("referrer", d->m_pageReferrer);
  }

  // For javascript and META-tag based redirections:
  //   - We don't take cross-domain-ness in consideration if we are the
  //   toplevel frame because the new URL may be in a different domain as the current URL
  //   but that's ok.
  //   - If we are not the toplevel frame then we check against the toplevelURL()
  if (parentPart())
      args.metaData().insert("cross-domain", toplevelURL().toString());

  KParts::BrowserArguments browserArgs;
  browserArgs.setLockHistory( d->m_redirectLockHistory );
  // _self: make sure we don't use any <base target=>'s

  if ( !urlSelected( u, 0, 0, "_self", args, browserArgs ) ) {
    // urlSelected didn't open a url, so emit completed ourselves
    emit completed();
  }
}

void KHTMLPart::slotRedirection(KIO::Job*, const QUrl& url)
{
  // the slave told us that we got redirected
  //qDebug() << "redirection by KIO to" << url;
  emit d->m_extension->setLocationBarUrl( url.toDisplayString() );
  d->m_workingURL = url;
}

bool KHTMLPart::setEncoding( const QString &name, bool override )
{
    d->m_encoding = name;
    d->m_haveEncoding = override;

    if( !url().isEmpty() ) {
        // reload document
        closeUrl();
        QUrl oldUrl = url();
        setUrl(QUrl());
        d->m_restored = true;
        openUrl(oldUrl);
        d->m_restored = false;
    }

    return true;
}

QString KHTMLPart::encoding() const
{
    if(d->m_haveEncoding && !d->m_encoding.isEmpty())
        return d->m_encoding;

    if(d->m_decoder && d->m_decoder->encoding())
        return QString(d->m_decoder->encoding());

    return defaultEncoding();
}

QString KHTMLPart::defaultEncoding() const
{
  QString encoding = settings()->encoding();
  if ( !encoding.isEmpty() )
    return encoding;
  // HTTP requires the default encoding to be latin1, when neither
  // the user nor the page requested a particular encoding.
  if ( url().scheme().startsWith( "http" ) )
    return "iso-8859-1";
  else
    return QTextCodec::codecForLocale()->name().toLower();
}

void KHTMLPart::setUserStyleSheet(const QUrl &url)
{
  if ( d->m_doc && d->m_doc->docLoader() )
    (void) new khtml::PartStyleSheetLoader(this, url.toString(), d->m_doc->docLoader());
}

void KHTMLPart::setUserStyleSheet(const QString &styleSheet)
{
  if ( d->m_doc )
    d->m_doc->setUserStyleSheet( styleSheet );
}

bool KHTMLPart::gotoAnchor( const QString &name )
{
  if (!d->m_doc)
    return false;

  HTMLCollectionImpl *anchors =
      new HTMLCollectionImpl( d->m_doc, HTMLCollectionImpl::DOC_ANCHORS);
  anchors->ref();
  NodeImpl *n = anchors->namedItem(name);
  anchors->deref();

  if(!n) {
      n = d->m_doc->getElementById( name );
  }

  d->m_doc->setCSSTarget(n); // Setting to null will clear the current target.

  // Implement the rule that "" and "top" both mean top of page as in other browsers.
  bool quirkyName = !n && !d->m_doc->inStrictMode() && (name.isEmpty() || name.toLower() == "top");

  if (quirkyName) {
      d->m_view->setContentsPos( d->m_view->contentsX(), 0);
      return true;
  } else if (!n) {
      // qDebug() << name << "not found";
      return false;
  }

  int x = 0, y = 0;
  int gox, dummy;
  HTMLElementImpl *a = static_cast<HTMLElementImpl *>(n);

  a->getUpperLeftCorner(x, y);
  if (x <= d->m_view->contentsX())
    gox = x - 10;
  else {
    gox = d->m_view->contentsX();
    if ( x + 10 > d->m_view->contentsX()+d->m_view->visibleWidth()) {
      a->getLowerRightCorner(x, dummy);
      gox = x - d->m_view->visibleWidth() + 10;
    }
  }

  d->m_view->setContentsPos(gox, y);

  return true;
}

bool KHTMLPart::nextAnchor()
{
  if (!d->m_doc)
    return false;
  d->m_view->focusNextPrevNode ( true );

  return true;
}

bool KHTMLPart::prevAnchor()
{
  if (!d->m_doc)
    return false;
  d->m_view->focusNextPrevNode ( false );

  return true;
}

void KHTMLPart::setStandardFont( const QString &name )
{
    d->m_settings->setStdFontName(name);
}

void KHTMLPart::setFixedFont( const QString &name )
{
    d->m_settings->setFixedFontName(name);
}

void KHTMLPart::setURLCursor( const QCursor &c )
{
  d->m_linkCursor = c;
}

QCursor KHTMLPart::urlCursor() const
{
  return d->m_linkCursor;
}

bool KHTMLPart::onlyLocalReferences() const
{
  return d->m_onlyLocalReferences;
}

void KHTMLPart::setOnlyLocalReferences(bool enable)
{
  d->m_onlyLocalReferences = enable;
}

bool KHTMLPart::forcePermitLocalImages() const
{
    return d->m_forcePermitLocalImages;
}

void KHTMLPart::setForcePermitLocalImages(bool enable)
{
    d->m_forcePermitLocalImages = enable;
}

void KHTMLPartPrivate::setFlagRecursively(
        bool KHTMLPartPrivate::*flag, bool value)
{
  // first set it on the current one
  this->*flag = value;

  // descend into child frames recursively
  {
    QList<khtml::ChildFrame*>::Iterator it = m_frames.begin();
    const QList<khtml::ChildFrame*>::Iterator itEnd = m_frames.end();
    for (; it != itEnd; ++it) {
      KHTMLPart* const part = qobject_cast<KHTMLPart *>( (*it)->m_part.data() );
      if (part)
        part->d->setFlagRecursively(flag, value);
    }/*next it*/
  }
  // do the same again for objects
  {
    QList<khtml::ChildFrame*>::Iterator it = m_objects.begin();
    const QList<khtml::ChildFrame*>::Iterator itEnd = m_objects.end();
    for (; it != itEnd; ++it) {
      KHTMLPart* const part = qobject_cast<KHTMLPart *>( (*it)->m_part.data() );
      if (part)
        part->d->setFlagRecursively(flag, value);
    }/*next it*/
  }
}

void KHTMLPart::initCaret()
{
  // initialize caret if not used yet
  if (d->editor_context.m_selection.state() == Selection::NONE) {
    if (d->m_doc) {
      NodeImpl *node;
      if (d->m_doc->isHTMLDocument()) {
        HTMLDocumentImpl* htmlDoc = static_cast<HTMLDocumentImpl*>(d->m_doc);
        node = htmlDoc->body();
      } else
        node = d->m_doc;
      if (!node) return;
      d->editor_context.m_selection.moveTo(Position(node, 0));
      d->editor_context.m_selection.setNeedsLayout();
      d->editor_context.m_selection.needsCaretRepaint();
    }
  }
}

static void setCaretInvisibleIfNeeded(KHTMLPart *part)
{
  // On contenteditable nodes, don't hide the caret
  if (!khtml::KHTMLPartAccessor::caret(part).caretPos().node()->isContentEditable())
    part->setCaretVisible(false);
}

void KHTMLPart::setCaretMode(bool enable)
{
  // qDebug() << enable;
  if (isCaretMode() == enable) return;
  d->setFlagRecursively(&KHTMLPartPrivate::m_caretMode, enable);
  // FIXME: this won't work on frames as expected
  if (!isEditable()) {
    if (enable) {
      initCaret();
      setCaretVisible(true);
//       view()->ensureCaretVisible();
    } else {
      setCaretInvisibleIfNeeded(this);
    }
  }
}

bool KHTMLPart::isCaretMode() const
{
  return d->m_caretMode;
}

void KHTMLPart::setEditable(bool enable)
{
  if (isEditable() == enable) return;
  d->setFlagRecursively(&KHTMLPartPrivate::m_designMode, enable);
  // FIXME: this won't work on frames as expected
  if (!isCaretMode()) {
    if (enable) {
      initCaret();
      setCaretVisible(true);
//       view()->ensureCaretVisible();
    } else
      setCaretInvisibleIfNeeded(this);
  }
}

bool KHTMLPart::isEditable() const
{
  return d->m_designMode;
}

khtml::EditorContext *KHTMLPart::editorContext() const {
    return &d->editor_context;
}

void KHTMLPart::setCaretPosition(DOM::Node node, long offset, bool extendSelection)
{
  Q_UNUSED(node);
  Q_UNUSED(offset);
  Q_UNUSED(extendSelection);
#ifndef KHTML_NO_CARET
#if 0
  qDebug() << "node: " << node.handle() << " nodeName: "
               << node.nodeName().string() << " offset: " << offset
               << " extendSelection " << extendSelection;
  if (view()->moveCaretTo(node.handle(), offset, !extendSelection))
    emitSelectionChanged();
  view()->ensureCaretVisible();
#endif
#endif // KHTML_NO_CARET
}

KHTMLPart::CaretDisplayPolicy KHTMLPart::caretDisplayPolicyNonFocused() const
{
#if 0
#ifndef KHTML_NO_CARET
  return (CaretDisplayPolicy)view()->caretDisplayPolicyNonFocused();
#else // KHTML_NO_CARET
  return CaretInvisible;
#endif // KHTML_NO_CARET
#endif
  return CaretInvisible;
}

void KHTMLPart::setCaretDisplayPolicyNonFocused(CaretDisplayPolicy policy)
{
  Q_UNUSED(policy);
#if 0
#ifndef KHTML_NO_CARET
  view()->setCaretDisplayPolicyNonFocused(policy);
#endif // KHTML_NO_CARET
#endif
}

void KHTMLPart::setCaretVisible(bool show)
{
  if (show) {
    NodeImpl *caretNode = d->editor_context.m_selection.caretPos().node();
    if (isCaretMode() || (caretNode && caretNode->isContentEditable())) {
        invalidateSelection();
        enableFindAheadActions(false);
    }
  } else {

    if (d->editor_context.m_caretBlinkTimer >= 0)
        killTimer(d->editor_context.m_caretBlinkTimer);
    clearCaretRectIfNeeded();

  }
}

void KHTMLPart::findTextBegin()
{
  d->m_find.findTextBegin();
}

bool KHTMLPart::initFindNode( bool selection, bool reverse, bool fromCursor )
{
  return d->m_find.initFindNode(selection, reverse, fromCursor);
}

void KHTMLPart::slotFind()
{
  KParts::ReadOnlyPart *part = currentFrame();
  if (!part)
    return;
  if (!part->inherits("KHTMLPart") )
  {
      qCritical() << "part is a" << part->metaObject()->className() << ", can't do a search into it";
      return;
  }
  static_cast<KHTMLPart *>( part )->findText();
}

void KHTMLPart::slotFindNext()
{
  KParts::ReadOnlyPart *part = currentFrame();
  if (!part)
    return;
  if (!part->inherits("KHTMLPart") )
  {
      qCritical() << "part is a" << part->metaObject()->className() << ", can't do a search into it";
      return;
  }
  static_cast<KHTMLPart *>( part )->findTextNext();
}

void KHTMLPart::slotFindPrev()
{
  KParts::ReadOnlyPart *part = currentFrame();
  if (!part)
    return;
  if (!part->inherits("KHTMLPart") )
  {
      qCritical() << "part is a" << part->metaObject()->className() << ", can't do a search into it";
      return;
  }
  static_cast<KHTMLPart *>( part )->findTextNext( true ); // reverse
}

void KHTMLPart::slotFindDone()
{
  // ### remove me
}

void KHTMLPart::slotFindAheadText()
{
  KHTMLPart *part = qobject_cast<KHTMLPart*>(currentFrame());
  if (!part)
    return;
  part->findText();
  KHTMLFindBar* findBar = part->d->m_find.findBar();
  findBar->setOptions(findBar->options() & ~FindLinksOnly);
}

void KHTMLPart::slotFindAheadLink()
{
  KHTMLPart *part = qobject_cast<KHTMLPart*>(currentFrame());
  if (!part)
    return;
  part->findText();
  KHTMLFindBar* findBar = part->d->m_find.findBar();
  findBar->setOptions(findBar->options() | FindLinksOnly);
}

void KHTMLPart::enableFindAheadActions( bool )
{
  // ### remove me
}

void KHTMLPart::slotFindDialogDestroyed()
{
  // ### remove me
}

void KHTMLPart::findText()
{
  if (parentPart())
      return parentPart()->findText();
  d->m_find.activate();
}

void KHTMLPart::findText( const QString &str, long options, QWidget *parent, KFindDialog *findDialog )
{
  if (parentPart())
      return parentPart()->findText(str, options, parent, findDialog);
  d->m_find.createNewKFind(str, options, parent, findDialog );
}

// New method
bool KHTMLPart::findTextNext( bool reverse )
{
  if (parentPart())
      return parentPart()->findTextNext( reverse );
  return d->m_find.findTextNext( reverse );
}

bool KHTMLPart::pFindTextNextInThisFrame( bool reverse )
{
  return d->m_find.findTextNext( reverse );
}

QString KHTMLPart::selectedTextAsHTML() const
{
  const Selection &sel = d->editor_context.m_selection;
  if(!hasSelection()) {
    // qDebug() << "Selection is not valid. Returning empty selection";
    return QString();
  }
  if(sel.start().offset() < 0 || sel.end().offset() < 0) {
    // qDebug() << "invalid values for end/startOffset " << sel.start().offset() << " " << sel.end().offset();
    return QString();
  }
  DOM::Range r = selection();
  if(r.isNull() || r.isDetached())
    return QString();
  int exceptioncode = 0; //ignore the result
  return r.handle()->toHTML(exceptioncode).string();
}

QString KHTMLPart::selectedText() const
{
  bool hasNewLine = true;
  bool seenTDTag = false;
  QString text;
  const Selection &sel = d->editor_context.m_selection;
  DOM::Node n = sel.start().node();
  while(!n.isNull()) {
      if(n.nodeType() == DOM::Node::TEXT_NODE && n.handle()->renderer()) {
        DOM::DOMStringImpl *dstr = static_cast<DOM::TextImpl*>(n.handle())->renderString();
        QString str(dstr->s, dstr->l);
        if(!str.isEmpty()) {
          if(seenTDTag) {
            text += "  ";
            seenTDTag = false;
          }
          hasNewLine = false;
          if(n == sel.start().node() && n == sel.end().node()) {
            int s = khtml::RenderPosition::fromDOMPosition(sel.start()).renderedOffset();
            int e = khtml::RenderPosition::fromDOMPosition(sel.end()).renderedOffset();
            text = str.mid(s, e-s);
          } else if(n == sel.start().node()) {
            text = str.mid(khtml::RenderPosition::fromDOMPosition(sel.start()).renderedOffset());
          } else if(n == sel.end().node()) {
            text += str.left(khtml::RenderPosition::fromDOMPosition(sel.end()).renderedOffset());
          } else
            text += str;
        }
      }
      else {
        // This is our simple HTML -> ASCII transformation:
        unsigned short id = n.elementId();
        switch(id) {
          case ID_TEXTAREA:
            text += static_cast<HTMLTextAreaElementImpl*>(n.handle())->value().string();
            break;
          case ID_INPUT:
            if (static_cast<HTMLInputElementImpl*>(n.handle())->inputType() != HTMLInputElementImpl::PASSWORD)
                text += static_cast<HTMLInputElementImpl*>(n.handle())->value().string();
            break;
          case ID_SELECT:
            text += static_cast<HTMLSelectElementImpl*>(n.handle())->value().string();
            break;
          case ID_BR:
            text += "\n";
            hasNewLine = true;
            break;
          case ID_IMG:
            text += static_cast<HTMLImageElementImpl*>(n.handle())->altText().string();
            break;
          case ID_TD:
            break;
          case ID_TH:
          case ID_HR:
          case ID_OL:
          case ID_UL:
          case ID_LI:
          case ID_DD:
          case ID_DL:
          case ID_DT:
          case ID_PRE:
          case ID_LISTING:
          case ID_BLOCKQUOTE:
          case ID_DIV:
            if (!hasNewLine)
               text += "\n";
            hasNewLine = true;
            break;
          case ID_P:
          case ID_TR:
          case ID_H1:
          case ID_H2:
          case ID_H3:
          case ID_H4:
          case ID_H5:
          case ID_H6:
            if (!hasNewLine)
               text += "\n";
            hasNewLine = true;
            break;
        }
      }
      if(n == sel.end().node()) break;
      DOM::Node next = n.firstChild();
      if(next.isNull()) next = n.nextSibling();
      while( next.isNull() && !n.parentNode().isNull() ) {
        n = n.parentNode();
        next = n.nextSibling();
        unsigned short id = n.elementId();
        switch(id) {
          case ID_TD:
            seenTDTag = true; //Add two spaces after a td if then followed by text.
            break;
          case ID_TH:
          case ID_HR:
          case ID_OL:
          case ID_UL:
          case ID_LI:
          case ID_DD:
          case ID_DL:
          case ID_DT:
          case ID_PRE:
          case ID_LISTING:
          case ID_BLOCKQUOTE:
          case ID_DIV:
            seenTDTag = false;
            if (!hasNewLine)
               text += "\n";
            hasNewLine = true;
            break;
          case ID_P:
          case ID_TR:
          case ID_H1:
          case ID_H2:
          case ID_H3:
          case ID_H4:
          case ID_H5:
          case ID_H6:
            if (!hasNewLine)
               text += "\n";
//            text += "\n";
            hasNewLine = true;
            break;
        }
      }

      n = next;
    }

    if(text.isEmpty())
        return QString();

    int start = 0;
    int end = text.length();

    // Strip leading LFs
    while ((start < end) && (text[start] == '\n'))
       ++start;

    // Strip excessive trailing LFs
    while ((start < (end-1)) && (text[end-1] == '\n') && (text[end-2] == '\n'))
       --end;

    return text.mid(start, end-start);
}

QString KHTMLPart::simplifiedSelectedText() const
{
    QString text = selectedText();
    text.replace(QChar(0xa0), ' ');
    // remove leading and trailing whitespace
    while (!text.isEmpty() && text[0].isSpace())
        text = text.mid(1);
    while (!text.isEmpty() && text[text.length()-1].isSpace())
        text.truncate(text.length()-1);
    return text;
}

bool KHTMLPart::hasSelection() const
{
    return !d->editor_context.m_selection.isEmpty() && !d->editor_context.m_selection.isCollapsed();
}

DOM::Range KHTMLPart::selection() const
{
    return d->editor_context.m_selection.toRange();
}

void KHTMLPart::selection(DOM::Node &s, long &so, DOM::Node &e, long &eo) const
{
    DOM::Range r = d->editor_context.m_selection.toRange();
    s = r.startContainer();
    so = r.startOffset();
    e = r.endContainer();
    eo = r.endOffset();
}

void KHTMLPart::setSelection( const DOM::Range &r )
{
    setCaret(r);
}

const Selection &KHTMLPart::caret() const
{
  return d->editor_context.m_selection;
}

const Selection &KHTMLPart::dragCaret() const
{
  return d->editor_context.m_dragCaret;
}

void KHTMLPart::setCaret(const Selection &s, bool closeTyping)
{
  if (d->editor_context.m_selection != s) {
    clearCaretRectIfNeeded();
    setFocusNodeIfNeeded(s);
    d->editor_context.m_selection = s;
    notifySelectionChanged(closeTyping);
  }
}

void KHTMLPart::setDragCaret(const DOM::Selection &dragCaret)
{
  if (d->editor_context.m_dragCaret != dragCaret) {
    d->editor_context.m_dragCaret.needsCaretRepaint();
    d->editor_context.m_dragCaret = dragCaret;
    d->editor_context.m_dragCaret.needsCaretRepaint();
  }
}

void KHTMLPart::clearSelection()
{
  clearCaretRectIfNeeded();
  setFocusNodeIfNeeded(d->editor_context.m_selection);
#ifdef APPLE_CHANGES
  d->editor_context.m_selection.clear();
#else
  d->editor_context.m_selection.collapse();
#endif
  notifySelectionChanged();
}

void KHTMLPart::invalidateSelection()
{
  clearCaretRectIfNeeded();
  d->editor_context.m_selection.setNeedsLayout();
  selectionLayoutChanged();
}

void KHTMLPart::setSelectionVisible(bool flag)
{
  if (d->editor_context.m_caretVisible == flag)
    return;

  clearCaretRectIfNeeded();
  setFocusNodeIfNeeded(d->editor_context.m_selection);
  d->editor_context.m_caretVisible = flag;
//   notifySelectionChanged();
}

#if 1
void KHTMLPart::slotClearSelection()
{
  if (!isCaretMode()
       && d->editor_context.m_selection.state() != Selection::NONE
       && !d->editor_context.m_selection.caretPos().node()->isContentEditable())
    clearCaretRectIfNeeded();
  bool hadSelection = hasSelection();
#ifdef APPLE_CHANGES
  d->editor_context.m_selection.clear();
#else
  d->editor_context.m_selection.collapse();
#endif
  if (hadSelection)
    notifySelectionChanged();
}
#endif

void KHTMLPart::clearCaretRectIfNeeded()
{
  if (d->editor_context.m_caretPaint) {
    d->editor_context.m_caretPaint = false;
    d->editor_context.m_selection.needsCaretRepaint();
  }
}

void KHTMLPart::setFocusNodeIfNeeded(const Selection &s)
{
  if (!xmlDocImpl() || s.state() == Selection::NONE)
    return;

  NodeImpl *n = s.start().node();
  NodeImpl *target = (n && n->isContentEditable()) ? n : 0;
  if (!target) {
    while (n && n != s.end().node()) {
      if (n->isContentEditable()) {
        target = n;
        break;
      }
      n = n->traverseNextNode();
    }
  }
  assert(target == 0 || target->isContentEditable());

  if (target) {
    for ( ; target && !target->isFocusable(); target = target->parentNode())
      {}
    if (target && target->isMouseFocusable())
      xmlDocImpl()->setFocusNode(target);
    else if (!target || !target->focused())
      xmlDocImpl()->setFocusNode(0);
  }
}

void KHTMLPart::selectionLayoutChanged()
{
  // kill any caret blink timer now running
  if (d->editor_context.m_caretBlinkTimer >= 0) {
    killTimer(d->editor_context.m_caretBlinkTimer);
    d->editor_context.m_caretBlinkTimer = -1;
  }

  // see if a new caret blink timer needs to be started
  if (d->editor_context.m_caretVisible
      && d->editor_context.m_selection.state() != Selection::NONE) {
    d->editor_context.m_caretPaint = isCaretMode()
        || d->editor_context.m_selection.caretPos().node()->isContentEditable();
    if (d->editor_context.m_caretBlinks && d->editor_context.m_caretPaint)
      d->editor_context.m_caretBlinkTimer = startTimer(qApp->cursorFlashTime() / 2);
    d->editor_context.m_selection.needsCaretRepaint();
    // make sure that caret is visible
    QRect r(d->editor_context.m_selection.getRepaintRect());
    if (d->editor_context.m_caretPaint)
        d->m_view->ensureVisible(r.x(), r.y());
  }

  if (d->m_doc)
    d->m_doc->updateSelection();

  // Always clear the x position used for vertical arrow navigation.
  // It will be restored by the vertical arrow navigation code if necessary.
  d->editor_context.m_xPosForVerticalArrowNavigation = d->editor_context.NoXPosForVerticalArrowNavigation;
}

void KHTMLPart::notifySelectionChanged(bool closeTyping)
{
  Editor *ed = d->editor_context.m_editor;
  selectionLayoutChanged();
  if (ed) {
    ed->clearTypingStyle();

    if (closeTyping)
        ed->closeTyping();
  }

  emitSelectionChanged();
}

void KHTMLPart::timerEvent(QTimerEvent *e)
{
  if (e->timerId() == d->editor_context.m_caretBlinkTimer) {
    if (d->editor_context.m_caretBlinks &&
        d->editor_context.m_selection.state() != Selection::NONE) {
      d->editor_context.m_caretPaint = !d->editor_context.m_caretPaint;
      d->editor_context.m_selection.needsCaretRepaint();
    }
  } else if (e->timerId() == d->m_DNSPrefetchTimer) {
      // qDebug() << "will lookup " << d->m_DNSPrefetchQueue.head() << d->m_numDNSPrefetchedNames;
      KIO::HostInfo::prefetchHost( d->m_DNSPrefetchQueue.dequeue() );
      if (d->m_DNSPrefetchQueue.isEmpty()) {
          killTimer( d->m_DNSPrefetchTimer );
          d->m_DNSPrefetchTimer = -1;
      }
  } else if (e->timerId() == d->m_DNSTTLTimer) {
      foreach (const QString &name, d->m_lookedupHosts)
          d->m_DNSPrefetchQueue.enqueue(name);
      if (d->m_DNSPrefetchTimer <= 0)
         d->m_DNSPrefetchTimer = startTimer( sDNSPrefetchTimerDelay );
  }
}

bool KHTMLPart::mayPrefetchHostname( const QString& name )
{
    if (d->m_bDNSPrefetch == DNSPrefetchDisabled)
        return false;

    if (d->m_numDNSPrefetchedNames >= sMaxDNSPrefetchPerPage)
        return false;

    if (d->m_bDNSPrefetch == DNSPrefetchOnlyWWWAndSLD) {
        int dots = name.count('.');
        if (dots > 2 || (dots == 2 &&  !name.startsWith("www.")))
            return false;
    }

    if ( d->m_lookedupHosts.contains( name ) )
        return false;

    d->m_DNSPrefetchQueue.enqueue( name );
    d->m_lookedupHosts.insert( name );
    d->m_numDNSPrefetchedNames++;

    if (d->m_DNSPrefetchTimer < 1)
        d->m_DNSPrefetchTimer = startTimer( sDNSPrefetchTimerDelay );
    if (d->m_DNSTTLTimer < 1)
        d->m_DNSTTLTimer = startTimer( sDNSTTLSeconds*1000 + 1 );

    return true;
}

void KHTMLPart::paintCaret(QPainter *p, const QRect &rect) const
{
  if (d->editor_context.m_caretPaint)
    d->editor_context.m_selection.paintCaret(p, rect);
}

void KHTMLPart::paintDragCaret(QPainter *p, const QRect &rect) const
{
  d->editor_context.m_dragCaret.paintCaret(p, rect);
}

DOM::Editor *KHTMLPart::editor() const {
  if (!d->editor_context.m_editor)
    const_cast<KHTMLPart *>(this)->d->editor_context.m_editor = new DOM::Editor(const_cast<KHTMLPart *>(this));
  return d->editor_context.m_editor;
}

void KHTMLPart::resetHoverText()
{
   if( !d->m_overURL.isEmpty() ) // Only if we were showing a link
   {
     d->m_overURL.clear();
     d->m_overURLTarget.clear();
     emit onURL( QString() );
     // revert to default statusbar text
     setStatusBarText(QString(), BarHoverText);
     emit d->m_extension->mouseOverInfo(KFileItem());
  }
}

void KHTMLPart::overURL( const QString &url, const QString &target, bool /*shiftPressed*/ )
{
  QUrl u = completeURL(url);

  // special case for <a href="">
  if ( url.isEmpty() ) {
    u = u.adjusted(QUrl::RemoveFilename);
  }

  emit onURL( url );

  if ( url.isEmpty() ) {
    setStatusBarText(Qt::escape(u.toDisplayString()), BarHoverText);
    return;
  }

  if ( d->isJavaScriptURL(url) ) {
    QString jscode = d->codeForJavaScriptURL( url );
    jscode = KStringHandler::rsqueeze( jscode, 80 ); // truncate if too long
    if (url.startsWith("javascript:window.open"))
      jscode += i18n(" (In new window)");
    setStatusBarText( Qt::escape( jscode ), BarHoverText );
    return;
  }

  KFileItem item(u, QString(), KFileItem::Unknown);
  emit d->m_extension->mouseOverInfo(item);
  const QString com = item.mimeComment();

  if ( !u.isValid() ) {
    setStatusBarText(Qt::escape(u.toDisplayString()), BarHoverText);
    return;
  }

  if ( u.isLocalFile() )
  {
    // TODO : use KIO::stat() and create a KFileItem out of its result,
    // to use KFileItem::statusBarText()

    QFileInfo info(u.toLocalFile());
    bool ok = info.exists();

    QString text = Qt::escape(u.toDisplayString());
    QString text2 = text;

    if (info.isSymLink())
    {
      QString tmp;
      if ( com.isEmpty() )
        tmp = i18n( "Symbolic Link");
      else
        tmp = i18n("%1 (Link)", com);
      text += " -> ";
      QString target = info.symLinkTarget();
      if (target.isEmpty()) {
        text2 += "  ";
        text2 += tmp;
        setStatusBarText(text2, BarHoverText);
        return;
      }

      text += target;
      text += "  ";
      text += tmp;
    }
    else if ( ok && info.isFile() )
    {
      if (info.size() < 1024)
        text = i18np("%2 (%1 byte)", "%2 (%1 bytes)", (long) info.size(), text2); // always put the URL last, in case it contains '%'
      else
      {
        float d = (float) info.size()/1024.0;
        text = i18n("%2 (%1 K)", QLocale().toString(d, 'f', 2), text2); // was %.2f
      }
      text += "  ";
      text += com;
    }
    else if ( ok && info.isDir() )
    {
      text += "  ";
      text += com;
    }
    else
    {
      text += "  ";
      text += com;
    }
    setStatusBarText(text, BarHoverText);
  }
  else
  {
    QString extra;
    if (target.toLower() == "_blank")
    {
      extra = i18n(" (In new window)");
    }
    else if (!target.isEmpty() &&
             (target.toLower() != "_top") &&
             (target.toLower() != "_self") &&
             (target.toLower() != "_parent"))
    {
      KHTMLPart *p = this;
      while (p->parentPart())
          p = p->parentPart();
      if (!p->frameExists(target))
        extra = i18n(" (In new window)");
      else
        extra = i18n(" (In other frame)");
    }

    if (u.scheme() == QLatin1String("mailto")) {
      QString mailtoMsg /* = QString::fromLatin1("<img src=%1>").arg(locate("icon", QString::fromLatin1("locolor/16x16/actions/mail_send.png")))*/;
      mailtoMsg += i18n("Email to: ") + QUrl::fromPercentEncoding(u.path().toLatin1());
      const QStringList queries = u.query().mid(1).split('&');
      QStringList::ConstIterator it = queries.begin();
      const QStringList::ConstIterator itEnd = queries.end();
      for (; it != itEnd; ++it)
        if ((*it).startsWith(QLatin1String("subject=")))
          mailtoMsg += i18n(" - Subject: ") + QUrl::fromPercentEncoding((*it).mid(8).toLatin1());
        else if ((*it).startsWith(QLatin1String("cc=")))
          mailtoMsg += i18n(" - CC: ") + QUrl::fromPercentEncoding((*it).mid(3).toLatin1());
        else if ((*it).startsWith(QLatin1String("bcc=")))
          mailtoMsg += i18n(" - BCC: ") + QUrl::fromPercentEncoding((*it).mid(4).toLatin1());
      mailtoMsg = Qt::escape(mailtoMsg);
      mailtoMsg.replace(QRegExp("([\n\r\t]|[ ]{10})"), QString());
      setStatusBarText("<qt>"+mailtoMsg, BarHoverText);
      return;
    }
   // Is this check necessary at all? (Frerich)
#if 0
    else if (u.scheme() == QLatin1String("http")) {
        DOM::Node hrefNode = nodeUnderMouse().parentNode();
        while (hrefNode.nodeName().string() != QLatin1String("A") && !hrefNode.isNull())
          hrefNode = hrefNode.parentNode();

        if (!hrefNode.isNull()) {
          DOM::Node hreflangNode = hrefNode.attributes().getNamedItem("HREFLANG");
          if (!hreflangNode.isNull()) {
            QString countryCode = hreflangNode.nodeValue().string().toLower();
            // Map the language code to an appropriate country code.
            if (countryCode == QLatin1String("en"))
              countryCode = QLatin1String("gb");
            QString flagImg = QLatin1String("<img src=%1>").arg(
                locate("locale", QLatin1String("l10n/")
                + countryCode
                + QLatin1String("/flag.png")));
            emit setStatusBarText(flagImg + u.toDisplayString() + extra);
          }
        }
      }
#endif
    setStatusBarText(Qt::escape(u.toDisplayString()) + extra, BarHoverText);
  }
}

//
// This executes in the active part on a click or other url selection action in
// that active part.
//
bool KHTMLPart::urlSelected( const QString &url, int button, int state, const QString &_target, const KParts::OpenUrlArguments& _args, const KParts::BrowserArguments& _browserArgs )
{
  KParts::OpenUrlArguments args = _args;
  KParts::BrowserArguments browserArgs = _browserArgs;
  bool hasTarget = false;

  QString target = _target;
  if ( target.isEmpty() && d->m_doc )
    target = d->m_doc->baseTarget();
  if ( !target.isEmpty() )
      hasTarget = true;

  if ( d->isJavaScriptURL(url) )
  {
    crossFrameExecuteScript( target, d->codeForJavaScriptURL(url) );
    return false;
  }

  QUrl cURL = completeURL(url);
  // special case for <a href="">  (IE removes filename, mozilla doesn't)
  if ( url.isEmpty() ) {
    cURL = cURL.adjusted(QUrl::RemoveFilename);
  }

  if ( !cURL.isValid() )
    // ### ERROR HANDLING
    return false;

  // qDebug() << this << "complete URL:" << cURL << "target=" << target;

  if ( state & Qt::ControlModifier )
  {
    emit d->m_extension->createNewWindow( cURL, args, browserArgs );
    return true;
  }

  if ( button == Qt::LeftButton && ( state & Qt::ShiftModifier ) )
  {
    KIO::MetaData metaData;
    metaData.insert( "referrer", d->m_referrer );
    KHTMLPopupGUIClient::saveURL( d->m_view, i18n( "Save As" ), cURL, metaData );
    return false;
  }

  if (!checkLinkSecurity(cURL,
                         ki18n( "<qt>This untrusted page links to<br /><b>%1</b>.<br />Do you want to follow the link?</qt>" ),
                         i18n( "Follow" )))
    return false;

  browserArgs.frameName = target;

  args.metaData().insert("main_frame_request",
                         parentPart() == 0 ? "TRUE":"FALSE");
  args.metaData().insert("ssl_parent_ip", d->m_ssl_parent_ip);
  args.metaData().insert("ssl_parent_cert", d->m_ssl_parent_cert);
  args.metaData().insert("PropagateHttpHeader", "true");
  args.metaData().insert("ssl_was_in_use", d->m_ssl_in_use ? "TRUE":"FALSE");
  args.metaData().insert("ssl_activate_warnings", "TRUE");

  if ( hasTarget && target != "_self" && target != "_top" && target != "_blank" && target != "_parent" )
  {
    // unknown frame names should open in a new window.
    khtml::ChildFrame *frame = recursiveFrameRequest( this, cURL, args, browserArgs, false );
    if ( frame )
    {
      args.metaData()["referrer"] = d->m_referrer;
      requestObject( frame, cURL, args, browserArgs );
      return true;
    }
  }

  if (!d->m_referrer.isEmpty() && !args.metaData().contains("referrer"))
    args.metaData()["referrer"] = d->m_referrer;

  if ( button == Qt::NoButton && (state & Qt::ShiftModifier) && (state & Qt::ControlModifier) )
  {
    emit d->m_extension->createNewWindow( cURL, args, browserArgs );
    return true;
  }

  if ( state & Qt::ShiftModifier)
  {
    KParts::WindowArgs winArgs;
    winArgs.setLowerWindow(true);
    emit d->m_extension->createNewWindow( cURL, args, browserArgs, winArgs );
    return true;
  }

  //If we're asked to open up an anchor in the current URL, in current window,
  //merely gotoanchor, and do not reload the new page. Note that this does
  //not apply if the URL is the same page, but without a ref
  if (cURL.hasFragment() && (!hasTarget || target == "_self"))
  {
    if (d->isLocalAnchorJump(cURL))
    {
      d->executeAnchorJump(cURL, browserArgs.lockHistory() );
      return false; // we jumped, but we didn't open a URL
    }
  }

  if ( !d->m_bComplete && !hasTarget )
    closeUrl();

  view()->viewport()->unsetCursor();
  emit d->m_extension->openUrlRequest( cURL, args, browserArgs );
  return true;
}

void KHTMLPart::slotViewDocumentSource()
{
  QUrl currentUrl(this->url());
  bool isTempFile = false;
  if (!(currentUrl.isLocalFile()) && KHTMLPageCache::self()->isComplete(d->m_cacheId))
  {
     QTemporaryFile sourceFile(QDir::tempPath() + QLatin1String("/XXXXXX") + defaultExtension());
     sourceFile.setAutoRemove(false);
     if (sourceFile.open()) {
        QDataStream stream ( &sourceFile );
        KHTMLPageCache::self()->saveData(d->m_cacheId, &stream);
        currentUrl = QUrl::fromLocalFile(sourceFile.fileName());
        isTempFile = true;
     }
  }

  (void) KRun::runUrl( currentUrl, QLatin1String("text/plain"), view(), isTempFile );
}

void KHTMLPart::slotViewPageInfo()
{
  Ui_KHTMLInfoDlg ui;

  QDialog *dlg = new QDialog(0);
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  dlg->setObjectName("KHTML Page Info Dialog");
  ui.setupUi(dlg);

  KGuiItem::assign(ui._close, KStandardGuiItem::close());
  connect(ui._close, SIGNAL(clicked()), dlg, SLOT(accept()));

  if (d->m_doc)
     ui._title->setText(d->m_doc->title().string().trimmed());

  // If it's a frame, set the caption to "Frame Information"
  if ( parentPart() && d->m_doc && d->m_doc->isHTMLDocument() ) {
     dlg->setWindowTitle(i18n("Frame Information"));
  }

  QString editStr;

  if (!d->m_pageServices.isEmpty())
    editStr = i18n("   <a href=\"%1\">[Properties]</a>", d->m_pageServices);

  QString squeezedURL = KStringHandler::csqueeze( url().toDisplayString(), 80 );
  ui._url->setText("<a href=\"" + url().toString() + "\">" + squeezedURL + "</a>" + editStr);
  if (lastModified().isEmpty())
  {
    ui._lastModified->hide();
    ui._lmLabel->hide();
  }
  else
    ui._lastModified->setText(lastModified());

  const QString& enc = encoding();
  if (enc.isEmpty()) {
    ui._eLabel->hide();
    ui._encoding->hide();
  } else {
    ui._encoding->setText(enc);
  }

  if (!xmlDocImpl() || xmlDocImpl()->parseMode() == DOM::DocumentImpl::Unknown) {
    ui._mode->hide();
    ui._modeLabel->hide();
  } else {
    switch (xmlDocImpl()->parseMode()) {
      case DOM::DocumentImpl::Compat:
        ui._mode->setText(i18nc("HTML rendering mode (see http://en.wikipedia.org/wiki/Quirks_mode)", "Quirks"));
        break;
      case DOM::DocumentImpl::Transitional:
        ui._mode->setText(i18nc("HTML rendering mode (see http://en.wikipedia.org/wiki/Quirks_mode)", "Almost standards"));
        break;
      case DOM::DocumentImpl::Strict:
      default: // others handled above
        ui._mode->setText(i18nc("HTML rendering mode (see http://en.wikipedia.org/wiki/Quirks_mode)", "Strict"));
        break;
    }
  }

  /* populate the list view now */
  const QStringList headers = d->m_httpHeaders.split("\n");

  QStringList::ConstIterator it = headers.begin();
  const QStringList::ConstIterator itEnd = headers.end();

  for (; it != itEnd; ++it) {
    const QStringList header = (*it).split(QRegExp(":[ ]+"));
    if (header.count() != 2)
       continue;
    QTreeWidgetItem *item = new QTreeWidgetItem(ui._headers);
    item->setText(0, header[0]);
    item->setText(1, header[1]);
  }

  dlg->show();
  /* put no code here */
}


void KHTMLPart::slotViewFrameSource()
{
  KParts::ReadOnlyPart *frame = currentFrame();
  if ( !frame )
    return;

  QUrl url = frame->url();
  bool isTempFile = false;
  if (!(url.isLocalFile()) && frame->inherits("KHTMLPart"))
  {
       long cacheId = static_cast<KHTMLPart *>(frame)->d->m_cacheId;

       if (KHTMLPageCache::self()->isComplete(cacheId))
       {
           QTemporaryFile sourceFile(QDir::tempPath() + QLatin1String("/XXXXXX") + defaultExtension());
           sourceFile.setAutoRemove(false);
           if (sourceFile.open()) {
               QDataStream stream ( &sourceFile );
               KHTMLPageCache::self()->saveData(cacheId, &stream);
               url = QUrl();
               url.setPath(sourceFile.fileName());
               isTempFile = true;
           }
     }
  }

  (void) KRun::runUrl( url, QLatin1String("text/plain"), view(), isTempFile );
}

QUrl KHTMLPart::backgroundURL() const
{
  // ### what about XML documents? get from CSS?
  if (!d->m_doc || !d->m_doc->isHTMLDocument())
    return QUrl();

  QString relURL = static_cast<HTMLDocumentImpl*>(d->m_doc)->body()->getAttribute( ATTR_BACKGROUND ).string();

  return url().resolved(QUrl(relURL));
}

void KHTMLPart::slotSaveBackground()
{
  KIO::MetaData metaData;
  metaData["referrer"] = d->m_referrer;
  KHTMLPopupGUIClient::saveURL( d->m_view, i18n("Save Background Image As"), backgroundURL(), metaData );
}

void KHTMLPart::slotSaveDocument()
{
  QUrl srcURL(url());

  if (srcURL.fileName().isEmpty())
    srcURL.setPath(srcURL.path() + "index" + defaultExtension());

  KIO::MetaData metaData;
  // Referre unknown?
  KHTMLPopupGUIClient::saveURL( d->m_view, i18n( "Save As" ), srcURL, metaData, "text/html", d->m_cacheId );
}

void KHTMLPart::slotSecurity()
{
//   qDebug() << "Meta Data:" << endl
//                   << d->m_ssl_peer_cert_subject
//                   << endl
//                   << d->m_ssl_peer_cert_issuer
//                   << endl
//                   << d->m_ssl_cipher
//                   << endl
//                   << d->m_ssl_cipher_desc
//                   << endl
//                   << d->m_ssl_cipher_version
//                   << endl
//                   << d->m_ssl_good_from
//                   << endl
//                   << d->m_ssl_good_until
//                   << endl
//                   << d->m_ssl_cert_state
//                   << endl;

  //### reenable with new signature
#if 0
  KSslInfoDialog *kid = new KSslInfoDialog(d->m_ssl_in_use, widget(), "kssl_info_dlg", true );

  const QStringList sl = d->m_ssl_peer_chain.split('\n', QString::SkipEmptyParts);
  QList<QSslCertificate> certChain;
  bool certChainOk = d->m_ssl_in_use;
  if (certChainOk) {
    foreach (const QString &s, sl) {
      certChain.append(QSslCertificate(s.toLatin1())); //or is it toLocal8Bit or whatever?
      if (certChain.last().isNull()) {
        certChainOk = false;
        break;
      }
    }
  }
  if (certChainOk) {
    kid->setup(certChain,
               d->m_ssl_peer_ip,
               url().toString(),
               d->m_ssl_cipher,
               d->m_ssl_cipher_desc,
               d->m_ssl_cipher_version,
               d->m_ssl_cipher_used_bits.toInt(),
               d->m_ssl_cipher_bits.toInt(),
               (KSSLCertificate::KSSLValidation) d->m_ssl_cert_state.toInt());
  }
  kid->exec();
  //the dialog deletes itself on close
#endif

    KSslInfoDialog *kid = new KSslInfoDialog(0);
    //### This is boilerplate code and it's copied from SlaveInterface.
    QStringList sl = d->m_ssl_peer_chain.split('\x01', QString::SkipEmptyParts);
    QList<QSslCertificate> certChain;
    bool decodedOk = true;
    foreach (const QString &s, sl) {
        certChain.append(QSslCertificate(s.toLatin1())); //or is it toLocal8Bit or whatever?
        if (certChain.last().isNull()) {
            decodedOk = false;
            break;
        }
    }

    if (decodedOk || true /*H4X*/) {
        kid->setSslInfo(certChain,
                        d->m_ssl_peer_ip,
                        url().host(),
                        d->m_ssl_protocol_version,
                        d->m_ssl_cipher,
                        d->m_ssl_cipher_used_bits.toInt(),
                        d->m_ssl_cipher_bits.toInt(),
                        KSslInfoDialog::errorsFromString(d->m_ssl_cert_errors));
        // qDebug() << "Showing SSL Info dialog";
        kid->exec();
        // qDebug() << "SSL Info dialog closed";
    } else {
        KMessageBox::information(0, i18n("The peer SSL certificate chain "
                                         "appears to be corrupt."),
                                 i18n("SSL"));
    }
}

void KHTMLPart::slotSaveFrame()
{
    KParts::ReadOnlyPart *frame = currentFrame();
    if ( !frame )
        return;

    QUrl srcURL( frame->url() );

    if ( srcURL.fileName().isEmpty() )
        srcURL.setPath( srcURL.path() + "index" + defaultExtension() );

    KIO::MetaData metaData;
    // Referrer unknown?
    KHTMLPopupGUIClient::saveURL( d->m_view, i18n( "Save Frame As" ), srcURL, metaData, "text/html" );
}

void KHTMLPart::slotSetEncoding(const QString &enc)
{
    d->m_autoDetectLanguage=KEncodingProber::None;
    setEncoding( enc, true);
}

void KHTMLPart::slotAutomaticDetectionLanguage(KEncodingProber::ProberType scri)
{
  d->m_autoDetectLanguage=scri;
  setEncoding( QString(), false );
}

void KHTMLPart::slotUseStylesheet()
{
  if (d->m_doc)
  {
    bool autoselect = (d->m_paUseStylesheet->currentItem() == 0);
    d->m_sheetUsed = autoselect ? QString() : d->m_paUseStylesheet->currentText();
    d->m_doc->updateStyleSelector();
  }
}

void KHTMLPart::updateActions()
{
  bool frames = false;

  QList<khtml::ChildFrame*>::ConstIterator it = d->m_frames.constBegin();
  const QList<khtml::ChildFrame*>::ConstIterator end = d->m_frames.constEnd();
  for (; it != end; ++it )
      if ( (*it)->m_type == khtml::ChildFrame::Frame )
      {
          frames = true;
          break;
      }

  if (d->m_paViewFrame)
    d->m_paViewFrame->setEnabled( frames );
  if (d->m_paSaveFrame)
    d->m_paSaveFrame->setEnabled( frames );

  if ( frames )
    d->m_paFind->setText( i18n( "&Find in Frame..." ) );
  else
    d->m_paFind->setText( i18n( "&Find..." ) );

  KParts::Part *frame = 0;

  if ( frames )
    frame = currentFrame();

  bool enableFindAndSelectAll = true;

  if ( frame )
    enableFindAndSelectAll = frame->inherits( "KHTMLPart" );

  d->m_paFind->setEnabled( enableFindAndSelectAll );
  d->m_paSelectAll->setEnabled( enableFindAndSelectAll );

  bool enablePrintFrame = false;

  if ( frame )
  {
    QObject *ext = KParts::BrowserExtension::childObject( frame );
    if ( ext )
      enablePrintFrame = ext->metaObject()->indexOfSlot( "print()" ) != -1;
  }

  d->m_paPrintFrame->setEnabled( enablePrintFrame );

  QString bgURL;

  // ### frames
  if ( d->m_doc && d->m_doc->isHTMLDocument() && static_cast<HTMLDocumentImpl*>(d->m_doc)->body() && !d->m_bClearing )
    bgURL = static_cast<HTMLDocumentImpl*>(d->m_doc)->body()->getAttribute( ATTR_BACKGROUND ).string();

  if (d->m_paSaveBackground)
    d->m_paSaveBackground->setEnabled( !bgURL.isEmpty() );

  if ( d->m_paDebugScript )
    d->m_paDebugScript->setEnabled( d->m_frame ? d->m_frame->m_jscript : 0L );
}

KParts::ScriptableExtension *KHTMLPart::scriptableExtension( const DOM::NodeImpl *frame) {
    const ConstFrameIt end = d->m_objects.constEnd();
    for(ConstFrameIt it = d->m_objects.constBegin(); it != end; ++it )
        if ((*it)->m_partContainerElement.data() == frame)
            return (*it)->m_scriptable.data();
    return 0L;
}

void KHTMLPart::loadFrameElement( DOM::HTMLPartContainerElementImpl *frame, const QString &url,
                                  const QString &frameName, const QStringList &params, bool isIFrame )
{
    //qDebug() << this << " requestFrame( ..., " << url << ", " << frameName << " )";
    khtml::ChildFrame* child;

    FrameIt it = d->m_frames.find( frameName );
    if ( it == d->m_frames.end() ) {
        child = new khtml::ChildFrame;
        //qDebug() << "inserting new frame into frame map " << frameName;
        child->m_name = frameName;
        d->m_frames.insert( d->m_frames.end(), child );
    } else {
        child = *it;
    }

    child->m_type = isIFrame ? khtml::ChildFrame::IFrame : khtml::ChildFrame::Frame;
    child->m_partContainerElement = frame;
    child->m_params = params;

    // If we do not have a part, make sure we create one.
    if (!child->m_part) {
        QStringList dummy; // the list of servicetypes handled by the part is now unused.
        QString     khtml = QString::fromLatin1("khtml");
        KParts::ReadOnlyPart* part = createPart(d->m_view->viewport(), this,
                                                QString::fromLatin1("text/html"),
                                                khtml, dummy, QStringList());
        // We navigate it to about:blank to setup an empty one, but we do it
        // before hooking up the signals and extensions, so that any sync emit
        // of completed by the kid doesn't cause us to be marked as completed.
        // (async ones are discovered by the presence of the KHTMLRun)
        // ### load event on the kid?
        navigateLocalProtocol(child, part, QUrl("about:blank"));
        connectToChildPart(child, part, "text/html" /* mimetype of the part, not what's being loaded */);
    }

    QUrl u = url.isEmpty() ? QUrl() : completeURL( url );

    // Since we don't specify args here a KHTMLRun will be used to determine the
    // mimetype, which will then be  passed down at the bottom of processObjectRequest
    // inside URLArgs to the part. In our particular case, this means that we can
    // use that inside KHTMLPart::openUrl to route things appropriately.
    child->m_bCompleted = false;
    if (!requestObject( child, u ) && !child->m_run) {
        child->m_bCompleted = true;
    }
}

QString KHTMLPart::requestFrameName()
{
   return QString::fromLatin1("<!--frame %1-->").arg(d->m_frameNameId++);
}

bool KHTMLPart::loadObjectElement( DOM::HTMLPartContainerElementImpl *frame, const QString &url,
                                   const QString &serviceType, const QStringList &params )
{
  //qDebug() << this << "frame=" << frame;
  khtml::ChildFrame *child = new khtml::ChildFrame;
  FrameIt it = d->m_objects.insert( d->m_objects.end(), child );
  (*it)->m_partContainerElement = frame;
  (*it)->m_type = khtml::ChildFrame::Object;
  (*it)->m_params = params;

  KParts::OpenUrlArguments args;
  args.setMimeType(serviceType);
  if (!requestObject( *it, completeURL( url ), args ) && !(*it)->m_run) {
      (*it)->m_bCompleted = true;
      return false;
  }
  return true;
}

bool KHTMLPart::requestObject( khtml::ChildFrame *child, const QUrl &url, const KParts::OpenUrlArguments &_args,
                               const KParts::BrowserArguments& browserArgs )
{
  // we always permit javascript: URLs here since they're basically just
  // empty pages (and checkLinkSecurity/KAuthorized doesn't know what to do with them)
  if (!d->isJavaScriptURL(url.toString()) && !checkLinkSecurity(url))
  {
    // qDebug() << this << "checkLinkSecurity refused";
    return false;
  }

  if (d->m_bClearing)
  {
    return false;
  }

  if ( child->m_bPreloaded )
  {
    if ( child->m_partContainerElement && child->m_part )
      child->m_partContainerElement.data()->setWidget( child->m_part.data()->widget() );

    child->m_bPreloaded = false;
    return true;
  }

  //qDebug() << "child=" << child << "child->m_part=" << child->m_part;

  KParts::OpenUrlArguments args( _args );

  if ( child->m_run ) {
      // qDebug() << "navigating ChildFrame while mimetype resolution was in progress...";
      child->m_run.data()->abort();
  }

  // ### Dubious -- the whole dir/ vs. img thing
  if ( child->m_part && !args.reload() && areUrlsForSamePage(child->m_part.data()->url(), url) )
    args.setMimeType(child->m_serviceType);

  child->m_browserArgs = browserArgs;
  child->m_args = args;

  // reload/soft-reload arguments are always inherited from parent
  child->m_args.setReload( arguments().reload() );
  child->m_browserArgs.softReload = d->m_extension->browserArguments().softReload;

  child->m_serviceName.clear();
  if (!d->m_referrer.isEmpty() && !child->m_args.metaData().contains( "referrer" ))
    child->m_args.metaData()["referrer"] = d->m_referrer;

  child->m_args.metaData().insert("PropagateHttpHeader", "true");
  child->m_args.metaData().insert("ssl_parent_ip", d->m_ssl_parent_ip);
  child->m_args.metaData().insert("ssl_parent_cert", d->m_ssl_parent_cert);
  child->m_args.metaData().insert("main_frame_request",
                                  parentPart() == 0 ? "TRUE":"FALSE");
  child->m_args.metaData().insert("ssl_was_in_use",
                                  d->m_ssl_in_use ? "TRUE":"FALSE");
  child->m_args.metaData().insert("ssl_activate_warnings", "TRUE");
  child->m_args.metaData().insert("cross-domain", toplevelURL().toString());

  // We know the frame will be text/html if the HTML says <frame src=""> or <frame src="about:blank">,
  // no need to KHTMLRun to figure out the mimetype"
  // ### What if we're inside an XML document?
  if ((url.isEmpty() || url.toString() == "about:blank" || url.scheme() == "javascript") && args.mimeType().isEmpty())
    args.setMimeType(QLatin1String("text/html"));

  if ( args.mimeType().isEmpty() ) {
    // qDebug() << "Running new KHTMLRun for" << this << "and child=" << child;
    child->m_run = new KHTMLRun( this, child, url, child->m_args, child->m_browserArgs, true );
    d->m_bComplete = false; // ensures we stop it in checkCompleted...
    return false;
  } else {
    return processObjectRequest( child, url, args.mimeType() );
  }
}

void KHTMLPart::childLoadFailure( khtml::ChildFrame *child )
{
  child->m_bCompleted = true;
  if ( child->m_partContainerElement )
    child->m_partContainerElement.data()->partLoadingErrorNotify();

  checkCompleted();
}

bool KHTMLPart::processObjectRequest( khtml::ChildFrame *child, const QUrl &_url, const QString &mimetype )
{
    // qDebug() << "trying to create part for" << mimetype << _url;

    // IMPORTANT: create a copy of the url here, because it is just a reference, which was likely to be given
    // by an emitting frame part (emit openUrlRequest( blahurl, ... ) . A few lines below we delete the part
    // though -> the reference becomes invalid -> crash is likely
    QUrl url( _url );

    // khtmlrun called us with empty url + mimetype to indicate a loading error,
    // we obviosuly failed; but we can return true here since we don't want it
    // doing anything more, while childLoadFailure is enough to notify our kid.
    if ( d->m_onlyLocalReferences || ( url.isEmpty() && mimetype.isEmpty() ) ) {
        childLoadFailure(child);
        return true;
    }

    // we also want to ignore any spurious requests due to closing when parser is being cleared. These should be
    // ignored entirely  --- the tail end of ::clear will clean things up.
    if (d->m_bClearing)
        return false;

    if (child->m_bNotify) {
        child->m_bNotify = false;
        if ( !child->m_browserArgs.lockHistory() )
            emit d->m_extension->openUrlNotify();
    }

    QMimeDatabase db;

    // Now, depending on mimetype and current state of the world, we may have
    // to create a new part or ask the user to save things, etc.
    //
    // We need a new part if there isn't one at all (doh) or the one that's there
    // is not for the mimetype we're loading.
    //
    // For these new types, we may have to ask the user to save it or not
    // (we don't if it's navigating the same type).
    // Further, we will want to ask if content-disposition suggests we ask for
    // saving, even if we're re-navigating.
    if ( !child->m_part || child->m_serviceType != mimetype ||
            (child->m_run && child->m_run.data()->serverSuggestsSave())) {
        // We often get here if we didn't know the mimetype in advance, and had to rely
        // on KRun to figure it out. In this case, we let the element check if it wants to
        // handle this mimetype itself, for e.g. objects containing images.
        if ( child->m_partContainerElement &&
                child->m_partContainerElement.data()->mimetypeHandledInternally(mimetype) ) {
            child->m_bCompleted = true;
            checkCompleted();
            return true;
        }

        // Before attempting to load a part, check if the user wants that.
        // Many don't like getting ZIP files embedded.
        // However we don't want to ask for flash and other plugin things.
        //
        // Note: this is fine for frames, since we will merely effectively ignore
        // the navigation if this happens
        if ( child->m_type != khtml::ChildFrame::Object && child->m_type != khtml::ChildFrame::IFrame ) {
            QString suggestedFileName;
            int disposition = 0;
            if ( KHTMLRun* run = child->m_run.data() ) {
                suggestedFileName = run->suggestedFileName();
                disposition = run->serverSuggestsSave() ?
                                 KParts::BrowserRun::AttachmentDisposition :
                                 KParts::BrowserRun::InlineDisposition;
            }

            KParts::BrowserOpenOrSaveQuestion dlg( widget(), url, mimetype );
            dlg.setSuggestedFileName( suggestedFileName );
            const KParts::BrowserOpenOrSaveQuestion::Result res = dlg.askEmbedOrSave( disposition );

            switch( res ) {
            case KParts::BrowserOpenOrSaveQuestion::Save:
                KHTMLPopupGUIClient::saveURL( widget(), i18n( "Save As" ), url, child->m_args.metaData(), QString(), 0, suggestedFileName );
                // fall-through
            case KParts::BrowserOpenOrSaveQuestion::Cancel:
                child->m_bCompleted = true;
                checkCompleted();
                return true; // done
            default: // Embed
                break;
            }
        }

        // Now, for frames and iframes, we always create a KHTMLPart anyway,
        // doing it in advance when registering the frame. So we want the
        // actual creation only for objects here.
        if ( child->m_type == khtml::ChildFrame::Object ) {
            QMimeType mime = db.mimeTypeForName(mimetype);
            if (mime.isValid()) {
                // Even for objects, however, we want to force a KHTMLPart for
                // html & xml, even  if the normally preferred part is another one,
                // so that we can script the target natively via contentDocument method.
                if (mime.inherits("text/html")
                    || mime.inherits("application/xml")) { // this includes xhtml and svg
                    child->m_serviceName = "khtml";
                } else {
                    if (!pluginsEnabled()) {
                        childLoadFailure(child);
                        return false;
                    }
                }
            }

            QStringList dummy; // the list of servicetypes handled by the part is now unused.
            KParts::ReadOnlyPart *part = createPart( d->m_view->viewport(), this, mimetype, child->m_serviceName, dummy, child->m_params );

            if ( !part ) {
                childLoadFailure(child);
                return false;
            }

            connectToChildPart( child, part, mimetype );
        }
    }

    checkEmitLoadEvent();

    // Some JS code in the load event may have destroyed the part
    // In that case, abort
    if ( !child->m_part )
        return false;

    if ( child->m_bPreloaded ) {
        if ( child->m_partContainerElement && child->m_part )
            child->m_partContainerElement.data()->setWidget( child->m_part.data()->widget() );

        child->m_bPreloaded = false;
        return true;
    }

    // reload/soft-reload arguments are always inherited from parent
    child->m_args.setReload( arguments().reload() );
    child->m_browserArgs.softReload = d->m_extension->browserArguments().softReload;

    // make sure the part has a way to find out about the mimetype.
    // we actually set it in child->m_args in requestObject already,
    // but it's useless if we had to use a KHTMLRun instance, as the
    // point the run object is to find out exactly the mimetype.
    child->m_args.setMimeType(mimetype);
    child->m_part.data()->setArguments( child->m_args );

    // if not a frame set child as completed
    // ### dubious.
    child->m_bCompleted = child->m_type == khtml::ChildFrame::Object;

    if ( child->m_extension )
        child->m_extension.data()->setBrowserArguments( child->m_browserArgs );

    return navigateChild( child, url );
}

bool KHTMLPart::navigateLocalProtocol( khtml::ChildFrame* /*child*/, KParts::ReadOnlyPart *inPart,
                                       const QUrl& url )
{
    if (!qobject_cast<KHTMLPart*>(inPart))
        return false;

    KHTMLPart* p = static_cast<KHTMLPart*>(static_cast<KParts::ReadOnlyPart *>(inPart));

    p->begin();

    // We may have to re-propagate the domain here if we go here due to navigation
    d->propagateInitialDomainAndBaseTo(p);

    // Support for javascript: sources
    if (d->isJavaScriptURL(url.toString())) {
        // See if we want to replace content with javascript: output..
        QVariant res = p->executeScript( DOM::Node(),
                                        d->codeForJavaScriptURL(url.toString()));
        if (res.type() == QVariant::String && p->d->m_redirectURL.isEmpty()) {
            p->begin();
            p->setAlwaysHonourDoctype(); // Disable public API compat; it messes with doctype
            // We recreated the document, so propagate domain again.
            d->propagateInitialDomainAndBaseTo(p);
            p->write( res.toString() );
            p->end();
        }
    } else {
        p->setUrl(url);
        // we need a body element. testcase: <iframe id="a"></iframe><script>alert(a.document.body);</script>
        p->write("<HTML><TITLE></TITLE><BODY></BODY></HTML>");
    }
    p->end();
    // we don't need to worry about child completion explicitly for KHTMLPart...
    // or do we?
    return true;
}

bool KHTMLPart::navigateChild( khtml::ChildFrame *child, const QUrl& url )
{
    if (url.scheme() == "javascript" || url.toString() == "about:blank") {
        return navigateLocalProtocol(child, child->m_part.data(), url);
    } else if ( !url.isEmpty() ) {
        // qDebug() << "opening" << url << "in frame" << child->m_part;
        bool b = child->m_part.data()->openUrl( url );
        if (child->m_bCompleted)
            checkCompleted();
        return b;
    } else {
        // empty URL -> no need to navigate
        child->m_bCompleted = true;
        checkCompleted();
        return true;
    }
}

void KHTMLPart::connectToChildPart( khtml::ChildFrame *child, KParts::ReadOnlyPart *part,
                                    const QString& mimetype)
{
    // qDebug() << "we:" << this << "kid:" << child << part << mimetype;

    part->setObjectName( child->m_name );

    // Cleanup any previous part for this childframe and its connections
    if ( KParts::ReadOnlyPart* p = child->m_part.data() ) {
      if (!qobject_cast<KHTMLPart*>(p) && child->m_jscript)
          child->m_jscript->clear();
      partManager()->removePart( p );
      delete p;
      child->m_scriptable.clear();
    }

    child->m_part = part;

    child->m_serviceType = mimetype;
    if ( child->m_partContainerElement && part->widget() )
      child->m_partContainerElement.data()->setWidget( part->widget() );

    if ( child->m_type != khtml::ChildFrame::Object )
      partManager()->addPart( part, false );
//  else
//      qDebug() << "AH! NO FRAME!!!!!";

    if (qobject_cast<KHTMLPart*>(part)) {
      static_cast<KHTMLPart*>(part)->d->m_frame = child;
    } else if (child->m_partContainerElement) {
      // See if this can be scripted..
      KParts::ScriptableExtension* scriptExt = KParts::ScriptableExtension::childObject(part);
      if (!scriptExt) {
        // Try to fall back to LiveConnectExtension compat
        KParts::LiveConnectExtension* lc = KParts::LiveConnectExtension::childObject(part);
        if (lc)
            scriptExt = KParts::ScriptableExtension::adapterFromLiveConnect(part, lc);
      }

      if (scriptExt)
        scriptExt->setHost(d->m_scriptableExtension);
      child->m_scriptable = scriptExt;
    }
    KParts::StatusBarExtension *sb = KParts::StatusBarExtension::childObject(part);
    if (sb)
      sb->setStatusBar( d->m_statusBarExtension->statusBar() );

    connect( part, SIGNAL(started(KIO::Job*)),
             this, SLOT(slotChildStarted(KIO::Job*)) );
    connect( part, SIGNAL(completed()),
             this, SLOT(slotChildCompleted()) );
    connect( part, SIGNAL(completed(bool)),
             this, SLOT(slotChildCompleted(bool)) );
    connect( part, SIGNAL(setStatusBarText(QString)),
                this, SIGNAL(setStatusBarText(QString)) );
    if ( part->inherits( "KHTMLPart" ) )
    {
      connect( this, SIGNAL(completed()),
               part, SLOT(slotParentCompleted()) );
      connect( this, SIGNAL(completed(bool)),
               part, SLOT(slotParentCompleted()) );
      // As soon as the child's document is created, we need to set its domain
      // (but we do so only once, so it can't be simply done in the child)
      connect( part, SIGNAL(docCreated()),
               this, SLOT(slotChildDocCreated()) );
    }

    child->m_extension = KParts::BrowserExtension::childObject( part );

    if ( KParts::BrowserExtension* kidBrowserExt = child->m_extension.data() )
    {
      connect( kidBrowserExt, SIGNAL(openUrlNotify()),
               d->m_extension, SIGNAL(openUrlNotify()) );

      connect( kidBrowserExt, SIGNAL(openUrlRequestDelayed(QUrl,KParts::OpenUrlArguments,KParts::BrowserArguments)),
               this, SLOT(slotChildURLRequest(QUrl,KParts::OpenUrlArguments,KParts::BrowserArguments)) );

      connect( kidBrowserExt, SIGNAL(createNewWindow(QUrl,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::WindowArgs,KParts::ReadOnlyPart**)),
               d->m_extension, SIGNAL(createNewWindow(QUrl,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::WindowArgs,KParts::ReadOnlyPart**)) );

      connect( kidBrowserExt, SIGNAL(popupMenu(QPoint,KFileItemList,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)),
             d->m_extension, SIGNAL(popupMenu(QPoint,KFileItemList,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)) );
      connect( kidBrowserExt, SIGNAL(popupMenu(QPoint,QUrl,mode_t,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)),
             d->m_extension, SIGNAL(popupMenu(QPoint,QUrl,mode_t,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)) );

      connect( kidBrowserExt, SIGNAL(infoMessage(QString)),
               d->m_extension, SIGNAL(infoMessage(QString)) );

      connect( kidBrowserExt, SIGNAL(requestFocus(KParts::ReadOnlyPart*)),
               this, SLOT(slotRequestFocus(KParts::ReadOnlyPart*)) );

      kidBrowserExt->setBrowserInterface( d->m_extension->browserInterface() );
    }
}

KParts::ReadOnlyPart *KHTMLPart::createPart( QWidget *parentWidget,
                                             QObject *parent, const QString &mimetype,
                                             QString &serviceName, QStringList &serviceTypes,
                                             const QStringList &params )
{
  QString constr;
  if ( !serviceName.isEmpty() )
    constr.append( QString::fromLatin1( "DesktopEntryName == '%1'" ).arg( serviceName ) );

  KService::List offers = KMimeTypeTrader::self()->query( mimetype, "KParts/ReadOnlyPart", constr );

  if ( offers.isEmpty() ) {
    int pos = mimetype.indexOf( "-plugin" );
    if (pos < 0)
        return 0L;
    QString stripped_mime = mimetype.left( pos );
    offers = KMimeTypeTrader::self()->query( stripped_mime, "KParts/ReadOnlyPart", constr );
    if ( offers.isEmpty() )
        return 0L;
  }

  KService::List::ConstIterator it = offers.constBegin();
  const KService::List::ConstIterator itEnd = offers.constEnd();
  for ( ; it != itEnd; ++it )
  {
    KService::Ptr service = (*it);

    KPluginLoader loader(*service);
    KPluginFactory* const factory = loader.factory();
    if ( factory ) {
      // Turn params into a QVariantList as expected by KPluginFactory
      QVariantList variantlist;
      Q_FOREACH(const QString& str, params)
          variantlist << QVariant(str);

      if ( service->serviceTypes().contains( "Browser/View" ) )
        variantlist << QString("Browser/View");

      KParts::ReadOnlyPart* part = factory->create<KParts::ReadOnlyPart>(parentWidget, parent, QString(), variantlist);
      if ( part ) {
        serviceTypes = service->serviceTypes();
        serviceName = service->name();
        return part;
      }
    } else {
      // TODO KMessageBox::error and i18n, like in KonqFactory::createView?
      qWarning() << QString("There was an error loading the module %1.\nThe diagnostics is:\n%2")
                      .arg(service->name()).arg(loader.errorString());
    }
  }
  return 0;
}

KParts::PartManager *KHTMLPart::partManager()
{
  if ( !d->m_manager && d->m_view )
  {
    d->m_manager = new KParts::PartManager( d->m_view->topLevelWidget(), this );
    d->m_manager->setObjectName( "khtml part manager" );
    d->m_manager->setAllowNestedParts( true );
    connect( d->m_manager, SIGNAL(activePartChanged(KParts::Part*)),
             this, SLOT(slotActiveFrameChanged(KParts::Part*)) );
    connect( d->m_manager, SIGNAL(partRemoved(KParts::Part*)),
             this, SLOT(slotPartRemoved(KParts::Part*)) );
  }

  return d->m_manager;
}

void KHTMLPart::submitFormAgain()
{
  disconnect(this, SIGNAL(completed()), this, SLOT(submitFormAgain()));
  if( d->m_doc && !d->m_doc->parsing() && d->m_submitForm)
    KHTMLPart::submitForm( d->m_submitForm->submitAction, d->m_submitForm->submitUrl, d->m_submitForm->submitFormData, d->m_submitForm->target, d->m_submitForm->submitContentType, d->m_submitForm->submitBoundary );

  delete d->m_submitForm;
  d->m_submitForm = 0;
}

void KHTMLPart::submitFormProxy( const char *action, const QString &url, const QByteArray &formData, const QString &_target, const QString& contentType, const QString& boundary )
{
  submitForm(action, url, formData, _target, contentType, boundary);
}

void KHTMLPart::submitForm( const char *action, const QString &url, const QByteArray &formData, const QString &_target, const QString& contentType, const QString& boundary )
{
  // qDebug() << this << "target=" << _target << "url=" << url;
  if (d->m_formNotification == KHTMLPart::Only) {
    emit formSubmitNotification(action, url, formData, _target, contentType, boundary);
    return;
  } else if (d->m_formNotification == KHTMLPart::Before) {
    emit formSubmitNotification(action, url, formData, _target, contentType, boundary);
  }

  QUrl u = completeURL( url );

  if ( !u.isValid() )
  {
    // ### ERROR HANDLING!
    return;
  }

  // Form security checks
  //
  /*
   * If these form security checks are still in this place in a month or two
   * I'm going to simply delete them.
   */

  /* This is separate for a reason.  It has to be _before_ all script, etc,
   * AND I don't want to break anything that uses checkLinkSecurity() in
   * other places.
   */

  if (!d->m_submitForm) {
    if (u.scheme() != "https" && u.scheme() != "mailto") {
      if (d->m_ssl_in_use) {    // Going from SSL -> nonSSL
        int rc = KMessageBox::warningContinueCancel(NULL, i18n("Warning:  This is a secure form but it is attempting to send your data back unencrypted."
                                                               "\nA third party may be able to intercept and view this information."
                                                               "\nAre you sure you wish to continue?"),
                                                    i18n("Network Transmission"),KGuiItem(i18n("&Send Unencrypted")));
        if (rc == KMessageBox::Cancel)
          return;
      } else {                  // Going from nonSSL -> nonSSL
        KSSLSettings kss(true);
        if (kss.warnOnUnencrypted()) {
          int rc = KMessageBox::warningContinueCancel(NULL,
                                                      i18n("Warning: Your data is about to be transmitted across the network unencrypted."
                                                           "\nAre you sure you wish to continue?"),
                                                      i18n("Network Transmission"),
                                                      KGuiItem(i18n("&Send Unencrypted")),
                                                      KStandardGuiItem::cancel(),
                                                      "WarnOnUnencryptedForm");
          // Move this setting into KSSL instead
          QString grpNotifMsgs = QLatin1String("Notification Messages");
          KConfigGroup cg( KSharedConfig::openConfig(), grpNotifMsgs );

          if (!cg.readEntry("WarnOnUnencryptedForm", true)) {
            cg.deleteEntry("WarnOnUnencryptedForm");
            cg.sync();
            kss.setWarnOnUnencrypted(false);
            kss.save();
          }
          if (rc == KMessageBox::Cancel)
            return;
        }
      }
    }

    if (u.scheme() == "mailto") {
      int rc = KMessageBox::warningContinueCancel(NULL,
                                                  i18n("This site is attempting to submit form data via email.\n"
                                                       "Do you want to continue?"),
                                                  i18n("Network Transmission"),
                                                  KGuiItem(i18n("&Send Email")),
                                                  KStandardGuiItem::cancel(),
                                                  "WarnTriedEmailSubmit");

      if (rc == KMessageBox::Cancel) {
        return;
      }
    }
  }

  // End form security checks
  //

  QString urlstring = u.toString();

  if ( d->isJavaScriptURL(urlstring) ) {
    crossFrameExecuteScript( _target, d->codeForJavaScriptURL(urlstring) );
    return;
  }

  if (!checkLinkSecurity(u,
                         ki18n( "<qt>The form will be submitted to <br /><b>%1</b><br />on your local filesystem.<br />Do you want to submit the form?</qt>" ),
                         i18n( "Submit" )))
    return;

  // OK. We're actually going to submit stuff. Clear any redirections,
  // we should win over them
  d->clearRedirection();

  KParts::OpenUrlArguments args;

  if (!d->m_referrer.isEmpty())
     args.metaData()["referrer"] = d->m_referrer;

  args.metaData().insert("PropagateHttpHeader", "true");
  args.metaData().insert("ssl_parent_ip", d->m_ssl_parent_ip);
  args.metaData().insert("ssl_parent_cert", d->m_ssl_parent_cert);
  args.metaData().insert("main_frame_request",
                         parentPart() == 0 ? "TRUE":"FALSE");
  args.metaData().insert("ssl_was_in_use", d->m_ssl_in_use ? "TRUE":"FALSE");
  args.metaData().insert("ssl_activate_warnings", "TRUE");
//WABA: When we post a form we should treat it as the main url
//the request should never be considered cross-domain
//args.metaData().insert("cross-domain", toplevelURL().toString());
  KParts::BrowserArguments browserArgs;
  browserArgs.frameName = _target.isEmpty() ? d->m_doc->baseTarget() : _target ;

  // Handle mailto: forms
  if (u.scheme() == "mailto") {
      // 1)  Check for attach= and strip it
      QString q = u.query().mid(1);
      QStringList nvps = q.split("&");
      bool triedToAttach = false;

      QStringList::Iterator nvp = nvps.begin();
      const QStringList::Iterator nvpEnd = nvps.end();

// cannot be a for loop as if something is removed we don't want to do ++nvp, as
// remove returns an iterator pointing to the next item

      while (nvp != nvpEnd) {
         const QStringList pair = (*nvp).split("=");
         if (pair.count() >= 2) {
            if (pair.first().toLower() == "attach") {
               nvp = nvps.erase(nvp);
               triedToAttach = true;
            } else {
               ++nvp;
            }
         } else {
            ++nvp;
         }
      }

      if (triedToAttach)
         KMessageBox::information(NULL, i18n("This site attempted to attach a file from your computer in the form submission. The attachment was removed for your protection."), i18n("KDE"), "WarnTriedAttach");

      // 2)  Append body=
      QString bodyEnc;
      if (contentType.toLower() == "multipart/form-data") {
         // FIXME: is this correct?  I suspect not
         bodyEnc = QLatin1String( QUrl::toPercentEncoding(QString::fromLatin1(formData.data(),
                                                           formData.size())));
      } else if (contentType.toLower() == "text/plain") {
         // Convention seems to be to decode, and s/&/\n/
         QString tmpbody = QString::fromLatin1(formData.data(),
                                               formData.size());
         tmpbody.replace(QRegExp("[&]"), "\n");
         tmpbody.replace(QRegExp("[+]"), " ");
         tmpbody = QUrl::fromPercentEncoding(tmpbody.toLatin1());  // Decode the rest of it
         bodyEnc = QLatin1String( QUrl::toPercentEncoding(tmpbody) );  // Recode for the URL
      } else {
         bodyEnc = QLatin1String( QUrl::toPercentEncoding(QString::fromLatin1(formData.data(),
                                                           formData.size())) );
      }

      nvps.append(QString("body=%1").arg(bodyEnc));
      q = nvps.join("&");
      u.setQuery(q);
  }

  if ( strcmp( action, "get" ) == 0 ) {
    if (u.scheme() != "mailto")
       u.setQuery( QString::fromLatin1( formData.data(), formData.size() ) );
    browserArgs.setDoPost( false );
  }
  else {
    browserArgs.postData = formData;
    browserArgs.setDoPost( true );

    // construct some user headers if necessary
    if (contentType.isNull() || contentType == "application/x-www-form-urlencoded")
      browserArgs.setContentType( "Content-Type: application/x-www-form-urlencoded" );
    else // contentType must be "multipart/form-data"
      browserArgs.setContentType( "Content-Type: " + contentType + "; boundary=" + boundary );
  }

  if ( d->m_doc->parsing() || d->m_runningScripts > 0 ) {
    if( d->m_submitForm ) {
      // qDebug() << "ABORTING!";
      return;
    }
    d->m_submitForm = new KHTMLPartPrivate::SubmitForm;
    d->m_submitForm->submitAction = action;
    d->m_submitForm->submitUrl = url;
    d->m_submitForm->submitFormData = formData;
    d->m_submitForm->target = _target;
    d->m_submitForm->submitContentType = contentType;
    d->m_submitForm->submitBoundary = boundary;
    connect(this, SIGNAL(completed()), this, SLOT(submitFormAgain()));
  }
  else
  {
    emit d->m_extension->openUrlRequest( u, args, browserArgs );
  }
}

void KHTMLPart::popupMenu( const QString &linkUrl )
{
  QUrl popupURL;
  QUrl linkKUrl;
  KParts::OpenUrlArguments args;
  KParts::BrowserArguments browserArgs;
  QString referrer;
  KParts::BrowserExtension::PopupFlags itemflags=KParts::BrowserExtension::ShowBookmark | KParts::BrowserExtension::ShowReload;

  if ( linkUrl.isEmpty() ) { // click on background
    KHTMLPart* khtmlPart = this;
    while ( khtmlPart->parentPart() )
    {
      khtmlPart=khtmlPart->parentPart();
    }
    popupURL = khtmlPart->url();
    referrer = khtmlPart->pageReferrer();
    if (hasSelection())
      itemflags = KParts::BrowserExtension::ShowTextSelectionItems;
    else
      itemflags |= KParts::BrowserExtension::ShowNavigationItems;
  } else {               // click on link
    popupURL = completeURL( linkUrl );
    linkKUrl = popupURL;
    referrer = this->referrer();
    itemflags |= KParts::BrowserExtension::IsLink;

    if (!(d->m_strSelectedURLTarget).isEmpty() &&
           (d->m_strSelectedURLTarget.toLower() != "_top") &&
           (d->m_strSelectedURLTarget.toLower() != "_self") &&
           (d->m_strSelectedURLTarget.toLower() != "_parent")) {
      if (d->m_strSelectedURLTarget.toLower() == "_blank")
        browserArgs.setForcesNewWindow(true);
      else {
        KHTMLPart *p = this;
        while (p->parentPart())
          p = p->parentPart();
        if (!p->frameExists(d->m_strSelectedURLTarget))
          browserArgs.setForcesNewWindow(true);
      }
    }
  }

  QMimeDatabase db;

  // Danger, Will Robinson. The Popup might stay around for a much
  // longer time than KHTMLPart. Deal with it.
  KHTMLPopupGUIClient* client = new KHTMLPopupGUIClient( this, linkKUrl );
  QPointer<QObject> guard( client );

  QString mimetype = QLatin1String( "text/html" );
  args.metaData()["referrer"] = referrer;

  if (!linkUrl.isEmpty())                                // over a link
  {
    if (popupURL.isLocalFile())                                // safe to do this
    {
      mimetype = db.mimeTypeForUrl(popupURL).name();
    }
    else                                                // look at "extension" of link
    {
      const QString fname(popupURL.fileName());
      if (!fname.isEmpty() && !popupURL.hasFragment() && popupURL.query().isEmpty())
      {
        QMimeType pmt = db.mimeTypeForFile(fname, QMimeDatabase::MatchExtension);

        // Further check for mime types guessed from the extension which,
        // on a web page, are more likely to be a script delivering content
        // of undecidable type. If the mime type from the extension is one
        // of these, don't use it.  Retain the original type 'text/html'.
        if (!pmt.isDefault() &&
            !pmt.inherits("application/x-perl") &&
            !pmt.inherits("application/x-perl-module") &&
            !pmt.inherits("application/x-php") &&
            !pmt.inherits("application/x-python-bytecode") &&
            !pmt.inherits("application/x-python") &&
            !pmt.inherits("application/x-shellscript"))
          mimetype = pmt.name();
      }
    }
  }

  args.setMimeType(mimetype);

  emit d->m_extension->popupMenu( QCursor::pos(), popupURL, S_IFREG /*always a file*/,
                                  args, browserArgs, itemflags,
                                  client->actionGroups() );

  if ( !guard.isNull() ) {
     delete client;
     emit popupMenu(linkUrl, QCursor::pos());
     d->m_strSelectedURL.clear();
     d->m_strSelectedURLTarget.clear();
  }
}

void KHTMLPart::slotParentCompleted()
{
  //qDebug() << this;
  if ( !d->m_redirectURL.isEmpty() && !d->m_redirectionTimer.isActive() )
  {
    //qDebug() << this << ": starting timer for child redirection -> " << d->m_redirectURL;
    d->m_redirectionTimer.setSingleShot( true );
    d->m_redirectionTimer.start( qMax(0, 1000 * d->m_delayRedirect) );
  }
}

void KHTMLPart::slotChildStarted( KIO::Job *job )
{
  khtml::ChildFrame *child = frame( sender() );

  assert( child );

  child->m_bCompleted = false;

  if ( d->m_bComplete )
  {
#if 0
    // WABA: Looks like this belongs somewhere else
    if ( !parentPart() ) // "toplevel" html document? if yes, then notify the hosting browser about the document (url) changes
    {
      emit d->m_extension->openURLNotify();
    }
#endif
    d->m_bComplete = false;
    emit started( job );
  }
}

void KHTMLPart::slotChildCompleted()
{
  slotChildCompleted( false );
}

void KHTMLPart::slotChildCompleted( bool pendingAction )
{
  khtml::ChildFrame *child = frame( sender() );

  if ( child ) {
    // qDebug() << this << "child=" << child << "m_partContainerElement=" << child->m_partContainerElement;
    child->m_bCompleted = true;
    child->m_bPendingRedirection = pendingAction;
    child->m_args = KParts::OpenUrlArguments();
    child->m_browserArgs = KParts::BrowserArguments();
    // dispatch load event. We don't do that for KHTMLPart's since their internal
    // load will be forwarded inside NodeImpl::dispatchWindowEvent
    if (!qobject_cast<KHTMLPart*>(child->m_part))
        QTimer::singleShot(0, child->m_partContainerElement.data(), SLOT(slotEmitLoadEvent()));
  }
  checkCompleted();
}

void KHTMLPart::slotChildDocCreated()
{
  // Set domain to the frameset's domain
  // This must only be done when loading the frameset initially (#22039),
  // not when following a link in a frame (#44162).
  if (KHTMLPart* htmlFrame = qobject_cast<KHTMLPart*>(sender()))
      d->propagateInitialDomainAndBaseTo(htmlFrame);

  // So it only happens once
  disconnect( sender(), SIGNAL(docCreated()), this, SLOT(slotChildDocCreated()) );
}

void KHTMLPartPrivate::propagateInitialDomainAndBaseTo(KHTMLPart* kid)
{
    // This method is used to propagate our domain and base information for
    // child frames, to provide them for about: or JavaScript: URLs
    if ( m_doc && kid->d->m_doc ) {
        DocumentImpl* kidDoc = kid->d->m_doc;
        if ( kidDoc->origin()->isEmpty() ) {
            kidDoc->setOrigin ( m_doc->origin() );
            kidDoc->setBaseURL( m_doc->baseURL() );
        }
    }
}

void KHTMLPart::slotChildURLRequest( const QUrl &url, const KParts::OpenUrlArguments& args, const KParts::BrowserArguments &browserArgs )
{
  khtml::ChildFrame *child = frame( sender()->parent() );
  KHTMLPart *callingHtmlPart = const_cast<KHTMLPart *>(dynamic_cast<const KHTMLPart *>(sender()->parent()));

  // TODO: handle child target correctly! currently the script are always executed for the parent
  QString urlStr = url.toString();
  if ( d->isJavaScriptURL(urlStr) ) {
      executeScript( DOM::Node(), d->codeForJavaScriptURL(urlStr) );
      return;
  }

  QString frameName = browserArgs.frameName.toLower();
  if ( !frameName.isEmpty() ) {
    if ( frameName == QLatin1String( "_top" ) )
    {
      emit d->m_extension->openUrlRequest( url, args, browserArgs );
      return;
    }
    else if ( frameName == QLatin1String( "_blank" ) )
    {
      emit d->m_extension->createNewWindow( url, args, browserArgs );
      return;
    }
    else if ( frameName == QLatin1String( "_parent" ) )
    {
      KParts::BrowserArguments newBrowserArgs( browserArgs );
      newBrowserArgs.frameName.clear();
      emit d->m_extension->openUrlRequest( url, args, newBrowserArgs );
      return;
    }
    else if ( frameName != QLatin1String( "_self" ) )
    {
      khtml::ChildFrame *_frame = recursiveFrameRequest( callingHtmlPart, url, args, browserArgs );

      if ( !_frame )
      {
        emit d->m_extension->openUrlRequest( url, args, browserArgs );
        return;
      }

      child = _frame;
    }
  }

  if ( child && child->m_type != khtml::ChildFrame::Object ) {
      // Inform someone that we are about to show something else.
      child->m_bNotify = true;
      requestObject( child, url, args, browserArgs );
  }  else if ( frameName== "_self" ) // this is for embedded objects (via <object>) which want to replace the current document
  {
      KParts::BrowserArguments newBrowserArgs( browserArgs );
      newBrowserArgs.frameName.clear();
      emit d->m_extension->openUrlRequest( url, args, newBrowserArgs );
  }
}

void KHTMLPart::slotRequestFocus( KParts::ReadOnlyPart * )
{
  emit d->m_extension->requestFocus(this);
}

khtml::ChildFrame *KHTMLPart::frame( const QObject *obj )
{
    assert( obj->inherits( "KParts::ReadOnlyPart" ) );
    const KParts::ReadOnlyPart* const part = static_cast<const KParts::ReadOnlyPart *>( obj );

    FrameIt it = d->m_frames.begin();
    const FrameIt end = d->m_frames.end();
    for (; it != end; ++it ) {
        if ((*it)->m_part.data() == part )
            return *it;
    }

    FrameIt oi = d->m_objects.begin();
    const FrameIt oiEnd = d->m_objects.end();
    for (; oi != oiEnd; ++oi ) {
        if ((*oi)->m_part.data() == part)
            return *oi;
    }

    return 0L;
}

//#define DEBUG_FINDFRAME

bool KHTMLPart::checkFrameAccess(KHTMLPart *callingHtmlPart)
{
  if (callingHtmlPart == this)
    return true; // trivial

  if (!xmlDocImpl()) {
#ifdef DEBUG_FINDFRAME
    qDebug() << "Empty part" << this << "URL = " << url();
#endif
    return false; // we are empty?
  }

  // now compare the domains
  if (callingHtmlPart && callingHtmlPart->xmlDocImpl() && xmlDocImpl())  {
    khtml::SecurityOrigin* actDomain = callingHtmlPart->xmlDocImpl()->origin();
    khtml::SecurityOrigin* destDomain = xmlDocImpl()->origin();

    if (actDomain->canAccess(destDomain))
      return true;
  }
#ifdef DEBUG_FINDFRAME
  else
  {
    qDebug() << "Unknown part/domain" << callingHtmlPart << "tries to access part" << this;
  }
#endif
  return false;
}

KHTMLPart *
KHTMLPart::findFrameParent( KParts::ReadOnlyPart *callingPart, const QString &f, khtml::ChildFrame **childFrame )
{
    return d->findFrameParent(callingPart, f, childFrame, false);
}

KHTMLPart* KHTMLPartPrivate::findFrameParent(KParts::ReadOnlyPart* callingPart,
                                             const QString& f, khtml::ChildFrame **childFrame, bool checkForNavigation)
{
#ifdef DEBUG_FINDFRAME
    qDebug() << q << "URL =" << q->url() << "name =" << q->objectName() << "findFrameParent(" << f << ")";
#endif
    // Check access
    KHTMLPart* const callingHtmlPart = qobject_cast<KHTMLPart *>(callingPart);

    if (!callingHtmlPart)
        return 0;

    if (!checkForNavigation && !q->checkFrameAccess(callingHtmlPart))
        return 0;

    if (!childFrame && !q->parentPart() && (q->objectName() == f)) {
        if (!checkForNavigation || callingHtmlPart->d->canNavigate(q))
            return q;
    }

    FrameIt it = m_frames.find( f );
    const FrameIt end = m_frames.end();
    if ( it != end )
    {
#ifdef DEBUG_FINDFRAME
        qDebug() << "FOUND!";
#endif
        if (!checkForNavigation || callingHtmlPart->d->canNavigate((*it)->m_part.data())) {
            if (childFrame)
                *childFrame = *it;
            return q;
        }
    }

    it = m_frames.begin();
    for (; it != end; ++it )
    {
        if ( KHTMLPart* p = qobject_cast<KHTMLPart*>((*it)->m_part.data()) )
        {
            KHTMLPart* const frameParent = p->d->findFrameParent(callingPart, f, childFrame, checkForNavigation);
            if (frameParent)
                return frameParent;
        }
    }
    return 0;
}

KHTMLPart* KHTMLPartPrivate::top()
{
    KHTMLPart* t = q;
    while (t->parentPart())
        t = t->parentPart();
    return t;
}

bool KHTMLPartPrivate::canNavigate(KParts::ReadOnlyPart* bCand)
{
    if (!bCand) // No part here (e.g. invalid url), reuse that frame
        return true;

    KHTMLPart* b = qobject_cast<KHTMLPart*>(bCand);
    if (!b) // Another kind of part? Not sure what to do...
        return false;

    // HTML5 gives conditions for this (a) being able to navigate b

    // 1) Same domain
    if (q->checkFrameAccess(b))
        return true;

    // 2) A is nested, with B its top
    if (q->parentPart() && top() == b)
        return true;

    // 3) B is 'auxilary' -- window.open with opener,
    // and A can navigate B's opener
    if (b->opener() && canNavigate(b->opener()))
        return true;

    // 4) B is not top-level, but an ancestor of it has same origin as A
    for (KHTMLPart* anc = b->parentPart(); anc; anc = anc->parentPart()) {
        if (anc->checkFrameAccess(q))
            return true;
    }

    return false;
}

KHTMLPart *KHTMLPart::findFrame( const QString &f )
{
  khtml::ChildFrame *childFrame;
  KHTMLPart *parentFrame = findFrameParent(this, f, &childFrame);
  if (parentFrame)
     return qobject_cast<KHTMLPart*>(childFrame->m_part.data());

  return 0;
}

KParts::ReadOnlyPart *KHTMLPart::findFramePart(const QString &f)
{
  khtml::ChildFrame *childFrame;
  return findFrameParent(this, f, &childFrame) ? childFrame->m_part.data() : 0L;
}

KParts::ReadOnlyPart *KHTMLPart::currentFrame() const
{
  KParts::ReadOnlyPart* part = (KParts::ReadOnlyPart*)(this);
  // Find active part in our frame manager, in case we are a frameset
  // and keep doing that (in case of nested framesets).
  // Just realized we could also do this recursively, calling part->currentFrame()...
  while ( part && part->inherits("KHTMLPart") &&
          static_cast<KHTMLPart *>(part)->d->m_frames.count() > 0 ) {
    KHTMLPart* frameset = static_cast<KHTMLPart *>(part);
    part = static_cast<KParts::ReadOnlyPart *>(frameset->partManager()->activePart());
    if ( !part ) return frameset;
  }
  return part;
}

bool KHTMLPart::frameExists( const QString &frameName )
{
  FrameIt it = d->m_frames.find( frameName );
  if ( it == d->m_frames.end() )
    return false;

  // WABA: We only return true if the child actually has a frame
  // set. Otherwise we might find our preloaded-selve.
  // This happens when we restore the frameset.
  return (!(*it)->m_partContainerElement.isNull());
}

void KHTMLPartPrivate::renameFrameForContainer(DOM::HTMLPartContainerElementImpl* cont,
                                               const QString& newName)
{
    for (int i = 0; i < m_frames.size(); ++i) {
        khtml::ChildFrame* f = m_frames[i];
        if (f->m_partContainerElement.data() == cont)
            f->m_name = newName;
    }
}

KJSProxy *KHTMLPart::framejScript(KParts::ReadOnlyPart *framePart)
{
  KHTMLPart* const kp = qobject_cast<KHTMLPart*>(framePart);
  if (kp)
    return kp->jScript();

  FrameIt it = d->m_frames.begin();
  const FrameIt itEnd = d->m_frames.end();

  for (; it != itEnd; ++it) {
    khtml::ChildFrame* frame = *it;
    if (framePart == frame->m_part.data()) {
      if (!frame->m_jscript)
        frame->m_jscript = new KJSProxy(frame);
      return frame->m_jscript;
    }
  }
  return 0L;
}

KHTMLPart *KHTMLPart::parentPart()
{
  return qobject_cast<KHTMLPart*>( parent() );
}

khtml::ChildFrame *KHTMLPart::recursiveFrameRequest( KHTMLPart *callingHtmlPart, const QUrl &url,
                                                     const KParts::OpenUrlArguments &args,
                                                     const KParts::BrowserArguments &browserArgs, bool callParent )
{
#ifdef DEBUG_FINDFRAME
  qDebug() << this << "frame = " << browserArgs.frameName << "url = " << url;
#endif
  khtml::ChildFrame *childFrame;
  KHTMLPart *childPart = findFrameParent(callingHtmlPart, browserArgs.frameName, &childFrame);
  if (childPart)
  {
     if (childPart == this)
        return childFrame;

     childPart->requestObject( childFrame, url, args, browserArgs );
     return 0;
  }

  if ( parentPart() && callParent )
  {
     khtml::ChildFrame *res = parentPart()->recursiveFrameRequest( callingHtmlPart, url, args, browserArgs, callParent );

     if ( res )
       parentPart()->requestObject( res, url, args, browserArgs );
  }

  return 0L;
}

#ifdef DEBUG_SAVESTATE
static int s_saveStateIndentLevel = 0;
#endif

void KHTMLPart::saveState( QDataStream &stream )
{
#ifdef DEBUG_SAVESTATE
  QString indent= QString().leftJustified( s_saveStateIndentLevel * 4, ' ' );
  const int indentLevel = s_saveStateIndentLevel++;
  qDebug() << indent << "saveState this=" << this << " '" << objectName() << "' saving URL " << url();
#endif

  stream << url() << (qint32)d->m_view->contentsX() << (qint32)d->m_view->contentsY()
         << (qint32) d->m_view->contentsWidth() << (qint32) d->m_view->contentsHeight() << (qint32) d->m_view->marginWidth() << (qint32) d->m_view->marginHeight();

  // save link cursor position
  int focusNodeNumber;
  if (!d->m_focusNodeRestored)
      focusNodeNumber = d->m_focusNodeNumber;
  else if (d->m_doc && d->m_doc->focusNode())
      focusNodeNumber = d->m_doc->nodeAbsIndex(d->m_doc->focusNode());
  else
      focusNodeNumber = -1;
  stream << focusNodeNumber;

  // Save the doc's cache id.
  stream << d->m_cacheId;

  // Save the state of the document (Most notably the state of any forms)
  QStringList docState;
  if (d->m_doc)
  {
     docState = d->m_doc->docState();
  }
  stream << d->m_encoding << d->m_sheetUsed << docState;

  stream << d->m_zoomFactor;
  stream << d->m_fontScaleFactor;

  stream << d->m_httpHeaders;
  stream << d->m_pageServices;
  stream << d->m_pageReferrer;

  // Save ssl data
  stream << d->m_ssl_in_use
         << d->m_ssl_peer_chain
         << d->m_ssl_peer_ip
         << d->m_ssl_cipher
         << d->m_ssl_protocol_version
         << d->m_ssl_cipher_used_bits
         << d->m_ssl_cipher_bits
         << d->m_ssl_cert_errors
         << d->m_ssl_parent_ip
         << d->m_ssl_parent_cert;


  QStringList frameNameLst, frameServiceTypeLst, frameServiceNameLst;
  QList<QUrl> frameURLLst;
  QList<QByteArray> frameStateBufferLst;
  QList<int> frameTypeLst;

  ConstFrameIt it = d->m_frames.constBegin();
  const ConstFrameIt end = d->m_frames.constEnd();
  for (; it != end; ++it )
  {
    if ( !(*it)->m_part )
       continue;

    frameNameLst << (*it)->m_name;
    frameServiceTypeLst << (*it)->m_serviceType;
    frameServiceNameLst << (*it)->m_serviceName;
    frameURLLst << (*it)->m_part.data()->url();

    QByteArray state;
    QDataStream frameStream( &state, QIODevice::WriteOnly );

    if ( (*it)->m_extension )
      (*it)->m_extension.data()->saveState( frameStream );

    frameStateBufferLst << state;

    frameTypeLst << int( (*it)->m_type );
  }

  // Save frame data
  stream << (quint32) frameNameLst.count();
  stream << frameNameLst << frameServiceTypeLst << frameServiceNameLst << frameURLLst << frameStateBufferLst << frameTypeLst;
#ifdef DEBUG_SAVESTATE
  s_saveStateIndentLevel = indentLevel;
#endif
}

void KHTMLPart::restoreState( QDataStream &stream )
{
  QUrl u;
  qint32 xOffset, yOffset, wContents, hContents, mWidth, mHeight;
  quint32 frameCount;
  QStringList frameNames, frameServiceTypes, docState, frameServiceNames;
  QList<int> frameTypes;
  QList<QUrl> frameURLs;
  QList<QByteArray> frameStateBuffers;
  QList<int> fSizes;
  QString encoding, sheetUsed;
  long old_cacheId = d->m_cacheId;

  stream >> u >> xOffset >> yOffset >> wContents >> hContents >> mWidth >> mHeight;

  d->m_view->setMarginWidth( mWidth );
  d->m_view->setMarginHeight( mHeight );

  // restore link cursor position
  // nth node is active. value is set in checkCompleted()
  stream >> d->m_focusNodeNumber;
  d->m_focusNodeRestored = false;

  stream >> d->m_cacheId;

  stream >> encoding >> sheetUsed >> docState;

  d->m_encoding = encoding;
  d->m_sheetUsed = sheetUsed;

  int zoomFactor;
  stream >> zoomFactor;
  setZoomFactor(zoomFactor);

  int fontScaleFactor;
  stream >> fontScaleFactor;
  setFontScaleFactor(fontScaleFactor);

  stream >> d->m_httpHeaders;
  stream >> d->m_pageServices;
  stream >> d->m_pageReferrer;

  // Restore ssl data
  stream >> d->m_ssl_in_use
         >> d->m_ssl_peer_chain
         >> d->m_ssl_peer_ip
         >> d->m_ssl_cipher
         >> d->m_ssl_protocol_version
         >> d->m_ssl_cipher_used_bits
         >> d->m_ssl_cipher_bits
         >> d->m_ssl_cert_errors
         >> d->m_ssl_parent_ip
         >> d->m_ssl_parent_cert;

  setPageSecurity( d->m_ssl_in_use ? Encrypted : NotCrypted );

  stream >> frameCount >> frameNames >> frameServiceTypes >> frameServiceNames
         >> frameURLs >> frameStateBuffers >> frameTypes;

  d->m_bComplete = false;
  d->m_bLoadEventEmitted = false;

//   qDebug() << "docState.count() = " << docState.count();
//   qDebug() << "m_url " << url() << " <-> " << u;
//   qDebug() << "m_frames.count() " << d->m_frames.count() << " <-> " << frameCount;

  if (d->m_cacheId == old_cacheId && signed(frameCount) == d->m_frames.count())
  {
    // Partial restore
    d->m_redirectionTimer.stop();

    FrameIt fIt = d->m_frames.begin();
    const FrameIt fEnd = d->m_frames.end();

    for (; fIt != fEnd; ++fIt )
        (*fIt)->m_bCompleted = false;

    fIt = d->m_frames.begin();

    QStringList::ConstIterator fNameIt = frameNames.constBegin();
    QStringList::ConstIterator fServiceTypeIt = frameServiceTypes.constBegin();
    QStringList::ConstIterator fServiceNameIt = frameServiceNames.constBegin();
    QList<QUrl>::ConstIterator fURLIt = frameURLs.constBegin();
    QList<QByteArray>::ConstIterator fBufferIt = frameStateBuffers.constBegin();
    QList<int>::ConstIterator fFrameTypeIt = frameTypes.constBegin();

    for (; fIt != fEnd; ++fIt, ++fNameIt, ++fServiceTypeIt, ++fServiceNameIt, ++fURLIt, ++fBufferIt, ++fFrameTypeIt )
    {
      khtml::ChildFrame* const child = *fIt;

//      qDebug() <<  *fNameIt  << " ---- " <<  *fServiceTypeIt;

      if ( child->m_name != *fNameIt || child->m_serviceType != *fServiceTypeIt )
      {
        child->m_bPreloaded = true;
        child->m_name = *fNameIt;
        child->m_serviceName = *fServiceNameIt;
        child->m_type = static_cast<khtml::ChildFrame::Type>(*fFrameTypeIt);
        processObjectRequest( child, *fURLIt, *fServiceTypeIt );
      }
      if ( child->m_part )
      {
        child->m_bCompleted = false;
        if ( child->m_extension && !(*fBufferIt).isEmpty() )
        {
          QDataStream frameStream( *fBufferIt );
          child->m_extension.data()->restoreState( frameStream );
        }
        else
          child->m_part.data()->openUrl( *fURLIt );
      }
    }

    KParts::OpenUrlArguments args( arguments() );
    args.setXOffset(xOffset);
    args.setYOffset(yOffset);
    setArguments(args);

    KParts::BrowserArguments browserArgs( d->m_extension->browserArguments() );
    browserArgs.docState = docState;
    d->m_extension->setBrowserArguments(browserArgs);

    d->m_view->resizeContents( wContents, hContents );
    d->m_view->setContentsPos( xOffset, yOffset );

    setUrl(u);
  }
  else
  {
    // Full restore.
    closeUrl();
    // We must force a clear because we want to be sure to delete all
    // frames.
    d->m_bCleared = false;
    clear();
    d->m_encoding = encoding;
    d->m_sheetUsed = sheetUsed;

    QStringList::ConstIterator fNameIt = frameNames.constBegin();
    const QStringList::ConstIterator fNameEnd = frameNames.constEnd();

    QStringList::ConstIterator fServiceTypeIt = frameServiceTypes.constBegin();
    QStringList::ConstIterator fServiceNameIt = frameServiceNames.constBegin();
    QList<QUrl>::ConstIterator fURLIt = frameURLs.constBegin();
    QList<QByteArray>::ConstIterator fBufferIt = frameStateBuffers.constBegin();
    QList<int>::ConstIterator fFrameTypeIt = frameTypes.constBegin();

    for (; fNameIt != fNameEnd; ++fNameIt, ++fServiceTypeIt, ++fServiceNameIt, ++fURLIt, ++fBufferIt, ++fFrameTypeIt )
    {
      khtml::ChildFrame* const newChild = new khtml::ChildFrame;
      newChild->m_bPreloaded = true;
      newChild->m_name = *fNameIt;
      newChild->m_serviceName = *fServiceNameIt;
      newChild->m_type = static_cast<khtml::ChildFrame::Type>(*fFrameTypeIt);

//      qDebug() << *fNameIt << " ---- " << *fServiceTypeIt;

      const FrameIt childFrame = d->m_frames.insert( d->m_frames.end(), newChild );

      processObjectRequest( *childFrame, *fURLIt, *fServiceTypeIt );

      (*childFrame)->m_bPreloaded = true;

      if ( (*childFrame)->m_part )
      {
        if ( (*childFrame)->m_extension && !(*fBufferIt).isEmpty() )
        {
          QDataStream frameStream( *fBufferIt );
          (*childFrame)->m_extension.data()->restoreState( frameStream );
        }
        else
          (*childFrame)->m_part.data()->openUrl( *fURLIt );
      }
    }

    KParts::OpenUrlArguments args( arguments() );
    args.setXOffset(xOffset);
    args.setYOffset(yOffset);
    setArguments(args);

    KParts::BrowserArguments browserArgs( d->m_extension->browserArguments() );
    browserArgs.docState = docState;
    d->m_extension->setBrowserArguments(browserArgs);

    if (!KHTMLPageCache::self()->isComplete(d->m_cacheId))
    {
       d->m_restored = true;
       openUrl( u );
       d->m_restored = false;
    }
    else
    {
       restoreURL( u );
    }
  }

}

void KHTMLPart::show()
{
  if ( widget() )
    widget()->show();
}

void KHTMLPart::hide()
{
  if ( widget() )
    widget()->hide();
}

DOM::Node KHTMLPart::nodeUnderMouse() const
{
    return d->m_view->nodeUnderMouse();
}

DOM::Node KHTMLPart::nonSharedNodeUnderMouse() const
{
    return d->m_view->nonSharedNodeUnderMouse();
}

void KHTMLPart::emitSelectionChanged()
{
    // Don't emit signals about our selection if this is a frameset;
    // the active frame has the selection (#187403)
    if (!d->m_activeFrame)
    {
        emit d->m_extension->enableAction( "copy", hasSelection() );
        emit d->m_extension->selectionInfo( selectedText() );
        emit selectionChanged();
    }
}

int KHTMLPart::zoomFactor() const
{
  return d->m_zoomFactor;
}

// ### make the list configurable ?
static const int zoomSizes[] = { 20, 40, 60, 80, 90, 95, 100, 105, 110, 120, 140, 160, 180, 200, 250, 300 };
static const int zoomSizeCount = (sizeof(zoomSizes) / sizeof(int));
static const int minZoom = 20;
static const int maxZoom = 300;

// My idea of useful stepping ;-) (LS)
extern const int KHTML_NO_EXPORT fastZoomSizes[] = { 20, 50, 75, 90, 100, 120, 150, 200, 300 };
extern const int KHTML_NO_EXPORT fastZoomSizeCount = sizeof fastZoomSizes / sizeof fastZoomSizes[0];

void KHTMLPart::slotIncZoom()
{
  zoomIn(zoomSizes, zoomSizeCount);
}

void KHTMLPart::slotDecZoom()
{
  zoomOut(zoomSizes, zoomSizeCount);
}

void KHTMLPart::slotIncZoomFast()
{
  zoomIn(fastZoomSizes, fastZoomSizeCount);
}

void KHTMLPart::slotDecZoomFast()
{
  zoomOut(fastZoomSizes, fastZoomSizeCount);
}

void KHTMLPart::zoomIn(const int stepping[], int count)
{
  int zoomFactor = d->m_zoomFactor;

  if (zoomFactor < maxZoom) {
    // find the entry nearest to the given zoomsizes
    for (int i = 0; i < count; ++i)
      if (stepping[i] > zoomFactor) {
        zoomFactor = stepping[i];
        break;
      }
    setZoomFactor(zoomFactor);
  }
}

void KHTMLPart::zoomOut(const int stepping[], int count)
{
    int zoomFactor = d->m_zoomFactor;
    if (zoomFactor > minZoom) {
      // find the entry nearest to the given zoomsizes
      for (int i = count-1; i >= 0; --i)
        if (stepping[i] < zoomFactor) {
          zoomFactor = stepping[i];
          break;
        }
      setZoomFactor(zoomFactor);
    }
}

void KHTMLPart::setZoomFactor (int percent)
{
  // ### zooming under 100% is majorly botched,
  //     so disable that for now.
  if (percent < 100) percent = 100;
  // ### if (percent < minZoom) percent = minZoom;

  if (percent > maxZoom) percent = maxZoom;
  if (d->m_zoomFactor == percent) return;
  d->m_zoomFactor = percent;

  updateZoomFactor();
}


void KHTMLPart::updateZoomFactor ()
{
  if(d->m_view) {
    QApplication::setOverrideCursor( Qt::WaitCursor );
    d->m_view->setZoomLevel( d->m_zoomFactor );
    QApplication::restoreOverrideCursor();
  }

  ConstFrameIt it = d->m_frames.constBegin();
  const ConstFrameIt end = d->m_frames.constEnd();
  for (; it != end; ++it ) {
      if ( KHTMLPart* p = qobject_cast<KHTMLPart*>((*it)->m_part.data()) )
          p->setZoomFactor(d->m_zoomFactor);
  }

  if ( d->m_guiProfile == BrowserViewGUI ) {
      d->m_paDecZoomFactor->setEnabled( d->m_zoomFactor > minZoom );
      d->m_paIncZoomFactor->setEnabled( d->m_zoomFactor < maxZoom );
  }
}

void KHTMLPart::slotIncFontSize()
{
  incFontSize(zoomSizes, zoomSizeCount);
}

void KHTMLPart::slotDecFontSize()
{
  decFontSize(zoomSizes, zoomSizeCount);
}

void KHTMLPart::slotIncFontSizeFast()
{
  incFontSize(fastZoomSizes, fastZoomSizeCount);
}

void KHTMLPart::slotDecFontSizeFast()
{
  decFontSize(fastZoomSizes, fastZoomSizeCount);
}

void KHTMLPart::incFontSize(const int stepping[], int count)
{
  int zoomFactor = d->m_fontScaleFactor;

  if (zoomFactor < maxZoom) {
    // find the entry nearest to the given zoomsizes
    for (int i = 0; i < count; ++i)
      if (stepping[i] > zoomFactor) {
        zoomFactor = stepping[i];
        break;
      }
    setFontScaleFactor(zoomFactor);
  }
}

void KHTMLPart::decFontSize(const int stepping[], int count)
{
    int zoomFactor = d->m_fontScaleFactor;
    if (zoomFactor > minZoom) {
      // find the entry nearest to the given zoomsizes
      for (int i = count-1; i >= 0; --i)
        if (stepping[i] < zoomFactor) {
          zoomFactor = stepping[i];
          break;
        }
      setFontScaleFactor(zoomFactor);
    }
}

void KHTMLPart::setFontScaleFactor(int percent)
{
  if (percent < minZoom) percent = minZoom;
  if (percent > maxZoom) percent = maxZoom;
  if (d->m_fontScaleFactor == percent) return;
  d->m_fontScaleFactor = percent;

  if (d->m_view && d->m_doc) {
    QApplication::setOverrideCursor( Qt::WaitCursor );
    if (d->m_doc->styleSelector())
      d->m_doc->styleSelector()->computeFontSizes(d->m_doc->logicalDpiY(), d->m_fontScaleFactor);
    d->m_doc->recalcStyle( NodeImpl::Force );
    QApplication::restoreOverrideCursor();
  }

  ConstFrameIt it = d->m_frames.constBegin();
  const ConstFrameIt end = d->m_frames.constEnd();
  for (; it != end; ++it ) {
    if ( KHTMLPart* p = qobject_cast<KHTMLPart*>((*it)->m_part.data()) )
      p->setFontScaleFactor(d->m_fontScaleFactor);
  }
}

int KHTMLPart::fontScaleFactor() const
{
  return d->m_fontScaleFactor;
}

void KHTMLPart::slotZoomView( int delta )
{
  if ( delta < 0 )
    slotIncZoom();
  else
    slotDecZoom();
}

void KHTMLPart::setStatusBarText( const QString& text, StatusBarPriority p)
{
  if (!d->m_statusMessagesEnabled)
    return;

  d->m_statusBarText[p] = text;

  // shift handling ?
  QString tobe = d->m_statusBarText[BarHoverText];
  if (tobe.isEmpty())
    tobe = d->m_statusBarText[BarOverrideText];
  if (tobe.isEmpty()) {
    tobe = d->m_statusBarText[BarDefaultText];
    if (!tobe.isEmpty() && d->m_jobspeed)
      tobe += " ";
    if (d->m_jobspeed)
      tobe += i18n( "(%1/s)" ,  KIO::convertSize( d->m_jobspeed ) );
  }
  tobe = "<qt>"+tobe;

  emit ReadOnlyPart::setStatusBarText(tobe);
}


void KHTMLPart::setJSStatusBarText( const QString &text )
{
  setStatusBarText(text, BarOverrideText);
}

void KHTMLPart::setJSDefaultStatusBarText( const QString &text )
{
  setStatusBarText(text, BarDefaultText);
}

QString KHTMLPart::jsStatusBarText() const
{
    return d->m_statusBarText[BarOverrideText];
}

QString KHTMLPart::jsDefaultStatusBarText() const
{
   return d->m_statusBarText[BarDefaultText];
}

QString KHTMLPart::referrer() const
{
   return d->m_referrer;
}

QString KHTMLPart::pageReferrer() const
{
   QUrl referrerURL = QUrl( d->m_pageReferrer );
   if (referrerURL.isValid())
   {
      QString protocol = referrerURL.scheme();

      if ((protocol == "http") ||
         ((protocol == "https") && (url().scheme() == "https")))
      {
          referrerURL.setFragment(QString());
          referrerURL.setUserName(QString());
          referrerURL.setPassword(QString());
          return referrerURL.toString();
      }
   }

   return QString();
}


QString KHTMLPart::lastModified() const
{
  if ( d->m_lastModified.isEmpty() && url().isLocalFile() ) {
    // Local file: set last-modified from the file's mtime.
    // Done on demand to save time when this isn't needed - but can lead
    // to slightly wrong results if updating the file on disk w/o reloading.
    QDateTime lastModif = QFileInfo( url().toLocalFile() ).lastModified();
    d->m_lastModified = lastModif.toString( Qt::LocalDate );
  }
  //qDebug() << d->m_lastModified;
  return d->m_lastModified;
}

void KHTMLPart::slotLoadImages()
{
    if (d->m_doc )
        d->m_doc->docLoader()->setAutoloadImages( !d->m_doc->docLoader()->autoloadImages() );

    ConstFrameIt it = d->m_frames.constBegin();
    const ConstFrameIt end = d->m_frames.constEnd();
    for (; it != end; ++it ) {
        if ( KHTMLPart* p = qobject_cast<KHTMLPart*>((*it)->m_part.data()) )
            p->slotLoadImages();
    }
}

void KHTMLPart::reparseConfiguration()
{
  KHTMLSettings *settings = KHTMLGlobal::defaultHTMLSettings();
  settings->init();

  setAutoloadImages( settings->autoLoadImages() );
  if (d->m_doc)
     d->m_doc->docLoader()->setShowAnimations( settings->showAnimations() );

  d->m_bOpenMiddleClick = settings->isOpenMiddleClickEnabled();
  d->m_bJScriptEnabled = settings->isJavaScriptEnabled(url().host());
  setDebugScript( settings->isJavaScriptDebugEnabled() );
  d->m_bJavaEnabled = settings->isJavaEnabled(url().host());
  d->m_bPluginsEnabled = settings->isPluginsEnabled(url().host());
  d->m_metaRefreshEnabled = settings->isAutoDelayedActionsEnabled ();

  delete d->m_settings;
  d->m_settings = new KHTMLSettings(*KHTMLGlobal::defaultHTMLSettings());

  QApplication::setOverrideCursor( Qt::WaitCursor );
  khtml::CSSStyleSelector::reparseConfiguration();
  if(d->m_doc) d->m_doc->updateStyleSelector();
  QApplication::restoreOverrideCursor();

  if (d->m_view) {
      KHTMLSettings::KSmoothScrollingMode ssm = d->m_settings->smoothScrolling();
      if (ssm == KHTMLSettings::KSmoothScrollingDisabled)
          d->m_view->setSmoothScrollingModeDefault(KHTMLView::SSMDisabled);
      else if (ssm == KHTMLSettings::KSmoothScrollingWhenEfficient)
          d->m_view->setSmoothScrollingModeDefault(KHTMLView::SSMWhenEfficient);
      else
          d->m_view->setSmoothScrollingModeDefault(KHTMLView::SSMEnabled);
  }

  if (KHTMLGlobal::defaultHTMLSettings()->isAdFilterEnabled())
     runAdFilter();
}

QStringList KHTMLPart::frameNames() const
{
  QStringList res;

  ConstFrameIt it = d->m_frames.constBegin();
  const ConstFrameIt end = d->m_frames.constEnd();
  for (; it != end; ++it )
    if (!(*it)->m_bPreloaded && (*it)->m_part)
      res += (*it)->m_name;

  return res;
}

QList<KParts::ReadOnlyPart*> KHTMLPart::frames() const
{
  QList<KParts::ReadOnlyPart*> res;

  ConstFrameIt it = d->m_frames.constBegin();
  const ConstFrameIt end = d->m_frames.constEnd();
  for (; it != end; ++it )
    if (!(*it)->m_bPreloaded && (*it)->m_part) // ### TODO: make sure that we always create an empty
                                               // KHTMLPart for frames so this never happens.
      res.append( (*it)->m_part.data() );

  return res;
}

bool KHTMLPart::openUrlInFrame( const QUrl &url, const KParts::OpenUrlArguments& args, const KParts::BrowserArguments &browserArgs)
{
  // qDebug() << this << url;
  FrameIt it = d->m_frames.find( browserArgs.frameName );

  if ( it == d->m_frames.end() )
    return false;

  // Inform someone that we are about to show something else.
  if ( !browserArgs.lockHistory() )
      emit d->m_extension->openUrlNotify();

  requestObject( *it, url, args, browserArgs );

  return true;
}

void KHTMLPart::setDNDEnabled( bool b )
{
  d->m_bDnd = b;
}

bool KHTMLPart::dndEnabled() const
{
  return d->m_bDnd;
}

void KHTMLPart::customEvent( QEvent *event )
{
  if ( khtml::MousePressEvent::test( event ) )
  {
    khtmlMousePressEvent( static_cast<khtml::MousePressEvent *>( event ) );
    return;
  }

  if ( khtml::MouseDoubleClickEvent::test( event ) )
  {
    khtmlMouseDoubleClickEvent( static_cast<khtml::MouseDoubleClickEvent *>( event ) );
    return;
  }

  if ( khtml::MouseMoveEvent::test( event ) )
  {
    khtmlMouseMoveEvent( static_cast<khtml::MouseMoveEvent *>( event ) );
    return;
  }

  if ( khtml::MouseReleaseEvent::test( event ) )
  {
    khtmlMouseReleaseEvent( static_cast<khtml::MouseReleaseEvent *>( event ) );
    return;
  }

  if ( khtml::DrawContentsEvent::test( event ) )
  {
    khtmlDrawContentsEvent( static_cast<khtml::DrawContentsEvent *>( event ) );
    return;
  }

  KParts::ReadOnlyPart::customEvent( event );
}

bool KHTMLPart::isPointInsideSelection(int x, int y)
{
  // Treat a collapsed selection like no selection.
  if (d->editor_context.m_selection.state() == Selection::CARET)
    return false;
  if (!xmlDocImpl()->renderer())
    return false;

  khtml::RenderObject::NodeInfo nodeInfo(true, true);
  xmlDocImpl()->renderer()->layer()->nodeAtPoint(nodeInfo, x, y);
  NodeImpl *innerNode = nodeInfo.innerNode();
  if (!innerNode || !innerNode->renderer())
    return false;

  return innerNode->isPointInsideSelection(x, y, d->editor_context.m_selection);
}

/** returns the position of the first inline text box of the line at
 * coordinate y in renderNode
 *
 * This is a helper function for line-by-line text selection.
 */
static bool firstRunAt(khtml::RenderObject *renderNode, int y, NodeImpl *&startNode, long &startOffset)
{
    for (khtml::RenderObject *n = renderNode; n; n = n->nextSibling()) {
        if (n->isText()) {
            khtml::RenderText* const textRenderer = static_cast<khtml::RenderText *>(n);
            for (khtml::InlineTextBox* box = textRenderer->firstTextBox(); box; box = box->nextTextBox()) {
                if (box->m_y == y && textRenderer->element()) {
                    startNode = textRenderer->element();
                    startOffset = box->m_start;
                    return true;
                }
            }
        }

        if (firstRunAt(n->firstChild(), y, startNode, startOffset)) {
            return true;
        }
    }

    return false;
}

/** returns the position of the last inline text box of the line at
 * coordinate y in renderNode
 *
 * This is a helper function for line-by-line text selection.
 */
static bool lastRunAt(khtml::RenderObject *renderNode, int y, NodeImpl *&endNode, long &endOffset)
{
    khtml::RenderObject *n = renderNode;
    if (!n) {
        return false;
    }
    khtml::RenderObject *next;
    while ((next = n->nextSibling())) {
        n = next;
    }

    while (1) {
        if (lastRunAt(n->firstChild(), y, endNode, endOffset)) {
            return true;
        }

        if (n->isText()) {
            khtml::RenderText* const textRenderer =  static_cast<khtml::RenderText *>(n);
            for (khtml::InlineTextBox* box = textRenderer->firstTextBox(); box; box = box->nextTextBox()) {
                if (box->m_y == y && textRenderer->element()) {
                    endNode = textRenderer->element();
                    endOffset = box->m_start + box->m_len;
                    return true;
                }
            }
        }

        if (n == renderNode) {
            return false;
        }

        n = n->previousSibling();
    }
}

void KHTMLPart::handleMousePressEventDoubleClick(khtml::MouseDoubleClickEvent *event)
{
    QMouseEvent *mouse = event->qmouseEvent();
    DOM::Node innerNode = event->innerNode();

    Selection selection;

    if (mouse->button() == Qt::LeftButton && !innerNode.isNull() && innerNode.handle()->renderer() &&
        innerNode.handle()->renderer()->shouldSelect()) {
        Position pos(innerNode.handle()->positionForCoordinates(event->x(), event->y()).position());
        if (pos.node() && (pos.node()->nodeType() == Node::TEXT_NODE || pos.node()->nodeType() == Node::CDATA_SECTION_NODE)) {
            selection.moveTo(pos);
            selection.expandUsingGranularity(Selection::WORD);
        }
    }

    if (selection.state() != Selection::CARET) {
        d->editor_context.beginSelectingText(Selection::WORD);
    }

    setCaret(selection);
    startAutoScroll();
}

void KHTMLPart::handleMousePressEventTripleClick(khtml::MouseDoubleClickEvent *event)
{
    QMouseEvent *mouse = event->qmouseEvent();
    DOM::Node innerNode = event->innerNode();

    Selection selection;

    if (mouse->button() == Qt::LeftButton && !innerNode.isNull() && innerNode.handle()->renderer() &&
        innerNode.handle()->renderer()->shouldSelect()) {
        Position pos(innerNode.handle()->positionForCoordinates(event->x(), event->y()).position());
        if (pos.node() && (pos.node()->nodeType() == Node::TEXT_NODE || pos.node()->nodeType() == Node::CDATA_SECTION_NODE)) {
            selection.moveTo(pos);
            selection.expandUsingGranularity(Selection::LINE);
        }
    }

    if (selection.state() != Selection::CARET) {
        d->editor_context.beginSelectingText(Selection::LINE);
    }

    setCaret(selection);
    startAutoScroll();
}

void KHTMLPart::handleMousePressEventSingleClick(khtml::MousePressEvent *event)
{
    QMouseEvent *mouse = event->qmouseEvent();
    DOM::Node innerNode = event->innerNode();

    if (mouse->button() == Qt::LeftButton) {
        Selection sel;

        if (!innerNode.isNull() && innerNode.handle()->renderer() &&
            innerNode.handle()->renderer()->shouldSelect()) {
            bool extendSelection = mouse->modifiers() & Qt::ShiftModifier;

            // Don't restart the selection when the mouse is pressed on an
            // existing selection so we can allow for text dragging.
            if (!extendSelection && isPointInsideSelection(event->x(), event->y())) {
                return;
            }
            Position pos(innerNode.handle()->positionForCoordinates(event->x(), event->y()).position());
            if (pos.isEmpty())
                pos = Position(innerNode.handle(), innerNode.handle()->caretMinOffset());
            // qDebug() << event->x() << event->y() << pos << endl;

            sel = caret();
            if (extendSelection && sel.notEmpty()) {
                sel.clearModifyBias();
                sel.setExtent(pos);
                if (d->editor_context.m_selectionGranularity != Selection::CHARACTER) {
                    sel.expandUsingGranularity(d->editor_context.m_selectionGranularity);
                }
                d->editor_context.m_beganSelectingText = true;
            } else {
                sel = pos;
                d->editor_context.m_selectionGranularity = Selection::CHARACTER;
            }
        }

        setCaret(sel);
        startAutoScroll();
    }
}

void KHTMLPart::khtmlMousePressEvent( khtml::MousePressEvent *event )
{
  DOM::DOMString url = event->url();
  QMouseEvent *_mouse = event->qmouseEvent();
  DOM::Node innerNode = event->innerNode();
  d->m_mousePressNode = innerNode;

  d->m_dragStartPos = QPoint(event->x(), event->y());

  if ( !event->url().isNull() ) {
    d->m_strSelectedURL = event->url().string();
    d->m_strSelectedURLTarget = event->target().string();
  }
  else {
    d->m_strSelectedURL.clear();
    d->m_strSelectedURLTarget.clear();
  }

  if ( _mouse->button() == Qt::LeftButton ||
       _mouse->button() == Qt::MidButton )
  {
    d->m_bMousePressed = true;

#ifdef KHTML_NO_SELECTION
    d->m_dragLastPos = _mouse->globalPos();
#else
    if ( _mouse->button() == Qt::LeftButton )
    {
      if ( (!d->m_strSelectedURL.isNull() && !isEditable())
                || (!d->m_mousePressNode.isNull() && d->m_mousePressNode.elementId() == ID_IMG) )
          return;

      d->editor_context.m_beganSelectingText = false;

      handleMousePressEventSingleClick(event);
    }
#endif
  }

  if ( _mouse->button() == Qt::RightButton )
  {
    popupMenu( d->m_strSelectedURL );
    // might be deleted, don't touch "this"
  }
}

void KHTMLPart::khtmlMouseDoubleClickEvent( khtml::MouseDoubleClickEvent *event )
{
  QMouseEvent *_mouse = event->qmouseEvent();
  if ( _mouse->button() == Qt::LeftButton )
  {
    d->m_bMousePressed = true;
    d->editor_context.m_beganSelectingText = false;

    if (event->clickCount() == 2) {
      handleMousePressEventDoubleClick(event);
      return;
    }

    if (event->clickCount() >= 3) {
      handleMousePressEventTripleClick(event);
      return;
    }
  }
}

#ifndef KHTML_NO_SELECTION
bool KHTMLPart::isExtendingSelection() const
 {
  // This is it, the whole detection. khtmlMousePressEvent only sets this
  // on LMB or MMB, but never on RMB. As text selection doesn't work for MMB,
  // it's sufficient to only rely on this flag to detect selection extension.
  return d->editor_context.m_beganSelectingText;
}

void KHTMLPart::extendSelectionTo(int x, int y, const DOM::Node &innerNode)
{
    // handle making selection
    Position pos(innerNode.handle()->positionForCoordinates(x, y).position());

    // Don't modify the selection if we're not on a node.
    if (pos.isEmpty())
        return;

    // Restart the selection if this is the first mouse move. This work is usually
    // done in khtmlMousePressEvent, but not if the mouse press was on an existing selection.
    Selection sel = caret();
    sel.clearModifyBias();
    if (!d->editor_context.m_beganSelectingText) {
        // We are beginning a selection during press-drag, when the original click
        // wasn't appropriate for one. Make sure to set the granularity.
        d->editor_context.beginSelectingText(Selection::CHARACTER);
        sel.moveTo(pos);
    }

    sel.setExtent(pos);
    if (d->editor_context.m_selectionGranularity != Selection::CHARACTER) {
        sel.expandUsingGranularity(d->editor_context.m_selectionGranularity);
    }
    setCaret(sel);

}
#endif // KHTML_NO_SELECTION

bool KHTMLPart::handleMouseMoveEventDrag(khtml::MouseMoveEvent *event)
{
#ifdef QT_NO_DRAGANDDROP
  return false;
#else
  if (!dndEnabled())
    return false;

  DOM::Node innerNode = event->innerNode();

  if( (d->m_bMousePressed &&
       ( (!d->m_strSelectedURL.isEmpty() && !isEditable())
        || (!d->m_mousePressNode.isNull() && d->m_mousePressNode.elementId() == ID_IMG) ) )
        && ( d->m_dragStartPos - QPoint(event->x(), event->y()) ).manhattanLength() > QApplication::startDragDistance() ) {

    DOM::DOMString url = event->url();

    QPixmap pix;
    HTMLImageElementImpl *img = 0L;
    QUrl u;

    // qDebug("****************** Event URL: %s", url.string().toLatin1().constData());
    // qDebug("****************** Event Target: %s", target.string().toLatin1().constData());

    // Normal image...
    if ( url.length() == 0 && innerNode.handle() && innerNode.handle()->id() == ID_IMG )
    {
      img = static_cast<HTMLImageElementImpl *>(innerNode.handle());
      u = QUrl( completeURL( khtml::parseURL(img->getAttribute(ATTR_SRC)).string() ) );
      pix = KIconLoader::global()->loadIcon("image-x-generic", KIconLoader::Desktop);
    }
    else
    {
      // Text or image link...
      u = completeURL( d->m_strSelectedURL );
      pix = KIO::pixmapForUrl(u, 0, KIconLoader::Desktop, KIconLoader::SizeMedium);
    }

    u.setPassword(QString());

    QDrag *drag = new QDrag( d->m_view->viewport() );
    QMap<QString, QString> metaDataMap;
    if ( !d->m_referrer.isEmpty() )
      metaDataMap.insert( "referrer", d->m_referrer );
    QMimeData* mimeData = new QMimeData();
    mimeData->setUrls( QList<QUrl>() << u );
    KUrlMimeData::setMetaData( metaDataMap, mimeData );
    drag->setMimeData( mimeData );

    if( img && img->complete() )
      drag->mimeData()->setImageData( img->currentImage() );

    if ( !pix.isNull() )
      drag->setPixmap( pix );

    stopAutoScroll();
    drag->start();

    // when we finish our drag, we need to undo our mouse press
    d->m_bMousePressed = false;
    d->m_strSelectedURL.clear();
    d->m_strSelectedURLTarget.clear();
    return true;
  }
  return false;
#endif // QT_NO_DRAGANDDROP
}

bool KHTMLPart::handleMouseMoveEventOver(khtml::MouseMoveEvent *event)
{
  // Mouse clicked -> do nothing
  if ( d->m_bMousePressed ) return false;

  DOM::DOMString url = event->url();

  // The mouse is over something
  if ( url.length() )
  {
    DOM::DOMString target = event->target();
    QMouseEvent *_mouse = event->qmouseEvent();
    DOM::Node innerNode = event->innerNode();

    bool shiftPressed = ( _mouse->modifiers() & Qt::ShiftModifier );

    // Image map
    if ( !innerNode.isNull() && innerNode.elementId() == ID_IMG )
    {
      HTMLImageElementImpl *i = static_cast<HTMLImageElementImpl *>(innerNode.handle());
      if ( i && i->isServerMap() )
      {
        khtml::RenderObject *r = i->renderer();
        if(r)
        {
          int absx, absy;
          r->absolutePosition(absx, absy);
          int x(event->x() - absx), y(event->y() - absy);

          d->m_overURL = url.string() + QString("?%1,%2").arg(x).arg(y);
          d->m_overURLTarget = target.string();
          overURL( d->m_overURL, target.string(), shiftPressed );
          return true;
        }
      }
    }

    // normal link
    if ( d->m_overURL.isEmpty() || d->m_overURL != url || d->m_overURLTarget != target )
    {
      d->m_overURL = url.string();
      d->m_overURLTarget = target.string();
      overURL( d->m_overURL, target.string(), shiftPressed );
    }
  }
  else  // Not over a link...
  {
    if( !d->m_overURL.isEmpty() ) // and we were over a link  -> reset to "default statusbar text"
    {
      // reset to "default statusbar text"
      resetHoverText();
    }
  }
  return true;
}

void KHTMLPart::handleMouseMoveEventSelection(khtml::MouseMoveEvent *event)
{
    // Mouse not pressed. Do nothing.
    if (!d->m_bMousePressed)
        return;

#ifdef KHTML_NO_SELECTION
    if (d->m_doc && d->m_view) {
        QPoint diff( mouse->globalPos() - d->m_dragLastPos );

        if (abs(diff.x()) > 64 || abs(diff.y()) > 64) {
            d->m_view->scrollBy(-diff.x(), -diff.y());
            d->m_dragLastPos = mouse->globalPos();
        }
    }
#else

    QMouseEvent *mouse = event->qmouseEvent();
    DOM::Node innerNode = event->innerNode();

    if ( (mouse->buttons() & Qt::LeftButton) == 0 || !innerNode.handle() || !innerNode.handle()->renderer() ||
        !innerNode.handle()->renderer()->shouldSelect())
            return;

    // handle making selection
    extendSelectionTo(event->x(), event->y(), innerNode);
#endif // KHTML_NO_SELECTION
}

void KHTMLPart::khtmlMouseMoveEvent( khtml::MouseMoveEvent *event )
{
    if (handleMouseMoveEventDrag(event))
        return;

    if (handleMouseMoveEventOver(event))
        return;

    handleMouseMoveEventSelection(event);
}

void KHTMLPart::khtmlMouseReleaseEvent( khtml::MouseReleaseEvent *event )
{
  DOM::Node innerNode = event->innerNode();
  d->m_mousePressNode = DOM::Node();

  if ( d->m_bMousePressed ) {
    setStatusBarText(QString(), BarHoverText);
    stopAutoScroll();
  }

  // Used to prevent mouseMoveEvent from initiating a drag before
  // the mouse is pressed again.
  d->m_bMousePressed = false;

#ifndef QT_NO_CLIPBOARD
  QMouseEvent *_mouse = event->qmouseEvent();
  if ((d->m_guiProfile == BrowserViewGUI) && (_mouse->button() == Qt::MidButton) && (event->url().isNull())) {
    // qDebug() << "MMB shouldOpen=" << d->m_bOpenMiddleClick;

    if (d->m_bOpenMiddleClick) {
      KHTMLPart *p = this;
      while (p->parentPart()) p = p->parentPart();
      p->d->m_extension->pasteRequest();
    }
  }
#endif

#ifndef KHTML_NO_SELECTION
  {

    // Clear the selection if the mouse didn't move after the last mouse press.
    // We do this so when clicking on the selection, the selection goes away.
    // However, if we are editing, place the caret.
    if (!d->editor_context.m_beganSelectingText
            && d->m_dragStartPos.x() == event->x()
            && d->m_dragStartPos.y() == event->y()
            && d->editor_context.m_selection.state() == Selection::RANGE) {
      Selection selection;
#ifdef APPLE_CHANGES
      if (d->editor_context.m_selection.base().node()->isContentEditable())
#endif
        selection.moveTo(d->editor_context.m_selection.base().node()->positionForCoordinates(event->x(), event->y()).position());
      setCaret(selection);
    }
    // get selected text and paste to the clipboard
#ifndef QT_NO_CLIPBOARD
    QString text = selectedText();
    text.replace(QChar(0xa0), ' ');
    if (!text.isEmpty()) {
        disconnect( qApp->clipboard(), SIGNAL(selectionChanged()), this, SLOT(slotClearSelection()));
        qApp->clipboard()->setText(text,QClipboard::Selection);
        connect( qApp->clipboard(), SIGNAL(selectionChanged()), SLOT(slotClearSelection()));
    }
#endif
    //qDebug() << "selectedText = " << text;
    emitSelectionChanged();
//qDebug() << "rel2: startBefEnd " << d->m_startBeforeEnd << " extAtEnd " << d->m_extendAtEnd << " (" << d->m_startOffset << ") - (" << d->m_endOffset << "), caretOfs " << d->caretOffset();
  }
#endif
}

void KHTMLPart::khtmlDrawContentsEvent( khtml::DrawContentsEvent * )
{
}

void KHTMLPart::guiActivateEvent( KParts::GUIActivateEvent *event )
{
  if ( event->activated() )
  {
    emitSelectionChanged();
    emit d->m_extension->enableAction( "print", d->m_doc != 0 );

    if ( !d->m_settings->autoLoadImages() && d->m_paLoadImages )
    {
        QList<QAction*> lst;
        lst.append( d->m_paLoadImages );
        plugActionList( "loadImages", lst );
    }
  }
}

void KHTMLPart::slotPrintFrame()
{
  if ( d->m_frames.count() == 0 )
    return;

  KParts::ReadOnlyPart *frame = currentFrame();
  if (!frame)
    return;

  KParts::BrowserExtension *ext = KParts::BrowserExtension::childObject( frame );

  if ( !ext )
    return;


  const QMetaObject *mo = ext->metaObject();


  if (mo->indexOfSlot( "print()") != -1)
    QMetaObject::invokeMethod(ext, "print()",  Qt::DirectConnection);
}

void KHTMLPart::slotSelectAll()
{
  KParts::ReadOnlyPart *part = currentFrame();
  if (part && part->inherits("KHTMLPart"))
    static_cast<KHTMLPart *>(part)->selectAll();
}

void KHTMLPart::startAutoScroll()
{
   connect(&d->m_scrollTimer, SIGNAL(timeout()), this, SLOT(slotAutoScroll()));
   d->m_scrollTimer.setSingleShot(false);
   d->m_scrollTimer.start(100);
}

void KHTMLPart::stopAutoScroll()
{
   disconnect(&d->m_scrollTimer, SIGNAL(timeout()), this, SLOT(slotAutoScroll()));
   if (d->m_scrollTimer.isActive())
       d->m_scrollTimer.stop();
}


void KHTMLPart::slotAutoScroll()
{
    if (d->m_view)
      d->m_view->doAutoScroll();
    else
      stopAutoScroll(); // Safety
}

void KHTMLPart::runAdFilter()
{
    if ( parentPart() )
        parentPart()->runAdFilter();

    if ( !d->m_doc )
        return;

    QSetIterator<khtml::CachedObject*> it( d->m_doc->docLoader()->m_docObjects );
    while (it.hasNext())
    {
        khtml::CachedObject* obj = it.next();
        if ( obj->type() == khtml::CachedObject::Image ) {
            khtml::CachedImage *image = static_cast<khtml::CachedImage *>(obj);
            bool wasBlocked = image->m_wasBlocked;
            image->m_wasBlocked = KHTMLGlobal::defaultHTMLSettings()->isAdFiltered( d->m_doc->completeURL( image->url().string() ) );
            if ( image->m_wasBlocked != wasBlocked )
                image->do_notify(QRect(QPoint(0,0), image->pixmap_size()));
        }
    }

    if ( KHTMLGlobal::defaultHTMLSettings()->isHideAdsEnabled() ) {
        for ( NodeImpl *nextNode, *node = d->m_doc; node; node = nextNode ) {

            // We might be deleting 'node' shortly.
            nextNode = node->traverseNextNode();

            if ( node->id() == ID_IMG ||
                 node->id() == ID_IFRAME ||
                 (node->id() == ID_INPUT && static_cast<HTMLInputElementImpl *>(node)->inputType() == HTMLInputElementImpl::IMAGE ))
            {
                if ( KHTMLGlobal::defaultHTMLSettings()->isAdFiltered( d->m_doc->completeURL( static_cast<ElementImpl *>(node)->getAttribute(ATTR_SRC).string() ) ) )
                {
                    // Since any kids of node will be deleted, too, fastforward nextNode
                    // until we get outside of node.
                    while (nextNode && nextNode->isAncestor(node))
                        nextNode = nextNode->traverseNextNode();

                    node->ref();
                    NodeImpl *parent = node->parent();
                    if( parent )
                    {
                        int exception = 0;
                        parent->removeChild(node, exception);
                    }
                    node->deref();
                }
            }
        }
    }
}

void KHTMLPart::selectAll()
{
  if (!d->m_doc) return;

  NodeImpl *first;
  if (d->m_doc->isHTMLDocument())
    first = static_cast<HTMLDocumentImpl*>(d->m_doc)->body();
  else
    first = d->m_doc;
  NodeImpl *next;

  // Look for first text/cdata node that has a renderer,
  // or first childless replaced element
  while ( first && !(first->renderer()
          && ((first->nodeType() == Node::TEXT_NODE || first->nodeType() == Node::CDATA_SECTION_NODE)
                || (first->renderer()->isReplaced() && !first->renderer()->firstChild()))))
  {
    next = first->firstChild();
    if ( !next ) next = first->nextSibling();
    while( first && !next )
    {
      first = first->parentNode();
      if ( first )
        next = first->nextSibling();
    }
    first = next;
  }

  NodeImpl *last;
  if (d->m_doc->isHTMLDocument())
    last = static_cast<HTMLDocumentImpl*>(d->m_doc)->body();
  else
    last = d->m_doc;
  // Look for last text/cdata node that has a renderer,
  // or last childless replaced element
  // ### Instead of changing this loop, use findLastSelectableNode
  // in render_table.cpp (LS)
  while ( last && !(last->renderer()
          && ((last->nodeType() == Node::TEXT_NODE || last->nodeType() == Node::CDATA_SECTION_NODE)
                || (last->renderer()->isReplaced() && !last->renderer()->lastChild()))))
  {
    next = last->lastChild();
    if ( !next ) next = last->previousSibling();
    while ( last && !next )
    {
      last = last->parentNode();
      if ( last )
        next = last->previousSibling();
    }
    last = next;
  }

  if ( !first || !last )
    return;
  Q_ASSERT(first->renderer());
  Q_ASSERT(last->renderer());
  d->editor_context.m_selection.moveTo(Position(first, 0), Position(last, last->nodeValue().length()));
  d->m_doc->updateSelection();

  emitSelectionChanged();
}

bool KHTMLPart::checkLinkSecurity(const QUrl &linkURL,const KLocalizedString &message, const QString &button)
{
  bool linkAllowed = true;

  if ( d->m_doc )
    linkAllowed = KUrlAuthorized::authorizeUrlAction("redirect", url(), linkURL);

  if ( !linkAllowed ) {
    khtml::Tokenizer *tokenizer = d->m_doc->tokenizer();
    if (tokenizer)
      tokenizer->setOnHold(true);

    int response = KMessageBox::Cancel;
    if (!message.isEmpty())
    {
            // Dangerous flag makes the Cancel button the default
            response = KMessageBox::warningContinueCancel( 0,
                                                           message.subs(Qt::escape(linkURL.toDisplayString())).toString(),
                                                           i18n( "Security Warning" ),
                                                           KGuiItem(button),
                                                           KStandardGuiItem::cancel(),
                                                           QString(), // no don't ask again info
                                                           KMessageBox::Notify | KMessageBox::Dangerous );
    }
    else
    {
            KMessageBox::error( 0,
                                i18n( "<qt>Access by untrusted page to<br /><b>%1</b><br /> denied.</qt>", Qt::escape(linkURL.toDisplayString())),
                                i18n( "Security Alert" ));
    }

    if (tokenizer)
       tokenizer->setOnHold(false);
    return (response==KMessageBox::Continue);
  }
  return true;
}

void KHTMLPart::slotPartRemoved( KParts::Part *part )
{
//    qDebug() << part;
    if ( part == d->m_activeFrame )
    {
        d->m_activeFrame = 0L;
        if ( !part->inherits( "KHTMLPart" ) )
        {
            if (factory()) {
                factory()->removeClient( part );
            }
            if (childClients().contains(part)) {
                removeChildClient( part );
            }
        }
    }
}

void KHTMLPart::slotActiveFrameChanged( KParts::Part *part )
{
//    qDebug() << this << "part=" << part;
    if ( part == this )
    {
        qCritical() << "strange error! we activated ourselves";
        assert( false );
        return;
    }
//    qDebug() << "d->m_activeFrame=" << d->m_activeFrame;
    if ( d->m_activeFrame && d->m_activeFrame->widget() && d->m_activeFrame->widget()->inherits( "QFrame" ) )
    {
        QFrame *frame = static_cast<QFrame *>( d->m_activeFrame->widget() );
        if (frame->frameStyle() != QFrame::NoFrame)
        {
           frame->setFrameStyle( QFrame::StyledPanel | QFrame::Sunken);
           frame->repaint();
        }
    }

    if( d->m_activeFrame && !d->m_activeFrame->inherits( "KHTMLPart" ) )
    {
        if (factory()) {
            factory()->removeClient( d->m_activeFrame );
        }
        removeChildClient( d->m_activeFrame );
    }
    if( part && !part->inherits( "KHTMLPart" ) )
    {
        if (factory()) {
            factory()->addClient( part );
        }
        insertChildClient( part );
    }


    d->m_activeFrame = part;

    if ( d->m_activeFrame && d->m_activeFrame->widget()->inherits( "QFrame" ) )
    {
        QFrame *frame = static_cast<QFrame *>( d->m_activeFrame->widget() );
        if (frame->frameStyle() != QFrame::NoFrame)
        {
           frame->setFrameStyle( QFrame::StyledPanel | QFrame::Plain);
           frame->repaint();
        }
        // qDebug() << "new active frame " << d->m_activeFrame;
    }

    updateActions();

    // (note: childObject returns 0 if the argument is 0)
    d->m_extension->setExtensionProxy( KParts::BrowserExtension::childObject( d->m_activeFrame ) );
}

void KHTMLPart::setActiveNode(const DOM::Node &node)
{
    if (!d->m_doc || !d->m_view)
        return;

    // Set the document's active node
    d->m_doc->setFocusNode(node.handle());

    // Scroll the view if necessary to ensure that the new focus node is visible
    QRect rect  = node.handle()->getRect();
    d->m_view->ensureVisible(rect.right(), rect.bottom());
    d->m_view->ensureVisible(rect.left(), rect.top());
}

DOM::Node KHTMLPart::activeNode() const
{
    return DOM::Node(d->m_doc?d->m_doc->focusNode():0);
}

DOM::EventListener *KHTMLPart::createHTMLEventListener( QString code, QString name, NodeImpl* node, bool svg )
{
  KJSProxy *proxy = jScript();

  if (!proxy)
    return 0;

  return proxy->createHTMLEventHandler( url().toString(), name, code, node, svg );
}

KHTMLPart *KHTMLPart::opener()
{
    return d->m_opener;
}

void KHTMLPart::setOpener(KHTMLPart *_opener)
{
    d->m_opener = _opener;
}

bool KHTMLPart::openedByJS()
{
    return d->m_openedByJS;
}

void KHTMLPart::setOpenedByJS(bool _openedByJS)
{
    d->m_openedByJS = _openedByJS;
}

void KHTMLPart::preloadStyleSheet(const QString &url, const QString &stylesheet)
{
    khtml::Cache::preloadStyleSheet(url, stylesheet);
}

void KHTMLPart::preloadScript(const QString &url, const QString &script)
{
    khtml::Cache::preloadScript(url, script);
}

long KHTMLPart::cacheId() const
{
  return d->m_cacheId;
}

bool KHTMLPart::restored() const
{
  return d->m_restored;
}

bool KHTMLPart::pluginPageQuestionAsked(const QString& mimetype) const
{
  // parentPart() should be const!
  KHTMLPart* parent = const_cast<KHTMLPart *>(this)->parentPart();
  if ( parent )
    return parent->pluginPageQuestionAsked(mimetype);

  return d->m_pluginPageQuestionAsked.contains(mimetype);
}

void KHTMLPart::setPluginPageQuestionAsked(const QString& mimetype)
{
  if ( parentPart() )
    parentPart()->setPluginPageQuestionAsked(mimetype);

  d->m_pluginPageQuestionAsked.append(mimetype);
}

KEncodingDetector *KHTMLPart::createDecoder()
{
    KEncodingDetector *dec = new KEncodingDetector();
    if( !d->m_encoding.isNull() )
        dec->setEncoding( d->m_encoding.toLatin1().constData(),
            d->m_haveEncoding ? KEncodingDetector::UserChosenEncoding : KEncodingDetector::EncodingFromHTTPHeader);
    else {
        // Inherit the default encoding from the parent frame if there is one.
        QByteArray defaultEncoding = (parentPart() && parentPart()->d->m_decoder)
            ? QByteArray( parentPart()->d->m_decoder->encoding() ) : settings()->encoding().toLatin1();
        dec->setEncoding(defaultEncoding.constData(), KEncodingDetector::DefaultEncoding);
    }

    if (d->m_doc)
        d->m_doc->setDecoder(dec);

    // convert from KEncodingProber::ProberType to KEncodingDetector::AutoDetectScript
    KEncodingDetector::AutoDetectScript scri;
    switch (d->m_autoDetectLanguage) {
        case KEncodingProber::None: scri = KEncodingDetector::None; break;
        case KEncodingProber::Universal: scri = KEncodingDetector::SemiautomaticDetection; break;
        case KEncodingProber::Arabic: scri = KEncodingDetector::Arabic; break;
        case KEncodingProber::Baltic: scri = KEncodingDetector::Baltic; break;
        case KEncodingProber::CentralEuropean: scri = KEncodingDetector::CentralEuropean; break;
        case KEncodingProber::ChineseSimplified: scri = KEncodingDetector::ChineseSimplified; break;
        case KEncodingProber::ChineseTraditional: scri = KEncodingDetector::ChineseTraditional; break;
        case KEncodingProber::Cyrillic: scri = KEncodingDetector::Cyrillic; break;
        case KEncodingProber::Greek: scri = KEncodingDetector::Greek; break;
        case KEncodingProber::Hebrew: scri = KEncodingDetector::Hebrew; break;
        case KEncodingProber::Japanese: scri = KEncodingDetector::Japanese; break;
        case KEncodingProber::Korean: scri = KEncodingDetector::Korean; break;
        case KEncodingProber::NorthernSaami: scri = KEncodingDetector::NorthernSaami; break;
        case KEncodingProber::Other: scri = KEncodingDetector::SemiautomaticDetection; break;
        case KEncodingProber::SouthEasternEurope: scri = KEncodingDetector::SouthEasternEurope; break;
        case KEncodingProber::Thai: scri = KEncodingDetector::Thai; break;
        case KEncodingProber::Turkish: scri = KEncodingDetector::Turkish; break;
        case KEncodingProber::Unicode: scri = KEncodingDetector::Unicode; break;
        case KEncodingProber::WesternEuropean: scri = KEncodingDetector::WesternEuropean; break;
        default: scri = KEncodingDetector::SemiautomaticDetection; break;
    }
    dec->setAutoDetectLanguage( scri );
    return dec;
}

void KHTMLPart::emitCaretPositionChanged(const DOM::Position &pos) {
  // pos must not be already converted to range-compliant coordinates
  Position rng_pos = pos.equivalentRangeCompliantPosition();
  Node node = rng_pos.node();
  emit caretPositionChanged(node, rng_pos.offset());
}

void KHTMLPart::restoreScrollPosition()
{
  const KParts::OpenUrlArguments args( arguments() );

  if ( url().hasFragment() && !d->m_restoreScrollPosition && !args.reload()) {
    if ( !d->m_doc || !d->m_doc->parsing() )
      disconnect(d->m_view, SIGNAL(finishedLayout()), this, SLOT(restoreScrollPosition()));
    if ( !gotoAnchor(QUrl(url()).fragment()) )
      gotoAnchor(url().fragment());
    return;
  }

  // Check whether the viewport has become large enough to encompass the stored
  // offsets. If the document has been fully loaded, force the new coordinates,
  // even if the canvas is too short (can happen when user resizes the window
  // during loading).
  if (d->m_view->contentsHeight() - d->m_view->visibleHeight() >= args.yOffset()
      || d->m_bComplete) {
    d->m_view->setContentsPos(args.xOffset(), args.yOffset());
    disconnect(d->m_view, SIGNAL(finishedLayout()), this, SLOT(restoreScrollPosition()));
  }
}


void KHTMLPart::openWallet(DOM::HTMLFormElementImpl *form)
{
#ifndef KHTML_NO_WALLET
  KHTMLPart *p;

  for (p = parentPart(); p && p->parentPart(); p = p->parentPart()) {
  }

  if (p) {
    p->openWallet(form);
    return;
  }

  if (onlyLocalReferences()) { // avoid triggering on local apps, thumbnails
    return;
  }

  if (d->m_wallet) {
    if (d->m_bWalletOpened) {
      if (d->m_wallet->isOpen()) {
        form->walletOpened(d->m_wallet);
        return;
      }
      d->m_wallet->deleteLater();
      d->m_wallet = 0L;
      d->m_bWalletOpened = false;
    }
  }

  if (!d->m_wq) {
    KWallet::Wallet *wallet = KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(), widget() ? widget()->topLevelWidget()->winId() : 0, KWallet::Wallet::Asynchronous);
    d->m_wq = new KHTMLWalletQueue(this);
    d->m_wq->wallet = wallet;
    connect(wallet, SIGNAL(walletOpened(bool)), d->m_wq, SLOT(walletOpened(bool)));
    connect(d->m_wq, SIGNAL(walletOpened(KWallet::Wallet*)), this, SLOT(walletOpened(KWallet::Wallet*)));
  }
  assert(form);
  d->m_wq->callers.append(KHTMLWalletQueue::Caller(form, form->document()));
#endif // KHTML_NO_WALLET
}


void KHTMLPart::saveToWallet(const QString& key, const QMap<QString,QString>& data)
{
#ifndef KHTML_NO_WALLET
  KHTMLPart *p;

  for (p = parentPart(); p && p->parentPart(); p = p->parentPart()) {
  }

  if (p) {
    p->saveToWallet(key, data);
    return;
  }

  if (d->m_wallet) {
    if (d->m_bWalletOpened) {
      if (d->m_wallet->isOpen()) {
        if (!d->m_wallet->hasFolder(KWallet::Wallet::FormDataFolder())) {
          d->m_wallet->createFolder(KWallet::Wallet::FormDataFolder());
        }
        d->m_wallet->setFolder(KWallet::Wallet::FormDataFolder());
        d->m_wallet->writeMap(key, data);
        return;
      }
      d->m_wallet->deleteLater();
      d->m_wallet = 0L;
      d->m_bWalletOpened = false;
    }
  }

  if (!d->m_wq) {
    KWallet::Wallet *wallet = KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(), widget() ? widget()->topLevelWidget()->winId() : 0, KWallet::Wallet::Asynchronous);
    d->m_wq = new KHTMLWalletQueue(this);
    d->m_wq->wallet = wallet;
    connect(wallet, SIGNAL(walletOpened(bool)), d->m_wq, SLOT(walletOpened(bool)));
    connect(d->m_wq, SIGNAL(walletOpened(KWallet::Wallet*)), this, SLOT(walletOpened(KWallet::Wallet*)));
  }
  d->m_wq->savers.append(qMakePair(key, data));
#endif // KHTML_NO_WALLET
}


void KHTMLPart::dequeueWallet(DOM::HTMLFormElementImpl *form) {
#ifndef KHTML_NO_WALLET
  KHTMLPart *p;

  for (p = parentPart(); p && p->parentPart(); p = p->parentPart()) {
  }

  if (p) {
    p->dequeueWallet(form);
    return;
  }

  if (d->m_wq) {
    d->m_wq->callers.removeAll(KHTMLWalletQueue::Caller(form, form->document()));
  }
#endif // KHTML_NO_WALLET
}


void KHTMLPart::walletOpened(KWallet::Wallet *wallet) {
#ifndef KHTML_NO_WALLET
  assert(!d->m_wallet);
  assert(d->m_wq);

  d->m_wq->deleteLater(); // safe?
  d->m_wq = 0L;

  if (!wallet) {
    d->m_bWalletOpened = false;
    return;
  }

  d->m_wallet = wallet;
  d->m_bWalletOpened = true;
  connect(d->m_wallet, SIGNAL(walletClosed()), SLOT(slotWalletClosed()));
  d->m_walletForms.clear();
  if (!d->m_statusBarWalletLabel) {
    d->m_statusBarWalletLabel = new KUrlLabel(d->m_statusBarExtension->statusBar());
    d->m_statusBarWalletLabel->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum));
    d->m_statusBarWalletLabel->setUseCursor(false);
    d->m_statusBarExtension->addStatusBarItem(d->m_statusBarWalletLabel, 0, false);
    d->m_statusBarWalletLabel->setPixmap(SmallIcon("wallet-open"));
    connect(d->m_statusBarWalletLabel, SIGNAL(leftClickedUrl()), SLOT(launchWalletManager()));
    connect(d->m_statusBarWalletLabel, SIGNAL(rightClickedUrl()), SLOT(walletMenu()));
  }
  d->m_statusBarWalletLabel->setToolTip(i18n("The wallet '%1' is open and being used for form data and passwords.", KWallet::Wallet::NetworkWallet()));
#endif // KHTML_NO_WALLET
}


KWallet::Wallet *KHTMLPart::wallet()
{
#ifndef KHTML_NO_WALLET
  KHTMLPart *p;

  for (p = parentPart(); p && p->parentPart(); p = p->parentPart())
    ;

  if (p)
    return p->wallet();

  return d->m_wallet;
#else
  return 0;
#endif // !KHTML_NO_WALLET
}


void KHTMLPart::slotWalletClosed()
{
#ifndef KHTML_NO_WALLET
  if (d->m_wallet) {
    d->m_wallet->deleteLater();
    d->m_wallet = 0L;
  }
  d->m_bWalletOpened = false;
  if (d->m_statusBarWalletLabel) {
    d->m_statusBarExtension->removeStatusBarItem(d->m_statusBarWalletLabel);
    delete d->m_statusBarWalletLabel;
    d->m_statusBarWalletLabel = 0L;
  }
#endif // KHTML_NO_WALLET
}

void KHTMLPart::launchWalletManager()
{
#ifndef KHTML_NO_WALLET
  QDBusInterface r("org.kde.kwalletmanager", "/kwalletmanager/MainWindow_1",
                      "org.kde.KMainWindow");
  if (!r.isValid()) {
    KToolInvocation::startServiceByDesktopName("kwalletmanager_show");
  } else {
    r.call(QDBus::NoBlock, "show");
    r.call(QDBus::NoBlock, "raise");
  }
#endif // KHTML_NO_WALLET
}

void KHTMLPart::walletMenu()
{
#ifndef KHTML_NO_WALLET
  QMenu *menu = new QMenu(0L);
  QActionGroup *menuActionGroup = new QActionGroup(menu);
  connect( menuActionGroup, SIGNAL(triggered(QAction*)), this, SLOT(removeStoredPasswordForm(QAction*)) );

  menu->addAction(i18n("&Close Wallet"), this, SLOT(slotWalletClosed()));

  if (d->m_view && d->m_view->nonPasswordStorableSite(toplevelURL().host())) {
    menu->addAction(i18n("&Allow storing passwords for this site"), this, SLOT(delNonPasswordStorableSite()));
  }

  // List currently removable form passwords
  for ( QStringList::ConstIterator it = d->m_walletForms.constBegin(); it != d->m_walletForms.constEnd(); ++it ) {
      QAction* action = menu->addAction( i18n("Remove password for form %1", *it) );
      action->setActionGroup(menuActionGroup);
      QVariant var(*it);
      action->setData(var);
  }

  KAcceleratorManager::manage(menu);
  menu->popup(QCursor::pos());
#endif // KHTML_NO_WALLET
}

void KHTMLPart::removeStoredPasswordForm(QAction* action)
{
#ifndef KHTML_NO_WALLET
  assert(action);
  assert(d->m_wallet);
  QVariant var(action->data());

  if(var.isNull() || !var.isValid() || var.type() != QVariant::String)
    return;

  QString key = var.toString();
  if (KWallet::Wallet::keyDoesNotExist(KWallet::Wallet::NetworkWallet(),
                                      KWallet::Wallet::FormDataFolder(),
                                      key))
    return; // failed


  if (!d->m_wallet->hasFolder(KWallet::Wallet::FormDataFolder()))
    return; // failed

  d->m_wallet->setFolder(KWallet::Wallet::FormDataFolder());
  if (d->m_wallet->removeEntry(key))
    return; // failed

  d->m_walletForms.removeAll(key);
#endif // KHTML_NO_WALLET
}

void KHTMLPart::addWalletFormKey(const QString& walletFormKey)
{
#ifndef KHTML_NO_WALLET

  if (parentPart()) {
    parentPart()->addWalletFormKey(walletFormKey);
    return;
  }

  if(!d->m_walletForms.contains(walletFormKey))
    d->m_walletForms.append(walletFormKey);
#endif // KHTML_NO_WALLET
}

void KHTMLPart::delNonPasswordStorableSite()
{
#ifndef KHTML_NO_WALLET
  if (d->m_view)
    d->m_view->delNonPasswordStorableSite(toplevelURL().host());
#endif // KHTML_NO_WALLET
}
void KHTMLPart::saveLoginInformation(const QString& host, const QString& key, const QMap<QString, QString>& walletMap)
{
#ifndef KHTML_NO_WALLET
  d->m_storePass.saveLoginInformation(host, key, walletMap);
#endif // KHTML_NO_WALLET
}

void KHTMLPart::slotToggleCaretMode()
{
  setCaretMode(d->m_paToggleCaretMode->isChecked());
}

void KHTMLPart::setFormNotification(KHTMLPart::FormNotification fn) {
  d->m_formNotification = fn;
}

KHTMLPart::FormNotification KHTMLPart::formNotification() const {
  return d->m_formNotification;
}

QUrl KHTMLPart::toplevelURL()
{
  KHTMLPart* part = this;
  while (part->parentPart())
    part = part->parentPart();

  if (!part)
    return QUrl();

  return part->url();
}

bool KHTMLPart::isModified() const
{
  if ( !d->m_doc )
    return false;

  return d->m_doc->unsubmittedFormChanges();
}

void KHTMLPart::setDebugScript( bool enable )
{
  unplugActionList( "debugScriptList" );
  if ( enable ) {
    if (!d->m_paDebugScript) {
      d->m_paDebugScript = new QAction( i18n( "JavaScript &Debugger" ), this );
      actionCollection()->addAction( "debugScript", d->m_paDebugScript );
      connect( d->m_paDebugScript, SIGNAL(triggered(bool)), this, SLOT(slotDebugScript()) );
    }
    d->m_paDebugScript->setEnabled( d->m_frame ? d->m_frame->m_jscript : 0L );
    QList<QAction*> lst;
    lst.append( d->m_paDebugScript );
    plugActionList( "debugScriptList", lst );
  }
  d->m_bJScriptDebugEnabled = enable;
}

void KHTMLPart::setSuppressedPopupIndicator( bool enable, KHTMLPart *originPart )
{
    if ( parentPart() ) {
        parentPart()->setSuppressedPopupIndicator( enable, originPart );
        return;
    }

    if ( enable && originPart ) {
        d->m_openableSuppressedPopups++;
        if ( d->m_suppressedPopupOriginParts.indexOf( originPart ) == -1 )
            d->m_suppressedPopupOriginParts.append( originPart );
    }

    if ( enable && !d->m_statusBarPopupLabel ) {
        d->m_statusBarPopupLabel = new KUrlLabel( d->m_statusBarExtension->statusBar() );
        d->m_statusBarPopupLabel->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Minimum ));
        d->m_statusBarPopupLabel->setUseCursor( false );
        d->m_statusBarExtension->addStatusBarItem( d->m_statusBarPopupLabel, 0, false );
        d->m_statusBarPopupLabel->setPixmap( SmallIcon( "window-suppressed") );

                d->m_statusBarPopupLabel->setToolTip(i18n("This page was prevented from opening a new window via JavaScript." ) );

        connect(d->m_statusBarPopupLabel, SIGNAL(leftClickedUrl()), SLOT(suppressedPopupMenu()));
        if (d->m_settings->jsPopupBlockerPassivePopup()) {
            QPixmap px;
            px = MainBarIcon( "window-suppressed" );
            KPassivePopup::message(i18n("Popup Window Blocked"),i18n("This page has attempted to open a popup window but was blocked.\nYou can click on this icon in the status bar to control this behavior\nor to open the popup."),px,d->m_statusBarPopupLabel);
        }
    } else if ( !enable && d->m_statusBarPopupLabel ) {
        d->m_statusBarPopupLabel->setToolTip("" );
        d->m_statusBarExtension->removeStatusBarItem( d->m_statusBarPopupLabel );
        delete d->m_statusBarPopupLabel;
        d->m_statusBarPopupLabel = 0L;
    }
}

void KHTMLPart::suppressedPopupMenu() {
  QMenu *m = new QMenu(0L);
  if ( d->m_openableSuppressedPopups )
      m->addAction(i18np("&Show Blocked Popup Window","&Show %1 Blocked Popup Windows", d->m_openableSuppressedPopups), this, SLOT(showSuppressedPopups()));
  QAction *a = m->addAction(i18n("Show Blocked Window Passive Popup &Notification"), this, SLOT(togglePopupPassivePopup()));
  a->setChecked(d->m_settings->jsPopupBlockerPassivePopup());
  m->addAction(i18n("&Configure JavaScript New Window Policies..."), this, SLOT(launchJSConfigDialog()));
  m->popup(QCursor::pos());
}

void KHTMLPart::togglePopupPassivePopup() {
  // Same hack as in disableJSErrorExtension()
  d->m_settings->setJSPopupBlockerPassivePopup( !d->m_settings->jsPopupBlockerPassivePopup() );
  emit configurationChanged();
}

void KHTMLPart::showSuppressedPopups() {
    foreach ( KHTMLPart* part, d->m_suppressedPopupOriginParts ) {
      if (part) {
        KJS::Window *w = KJS::Window::retrieveWindow( part );
        if (w) {
          w->showSuppressedWindows();
          w->forgetSuppressedWindows();
        }
      }
    }
    setSuppressedPopupIndicator( false );
    d->m_openableSuppressedPopups = 0;
    d->m_suppressedPopupOriginParts.clear();
}

// Extension to use for "view document source", "save as" etc.
// Using the right extension can help the viewer get into the right mode (#40496)
QString KHTMLPart::defaultExtension() const
{
    if ( !d->m_doc )
        return ".html";
    if ( !d->m_doc->isHTMLDocument() )
        return ".xml";
    return d->m_doc->htmlMode() == DOM::DocumentImpl::XHtml ? ".xhtml" : ".html";
}

bool KHTMLPart::inProgress() const
{
    if (!d->m_bComplete || d->m_runningScripts || (d->m_doc && d->m_doc->parsing()))
        return true;

    // Any frame that hasn't completed yet ?
    ConstFrameIt it = d->m_frames.constBegin();
    const ConstFrameIt end = d->m_frames.constEnd();
    for (; it != end; ++it ) {
        if ((*it)->m_run || !(*it)->m_bCompleted)
            return true;
    }

    return d->m_submitForm || !d->m_redirectURL.isEmpty() || d->m_redirectionTimer.isActive() || d->m_job;
}

using namespace KParts;
#include "moc_khtmlpart_p.cpp"
#ifndef KHTML_NO_WALLET
#include "moc_khtml_wallet_p.cpp"
#endif

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
