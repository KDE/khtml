/* This file is part of the KDE project
 *
 * Copyright (C) 2000-2003 Simon Hausmann <hausmann@kde.org>
 *               2001-2003 George Staikos <staikos@kde.org>
 *               2001-2003 Laurent Montel <montel@kde.org>
 *               2001-2003 Dirk Mueller <mueller@kde.org>
 *               2001-2003 Waldo Bastian <bastian@kde.org>
 *               2001-2003 David Faure <faure@kde.org>
 *               2001-2003 Daniel Naber <dnaber@kde.org>
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

#include "khtml_ext.h"
#include "khtmlview.h"
#include "khtml_pagecache.h"
#include "rendering/render_form.h"
#include "rendering/render_image.h"
#include "html/html_imageimpl.h"
#include "misc/loader.h"
#include "dom/html_form.h"
#include "dom/html_image.h"
#include "dom/dom_string.h"
#include "dom/html_document.h"
#include "dom/dom_element.h"
#include "xml/dom_elementimpl.h"
#include <QClipboard>
#include <QtCore/QFileInfo>
#include <QFileDialog>
#include <QMenu>
#include <QtCore/QUrl>
#include <QtCore/QMetaEnum>
#include <QtCore/QMimeData>
#include <qstandardpaths.h>
#include <qinputdialog.h>
#include <assert.h>

#include <kconfiggroup.h>
#include <QDebug>
#include <klocalizedstring.h>
#include <kjobuidelegate.h>
#include <kio/job.h>
#include <kshell.h>
#include <qsavefile.h>
#include <kstringhandler.h>
#include <ktoolinvocation.h>
#include <kmessagebox.h>
#include <krun.h>
#include <kurifilter.h>
#include <kdesktopfile.h>
#include <qtemporaryfile.h>
#include "khtml_global.h"
#include <kstandardaction.h>
#include <kactioncollection.h>
#include <kactionmenu.h>

#include "khtmlpart_p.h"

KHTMLPartBrowserExtension::KHTMLPartBrowserExtension( KHTMLPart *parent )
: KParts::BrowserExtension( parent )
{
    m_part = parent;
    setURLDropHandlingEnabled( true );

    enableAction( "cut", false );
    enableAction( "copy", false );
    enableAction( "paste", false );

    m_connectedToClipboard = false;
}

int KHTMLPartBrowserExtension::xOffset()
{
    return m_part->view()->contentsX();
}

int KHTMLPartBrowserExtension::yOffset()
{
  return m_part->view()->contentsY();
}

void KHTMLPartBrowserExtension::saveState( QDataStream &stream )
{
  //qDebug() << "saveState!";
  m_part->saveState( stream );
}

void KHTMLPartBrowserExtension::restoreState( QDataStream &stream )
{
  //qDebug() << "restoreState!";
  m_part->restoreState( stream );
}

void KHTMLPartBrowserExtension::editableWidgetFocused( QWidget *widget )
{
    m_editableFormWidget = widget;
    updateEditActions();

    if ( !m_connectedToClipboard && m_editableFormWidget )
    {
        connect( QApplication::clipboard(), SIGNAL(dataChanged()),
                 this, SLOT(updateEditActions()) );

        if ( m_editableFormWidget->inherits( "QLineEdit" ) || m_editableFormWidget->inherits( "QTextEdit" ) )
            connect( m_editableFormWidget, SIGNAL(selectionChanged()),
                     this, SLOT(updateEditActions()) );

        m_connectedToClipboard = true;
    }
    editableWidgetFocused();
}

void KHTMLPartBrowserExtension::editableWidgetBlurred( QWidget * /*widget*/ )
{
    QWidget *oldWidget = m_editableFormWidget;

    m_editableFormWidget = 0;
    enableAction( "cut", false );
    enableAction( "paste", false );
    m_part->emitSelectionChanged();

    if ( m_connectedToClipboard )
    {
        disconnect( QApplication::clipboard(), SIGNAL(dataChanged()),
                    this, SLOT(updateEditActions()) );

        if ( oldWidget )
        {
            if ( oldWidget->inherits( "QLineEdit" ) || oldWidget->inherits( "QTextEdit" ) )
                disconnect( oldWidget, SIGNAL(selectionChanged()),
                            this, SLOT(updateEditActions()) );
        }

        m_connectedToClipboard = false;
    }
    editableWidgetBlurred();
}

void KHTMLPartBrowserExtension::setExtensionProxy( KParts::BrowserExtension *proxy )
{
    if ( m_extensionProxy )
    {
        disconnect( m_extensionProxy, SIGNAL(enableAction(const char*,bool)),
                    this, SLOT(extensionProxyActionEnabled(const char*,bool)) );
        if ( m_extensionProxy->inherits( "KHTMLPartBrowserExtension" ) )
        {
            disconnect( m_extensionProxy, SIGNAL(editableWidgetFocused()),
                        this, SLOT(extensionProxyEditableWidgetFocused()) );
            disconnect( m_extensionProxy, SIGNAL(editableWidgetBlurred()),
                        this, SLOT(extensionProxyEditableWidgetBlurred()) );
        }
    }

    m_extensionProxy = proxy;

    if ( m_extensionProxy )
    {
        connect( m_extensionProxy, SIGNAL(enableAction(const char*,bool)),
                 this, SLOT(extensionProxyActionEnabled(const char*,bool)) );
        if ( m_extensionProxy->inherits( "KHTMLPartBrowserExtension" ) )
        {
            connect( m_extensionProxy, SIGNAL(editableWidgetFocused()),
                     this, SLOT(extensionProxyEditableWidgetFocused()) );
            connect( m_extensionProxy, SIGNAL(editableWidgetBlurred()),
                     this, SLOT(extensionProxyEditableWidgetBlurred()) );
        }

        enableAction( "cut", m_extensionProxy->isActionEnabled( "cut" ) );
        enableAction( "copy", m_extensionProxy->isActionEnabled( "copy" ) );
        enableAction( "paste", m_extensionProxy->isActionEnabled( "paste" ) );
    }
    else
    {
        updateEditActions();
        enableAction( "copy", false ); // ### re-check this
    }
}

