/* This file is part of the KDE project
 *
 * Copyright (C) 2003 Koos Vriezen <koos.vriezen@xs4all.nl>
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

#include "kjavaappletviewer.h"

#include "kjavaappletwidget.h"
#include "kjavaappletserver.h"

#include <stdio.h>

#include <QDialogButtonBox>
#include <QtCore/QDir>
#include <QtCore/QPair>
#include <QtCore/QTimer>
#include <QtCore/QPointer>
#include <QLabel>
#include <QStatusBar>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusConnection>

#include <kurlauthorized.h>
#include <kaboutdata.h>
#include <klocalizedstring.h>
#include <kiconloader.h>
#include <QDebug>
#include <kconfig.h>
#include <kio/authinfo.h>
#include <kio/global.h>
#include <kusertimestamp.h>


K_EXPORT_PLUGIN( KJavaAppletViewerFactory )

KAboutData *KJavaAppletViewerFactory::s_aboutData = 0;
KIconLoader *KJavaAppletViewerFactory::s_iconLoader = 0;

KJavaAppletViewerFactory::KJavaAppletViewerFactory () {
    s_aboutData = new KAboutData("KJavaAppletViewer", QString(), i18n("KDE Java Applet Plugin"), "1.0");
    s_iconLoader = new KIconLoader("kjava");
}

KJavaAppletViewerFactory::~KJavaAppletViewerFactory () {
    delete s_iconLoader;
    delete s_aboutData;
}

QObject *KJavaAppletViewerFactory::create(const char *, QWidget *wparent, QObject *parent,
                                          const QVariantList & args, const QString &) {
    QStringList argsStrings;
    for (int i = 0; i < args.size(); ++i)
        argsStrings.append(args[i].toString());
    return new KJavaAppletViewer (wparent, parent, argsStrings);
}

//-----------------------------------------------------------------------------

class KJavaServerMaintainer;

class KJavaServerMaintainer {
public:
    KJavaServerMaintainer () { }
    ~KJavaServerMaintainer ();

    KJavaAppletContext * getContext (QObject*, const QString &);
    void releaseContext (QObject*, const QString &);
    void setServer (KJavaAppletServer * s);
    QPointer <KJavaAppletServer> server;
private:
    typedef QMap <QPair <QObject*, QString>, QPair <KJavaAppletContext*, int> >
            ContextMap;
    ContextMap m_contextmap;
};
Q_GLOBAL_STATIC(KJavaServerMaintainer, serverMaintainer)

KJavaServerMaintainer::~KJavaServerMaintainer () {
    delete server;
}

KJavaAppletContext * KJavaServerMaintainer::getContext (QObject * w, const QString & doc) {
    QPair<QObject*,QString> key = qMakePair (w, doc);
    ContextMap::iterator it = m_contextmap.find (key);
    if (it != m_contextmap.end ()) {
        ++((*it).second);
        return (*it).first;
    }
    KJavaAppletContext* const context = new KJavaAppletContext ();
    m_contextmap.insert (key, qMakePair(context, 1));
    return context;
}

void KJavaServerMaintainer::releaseContext (QObject * w, const QString & doc) {
    ContextMap::iterator it = m_contextmap.find (qMakePair (w, doc));
    if (it != m_contextmap.end () && --(*it).second <= 0) {
        // qDebug() << "KJavaServerMaintainer::releaseContext";
        (*it).first->deleteLater ();
        m_contextmap.erase (it);
    }
}

inline void KJavaServerMaintainer::setServer (KJavaAppletServer * s) {
    if (!server)
        server = s;
}

//-----------------------------------------------------------------------------

AppletParameterDialog::AppletParameterDialog (KJavaAppletWidget * parent)
    : QDialog(parent), m_appletWidget (parent)
{
    setObjectName( "paramdialog" );
    setWindowTitle( i18n ("Applet Parameters") );
    setModal( true );

    KJavaApplet* const applet = parent->applet ();
    table = new QTableWidget (30, 2, this);
    table->setMinimumSize (QSize (600, 400));
    table->setColumnWidth (0, 200);
    table->setColumnWidth (1, 340);
    QTableWidgetItem* const header1 = new QTableWidgetItem(i18n ("Parameter"));
    QTableWidgetItem* const header2 = new QTableWidgetItem(i18n ("Value"));
    table->setHorizontalHeaderItem(1, header1);
    table->setHorizontalHeaderItem(2, header2);
    QTableWidgetItem * tit = new QTableWidgetItem(i18n("Class"));
    tit->setFlags( tit->flags()^Qt::ItemIsEditable );
    table->setItem (0, 0, tit);
    tit = new QTableWidgetItem(applet->appletClass());
    tit->setFlags( tit->flags()|Qt::ItemIsEditable );
    table->setItem (0, 1, tit);
    tit = new QTableWidgetItem (i18n ("Base URL"));
    tit->setFlags( tit->flags()^Qt::ItemIsEditable );
    table->setItem (1, 0, tit);
    tit = new QTableWidgetItem(applet->baseURL());
    tit->setFlags( tit->flags()|Qt::ItemIsEditable );
    table->setItem (1, 1, tit);
    tit = new QTableWidgetItem(i18n ("Archives"));
    tit->setFlags( tit->flags()^Qt::ItemIsEditable );
    table->setItem (2, 0, tit);
    tit = new QTableWidgetItem(applet->archives());
    tit->setFlags( tit->flags()|Qt::ItemIsEditable );
    table->setItem (2, 1, tit);
    QMap<QString,QString>::const_iterator it = applet->getParams().constBegin();
    const QMap<QString,QString>::const_iterator itEnd = applet->getParams().constEnd();
    for (int count = 2; it != itEnd; ++it) {
        tit = new QTableWidgetItem (it.key ());
        tit->setFlags( tit->flags()|Qt::ItemIsEditable );
        table->setItem (++count, 0, tit);
        tit = new QTableWidgetItem(it.value());
        tit->setFlags( tit->flags()|Qt::ItemIsEditable );
        table->setItem (count, 1, tit);
    }

    QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
    buttonBox->setStandardButtons(QDialogButtonBox::Close);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(slotClose()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(slotClose()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(table);
    layout->addWidget(buttonBox);
    setLayout(layout);
}

void AppletParameterDialog::slotClose () {
    table->setRangeSelected(QTableWidgetSelectionRange(0, 0, 0, 0), true);
    KJavaApplet* const applet = m_appletWidget->applet ();
    applet->setAppletClass (table->item (0, 1)->text ());
    applet->setBaseURL (table->item (1, 1)->text ());
    applet->setArchives (table->item (2, 1)->text ());
    const int lim = table->rowCount();
    for (int i = 3; i < lim; ++i) {
        if (table->item (i, 0) && table->item (i, 1) && !table->item (i, 0)->text ().isEmpty ())
            applet->setParameter (table->item (i, 0)->text (),
                                  table->item (i, 1)->text ());
    }
    hide ();
}
//-----------------------------------------------------------------------------

class CoverWidget : public QWidget {
    KJavaAppletWidget * m_appletwidget;
public:
    CoverWidget (QWidget *);
    ~CoverWidget () {}
    KJavaAppletWidget * appletWidget () const;
protected:
    void resizeEvent (QResizeEvent * e);
};

inline CoverWidget::CoverWidget (QWidget * parent)
 : QWidget (parent )
{
    setObjectName( "KJavaAppletViewer Widget");
    m_appletwidget = new KJavaAppletWidget (this);
    setFocusProxy (m_appletwidget);
}

inline KJavaAppletWidget * CoverWidget::appletWidget () const {
    return m_appletwidget;
}

void CoverWidget::resizeEvent (QResizeEvent * e) {
    m_appletwidget->resize (e->size().width(), e->size().height());
}

//-----------------------------------------------------------------------------

class StatusBarIcon : public QLabel {
public:
    StatusBarIcon (QWidget * parent) : QLabel (parent) {
        setPixmap ( KJavaAppletViewerFactory::iconLoader()->loadIcon("java", KIconLoader::Small) );
    }
protected:
    void mousePressEvent (QMouseEvent *) {
        serverMaintainer()->server->showConsole ();
    }
};

//-----------------------------------------------------------------------------

KJavaAppletViewer::KJavaAppletViewer (QWidget * wparent,
                 QObject * parent, const QStringList & args)
 : KParts::ReadOnlyPart (parent),
   m_browserextension (new KJavaAppletViewerBrowserExtension (this)),
   m_liveconnect (new KJavaAppletViewerLiveConnectExtension (this)),
   m_statusbar (new KParts::StatusBarExtension (this)),
   m_statusbar_icon (0L),
   m_closed (true)
{
    m_view = new CoverWidget (wparent);
    QString classname, classid, codebase, khtml_codebase, src_param;
    QString appletname;
    int width = -1;
    int height = -1;
    KJavaApplet* const applet = m_view->appletWidget()->applet ();
    QStringList::const_iterator it = args.begin();
    const QStringList::const_iterator itEnd = args.end();
    for ( ; it != itEnd; ++it) {
        const int equalPos = (*it).indexOf("=");
        if (equalPos > 0) {
            const QString name = (*it).left (equalPos).toUpper ();
            QString value = (*it).right ((*it).length () - equalPos - 1);
            if (value.at(0)=='\"')
                value = value.right (value.length () - 1);
            if (value.at (value.length () - 1) == '\"')
                value.truncate (value.length () - 1);
            // qDebug() << "name=" << name << " value=" << value;
            if (!name.isEmpty()) {
                const QString name_lower = name.toLower ();
                if (name == "__KHTML__PLUGINBASEURL" ||
                    name == "BASEURL") {
                    baseurl = QUrl (QUrl(value).resolved(QUrl("."))).toString();
                } else if (name == "__KHTML__CODEBASE")
                    khtml_codebase = value;
                else if (name_lower == QLatin1String("codebase") ||
                         name_lower == QLatin1String("java_codebase")) {
                    if (!value.isEmpty ())
                        codebase = value;
                } else if (name == "__KHTML__CLASSID")
                //else if (name.toLower()==QLatin1String("classid"))
                    classid = value;
                else if (name_lower == QLatin1String("code") ||
                         name_lower == QLatin1String("java_code"))
                    classname = value;
                else if (name_lower == QLatin1String("src"))
                    src_param = value;
                else if (name_lower == QLatin1String("archive") ||
                         name_lower == QLatin1String("java_archive") ||
                         name_lower.startsWith(QLatin1String("cache_archive")))
                    applet->setArchives (value);
                else if (name_lower == QLatin1String("name"))
                    appletname = value;
                else if (name_lower == QLatin1String("width"))
                    width = value.toInt();
                else if (name_lower == QLatin1String("height"))
                    height = value.toInt();
                if (!name.startsWith(QLatin1String("__KHTML__"))) {
                    applet->setParameter (name, value);
                }
            }
        }
    }
    if (!classid.isEmpty ()) {
        applet->setParameter ("CLSID", classid);
        // qDebug() << "classid=" << classid << classid.startsWith("clsid:");
        if (classid.startsWith(QLatin1String("clsid:")))
            // codeBase contains the URL to the plugin page
            khtml_codebase = baseurl;
        else if (classname.isEmpty () && classid.startsWith(QLatin1String("java:")))
            classname = classid.mid(5);
    }
    if (classname.isEmpty ())
        classname = src_param;
    else if (!src_param.isEmpty ())
        applet->setParameter (QString ("SRC"), src_param);
    if (codebase.isEmpty ())
        codebase = khtml_codebase;
    if (baseurl.isEmpty ()) {
        // not embedded in khtml
        QString pwd = QDir().absolutePath ();
        if (!pwd.endsWith ( QString(QDir::separator ())))
            pwd += QDir::separator ();
        baseurl = QUrl(QUrl(pwd).resolved(QUrl(codebase))).toString();
    }
    if (width > 0 && height > 0) {
        m_view->resize (width, height);
        applet->setSize( QSize( width, height ) );
    }
    if (appletname.isEmpty())
        appletname = classname;
    applet->setAppletName (appletname);
    applet->setBaseURL (baseurl);
    // check codebase first
    const QUrl kbaseURL( baseurl );
    const QUrl newURL(kbaseURL.resolved(QUrl(codebase)));
    if (KUrlAuthorized::authorizeUrlAction("redirect", QUrl(baseurl), newURL))
        applet->setCodeBase (newURL.toString());
    applet->setAppletClass (classname);
    KJavaAppletContext* const cxt = serverMaintainer()->getContext (parent, baseurl);
    applet->setAppletContext (cxt);

    KJavaAppletServer* const server = cxt->getServer ();

    serverMaintainer()->setServer (server);

    if (!server->usingKIO ()) {
        /* if this page needs authentication */
        KIO::AuthInfo info;
        info.url = QUrl(baseurl);
        info.verifyPath = true;
        QByteArray params;
        { QDataStream stream(&params, QIODevice::WriteOnly); stream << info; }

        // make the call
        QDBusInterface kpasswdserver("org.kde.kded5", "/modules/kpasswdserver", "org.kde.KPasswdServer");
        QDBusReply<QByteArray> reply = kpasswdserver.call ("checkAuthInfo", params, qlonglong(m_view->topLevelWidget()->winId()), qlonglong(KUserTimestamp::userTimestamp()));

        if (!reply.isValid()) {
            qWarning() << "checkAuthInfo DBUS call failed: " << reply.error().message();
        } else {
            KIO::AuthInfo authResult;
            QDataStream stream2(reply.value());
            stream2 >> authResult;
            applet->setUser (authResult.username);
            applet->setPassword (authResult.password);
            applet->setAuthName (authResult.realmValue);
        }
    }

    /* install event filter for close events */
    if (wparent)
        wparent->topLevelWidget ()->installEventFilter (this);

    setComponentData (KJavaAppletViewerFactory::componentData());
    KParts::Part::setWidget (m_view);

    connect (applet->getContext(), SIGNAL(appletLoaded()), this, SLOT(appletLoaded()));
    connect (applet->getContext(), SIGNAL(showDocument(QString,QString)), m_browserextension, SLOT(showDocument(QString,QString)));
    connect (applet->getContext(), SIGNAL(showStatus(QString)), this, SLOT(infoMessage(QString)));
    connect (applet, SIGNAL(jsEvent(QStringList)), m_liveconnect, SLOT(jsEvent(QStringList)));
}

