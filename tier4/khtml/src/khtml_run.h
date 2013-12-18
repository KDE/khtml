/* This file is part of the KDE project
 *
 * Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
 *                     1999 Lars Knoll <knoll@kde.org>
 *                     1999 Antti Koivisto <koivisto@kde.org>
 *                     2000 Simon Hausmann <hausmann@kde.org>
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
#ifndef __khtml_run_h__
#define __khtml_run_h__

#include <kparts/browserrun.h>
#include <kservice.h>
#include <kparts/browserextension.h>

#include <QUrl>

class KHTMLPart;

namespace khtml
{
  class ChildFrame;
}

class KHTMLRun : public KParts::BrowserRun
{
  Q_OBJECT
public:
  KHTMLRun( KHTMLPart *part, khtml::ChildFrame *child, const QUrl &url,
            const KParts::OpenUrlArguments& args,
            const KParts::BrowserArguments &browserArgs,
            bool hideErrorDialog );

  virtual void foundMimeType( const QString &mimetype );
  virtual void handleError( KJob* job );

  //KHTMLPart *htmlPart() const;

protected:
  virtual void save(const QUrl & url, const QString & suggestedFilename);

private:
  khtml::ChildFrame *m_child;
};

#endif