void KHTMLPartBrowserExtension::cut()
{
    if ( m_extensionProxy )
    {
        callExtensionProxyMethod( "cut" );
        return;
    }

    if ( !m_editableFormWidget )
        return;

    QLineEdit* lineEdit = qobject_cast<QLineEdit *>( m_editableFormWidget );
    if ( lineEdit && !lineEdit->isReadOnly() )
        lineEdit->cut();
    QTextEdit* textEdit = qobject_cast<QTextEdit *>( m_editableFormWidget );
    if ( textEdit && !textEdit->isReadOnly() )
        textEdit->cut();
}

void KHTMLPartBrowserExtension::copy()
{
    if ( m_extensionProxy )
    {
        callExtensionProxyMethod( "copy" );
        return;
    }

    if ( !m_editableFormWidget )
    {
        // get selected text and paste to the clipboard
        QString text = m_part->selectedText();
        text.replace( QChar( 0xa0 ), ' ' );
        //qDebug() << text;

        QClipboard *cb = QApplication::clipboard();
        disconnect( cb, SIGNAL(selectionChanged()), m_part, SLOT(slotClearSelection()) );
#ifndef QT_NO_MIMECLIPBOARD
  QString htmltext;
  /*
   * When selectionModeEnabled, that means the user has just selected
   * the text, not ctrl+c to copy it.  The selection clipboard
   * doesn't seem to support mime type, so to save time, don't calculate
   * the selected text as html.
   * optomisation disabled for now until everything else works.
  */
  //if(!cb->selectionModeEnabled())
      htmltext = m_part->selectedTextAsHTML();
  QMimeData *mimeData = new QMimeData;
  mimeData->setText(text);
  if(!htmltext.isEmpty()) {
      htmltext.replace( QChar( 0xa0 ), ' ' );
      mimeData->setHtml(htmltext);
  }
        cb->setMimeData(mimeData);
#else
  cb->setText(text);
#endif

        connect( cb, SIGNAL(selectionChanged()), m_part, SLOT(slotClearSelection()) );
    }
    else
    {
        QLineEdit* lineEdit = qobject_cast<QLineEdit *>( m_editableFormWidget );
        if ( lineEdit )
            lineEdit->copy();
        QTextEdit* textEdit = qobject_cast<QTextEdit *>( m_editableFormWidget );
        if ( textEdit )
            textEdit->copy();
    }
}

void KHTMLPartBrowserExtension::searchProvider()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (action) {
        QUrl url = action->data().toUrl();
        if (url.host().isEmpty()) {
            KUriFilterData data(action->data().toString());
            if (KUriFilter::self()->filterSearchUri(data, KUriFilter::WebShortcutFilter))
                url = data.uri();
        }

        KParts::BrowserArguments browserArgs;
        browserArgs.frameName = "_blank";
        emit m_part->browserExtension()->openUrlRequest( url, KParts::OpenUrlArguments(), browserArgs );
    }
}

void KHTMLPartBrowserExtension::paste()
{
    if ( m_extensionProxy )
    {
        callExtensionProxyMethod( "paste" );
        return;
    }

    if ( !m_editableFormWidget )
        return;

    QLineEdit* lineEdit = qobject_cast<QLineEdit *>( m_editableFormWidget );
    if ( lineEdit && !lineEdit->isReadOnly() )
        lineEdit->paste();
    QTextEdit* textEdit = qobject_cast<QTextEdit *>( m_editableFormWidget );
    if ( textEdit && !textEdit->isReadOnly() )
        textEdit->paste();
}

void KHTMLPartBrowserExtension::callExtensionProxyMethod( const char *method )
{
    if ( !m_extensionProxy )
        return;

    QMetaObject::invokeMethod(m_extensionProxy, method, Qt::DirectConnection);
}

void KHTMLPartBrowserExtension::updateEditActions()
{
    if ( !m_editableFormWidget )
    {
        enableAction( "cut", false );
        enableAction( "copy", false );
        enableAction( "paste", false );
        return;
    }

    // ### duplicated from KonqMainWindow::slotClipboardDataChanged
#ifndef QT_NO_MIMECLIPBOARD // Handle minimalized versions of Qt Embedded
    const QMimeData *data = QApplication::clipboard()->mimeData();
    enableAction("paste", data->hasText());
#else
    QString data=QApplication::clipboard()->text();
    enableAction( "paste", data.contains("://"));
#endif
    bool hasSelection = false;

    if( m_editableFormWidget) {
        if ( qobject_cast<QLineEdit*>(m_editableFormWidget))
            hasSelection = static_cast<QLineEdit *>( &(*m_editableFormWidget) )->hasSelectedText();
        else if(qobject_cast<QTextEdit*>(m_editableFormWidget))
            hasSelection = static_cast<QTextEdit *>( &(*m_editableFormWidget) )->textCursor().hasSelection();
    }

    enableAction( "copy", hasSelection );
    enableAction( "cut", hasSelection );
}

void KHTMLPartBrowserExtension::extensionProxyEditableWidgetFocused() {
  editableWidgetFocused();
}

void KHTMLPartBrowserExtension::extensionProxyEditableWidgetBlurred() {
  editableWidgetBlurred();
}

void KHTMLPartBrowserExtension::extensionProxyActionEnabled( const char *action, bool enable )
{
    // only forward enableAction calls for actions we actually do forward
    if ( strcmp( action, "cut" ) == 0 ||
         strcmp( action, "copy" ) == 0 ||
         strcmp( action, "paste" ) == 0 ) {
        enableAction( action, enable );
    }
}

void KHTMLPartBrowserExtension::reparseConfiguration()
{
  m_part->reparseConfiguration();
}

void KHTMLPartBrowserExtension::print()
{
  m_part->view()->print();
}

void KHTMLPartBrowserExtension::disableScrolling()
{
  QScrollArea *scrollArea = m_part->view();
  if (scrollArea) {
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  }
}