CoverWidget * KJavaAppletViewer::view () const
{
   return m_view;
}


bool KJavaAppletViewer::eventFilter (QObject *o, QEvent *e) {
    if (m_liveconnect->jsSessions () > 0) {
        switch (e->type()) {
            case QEvent::Destroy:
            case QEvent::Close:
            case QEvent::Quit:
                return true;
            default:
                break;
        }
    }
    return KParts::ReadOnlyPart::eventFilter(o,e);
}

KJavaAppletViewer::~KJavaAppletViewer () {
    m_view = 0L;
    serverMaintainer()->releaseContext (parent(), baseurl);
    if (m_statusbar_icon) {
        m_statusbar->removeStatusBarItem (m_statusbar_icon);
        delete m_statusbar_icon;
    }
}

bool KJavaAppletViewer::openUrl (const QUrl & url) {
    if (!m_view) return false;
    m_closed = false;
    KJavaAppletWidget* const w = m_view->appletWidget ();
    KJavaApplet* const applet = w->applet ();
    if (applet->isCreated ())
        applet->stop ();
    if (applet->appletClass ().isEmpty ()) {
        // preview without setting a class?
        if (applet->baseURL ().isEmpty ()) {
            QUrl urlInfo(url);
            applet->setAppletClass(urlInfo.fileName());
            applet->setBaseURL(KIO::upUrl(urlInfo).toString());
        } else
            applet->setAppletClass(url.toString());
        AppletParameterDialog (w).exec ();
        applet->setSize (w->sizeHint());
    }
    if (!m_statusbar_icon) {
        QStatusBar *sb = m_statusbar->statusBar();
        if (sb) {
            m_statusbar_icon = new StatusBarIcon (sb);
            m_statusbar->addStatusBarItem (m_statusbar_icon, 0, false);
        }
    }
    // delay showApplet if size is unknown and m_view not shown
    if (applet->size().width() > 0 || m_view->isVisible())
        w->showApplet ();
    else
        QTimer::singleShot (10, this, SLOT (delayedCreateTimeOut()));
    if (!applet->failed ())
        emit started (0L);
    return url.isValid ();
}

