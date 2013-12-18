// -*- c++ -*-

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

#ifndef KJAVAAPPLETVIEWER_H
#define KJAVAAPPLETVIEWER_H

#include <kparts/part.h>
#include <kparts/browserextension.h>
#include <kparts/statusbarextension.h>
#include <kpluginfactory.h>
#include <QDialog>
#include <QUrl>
#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QPointer>

#include "kjavaappletwidget.h"

class QTableWidget;
class QLabel;
class KJavaProcess;
class KJavaAppletViewer;
class KAboutData;
class KComponentData;
class KConfig;
class CoverWidget;

class KJavaAppletViewerBrowserExtension : public KParts::BrowserExtension {
    Q_OBJECT
public:
    KJavaAppletViewerBrowserExtension (KJavaAppletViewer *parent);
    void urlChanged (const QString & url);
    void setLoadingProgress (int percentage);

    void setBrowserArguments(const KParts::BrowserArguments & args);
    void saveState (QDataStream & stream);
    void restoreState (QDataStream & stream);
public Q_SLOTS:
    void showDocument (const QString & doc, const QString & frame);
};

class KJavaAppletViewerLiveConnectExtension : public KParts::LiveConnectExtension {
    Q_OBJECT
public:
    KJavaAppletViewerLiveConnectExtension(KJavaAppletViewer * parent);

    bool get (const unsigned long objid, const QString & field, KParts::LiveConnectExtension::Type & type, unsigned long & retobjid, QString & value);
    bool put(const unsigned long, const QString & field, const QString & value);
    bool call (const unsigned long , const QString & func, const QStringList & args, KParts::LiveConnectExtension::Type & type, unsigned long & retobjid, QString & value);
    void unregister (const unsigned long objid);

    int jsSessions () const { return m_jssessions; }
public Q_SLOTS:
    void jsEvent (const QStringList & args);
Q_SIGNALS:
    void partEvent (const unsigned long objid, const QString & event, const KParts::LiveConnectExtension::ArgList & args);

private:
    KJavaAppletViewer * m_viewer;
    static int m_jssessions;
};

class KJavaAppletViewer : public KParts::ReadOnlyPart {
    Q_OBJECT
public:
    KJavaAppletViewer (QWidget * wparent, QObject * parent, const QStringList &args);
    ~KJavaAppletViewer ();
    CoverWidget * view () const;

    KJavaAppletViewerBrowserExtension * browserextension() const
        { return m_browserextension; }
    KParts::LiveConnectExtension * liveConnectExtension () const
        { return m_liveconnect; }

    bool eventFilter (QObject *o, QEvent *e);

    bool appletAlive () const;
public Q_SLOTS:
    virtual bool openUrl (const QUrl & url);
    virtual bool closeUrl ();
    void appletLoaded ();
    void infoMessage (const QString &);
protected:
    bool openFile();
private Q_SLOTS:
    void delayedCreateTimeOut ();
private:
    QPointer <CoverWidget> m_view;
    KConfig * m_config;
    KJavaProcess * process;
    KJavaAppletViewerBrowserExtension * m_browserextension;
    KJavaAppletViewerLiveConnectExtension * m_liveconnect;
    KParts::StatusBarExtension * m_statusbar;
    QPointer <QLabel> m_statusbar_icon;
    QString baseurl;
    bool m_closed;
};

class KJavaAppletViewerFactory : public KPluginFactory {
    Q_OBJECT
public:
    KJavaAppletViewerFactory ();
    virtual ~KJavaAppletViewerFactory ();
    virtual QObject *create(const char *, QWidget *wparent, QObject *parent,
                            const QVariantList & args, const QString &);
    static const KAboutData &componentData() { return *s_aboutData; }
    static KIconLoader * iconLoader () { return s_iconLoader; }
private:
    static KAboutData  *s_aboutData;
    static KIconLoader * s_iconLoader;
};

class AppletParameterDialog : public QDialog {
    Q_OBJECT
public:
    AppletParameterDialog (KJavaAppletWidget * parent);
protected Q_SLOTS:
    void slotClose ();
private:
    KJavaAppletWidget * m_appletWidget;
    QTableWidget * table;
};

#endif
