/* This file is part of the KDE project
 *
 * Copyright (C) 2000 Waldo Bastian <bastian@kde.org>
 * Copyright (C) 2007 Nick Shaforostoff <shafff@ukr.net>
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

#include "khtml_pagecache.h"

#include <kcompressiondevice.h>
#include <QTemporaryFile>

#include <QQueue>
#include <QHash>
#include <QList>
#include <QtCore/QTimer>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>

// We keep 12 pages in memory.
#ifndef KHTML_PAGE_CACHE_SIZE
#define KHTML_PAGE_CACHE_SIZE 12
#endif

template class QList<KHTMLPageCacheDelivery*>;
class KHTMLPageCacheEntry
{
  friend class KHTMLPageCache;
public:
  KHTMLPageCacheEntry(long id);

  ~KHTMLPageCacheEntry();

  void addData(const QByteArray &data);
  void endData();

  bool isComplete() const {return m_complete;}
  QString fileName() const {return m_fileName;}

  KHTMLPageCacheDelivery *fetchData(QObject *recvObj, const char *recvSlot);
private:
  long m_id;
  bool m_complete;
  QByteArray m_buffer;
  QIODevice* m_file;
  QString m_fileName;
};

class KHTMLPageCachePrivate
{
public:
  long newId;
  bool deliveryActive;
  QHash<int, KHTMLPageCacheEntry*> dict;
  QList<KHTMLPageCacheDelivery*> delivery;
  QQueue<long> expireQueue;
};

KHTMLPageCacheEntry::KHTMLPageCacheEntry(long id)
    : m_id(id)
    , m_complete(false)
{
    QTemporaryFile f(QDir::tempPath() + "/khtmlcacheXXXXXX.tmp");
    f.open();
    m_fileName = f.fileName();
    f.setAutoRemove(false);

    m_file = new KCompressionDevice(m_fileName, KCompressionDevice::GZip);
    m_file->open(QIODevice::WriteOnly);
}

KHTMLPageCacheEntry::~KHTMLPageCacheEntry()
{
  delete m_file;
  QFile::remove(m_fileName);
}


void
KHTMLPageCacheEntry::addData(const QByteArray &data)
{
    m_buffer+=data;
}

void
KHTMLPageCacheEntry::endData()
{
  m_complete = true;
 m_file->write(m_buffer);
 m_buffer.clear();
 m_file->close();
}


KHTMLPageCacheDelivery *
KHTMLPageCacheEntry::fetchData(QObject *recvObj, const char *recvSlot)
{
  // Duplicate fd so that entry can be safely deleted while delivering the data.
  KCompressionDevice* dev = new KCompressionDevice(m_fileName, KCompressionDevice::GZip);
  dev->open(QIODevice::ReadOnly);
  KHTMLPageCacheDelivery *delivery = new KHTMLPageCacheDelivery(dev); // takes ownership of dev

  recvObj->connect(delivery, SIGNAL(emitData(QByteArray)), recvSlot);
  delivery->recvObj = recvObj;
  return delivery;
}

class KHTMLPageCacheSingleton
{
public:
  KHTMLPageCache instance;
};

Q_GLOBAL_STATIC(KHTMLPageCacheSingleton, _self)

KHTMLPageCache *
KHTMLPageCache::self()
{
  return &_self()->instance;
}

KHTMLPageCache::KHTMLPageCache()
	:d( new KHTMLPageCachePrivate)
{
  d->newId = 1;
  d->deliveryActive = false;
}

KHTMLPageCache::~KHTMLPageCache()
{
  qDeleteAll(d->dict);
  qDeleteAll(d->delivery);
  delete d;
}

long
KHTMLPageCache::createCacheEntry()
{

  KHTMLPageCacheEntry *entry = new KHTMLPageCacheEntry(d->newId);
  d->dict.insert(d->newId, entry);
  d->expireQueue.append(d->newId);
  if (d->expireQueue.count() > KHTML_PAGE_CACHE_SIZE)
     delete d->dict.take(d->expireQueue.dequeue());
  return (d->newId++);
}

void
KHTMLPageCache::addData(long id, const QByteArray &data)
{

  KHTMLPageCacheEntry *entry = d->dict.value( id );
  if (entry)
     entry->addData(data);
}

void
KHTMLPageCache::endData(long id)
{
  KHTMLPageCacheEntry *entry = d->dict.value( id );
  if (entry)
     entry->endData();
}

void
KHTMLPageCache::cancelEntry(long id)
{
  KHTMLPageCacheEntry *entry = d->dict.take( id );
  if (entry)
  {
     d->expireQueue.removeAll(entry->m_id);
     delete entry;
  }
}

bool
KHTMLPageCache::isValid(long id)
{
  return d->dict.contains(id);
}

bool
KHTMLPageCache::isComplete(long id)
{
  KHTMLPageCacheEntry *entry = d->dict.value( id );
  if (entry)
     return entry->isComplete();
  return false;
}

void
KHTMLPageCache::fetchData(long id, QObject *recvObj, const char *recvSlot)
{
  KHTMLPageCacheEntry *entry = d->dict.value( id );
  if (!entry || !entry->isComplete()) return;

  // Make this entry the most recent entry.
  d->expireQueue.removeAll(entry->m_id);
  d->expireQueue.enqueue(entry->m_id);

  d->delivery.append( entry->fetchData(recvObj, recvSlot) );
  if (!d->deliveryActive)
  {
     d->deliveryActive = true;
     QTimer::singleShot(20, this, SLOT(sendData()));
  }
}

void
KHTMLPageCache::cancelFetch(QObject *recvObj)
{
  QMutableListIterator<KHTMLPageCacheDelivery*> it( d->delivery );
  while (it.hasNext()) {
      KHTMLPageCacheDelivery* delivery = it.next();
      if (delivery->recvObj == recvObj)
      {
         delete delivery;
         it.remove();
      }
  }
}

void
KHTMLPageCache::sendData()
{
  if (d->delivery.isEmpty())
  {
     d->deliveryActive = false;
     return;
  }

  KHTMLPageCacheDelivery *delivery = d->delivery.takeFirst();
  assert(delivery);

  QByteArray byteArray(delivery->file->read(64*1024));
  delivery->emitData(byteArray);

  //put back in queue
  if (delivery->file->atEnd())
  {
    // done.
    delivery->file->close();
    delivery->emitData(QByteArray()); // Empty array
    delete delivery;
  }
  else
    d->delivery.append( delivery );

  QTimer::singleShot(0, this, SLOT(sendData()));
}

void
KHTMLPageCache::saveData(long id, QDataStream *str)
{
  assert(d->dict.contains( id ));
  KHTMLPageCacheEntry *entry = d->dict.value( id );

  if (!entry->isComplete())
  {
      QTimer::singleShot(20, this, SLOT(saveData()));
      return;
  }

  KCompressionDevice file(entry->fileName(), KCompressionDevice::GZip);
  if (!file.open(QIODevice::ReadOnly))
    return;

  const QByteArray byteArray(file.readAll());
  file.close();

  str->writeRawData(byteArray.constData(), byteArray.length());

}

KHTMLPageCacheDelivery::~KHTMLPageCacheDelivery()
{
  file->close();
  delete file;
}