bool KJavaAppletViewer::closeUrl () {
    // qDebug() << "closeUrl";
    m_closed = true;
    KJavaApplet* const applet = m_view->appletWidget ()->applet ();
    if (applet->isCreated ())
        applet->stop ();
    applet->getContext()->getServer()->endWaitForReturnData();
    return true;
}

bool KJavaAppletViewer::appletAlive () const {
    return !m_closed && m_view &&
           m_view->appletWidget ()->applet () &&
           m_view->appletWidget ()->applet ()->isAlive ();
}

bool KJavaAppletViewer::openFile () {
    return false;
}

void KJavaAppletViewer::delayedCreateTimeOut () {
    KJavaAppletWidget* const w = m_view->appletWidget ();
    if (!w->applet ()->isCreated () && !m_closed)
        w->showApplet ();
}

void KJavaAppletViewer::appletLoaded () {
    if (!m_view) return;
    KJavaApplet* const applet = m_view->appletWidget ()->applet ();
    if (applet->isAlive() || applet->failed())
        emit completed();
}

void KJavaAppletViewer::infoMessage (const QString & msg) {
    m_browserextension->infoMessage(msg);
}

//---------------------------------------------------------------------

KJavaAppletViewerBrowserExtension::KJavaAppletViewerBrowserExtension (KJavaAppletViewer * parent)
  : KParts::BrowserExtension (parent )
{
    setObjectName( "KJavaAppletViewer Browser Extension" );
}