class KHTMLPopupGUIClient::KHTMLPopupGUIClientPrivate
{
public:
  KHTMLPart *m_khtml;
  QUrl m_url;
  QUrl m_imageURL;
  QPixmap m_pixmap;
  QString m_suggestedFilename;
    KActionCollection* m_actionCollection;
    KParts::BrowserExtension::ActionGroupMap actionGroups;
};


KHTMLPopupGUIClient::KHTMLPopupGUIClient( KHTMLPart *khtml, const QUrl &url )
    : QObject( khtml ), d(new KHTMLPopupGUIClientPrivate)
{
    d->m_khtml = khtml;
    d->m_url = url;
    d->m_actionCollection = new KActionCollection(this);
    bool isImage = false;
    bool hasSelection = khtml->hasSelection();

    DOM::Element e = khtml->nodeUnderMouse();

    if ( !e.isNull() && (e.elementId() == ID_IMG ||
                         (e.elementId() == ID_INPUT && !static_cast<DOM::HTMLInputElement>(e).src().isEmpty())))
    {
        if (e.elementId() == ID_IMG) {
            DOM::HTMLImageElementImpl *ie = static_cast<DOM::HTMLImageElementImpl*>(e.handle());
            khtml::RenderImage *ri = dynamic_cast<khtml::RenderImage*>(ie->renderer());
            if (ri && ri->contentObject()) {
                d->m_suggestedFilename = static_cast<khtml::CachedImage*>(ri->contentObject())->suggestedFilename();
            }
        }
        isImage=true;
    }

    if (hasSelection) {
        QList<QAction *> editActions;
        QAction* copyAction = d->m_actionCollection->addAction( KStandardAction::Copy, "copy",
                                                                d->m_khtml->browserExtension(), SLOT(copy()) );

        copyAction->setText(i18n("&Copy Text"));
        copyAction->setEnabled(d->m_khtml->browserExtension()->isActionEnabled( "copy" ));
        editActions.append(copyAction);

        editActions.append(khtml->actionCollection()->action("selectAll"));

        addSearchActions(editActions);

        QString selectedTextURL = selectedTextAsOneLine(d->m_khtml);
        if ( selectedTextURL.contains("://") && QUrl(selectedTextURL).isValid() ) {
            if (selectedTextURL.length() > 18) {
                selectedTextURL.truncate(15);
                selectedTextURL += "...";
            }
            QAction *action = new QAction(i18n("Open '%1'", selectedTextURL), this);
            d->m_actionCollection->addAction( "openSelection", action );
            action->setIcon( QIcon::fromTheme( "window-new" ) );
            connect( action, SIGNAL(triggered(bool)), this, SLOT(openSelection()) );
            editActions.append(action);
        }

        QAction* separator = new QAction(d->m_actionCollection);
        separator->setSeparator(true);
        editActions.append(separator);

        d->actionGroups.insert("editactions", editActions);
    }

    if (!url.isEmpty()) {
        QList<QAction *> linkActions;
        if (url.scheme() == "mailto") {
            QAction *action = new QAction( i18n( "&Copy Email Address" ), this );
            d->m_actionCollection->addAction( "copylinklocation", action );
            connect( action, SIGNAL(triggered(bool)), this, SLOT(slotCopyLinkLocation()) );
            linkActions.append(action);
        } else {
            QAction *action = new QAction( i18n( "&Save Link As..." ), this );
            d->m_actionCollection->addAction( "savelinkas", action );
            connect( action, SIGNAL(triggered(bool)), this, SLOT(slotSaveLinkAs()) );
            linkActions.append(action);

            action = new QAction( i18n( "&Copy Link Address" ), this );
            d->m_actionCollection->addAction( "copylinklocation", action );
            connect( action, SIGNAL(triggered(bool)), this, SLOT(slotCopyLinkLocation()) );
            linkActions.append(action);
        }
        d->actionGroups.insert("linkactions", linkActions);
    }

    QList<QAction *> partActions;
    // frameset? -> add "Reload Frame" etc.
    if (!hasSelection) {
        if ( khtml->parentPart() ) {
            KActionMenu* menu = new KActionMenu( i18nc("@title:menu HTML frame/iframe", "Frame"), this);
            QAction *action = new QAction( i18n( "Open in New &Window" ), this );
            d->m_actionCollection->addAction( "frameinwindow", action );
            action->setIcon( QIcon::fromTheme( "window-new" ) );
            connect( action, SIGNAL(triggered(bool)), this, SLOT(slotFrameInWindow()) );
            menu->addAction(action);

            action = new QAction( i18n( "Open in &This Window" ), this );
            d->m_actionCollection->addAction( "frameintop", action );
            connect( action, SIGNAL(triggered(bool)), this, SLOT(slotFrameInTop()) );
            menu->addAction(action);

            action = new QAction( i18n( "Open in &New Tab" ), this );
            d->m_actionCollection->addAction( "frameintab", action );
            action->setIcon( QIcon::fromTheme( "tab-new" ) );
            connect( action, SIGNAL(triggered(bool)), this, SLOT(slotFrameInTab()) );
            menu->addAction(action);

            action = new QAction(d->m_actionCollection);
            action->setSeparator(true);
            menu->addAction(action);

            action = new QAction( i18n( "Reload Frame" ), this );
            d->m_actionCollection->addAction( "reloadframe", action );
            connect( action, SIGNAL(triggered(bool)), this, SLOT(slotReloadFrame()) );
            menu->addAction(action);

            action = new QAction( i18n( "Print Frame..." ), this );
            d->m_actionCollection->addAction( "printFrame", action );
            action->setIcon( QIcon::fromTheme( "document-print-frame" ) );
            connect( action, SIGNAL(triggered(bool)), d->m_khtml->browserExtension(), SLOT(print()) );
            menu->addAction(action);

            action = new QAction( i18n( "Save &Frame As..." ), this );
            d->m_actionCollection->addAction( "saveFrame", action );
            connect( action, SIGNAL(triggered(bool)), d->m_khtml, SLOT(slotSaveFrame()) );
            menu->addAction(action);

            action = new QAction( i18n( "View Frame Source" ), this );
            d->m_actionCollection->addAction( "viewFrameSource", action );
            connect( action, SIGNAL(triggered(bool)), d->m_khtml, SLOT(slotViewDocumentSource()) );
            menu->addAction(action);

            action = new QAction( i18n( "View Frame Information" ), this );
            d->m_actionCollection->addAction( "viewFrameInfo", action );
            connect( action, SIGNAL(triggered(bool)), d->m_khtml, SLOT(slotViewPageInfo()) );

            action = new QAction(d->m_actionCollection);
            action->setSeparator(true);
            menu->addAction(action);

            if ( KHTMLGlobal::defaultHTMLSettings()->isAdFilterEnabled() ) {
                if ( khtml->d->m_frame->m_type == khtml::ChildFrame::IFrame ) {
                    action = new QAction( i18n( "Block IFrame..." ), this );
                    d->m_actionCollection->addAction( "blockiframe", action );
                    connect( action, SIGNAL(triggered(bool)), this, SLOT(slotBlockIFrame()) );
                    menu->addAction(action);
                }
            }

            partActions.append(menu);
        }
    }

    if (isImage) {
        if ( e.elementId() == ID_IMG ) {
            d->m_imageURL = QUrl( static_cast<DOM::HTMLImageElement>( e ).src().string() );
            DOM::HTMLImageElementImpl *imageimpl = static_cast<DOM::HTMLImageElementImpl *>( e.handle() );
            Q_ASSERT(imageimpl);
            if(imageimpl) // should be true always.  right?
            {
                if(imageimpl->complete()) {
                    d->m_pixmap = imageimpl->currentPixmap();
                }
            }
        }
        else
            d->m_imageURL = QUrl( static_cast<DOM::HTMLInputElement>( e ).src().string() );
        QAction *action = new QAction( i18n( "Save Image As..." ), this );
        d->m_actionCollection->addAction( "saveimageas", action );
        connect( action, SIGNAL(triggered(bool)), this, SLOT(slotSaveImageAs()) );
        partActions.append(action);

        action = new QAction( i18n( "Send Image..." ), this );
        d->m_actionCollection->addAction( "sendimage", action );
        connect( action, SIGNAL(triggered(bool)), this, SLOT(slotSendImage()) );
        partActions.append(action);

#ifndef QT_NO_MIMECLIPBOARD
        action = new QAction( i18n( "Copy Image" ), this );
        d->m_actionCollection->addAction( "copyimage", action );
        action->setEnabled(!d->m_pixmap.isNull());
        connect( action, SIGNAL(triggered(bool)), this, SLOT(slotCopyImage()) );
        partActions.append(action);
#endif

        if(d->m_pixmap.isNull()) {    //fallback to image location if still loading the image.  this will always be true if ifdef QT_NO_MIMECLIPBOARD
            action = new QAction( i18n( "Copy Image Location" ), this );
            d->m_actionCollection->addAction( "copyimagelocation", action );
            connect( action, SIGNAL(triggered(bool)), this, SLOT(slotCopyImageLocation()) );
            partActions.append(action);
        }

        QString actionText = d->m_suggestedFilename.isEmpty() ?
                                   KStringHandler::csqueeze(d->m_imageURL.fileName()+d->m_imageURL.query(), 25)
                                   : d->m_suggestedFilename;
        action = new QAction( i18n("View Image (%1)", actionText.replace("&", "&&")), this );
        d->m_actionCollection->addAction( "viewimage", action );
        connect( action, SIGNAL(triggered(bool)), this, SLOT(slotViewImage()) );
        partActions.append(action);

        if (KHTMLGlobal::defaultHTMLSettings()->isAdFilterEnabled()) {
            action = new QAction( i18n( "Block Image..." ), this );
            d->m_actionCollection->addAction( "blockimage", action );
            connect( action, SIGNAL(triggered(bool)), this, SLOT(slotBlockImage()) );
            partActions.append(action);

            if (!d->m_imageURL.host().isEmpty() &&
                !d->m_imageURL.scheme().isEmpty())
            {
                action = new QAction( i18n( "Block Images From %1" , d->m_imageURL.host()), this );
                d->m_actionCollection->addAction( "blockhost", action );
                connect( action, SIGNAL(triggered(bool)), this, SLOT(slotBlockHost()) );
                partActions.append(action);
            }
        }
        QAction* separator = new QAction(d->m_actionCollection);
        separator->setSeparator(true);
        partActions.append(separator);
    }

    if ( isImage || url.isEmpty() ) {
        QAction *action = new QAction( i18n( "Stop Animations" ), this );
        d->m_actionCollection->addAction( "stopanimations", action );
        connect( action, SIGNAL(triggered(bool)), this, SLOT(slotStopAnimations()) );
        partActions.append(action);
        QAction* separator = new QAction(d->m_actionCollection);
        separator->setSeparator(true);
        partActions.append(separator);
    }
    if (!hasSelection && url.isEmpty()) { // only when right-clicking on the page itself
        partActions.append(khtml->actionCollection()->action("viewDocumentSource"));
    }
    if (!hasSelection && url.isEmpty() && !isImage) {
        partActions.append(khtml->actionCollection()->action("setEncoding"));
    }
    d->actionGroups.insert("partactions", partActions);
}

