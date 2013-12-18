// -*- c-basic-offset: 2 -*-
/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2003 Apple Computer, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _XMLHTTPREQUEST_H_
#define _XMLHTTPREQUEST_H_

#include "ecma/kjs_binding.h"
#include "ecma/kjs_dom.h"
#include <kencodingdetector.h>
#include "kio/jobclasses.h"
#include <QtCore/QPointer>
#include <QtCore/QHash>

#include "xml/dom_nodeimpl.h"

namespace KJS {

  class JSEventListener;
  class XMLHttpRequestQObject;

  class CaseInsensitiveString
  {
  public:
    CaseInsensitiveString(const char* s) : str(QLatin1String(s)) { }
    CaseInsensitiveString(const QString& s) : str(s) { }

    QString original() const { return str; }
    QString toLower() const { return str.toLower(); }

  private:
    QString str;
  };

  inline bool operator==(const CaseInsensitiveString& a,
                         const CaseInsensitiveString& b)
  {
      return a.original().compare(b.original(), Qt::CaseInsensitive) == 0;
  }

  inline uint qHash(const CaseInsensitiveString& key)
  {
    return qHash(key.toLower());
  }

  typedef QHash<CaseInsensitiveString, QString> HTTPHeaderMap;

  // these exact numeric values are important because JS expects them
  enum XMLHttpRequestState {
    XHRS_Uninitialized = 0,
    XHRS_Open = 1,
    XHRS_Sent = 2,
    XHRS_Receiving = 3,
    XHRS_Loaded = 4
  };

  class XMLHttpRequestConstructorImp : public JSObject {
  public:
    XMLHttpRequestConstructorImp(ExecState *exec, DOM::DocumentImpl* d);
    virtual bool implementsConstruct() const;
    using KJS::JSObject::construct;
    virtual JSObject *construct(ExecState *exec, const List &args);
  private:
    SharedPtr<DOM::DocumentImpl> doc;
  };

  class XMLHttpRequest : public DOMObject, public DOM::EventTargetImpl {
  public:
    XMLHttpRequest(ExecState *, DOM::DocumentImpl* d);
    ~XMLHttpRequest();

    virtual Type eventTargetType() const { return XML_HTTP_REQUEST; }

    using KJS::JSObject::getOwnPropertySlot;
    bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    JSValue *getValueProperty(ExecState *exec, int token) const;
    using KJS::JSObject::put;
    virtual void put(ExecState *exec, const Identifier &propertyName, JSValue *value, int attr = None);
    void putValueProperty(ExecState *exec, int token, JSValue *value, int /*attr*/);
    virtual bool toBoolean(ExecState *) const { return true; }
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { Onload, Onreadystatechange, ReadyState, ResponseText, ResponseXML, Status, StatusText, Abort,
           GetAllResponseHeaders, GetResponseHeader, Open, Send, SetRequestHeader,
           OverrideMIMEType };

  private:
    friend class XMLHttpRequestProtoFunc;
    friend class XMLHttpRequestQObject;

    JSValue *getStatusText() const;
    JSValue *getStatus() const;
    bool urlMatchesDocumentDomain(const QUrl&) const;

    XMLHttpRequestQObject *qObject;

#ifdef APPLE_CHANGES
    void slotData( KIO::Job* job, const char *data, int size );
#else
    void slotData( KIO::Job* job, const QByteArray &data );
#endif
    void slotFinished( KJob* );
    void slotRedirection( KIO::Job*, const QUrl& );

    void processSyncLoadResults(const QByteArray &data, const QUrl &finalURL, const QString &headers);

    void open(const QString& _method, const QUrl& _url, bool _async, int& ec);
    void send(const QString& _body, int& ec);
    void abort();
    void setRequestHeader(const QString& name, const QString& value, int& ec);
    void overrideMIMEType(const QString& override);
    JSValue *getAllResponseHeaders(int& ec) const;
    JSValue *getResponseHeader(const QString& name, int& ec) const;

    void changeState(XMLHttpRequestState newState);

    QPointer<DOM::DocumentImpl> doc;

    QUrl url;
    QString m_method;
    bool async;
    HTTPHeaderMap m_requestHeaders;
    QString m_mimeTypeOverride;
    QString contentType;

    KIO::StoredTransferJob * job;

    XMLHttpRequestState m_state;
    JSEventListener *onReadyStateChangeListener;
    JSEventListener *onLoadListener;

    KEncodingDetector *decoder;
    bool              binaryMode; // just byte-expand data,
                                  // don't pass it through the decoder.
    void clearDecoder();
                                  
    QString encoding;
    QString responseHeaders;

    QString response;
    mutable bool createdDocument;
    mutable bool typeIsXML;
    mutable SharedPtr<DOM::DocumentImpl> responseXML;

    bool aborted;
  };


  class XMLHttpRequestQObject : public QObject {
    Q_OBJECT

  public:
    XMLHttpRequestQObject(XMLHttpRequest *_jsObject);

  public Q_SLOTS:
    void slotData( KIO::Job* job, const QByteArray &data );
    void slotFinished( KJob* job );
    void slotRedirection( KIO::Job* job, const QUrl& url);

  private:
    XMLHttpRequest *jsObject;
  };

} // namespace

#endif
