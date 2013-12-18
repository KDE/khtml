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

#ifndef __khtml_ext_h__
#define __khtml_ext_h__

#include "khtml_part.h"

#include <QtCore/QPointer>

#include <kselectaction.h>
#include <kparts/textextension.h>
#include <kparts/htmlextension.h>
#include <kio/global.h>

/**
 * This is the BrowserExtension for a KHTMLPart document. Please see the KParts documentation for
 * more information about the BrowserExtension.
 */
class KHTMLPartBrowserExtension : public KParts::BrowserExtension
{
  Q_OBJECT
  friend class KHTMLPart;
  friend class KHTMLView;
public:
  KHTMLPartBrowserExtension( KHTMLPart *parent );

  virtual int xOffset();
  virtual int yOffset();

  virtual void saveState( QDataStream &stream );
  virtual void restoreState( QDataStream &stream );

    // internal
    void editableWidgetFocused( QWidget *widget );
    void editableWidgetBlurred( QWidget *widget );

    void setExtensionProxy( KParts::BrowserExtension *proxyExtension );

public Q_SLOTS:
    void cut();
    void copy();
    void paste();
    void searchProvider();
    void reparseConfiguration();
    void print();
    void disableScrolling();

    // internal . updates the state of the cut/copt/paste action based
    // on whether data is available in the clipboard
    void updateEditActions();

private Q_SLOTS:
    // connected to a frame's browserextensions enableAction signal
    void extensionProxyActionEnabled( const char *action, bool enable );
    void extensionProxyEditableWidgetFocused();
    void extensionProxyEditableWidgetBlurred();

Q_SIGNALS:
    void editableWidgetFocused();
    void editableWidgetBlurred();
private:
    void callExtensionProxyMethod( const char *method );

    KHTMLPart *m_part;
    QPointer<QWidget> m_editableFormWidget;
    QPointer<KParts::BrowserExtension> m_extensionProxy;
    bool m_connectedToClipboard;
};

class KHTMLPartBrowserHostExtension : public KParts::BrowserHostExtension
{
public:
  KHTMLPartBrowserHostExtension( KHTMLPart *part );
  virtual ~KHTMLPartBrowserHostExtension();

  virtual QStringList frameNames() const;

  virtual const QList<KParts::ReadOnlyPart*> frames() const;

  virtual BrowserHostExtension* findFrameParent( KParts::ReadOnlyPart *callingPart, const QString &frame );

    virtual bool openUrlInFrame(const QUrl &url, const KParts::OpenUrlArguments& arguments, const KParts::BrowserArguments &browserArguments);

private:
  KHTMLPart *m_part;
};

/**
 * @internal
 * INTERNAL class. *NOT* part of the public API.
 */
class KHTMLPopupGUIClient : public QObject
{
  Q_OBJECT
public:
  KHTMLPopupGUIClient( KHTMLPart *khtml, const QUrl &url );
  virtual ~KHTMLPopupGUIClient();

    KParts::BrowserExtension::ActionGroupMap actionGroups() const;

  static void saveURL( QWidget *parent, const QString &caption, const QUrl &url,
                       const QMap<QString, QString> &metaData = KIO::MetaData(),
                       const QString &filter = QString(), long cacheId = 0,
                       const QString &suggestedFilename = QString() );

  static void saveURL( QWidget* parent, const QUrl &url, const QUrl &destination,
                       const QMap<QString, QString> &metaData = KIO::MetaData(),
                       long cacheId = 0 );

    static QString selectedTextAsOneLine(KHTMLPart* part);

private Q_SLOTS:
  void slotSaveLinkAs();
  void slotSaveImageAs();
  void slotCopyLinkLocation();
  void slotSendImage();
  void slotStopAnimations();
  void slotCopyImageLocation();
  void slotCopyImage();
  void slotViewImage();
  void slotReloadFrame();
  void slotFrameInWindow();
  void slotFrameInTop();
  void slotFrameInTab();
  void slotBlockImage();
  void slotBlockHost();
  void slotBlockIFrame();
    void openSelection();

private:
    void addSearchActions(QList<QAction *>& editActions);

  class KHTMLPopupGUIClientPrivate;
  KHTMLPopupGUIClientPrivate* const d;
};

class KHTMLZoomFactorAction : public KSelectAction
{
    Q_OBJECT
public:
    KHTMLZoomFactorAction(KHTMLPart *part, bool direction, const QString& iconName, const QString& text, QObject *parent);
    virtual ~KHTMLZoomFactorAction();

protected Q_SLOTS:
    void slotTriggered(QAction* action);
private:
    void init(KHTMLPart *part, bool direction);
private:
    bool m_direction;
    KHTMLPart *m_part;
};

/**
 * @internal
 * Implements the TextExtension interface
 */
class KHTMLTextExtension : public KParts::TextExtension
{
    Q_OBJECT
public:
    KHTMLTextExtension(KHTMLPart* part);

    virtual bool hasSelection() const;
    virtual QString selectedText(Format format) const;
    virtual QString completeText(Format format) const;

    KHTMLPart* part() const;
};

/**
 * @internal
 * Implements the HtmlExtension interface
 */
class KHTMLHtmlExtension : public KParts::HtmlExtension,
                           public KParts::SelectorInterface,
                           public KParts::HtmlSettingsInterface
{
    Q_OBJECT
    Q_INTERFACES(KParts::SelectorInterface)
    Q_INTERFACES(KParts::HtmlSettingsInterface)

public:
    KHTMLHtmlExtension(KHTMLPart* part);

    // HtmlExtension
    virtual QUrl baseUrl() const;
    virtual bool hasSelection() const;

    // SelectorInterface
    virtual QueryMethods supportedQueryMethods() const;
    virtual Element querySelector(const QString& query, QueryMethod method) const;
    virtual QList<Element> querySelectorAll(const QString& query, QueryMethod method) const;

    // SettingsInterface
    virtual QVariant htmlSettingsProperty(HtmlSettingsType type) const;
    virtual bool setHtmlSettingsProperty(HtmlSettingsType type, const QVariant& value);

    KHTMLPart* part() const;
};

#endif