KHTMLPopupGUIClient::~KHTMLPopupGUIClient()
{
    delete d->m_actionCollection;
    delete d;
}

void KHTMLPopupGUIClient::addSearchActions(QList<QAction *>& editActions)
{
    QString selectedText = d->m_khtml->simplifiedSelectedText();
    // replace linefeeds with spaces
    selectedText.replace(QChar(10), QChar(32)).trimmed();

    if (selectedText.isEmpty())
        return;

    KUriFilterData data(selectedText);
    QStringList alternateProviders;
    alternateProviders << "google" << "google_groups" << "google_news" << "webster" << "dmoz" << "wikipedia";
    data.setAlternateSearchProviders(alternateProviders);
    data.setAlternateDefaultSearchProvider("google");

    if (KUriFilter::self()->filterSearchUri(data, KUriFilter::NormalTextFilter)) {
        const QString squeezedText = KStringHandler::rsqueeze(selectedText, 21);
        QAction *action = new QAction(i18n("Search for '%1' with %2",
                                           squeezedText, data.searchProvider()), this);
        action->setData(QUrl(data.uri()));
        action->setIcon(QIcon::fromTheme(data.iconName()));
        connect(action, SIGNAL(triggered(bool)), d->m_khtml->browserExtension(), SLOT(searchProvider()));
        d->m_actionCollection->addAction("defaultSearchProvider", action);
        editActions.append(action);

        const QStringList preferredSearchProviders = data.preferredSearchProviders();
        if (!preferredSearchProviders.isEmpty()) {
            KActionMenu* providerList = new KActionMenu(i18n("Search for '%1' with", squeezedText), this);
            Q_FOREACH(const QString &searchProvider, preferredSearchProviders) {
                if (searchProvider == data.searchProvider())
                    continue;
                QAction *action = new QAction(searchProvider, this);
                action->setData(data.queryForPreferredSearchProvider(searchProvider));
                d->m_actionCollection->addAction(searchProvider, action);
                action->setIcon(QIcon::fromTheme(data.iconNameForPreferredSearchProvider(searchProvider)));
                connect(action, SIGNAL(triggered(bool)), d->m_khtml->browserExtension(), SLOT(searchProvider()));
                providerList->addAction(action);
            }
            d->m_actionCollection->addAction("searchProviderList", providerList);
            editActions.append(providerList);
        }
    }
}

