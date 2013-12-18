/* This file is part of the KDE project
 *
 * Copyright (C) 2008 Bernhard Beschow <bbeschow AT cs DOT tu-berlin DOT de>
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
#ifndef __khtml_find_p_h__
#define __khtml_find_p_h__

#include <QObject>

#include "xml/dom_nodeimpl.h"

#include <kfind.h>

#include <QStringList>
#include <QPointer>
#include "khtmlfindbar.h"

class KHTMLPart;
class QString;
class QWidget;
class KFindDialog;
class KFind;

/**
 * This class implements the find activity for the @p KHTMLPart.
 *
 * 
 *
 * @author Bernhard Beschow <bbeschow cs tu berlin de>
 */
class KHTMLFind : public QObject
{
  Q_OBJECT
public:
  KHTMLFind( KHTMLPart *part, KHTMLFind *parent=0 );
  ~KHTMLFind();
  void findTextBegin();
  bool initFindNode( bool selection, bool reverse, bool fromCursor );
  void createNewKFind( const QString &str, long options, QWidget *parent, KFindDialog *findDialog );
  bool findTextNext( bool reverse = false );
  KHTMLFindBar *findBar() const { return m_parent ? m_parent->findBar() : m_findDialog.data(); }

public Q_SLOTS:
  void activate();
  void deactivate();

private Q_SLOTS:
  void slotFindDestroyed();
  void slotSelectionChanged();
  void slotHighlight( const QString &, int index, int length );
  void slotSearchChanged();
  void slotFindNext();
  void slotFindPrevious();

Q_SIGNALS:
  void foundMatch( const DOM::Selection &selection, int length );

protected:
  KFind *find() const { return m_find; }

private:
  KHTMLPart *m_part;

  struct StringPortion
  {
      // Just basic ref/deref on our node to make sure it doesn't get deleted
      StringPortion( int i, DOM::NodeImpl* n ) : index(i), node(n) { if (node) node->ref(); }
      StringPortion() : index(0), node(0) {} // for QValueList
      StringPortion( const StringPortion& other ) : node(0) { operator=(other); }
      StringPortion& operator=( const StringPortion& other ) {
          index=other.index;
          if (other.node) other.node->ref();
          if (node) node->deref();
          node=other.node;
          return *this;
      }
      ~StringPortion() { if (node) node->deref(); }

      int index;
      DOM::NodeImpl *node;
  };
  QList<StringPortion> m_stringPortions;

  KFind *m_find;
  KHTMLFind *m_parent;
  QPointer<KHTMLFindBar> m_findDialog;

  struct findState
  {
    findState() : options( 0 ), last_dir( -1 ) {}
    QStringList history;
    QString text;
    int options;
    int last_dir; // -1=unknown,0=forward,1=backward
  };

  findState m_lastFindState;

  DOM::NodeImpl *m_findNode; // current node
  DOM::NodeImpl *m_findNodeEnd; // end node
  DOM::NodeImpl *m_findNodeStart; // start node
  DOM::NodeImpl *m_findNodePrevious; // previous node used for find
  int m_findPos; // current pos in current node
  int m_findPosEnd; // pos in end node
  int m_findPosStart; // pos in start node
};

#endif
