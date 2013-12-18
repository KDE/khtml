/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "security_origin.h"

#include <wtf/RefPtr.h>

#include <kprotocolinfo.h>

namespace khtml {

static bool isDefaultPortForProtocol(unsigned short port, const QString& proto)
{
    return (port == 80  && proto == QLatin1String("http")) ||
           (port == 443 && proto == QLatin1String("https"));
}

SecurityOrigin::SecurityOrigin(const QUrl& url) :
      m_protocol(url.scheme())
    , m_host(url.host().toLower())
    , m_port(url.port())
    , m_domainWasSetInDOM(false)
    , m_isUnique(false)
{
    // These protocols do not create security origins; the owner frame provides the origin
    if (m_protocol == "about" || m_protocol == "javascript")
        m_protocol = "";

    // For edge case URLs that were probably misparsed, make sure that the origin is unique.
    if (m_host.isEmpty() && KProtocolInfo::protocolClass(m_protocol) == QLatin1String(":internet"))
        m_isUnique = true;

    // document.domain starts as m_host, but can be set by the DOM.
    m_domain = m_host;

    if (url.port() == -1 || isDefaultPortForProtocol(m_port, m_protocol))
        m_port = 0;
}

SecurityOrigin::SecurityOrigin(const SecurityOrigin* other) : 
      m_protocol(other->m_protocol)
    , m_host(other->m_host)
    , m_domain(other->m_domain)
    , m_port(other->m_port)
    , m_domainWasSetInDOM(other->m_domainWasSetInDOM)
    , m_isUnique(other->m_isUnique)    
{
}

bool SecurityOrigin::isEmpty() const
{
    return m_protocol.isEmpty();
}

SecurityOrigin* SecurityOrigin::create(const QUrl& url)
{
    if (!url.isValid())
        return new SecurityOrigin(QUrl());
    return new SecurityOrigin(url);
}

SecurityOrigin* SecurityOrigin::createEmpty()
{
    return create(QUrl());
}

void SecurityOrigin::setDomainFromDOM(const QString& newDomain)
{
    m_domainWasSetInDOM = true;
    m_domain = newDomain.toLower();
}

bool SecurityOrigin::canAccess(const SecurityOrigin* other) const
{  
    if (isUnique() || other->isUnique())
        return false;

    // Here are two cases where we should permit access:
    //
    // 1) Neither document has set document.domain. In this case, we insist
    //    that the scheme, host, and port of the URLs match.
    //
    // 2) Both documents have set document.domain. In this case, we insist
    //    that the documents have set document.domain to the same value and
    //    that the scheme of the URLs match.
    //
    // This matches the behavior of Firefox 2 and Internet Explorer 6.
    //
    // Internet Explorer 7 and Opera 9 are more strict in that they require
    // the port numbers to match when both pages have document.domain set.
    //
    // FIXME: Evaluate whether we can tighten this policy to require matched
    //        port numbers.
    //
    // Opera 9 allows access when only one page has set document.domain, but
    // this is a security vulnerability.

    if (m_protocol == other->m_protocol) {
        if (!m_domainWasSetInDOM && !other->m_domainWasSetInDOM) {
            if (m_host == other->m_host && m_port == other->m_port)
                return true;
        } else if (m_domainWasSetInDOM && other->m_domainWasSetInDOM) {
            if (m_domain == other->m_domain)
                return true;
        }
    }
    
    return false;
}

bool SecurityOrigin::canRequest(const QUrl& url) const
{
    if (isUnique())
        return false;

    WTF::RefPtr<SecurityOrigin> targetOrigin = SecurityOrigin::create(url);
    if (targetOrigin->isUnique())
        return false;

    // We call isSameSchemeHostPort here instead of canAccess because we want
    // to ignore document.domain effects.
    if (isSameSchemeHostPort(targetOrigin.get()))
        return true;

    return false;
}

bool SecurityOrigin::taintsCanvas(const QUrl& url) const
{
    if (canRequest(url))
        return false;

    // This function exists because we treat data URLs as having a unique origin,
    // contrary to the current (9/19/2009) draft of the HTML5 specification.
    // We still want to let folks paint data URLs onto untainted canvases, so
    // we special case data URLs below. If we change to match HTML5 w.r.t.
    // data URL security, then we can remove this function in favor of
    // !canRequest.
    if (url.scheme() == QLatin1String("data"))
        return false;

    return true;
}

void SecurityOrigin::makeUnique()
{
    m_isUnique = true;
}

QString SecurityOrigin::toString() const
{
    if (isEmpty())
        return "null";

    if (isUnique())
        return "null";

    if (m_protocol == "file")
        return QString("file://");

    QString result;
    result += m_protocol;
    result += "://";
    result += m_host;

    if (m_port) {
        result += ":";
        result += QString::number(m_port);
    }
    
    return result;
}

SecurityOrigin* SecurityOrigin::createFromString(const QString& originString)
{
    return SecurityOrigin::create(QUrl(originString));
}

bool SecurityOrigin::isSameSchemeHostPort(const SecurityOrigin* other) const 
{
    if (m_host != other->m_host)
        return false;

    if (m_protocol != other->m_protocol)
        return false;

    if (m_port != other->m_port)
        return false;

    return true;
}


} // namespace khtml