QString KHTMLPopupGUIClient::selectedTextAsOneLine(KHTMLPart* part)
{
    QString text = part->simplifiedSelectedText();
    // in addition to what simplifiedSelectedText does,
    // remove linefeeds and any whitespace surrounding it (#113177),
    // to get it all in a single line.
    text.remove(QRegExp("[\\s]*\\n+[\\s]*"));
    return text;
}

void KHTMLPopupGUIClient::openSelection()
{
    KParts::BrowserArguments browserArgs;
    browserArgs.frameName = "_blank";

    QUrl url(selectedTextAsOneLine(d->m_khtml));
    emit d->m_khtml->browserExtension()->openUrlRequest(url, KParts::OpenUrlArguments(), browserArgs);
}

KParts::BrowserExtension::ActionGroupMap KHTMLPopupGUIClient::actionGroups() const
{
    return d->actionGroups;
}

void KHTMLPopupGUIClient::slotSaveLinkAs()
{
  KIO::MetaData metaData;
  metaData["referrer"] = d->m_khtml->referrer();
  saveURL( d->m_khtml->widget(), i18n( "Save Link As" ), d->m_url, metaData );
}

void KHTMLPopupGUIClient::slotSendImage()
{
    QStringList urls;
    urls.append( d->m_imageURL.url());
    QString subject = d->m_imageURL.url();
    KToolInvocation::invokeMailer(QString(), QString(), QString(), subject,
                       QString(), //body
                       QString(),
                       urls); // attachments


}

void KHTMLPopupGUIClient::slotSaveImageAs()
{
  KIO::MetaData metaData;
  metaData["referrer"] = d->m_khtml->referrer();
  saveURL( d->m_khtml->widget(), i18n( "Save Image As" ), d->m_imageURL, metaData, QString(), 0, d->m_suggestedFilename );
}

void KHTMLPopupGUIClient::slotBlockHost()
{
    QString name=d->m_imageURL.scheme()+"://"+d->m_imageURL.host()+"/*";
    KHTMLGlobal::defaultHTMLSettings()->addAdFilter( name );
    d->m_khtml->reparseConfiguration();
}

void KHTMLPopupGUIClient::slotBlockImage()
{
    bool ok = false;

    QString url = QInputDialog::getText( d->m_khtml->widget(), i18n("Add URL to Filter"),
                                         i18n("Enter the URL:"), QLineEdit::Normal,
                                         d->m_imageURL.url(), &ok);
    if ( ok ) {
        KHTMLGlobal::defaultHTMLSettings()->addAdFilter( url );
        d->m_khtml->reparseConfiguration();
    }
}

void KHTMLPopupGUIClient::slotBlockIFrame()
{
    bool ok = false;
    QString url = QInputDialog::getText( d->m_khtml->widget(), i18n( "Add URL to Filter"),
                                         i18n("Enter the URL:"), QLineEdit::Normal,
                                         d->m_khtml->url().toString(), &ok );
    if ( ok ) {
        KHTMLGlobal::defaultHTMLSettings()->addAdFilter( url );
        d->m_khtml->reparseConfiguration();
    }
}

void KHTMLPopupGUIClient::slotCopyLinkLocation()
{
  QUrl safeURL(d->m_url);
  safeURL.setPassword(QString());
#ifndef QT_NO_MIMECLIPBOARD
  // Set it in both the mouse selection and in the clipboard
  QMimeData* mimeData = new QMimeData;
  mimeData->setUrls( QList<QUrl>() << safeURL );
  QApplication::clipboard()->setMimeData( mimeData, QClipboard::Clipboard );

  mimeData = new QMimeData;
  mimeData->setUrls( QList<QUrl>() << safeURL );
  QApplication::clipboard()->setMimeData( mimeData, QClipboard::Selection );

#else
  QApplication::clipboard()->setText( safeURL.url() ); //FIXME(E): Handle multiple entries
#endif
}

void KHTMLPopupGUIClient::slotStopAnimations()
{
  d->m_khtml->stopAnimations();
}

void KHTMLPopupGUIClient::slotCopyImage()
{
#ifndef QT_NO_MIMECLIPBOARD
  QUrl safeURL(d->m_imageURL);
  safeURL.setPassword(QString());

  // Set it in both the mouse selection and in the clipboard
  QMimeData* mimeData = new QMimeData;
  mimeData->setImageData( d->m_pixmap );
  mimeData->setUrls( QList<QUrl>() << safeURL );
  QApplication::clipboard()->setMimeData( mimeData, QClipboard::Clipboard );

  mimeData = new QMimeData;
  mimeData->setImageData( d->m_pixmap );
  mimeData->setUrls( QList<QUrl>() << safeURL );
  QApplication::clipboard()->setMimeData( mimeData, QClipboard::Selection );
#else
  // qDebug() << "slotCopyImage called when the clipboard does not support this.  This should not be possible.";
#endif
}