void KJavaAppletViewerBrowserExtension::urlChanged (const QString & url) {
    emit setLocationBarUrl (url);
}

void KJavaAppletViewerBrowserExtension::setLoadingProgress (int percentage) {
    emit loadingProgress (percentage);
}

void KJavaAppletViewerBrowserExtension::setBrowserArguments (const KParts::BrowserArguments & /*args*/) {
}

void KJavaAppletViewerBrowserExtension::saveState (QDataStream & stream) {
    KJavaApplet* const applet = static_cast<KJavaAppletViewer*>(parent())->view()->appletWidget ()->applet ();
    stream << applet->appletClass();
    stream << applet->baseURL();
    stream << applet->archives();
    stream << applet->getParams().size ();
    QMap<QString,QString>::const_iterator it = applet->getParams().constBegin();
    const QMap<QString,QString>::const_iterator itEnd = applet->getParams().constEnd();
    for ( ; it != itEnd; ++it) {
        stream << it.key ();
        stream << it.value ();
    }
}

void KJavaAppletViewerBrowserExtension::restoreState (QDataStream & stream) {
    KJavaAppletWidget* const w = static_cast<KJavaAppletViewer*>(parent())->view()->appletWidget();
    KJavaApplet* const applet = w->applet ();
    QString key, val;
    int paramcount;
    stream >> val;
    applet->setAppletClass (val);
    stream >> val;
    applet->setBaseURL (val);
    stream >> val;
    applet->setArchives (val);
    stream >> paramcount;
    for (int i = 0; i < paramcount; ++i) {
        stream >> key;
        stream >> val;
        applet->setParameter (key, val);
        // qDebug() << "restoreState key:" << key << " val:" << val;
    }
    applet->setSize (w->sizeHint ());
    if (w->isVisible())
        w->showApplet ();
}

