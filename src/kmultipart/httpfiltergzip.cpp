/*
   This file is part of the KDE libraries
   Copyright (c) 2002 Waldo Bastian <bastian@kde.org>
   Copyright 2009 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "httpfiltergzip_p.h"
#include <kcompressiondevice.h>
#include <kfilterbase.h>
#include <QDebug>

HTTPFilterGZip::HTTPFilterGZip()
    : m_firstData(true),
      m_finished(false)
{
    // We can't use KFilterDev because it assumes it can read as much data as necessary
    // from the underlying device. It's a pull strategy, while we have to do
    // a push strategy.
    m_gzipFilter = KCompressionDevice::filterForCompressionType(KCompressionDevice::GZip);
}

HTTPFilterGZip::~HTTPFilterGZip()
{
    m_gzipFilter->terminate();
    delete m_gzipFilter;

}

/*
  The data format used by the zlib library is described by RFCs (Request for
  Comments) 1950 to 1952 in the files ftp://ds.internic.net/rfc/rfc1950.txt
  (zlib format), rfc1951.txt (deflate format) and rfc1952.txt (gzip format).

  Use /usr/include/zlib.h as the primary source of documentation though.
*/

void
HTTPFilterGZip::slotInput(const QByteArray &d)
{
    if (d.isEmpty())
        return;

    //qDebug() << "Got" << d.size() << "bytes as input";
    if (m_firstData) {
        m_gzipFilter->setFilterFlags(KFilterBase::WithHeaders);
        m_gzipFilter->init(QIODevice::ReadOnly);
        m_firstData = false;
    }

    m_gzipFilter->setInBuffer(d.constData(), d.size());

    while (!m_gzipFilter->inBufferEmpty() && !m_finished) {
        char buf[8192];
        m_gzipFilter->setOutBuffer(buf, sizeof(buf));
        KFilterBase::Result result = m_gzipFilter->uncompress();
        //qDebug() << "uncompress returned" << result;
        switch (result) {
        case KFilterBase::Ok:
        case KFilterBase::End:
        {
            const int bytesOut = sizeof(buf) - m_gzipFilter->outBufferAvailable();
            if (bytesOut) {
                emit output(QByteArray(buf, bytesOut));
            }
            if (result == KFilterBase::End) {
                //qDebug() << "done, bHasFinished=true";
                emit output(QByteArray());
                m_finished = true;
            }
            break;
        }
        case KFilterBase::Error:
            qDebug() << "Error from KGZipFilter";
            emit error(tr("Receiving corrupt data."));
            m_finished = true; // exit this while loop
            break;
        }
    }
}