void KHTMLPopupGUIClient::slotCopyImageLocation()
{
  QUrl safeURL(d->m_imageURL);
  safeURL.setPassword(QString());
#ifndef QT_NO_MIMECLIPBOARD
  // Set it in both the mouse selection and in the clipboard
  QMimeData* mimeData = new QMimeData;
  mimeData->setUrls( QList<QUrl>() << safeURL );
  QApplication::clipboard()->setMimeData( mimeData, QClipboard::Clipboard );
  mimeData = new QMimeData;
  mimeData->setUrls( QList<QUrl>() << safeURL );
  QApplication::clipboard()->setMimeData( mimeData, QClipboard::Selection );
#else
  QApplication::clipboard()->setText( safeURL.url() ); //FIXME(E): Handle multiple entries
#endif
}

void KHTMLPopupGUIClient::slotViewImage()
{
  d->m_khtml->browserExtension()->createNewWindow(d->m_imageURL);
}

void KHTMLPopupGUIClient::slotReloadFrame()
{
  KParts::OpenUrlArguments args = d->m_khtml->arguments();
  args.setReload( true );
  args.metaData()["referrer"] = d->m_khtml->pageReferrer();
  // reload document
  d->m_khtml->closeUrl();
  d->m_khtml->setArguments( args );
  d->m_khtml->openUrl( d->m_khtml->url() );
}

void KHTMLPopupGUIClient::slotFrameInWindow()
{
  KParts::OpenUrlArguments args = d->m_khtml->arguments();
  args.metaData()["referrer"] = d->m_khtml->pageReferrer();
  KParts::BrowserArguments browserArgs( d->m_khtml->browserExtension()->browserArguments() );
  browserArgs.setForcesNewWindow(true);
  emit d->m_khtml->browserExtension()->createNewWindow( d->m_khtml->url(), args, browserArgs );
}

void KHTMLPopupGUIClient::slotFrameInTop()
{
  KParts::OpenUrlArguments args = d->m_khtml->arguments();
  args.metaData()["referrer"] = d->m_khtml->pageReferrer();
  KParts::BrowserArguments browserArgs( d->m_khtml->browserExtension()->browserArguments() );
  browserArgs.frameName = "_top";
  emit d->m_khtml->browserExtension()->openUrlRequest( d->m_khtml->url(), args, browserArgs );
}

void KHTMLPopupGUIClient::slotFrameInTab()
{
  KParts::OpenUrlArguments args = d->m_khtml->arguments();
  args.metaData()["referrer"] = d->m_khtml->pageReferrer();
  KParts::BrowserArguments browserArgs( d->m_khtml->browserExtension()->browserArguments() );
  browserArgs.setNewTab(true);
  emit d->m_khtml->browserExtension()->createNewWindow( d->m_khtml->url(), args, browserArgs );
}

void KHTMLPopupGUIClient::saveURL( QWidget *parent, const QString &caption,
                                   const QUrl &url,
                                   const QMap<QString, QString> &metadata,
                                   const QString &filter, long cacheId,
                                   const QString & suggestedFilename )
{
  QString name = QLatin1String( "index.html" );
  if ( !suggestedFilename.isEmpty() )
    name = suggestedFilename;
  else if ( !url.fileName().isEmpty() )
    name = url.fileName();

  QUrl destURL;
  int query;
  do {
    query = KMessageBox::Yes;
    // convert filename to URL using fromLocalFile to avoid trouble with ':' in filenames (#184202)
    destURL = QFileDialog::getSaveFileUrl( parent, caption, QUrl::fromLocalFile(name), filter );
      if( destURL.isLocalFile() )
      {
        QFileInfo info( destURL.toLocalFile() );
        if( info.exists() ) {
          // TODO: use KIO::RenameDlg (shows more information)
          query = KMessageBox::warningContinueCancel( parent, i18n( "A file named \"%1\" already exists. " "Are you sure you want to overwrite it?" ,  info.fileName() ), i18n( "Overwrite File?" ), KGuiItem(i18n( "Overwrite" )) );
        }
       }
   } while ( query == KMessageBox::Cancel );

  if ( destURL.isValid() )
    saveURL(parent, url, destURL, metadata, cacheId);
}

void KHTMLPopupGUIClient::saveURL( QWidget* parent, const QUrl &url, const QUrl &destURL,
                                   const QMap<QString, QString> &metadata,
                                   long cacheId )
{
    if ( destURL.isValid() )
    {
        bool saved = false;
        if (KHTMLPageCache::self()->isComplete(cacheId))
        {
            if (destURL.isLocalFile())
            {
                QSaveFile destFile(destURL.toLocalFile());
                if (destFile.open(QIODevice::WriteOnly)) {
                    QDataStream stream(&destFile);
                    KHTMLPageCache::self()->saveData(cacheId, &stream);
                    destFile.commit();
                    saved = true;
                }
            }
            else
            {
                // save to temp file, then move to final destination.
                QTemporaryFile destFile;
                if (destFile.open())
                {
                    QDataStream stream ( &destFile );
                    KHTMLPageCache::self()->saveData(cacheId, &stream);
                    QUrl url2 = QUrl();
                    url2.setPath(destFile.fileName());
                    KIO::file_move(url2, destURL, -1, KIO::Overwrite);
                    saved = true;
                }
            }
        }
        if(!saved)
        {
          // DownloadManager <-> konqueror integration
          // find if the integration is enabled
          // the empty key  means no integration
          // only use download manager for non-local urls!
          bool downloadViaKIO = true;
          if ( !url.isLocalFile() )
          {
            KConfigGroup cfg = KSharedConfig::openConfig("konquerorrc", KConfig::NoGlobals)->group("HTML Settings");
            QString downloadManger = cfg.readPathEntry("DownloadManager", QString());
            if (!downloadManger.isEmpty())
            {
                // then find the download manager location
                // qDebug() << "Using: "<<downloadManger <<" as Download Manager";
                QString cmd = QStandardPaths::findExecutable(downloadManger);
                if (cmd.isEmpty())
                {
                    QString errMsg=i18n("The Download Manager (%1) could not be found in your $PATH ", downloadManger);
                    QString errMsgEx= i18n("Try to reinstall it  \n\nThe integration with Konqueror will be disabled.");
                    KMessageBox::detailedSorry(0,errMsg,errMsgEx);
                    cfg.writePathEntry("DownloadManager",QString());
                    cfg.sync ();
                }
                else
                {
                    downloadViaKIO = false;
                    QUrl cleanDest = destURL;
                    cleanDest.setPassword( QString() ); // don't put password into commandline
                    cmd += ' ' + KShell::quoteArg(url.url()) + ' ' +
                           KShell::quoteArg(cleanDest.url());
                    // qDebug() << "Calling command  "<<cmd;
                    KRun::runCommand(cmd, parent->topLevelWidget());
                }
            }
          }

          if ( downloadViaKIO )
          {
              KParts::BrowserRun::saveUrlUsingKIO(url, destURL, parent, metadata);
          }
        } //end if(!saved)
    }
}