void KJavaAppletViewerBrowserExtension::showDocument (const QString & doc,
                                                      const QString & frame) {
    const QUrl url (doc);
    KParts::BrowserArguments browserArgs;
    browserArgs.frameName = frame;
    emit openUrlRequest(url, KParts::OpenUrlArguments(), browserArgs);
}

//-----------------------------------------------------------------------------

KJavaAppletViewerLiveConnectExtension::KJavaAppletViewerLiveConnectExtension(KJavaAppletViewer * parent)
    : KParts::LiveConnectExtension (parent ), m_viewer (parent)
{
    setObjectName( "KJavaAppletViewer LiveConnect Extension" );
}

bool KJavaAppletViewerLiveConnectExtension::get (
        const unsigned long objid, const QString & name,
        KParts::LiveConnectExtension::Type & type,
        unsigned long & rid, QString & value)
{
    if (!m_viewer->appletAlive ())
        return false;
    QStringList args, ret_args;
    KJavaApplet* const applet = m_viewer->view ()->appletWidget ()->applet ();
    args.append (QString::number (applet->appletId ()));
    args.append (QString::number ((int) objid));
    args.append (name);
    m_jssessions++;
    const bool ret = applet->getContext()->getMember (args, ret_args);
    m_jssessions--;
    if (!ret || ret_args.count() != 3) return false;
    bool ok;
    int itype = ret_args[0].toInt (&ok);
    if (!ok || itype < 0) return false;
    type = (KParts::LiveConnectExtension::Type) itype;
    rid = ret_args[1].toInt (&ok);
    if (!ok) return false;
    value = ret_args[2];
    return true;
}