KHTMLPartBrowserHostExtension::KHTMLPartBrowserHostExtension( KHTMLPart *part )
: KParts::BrowserHostExtension( part )
{
  m_part = part;
}

KHTMLPartBrowserHostExtension::~KHTMLPartBrowserHostExtension()
{
}

QStringList KHTMLPartBrowserHostExtension::frameNames() const
{
  return m_part->frameNames();
}

const QList<KParts::ReadOnlyPart*> KHTMLPartBrowserHostExtension::frames() const
{
  return m_part->frames();
}

bool KHTMLPartBrowserHostExtension::openUrlInFrame(const QUrl &url, const KParts::OpenUrlArguments& arguments, const KParts::BrowserArguments &browserArguments)
{
  return m_part->openUrlInFrame( url, arguments, browserArguments );
}

KParts::BrowserHostExtension* KHTMLPartBrowserHostExtension::findFrameParent( KParts::ReadOnlyPart
      *callingPart, const QString &frame )
{
    KHTMLPart *parentPart = m_part->d->findFrameParent(callingPart, frame, 0, true /* navigation*/);
    if (parentPart)
       return parentPart->browserHostExtension();
    return 0;
}


// defined in khtml_part.cpp
extern const int KHTML_NO_EXPORT fastZoomSizes[];
extern const int KHTML_NO_EXPORT fastZoomSizeCount;

KHTMLZoomFactorAction::KHTMLZoomFactorAction( KHTMLPart *part, bool direction, const QString &icon, const QString &text, QObject *parent )
    : KSelectAction( text, parent )
{
    setIcon( QIcon::fromTheme( icon ) );

    setToolBarMode(MenuMode);
    setToolButtonPopupMode(QToolButton::DelayedPopup);

    init(part, direction);
}

void KHTMLZoomFactorAction::init(KHTMLPart *part, bool direction)
{
    m_direction = direction;
    m_part = part;

    // xgettext: no-c-format
    addAction( i18n( "Default Font Size (100%)" ) );

    int m = m_direction ? 1 : -1;
    int ofs = fastZoomSizeCount / 2;       // take index of 100%

    // this only works if there is an odd number of elements in fastZoomSizes[]
    for ( int i = m; i != m*(ofs+1); i += m )
    {
        int num = i * m;
        QString numStr = QString::number( num );
        if ( num > 0 ) numStr.prepend( QLatin1Char('+') );

        // xgettext: no-c-format
        addAction( i18n( "%1%" ,  fastZoomSizes[ofs + i] ) );
    }

    connect( selectableActionGroup(), SIGNAL(triggered(QAction*)), this, SLOT(slotTriggered(QAction*)) );
}

KHTMLZoomFactorAction::~KHTMLZoomFactorAction()
{
}

void KHTMLZoomFactorAction::slotTriggered(QAction* action)
{
    int idx = selectableActionGroup()->actions().indexOf(action);

    if (idx == 0)
        m_part->setFontScaleFactor(100);
    else
        m_part->setFontScaleFactor(fastZoomSizes[fastZoomSizeCount/2 + (m_direction ? 1 : -1)*idx]);
    setCurrentAction( 0L );
}

KHTMLTextExtension::KHTMLTextExtension(KHTMLPart* part)
    : KParts::TextExtension(part)
{
    connect(part, SIGNAL(selectionChanged()), this, SIGNAL(selectionChanged()));
}

KHTMLPart* KHTMLTextExtension::part() const
{
    return static_cast<KHTMLPart*>(parent());
}

bool KHTMLTextExtension::hasSelection() const
{
    return part()->hasSelection();
}

QString KHTMLTextExtension::selectedText(Format format) const
{
    switch(format) {
    case PlainText:
        return part()->selectedText();
    case HTML:
        return part()->selectedTextAsHTML();
    }
    return QString();
}

QString KHTMLTextExtension::completeText(Format format) const
{
    switch(format) {
    case PlainText:
        return part()->htmlDocument().body().innerText().string();
    case HTML:
        return part()->htmlDocument().body().innerHTML().string();
    }
    return QString();
}

////

KHTMLHtmlExtension::KHTMLHtmlExtension(KHTMLPart* part)
    : KParts::HtmlExtension(part)
{
}

QUrl KHTMLHtmlExtension::baseUrl() const
{
    return part()->baseURL();
}

bool KHTMLHtmlExtension::hasSelection() const
{
    return part()->hasSelection();
}

KParts::SelectorInterface::QueryMethods KHTMLHtmlExtension::supportedQueryMethods() const
{
    return (KParts::SelectorInterface::SelectedContent | KParts::SelectorInterface::EntireContent);
}

static KParts::SelectorInterface::Element convertDomElement(const DOM::ElementImpl* domElem)
{
    KParts::SelectorInterface::Element elem;
    elem.setTagName(domElem->tagName().string());
    const DOM::NamedAttrMapImpl* attrMap = domElem->attributes(true /*readonly*/);
    if (attrMap) {
        for (unsigned i = 0; i < attrMap->length(); ++i) {
            const DOM::AttributeImpl& attr = attrMap->attributeAt(i);
            elem.setAttribute(attr.localName().string(), attr.value().string());
            // we could have a setAttributeNS too.
        }
    }
    return elem;
}

KParts::SelectorInterface::Element KHTMLHtmlExtension::querySelector(const QString& query, KParts::SelectorInterface::QueryMethod method) const
{
    KParts::SelectorInterface::Element element;

    // If the specified method is None, return an empty list; similarly
    // if the document is null, which may be possible in case of an error
    if (method == KParts::SelectorInterface::None || part()->document().isNull())
        return element;

    if (!(supportedQueryMethods() & method))
        return element;

    switch (method) {
    case KParts::SelectorInterface::EntireContent: {
        int ec = 0; // exceptions are ignored
        WTF::RefPtr<DOM::ElementImpl> domElem = part()->document().handle()->querySelector(query, ec);
        element = convertDomElement(domElem.get());
        break;
    }
    case KParts::SelectorInterface::SelectedContent:
        if (part()->hasSelection()) {
            DOM::Element domElem = part()->selection().cloneContents().querySelector(query);
            element = convertDomElement(static_cast<DOM::ElementImpl*>(domElem.handle()));
        }
        break;
    default:
        break;
    }

    return element;
}

QList<KParts::SelectorInterface::Element> KHTMLHtmlExtension::querySelectorAll(const QString& query, KParts::SelectorInterface::QueryMethod method) const
{
    QList<KParts::SelectorInterface::Element> elements;

    // If the specified method is None, return an empty list; similarly
    // if the document is null, which may be possible in case of an error
    if (method == KParts::SelectorInterface::None || part()->document().isNull())
        return elements;

    // If the specified method is not supported, return an empty list...
    if (!(supportedQueryMethods() & method))
        return elements;

    switch (method) {
    case KParts::SelectorInterface::EntireContent: {
        int ec = 0; // exceptions are ignored
        WTF::RefPtr<DOM::NodeListImpl> nodes = part()->document().handle()->querySelectorAll(query, ec);
        const unsigned long len = nodes->length();
        elements.reserve(len);
        for (unsigned long i = 0; i < len; ++i) {
            DOM::NodeImpl* node = nodes->item(i);
            if (node->isElementNode()) { // should be always true
                elements.append(convertDomElement(static_cast<DOM::ElementImpl*>(node)));
            }
        }
        break;
    }
    case KParts::SelectorInterface::SelectedContent:
        if (part()->hasSelection()) {
            DOM::NodeList nodes = part()->selection().cloneContents().querySelectorAll(query);
            const unsigned long len = nodes.length();
            for (unsigned long i = 0; i < len; ++i) {
                DOM::NodeImpl* node = nodes.item(i).handle();
                if (node->isElementNode())
                    elements.append(convertDomElement(static_cast<DOM::ElementImpl*>(node)));
            }
        }
        break;
    default:
        break;
    }

    return elements;
}

QVariant KHTMLHtmlExtension::htmlSettingsProperty(HtmlSettingsInterface::HtmlSettingsType type) const
{
    if (part()) {
        switch (type) {
        case KParts::HtmlSettingsInterface::AutoLoadImages:
            return part()->autoloadImages();
        case KParts::HtmlSettingsInterface::DnsPrefetchEnabled:
            return (part()->dnsPrefetch() == KHTMLPart::DNSPrefetchEnabled);
        case KParts::HtmlSettingsInterface::JavaEnabled:
            return part()->javaEnabled();
        case KParts::HtmlSettingsInterface::JavascriptEnabled:
            return part()->jScriptEnabled();
        case KParts::HtmlSettingsInterface::MetaRefreshEnabled:
            return part()->metaRefreshEnabled();
        case KParts::HtmlSettingsInterface::PluginsEnabled:
            return part()->pluginsEnabled();
        default:
            break;
        }
    }
    return QVariant();
}

bool KHTMLHtmlExtension::setHtmlSettingsProperty(HtmlSettingsInterface::HtmlSettingsType type, const QVariant& value)
{
    KHTMLPart* p = part();

    if (p) {
        switch (type) {
        case KParts::HtmlSettingsInterface::AutoLoadImages:
            p->setAutoloadImages(value.toBool());
            return true;
        case KParts::HtmlSettingsInterface::DnsPrefetchEnabled:
            p->setDNSPrefetch((value.toBool() ? KHTMLPart::DNSPrefetchEnabled : KHTMLPart::DNSPrefetchDisabled));
            return true;
        case KParts::HtmlSettingsInterface::JavaEnabled:
            p->setJavaEnabled(value.toBool());
            return true;
        case KParts::HtmlSettingsInterface::JavascriptEnabled:
            p->setJScriptEnabled(value.toBool());
            return true;
        case KParts::HtmlSettingsInterface::MetaRefreshEnabled:
            p->setMetaRefreshEnabled(value.toBool());
            return true;
        case KParts::HtmlSettingsInterface::PluginsEnabled:
            p->setPluginsEnabled(value.toBool());
            return true;
        case KParts::HtmlSettingsInterface::UserDefinedStyleSheetURL: {
            const QUrl url (value.toUrl());
            if (url.scheme() == QLatin1String("data")) {
                const QByteArray data (url.path(QUrl::FullyEncoded).toLatin1());
                if (!data.isEmpty()) {
                    const int index = data.indexOf(',');
                    const QByteArray decodedData ((index > -1 ? QByteArray::fromBase64(data.mid(index)) : QByteArray()));
                    p->setUserStyleSheet(QString::fromUtf8(decodedData.constData(), decodedData.size()));
                }
            } else {
                p->setUserStyleSheet(url);
            }
            return true;
        }
        default:
            break; // Unsupported property...
        }
    }

    return false;
}


KHTMLPart* KHTMLHtmlExtension::part() const
{
    return static_cast<KHTMLPart*>(parent());
}