bool KJavaAppletViewerLiveConnectExtension::put(const unsigned long objid, const QString & name, const QString & value)
{
    if (!m_viewer->appletAlive ())
        return false;
    QStringList args;
    KJavaApplet* const applet = m_viewer->view ()->appletWidget ()->applet ();
    args.append (QString::number (applet->appletId ()));
    args.append (QString::number ((int) objid));
    args.append (name);
    args.append (value);
    ++m_jssessions;
    const bool ret = applet->getContext()->putMember (args);
    --m_jssessions;
    return ret;
}

bool KJavaAppletViewerLiveConnectExtension::call( const unsigned long objid, const QString & func, const QStringList & fargs, KParts::LiveConnectExtension::Type & type, unsigned long & retobjid, QString & value )
{
    if (!m_viewer->appletAlive ())
        return false;
    KJavaApplet* const applet = m_viewer->view ()->appletWidget ()->applet ();
    QStringList args, ret_args;
    args.append (QString::number (applet->appletId ()));
    args.append (QString::number ((int) objid));
    args.append (func);
    {
        QStringList::const_iterator it = fargs.begin();
        const QStringList::const_iterator itEnd = fargs.end();
	for ( ; it != itEnd; ++it)
            args.append(*it);
    }

    ++m_jssessions;
    const bool ret = applet->getContext()->callMember (args, ret_args);
    --m_jssessions;
    if (!ret || ret_args.count () != 3) return false;
    bool ok;
    const int itype = ret_args[0].toInt (&ok);
    if (!ok || itype < 0) return false;
    type = (KParts::LiveConnectExtension::Type) itype;
    retobjid = ret_args[1].toInt (&ok);
    if (!ok) return false;
    value = ret_args[2];
    return true;
}

void KJavaAppletViewerLiveConnectExtension::unregister(const unsigned long objid)
{
    if (!m_viewer->view () || !m_viewer->view ())
        return;
    KJavaApplet* const applet = m_viewer->view ()->appletWidget ()->applet ();
    if (!applet || objid == 0) {
        // typically a gc after a function call on the applet,
        // no need to send to the jvm
        return;
    }
    QStringList args;
    args.append (QString::number (applet->appletId ()));
    args.append (QString::number ((int) objid));
    applet->getContext()->derefObject (args);
}

void KJavaAppletViewerLiveConnectExtension::jsEvent (const QStringList & args) {
    if (args.count () < 2 || !m_viewer->appletAlive ())
        return;
    bool ok;
    QStringList::ConstIterator it = args.begin();
    const QStringList::ConstIterator itEnd = args.end();
    const unsigned long objid = (*it).toInt(&ok);
    ++it;
    const QString event = (*it);
    ++it;
    KParts::LiveConnectExtension::ArgList arglist;

    for (; it != itEnd; ++it) {
        // take a deep breath here
        const QStringList::ConstIterator prev = it++;
	arglist.push_back(KParts::LiveConnectExtension::ArgList::value_type((KParts::LiveConnectExtension::Type) (*prev).toInt(), (*it)));
    }
    emit partEvent (objid, event, arglist);
}

int KJavaAppletViewerLiveConnectExtension::m_jssessions = 0;

//-----------------------------------------------------------------------------

