/* This file is part of the KDE project
 *
 * Copyright (C) 2008 Bernhard Beschow <bbeschow cs tu berlin de>
 *           (C) 2009 Germain Garand <germain@ebooksfrance.org>
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

#include "khtmlfind_p.h"

#include "khtml_part.h"
#include "khtmlviewbar.h"
#include "khtmlfindbar.h"

#include "dom/html_document.h"
#include "html/html_documentimpl.h"
#include "rendering/render_text.h"
#include "rendering/render_replaced.h"
#include "xml/dom_selection.h"

#include "khtmlview.h"

#include <QClipboard>

#include "rendering/render_form.h"

#define d this

using khtml::RenderPosition;

using namespace DOM;

KHTMLFind::KHTMLFind( KHTMLPart *part, KHTMLFind *parent ) :
  m_part( part ),
  m_find( 0 ),
  m_parent( parent ),
  m_findDialog( 0 )
{
  connect( part, SIGNAL(selectionChanged()), this, SLOT(slotSelectionChanged()) );
}


KHTMLFind::~KHTMLFind()
{
  d->m_find = 0; // deleted by its parent, the view.
}

void KHTMLFind::findTextBegin()
{
  d->m_findPos = -1;
  d->m_findNode = 0;
  d->m_findPosEnd = -1;
  d->m_findNodeEnd= 0;
  d->m_findPosStart = -1;
  d->m_findNodeStart = 0;
  d->m_findNodePrevious = 0;
  delete d->m_find;
  d->m_find = 0L;
}

bool KHTMLFind::initFindNode( bool selection, bool reverse, bool fromCursor )
{
    if ( m_part->document().isNull() )
        return false;

    DOM::NodeImpl* firstNode = 0L;
    if (m_part->document().isHTMLDocument())
      firstNode = m_part->htmlDocument().body().handle();
    else
      firstNode = m_part->document().handle();

    if ( !firstNode )
    {
      //qDebug() << "no first node (body or doc) -> return false";
      return false;
    }
    if ( selection && m_part->hasSelection() )
    {
      //qDebug() << "using selection";
      const Selection &sel = m_part->caret();
      if ( !fromCursor )
      {
        d->m_findNode = reverse ? sel.end().node() : sel.start().node();
        d->m_findPos = reverse ? sel.end().offset() : sel.start().offset();
      }
      d->m_findNodeEnd = reverse ? sel.start().node() : sel.end().node();
      d->m_findPosEnd = reverse ? sel.start().offset() : sel.end().offset();
      d->m_findNodeStart = !reverse ? sel.start().node() : sel.end().node();
      d->m_findPosStart = !reverse ? sel.start().offset() : sel.end().offset();
      d->m_findNodePrevious = d->m_findNodeStart;
    }
    else // whole document
    {
      //qDebug() << "whole doc";
      if ( !fromCursor )
      {
        d->m_findNode = firstNode;
        d->m_findPos = reverse ? -1 : 0;
      }
      d->m_findNodeEnd = reverse ? firstNode : 0;
      d->m_findPosEnd = reverse ? 0 : -1;
      d->m_findNodeStart = !reverse ? firstNode : 0;
      d->m_findPosStart = !reverse ? 0 : -1;
      d->m_findNodePrevious = d->m_findNodeStart;
      if ( reverse )
      {
        // Need to find out the really last object, to start from it
        khtml::RenderObject* obj = d->m_findNode ? d->m_findNode->renderer() : 0;
        if ( obj )
        {
          // find the last object in the render tree
          while ( obj->lastChild() )
          {
              obj = obj->lastChild();
          }
          // now get the last object with a NodeImpl associated
          while ( !obj->element() && obj->objectAbove() )
          {
             obj = obj->objectAbove();
          }
          d->m_findNode = obj->element();
        }
      }
    }
    return true;
}

void KHTMLFind::deactivate()
{
  // qDebug();
  d->m_lastFindState.options = d->m_findDialog->options();
  d->m_lastFindState.history = d->m_findDialog->findHistory();
  if (!m_parent) {
      d->m_findDialog->hide();
      d->m_findDialog->disconnect();
      d->m_findDialog->deleteLater();
  }
  d->m_findDialog = 0L;

  // if the selection is limited to a single link, that link gets focus
  const DOM::Selection sel = m_part->caret();
  if(sel.start().node() == sel.end().node())
  {
    bool isLink = false;

    // checks whether the node has a <A> parent
    DOM::NodeImpl *parent = sel.start().node();
    while ( parent )
    {
      if ( parent->nodeType() == Node::ELEMENT_NODE && parent->id() == ID_A )
      {
        isLink = true;
        break;
      }
      parent = parent->parentNode();
    }

    if(isLink == true)
    {
      static_cast<DOM::DocumentImpl *>( m_part->document().handle() )->setFocusNode( parent );
    }
  }
}

void KHTMLFind::slotFindDestroyed()
{
  d->m_find = 0;
}

void KHTMLFind::activate()
{
  // First do some init to make sure we can search in this frame
  if ( m_part->document().isNull() )
    return;

  // Raise if already opened
  if ( d->m_findDialog && !m_parent )
  {
    m_part->pBottomViewBar()->showBarWidget( d->m_findDialog );
    return;
  }

  // The lineedit of the dialog would make khtml lose its selection, otherwise
#ifndef QT_NO_CLIPBOARD
  disconnect( qApp->clipboard(), SIGNAL(selectionChanged()), m_part, SLOT(slotClearSelection()) );
#endif

  if (m_parent)
    d->m_findDialog  = m_parent->findBar();
  else
  {
    // Now show the dialog in which the user can choose options.
    d->m_findDialog = new KHTMLFindBar( m_part->widget() );
    d->m_findDialog->setHasSelection( m_part->hasSelection() );
    d->m_findDialog->setHasCursor( d->m_findNode != 0 );
#if 0
    if ( d->m_findNode ) // has a cursor -> default to 'FromCursor'
      d->m_lastFindState.options |= KFind::FromCursor;
#endif

    // TODO? optionsDialog.setPattern( d->m_lastFindState.text );
    d->m_findDialog->setFindHistory( d->m_lastFindState.history );
    d->m_findDialog->setOptions( d->m_lastFindState.options );
    d->m_findDialog->setFocus();

    d->m_lastFindState.options = -1; // force update in findTextNext
    d->m_lastFindState.last_dir = -1;

    m_part->pBottomViewBar()->addBarWidget( d->m_findDialog );
    m_part->pBottomViewBar()->showBarWidget( d->m_findDialog );
    connect( d->m_findDialog, SIGNAL(searchChanged()), this, SLOT(slotSearchChanged()) );
    connect( d->m_findDialog, SIGNAL(findNextClicked()), this, SLOT(slotFindNext()) );
    connect( d->m_findDialog, SIGNAL(findPreviousClicked()), this, SLOT(slotFindPrevious()) );
    connect( d->m_findDialog, SIGNAL(hideMe()), this, SLOT(deactivate()) );
  }
#ifndef QT_NO_CLIPBOARD
    connect( qApp->clipboard(), SIGNAL(selectionChanged()), m_part, SLOT(slotClearSelection()) );
#endif
  if (m_findDialog) {
    createNewKFind( m_findDialog->pattern() , 0 /*options*/, m_findDialog, 0 );
  } else if (m_parent && m_parent->find()) {
    createNewKFind( m_parent->find()->pattern(), m_parent->find()->options(), static_cast<QWidget*>(m_parent->find()->parent()), 0 );
  }
}

// ### this crawling through the render tree sucks. There should be another way to
//     do that.
static inline KHTMLPart* innerPart( khtml::RenderObject *ro ) {
    if (!ro || !ro->isWidget() || ro->isFormElement())
        return 0;
    KHTMLView* v = qobject_cast<KHTMLView*>( static_cast<khtml::RenderWidget*>(ro)->widget() );
    return v ? v->part() : 0;
}
static inline KHTMLPart* innerPartFromNode( DOM::NodeImpl *node ) {
    return (node && node->renderer() ? innerPart( node->renderer() ) : 0);
}

void KHTMLFind::createNewKFind( const QString &str, long options, QWidget *parent, KFindDialog *findDialog )
{
  // First do some init to make sure we can search in this frame
  if ( m_part->document().isNull() )
    return;

  if (m_findNode) {
    if (KHTMLPart* p = innerPartFromNode(m_findNode)) {
      p->clearSelection();
      p->findTextBegin();
    }
  }

  // Create the KFind object
  delete d->m_find;
  d->m_find = new KFind( str, options, parent, findDialog );
  d->m_find->closeFindNextDialog(); // we use KFindDialog non-modal, so we don't want other dlg popping up
  connect( d->m_find, SIGNAL(highlight(QString,int,int)),
           this, SLOT(slotHighlight(QString,int,int)) );
  connect( d->m_find, SIGNAL(destroyed()),
           this, SLOT(slotFindDestroyed()) );
  //connect(d->m_find, SIGNAL(findNext()),
  //        this, SLOT(slotFindNext()) );

  if ( !findDialog )
  {
    d->m_lastFindState.options = options;
    initFindNode( options & KFind::SelectedText,
                  options & KFind::FindBackwards,
                  options & KFind::FromCursor );
  }
}

bool KHTMLFind::findTextNext( bool reverse )
{
  if (!d->m_find)
  {
    // We didn't show the find dialog yet, let's do it then (#49442)
    activate();

    // FIXME Ugly hack: activate() may not create KFind object, so check whether it was created
    if (!d->m_find)
      return false;

    // It also means the user is trying to match a previous pattern, so try and
    // restore the last saved pattern.
    if (!m_parent && (!d->m_findDialog || !d->m_findDialog->restoreLastPatternFromHistory()))
         return false;
  }

  long options = 0;
  if ( d->m_findDialog ) // 0 when we close the dialog
  {
    // there is a search dialog
    // make sure pattern from search dialog is used
    // (### in fact pattern changes should always trigger a reconstruction of the KFind object cf. slotSearchChanged
    //   - so make this an assert)
    if ( (d->m_find->pattern() != d->m_findDialog->pattern()) ) {
      d->m_find->setPattern( d->m_findDialog->pattern() );
      d->m_find->resetCounts();
    }

    // make sure options from search dialog are used
    options = d->m_findDialog->options();
    if ( d->m_lastFindState.options != options )
    {
      d->m_find->setOptions( options );

      if ( options & KFind::SelectedText ) //#### FIXME find in selection for frames!
        Q_ASSERT( m_part->hasSelection() );

      long difference = d->m_lastFindState.options ^ options;
      if ( difference & (KFind::SelectedText | KFind::FromCursor ) )
      {
          // Important options changed -> reset search range
        (void) initFindNode( options & KFind::SelectedText,
                             options & KFind::FindBackwards,
                             options & KFind::FromCursor );
      }
      d->m_lastFindState.options = options;
    }
  } else {
    // no dialog
    options = d->m_lastFindState.options;
  }

  // only adopt options for search direction manually
  if( reverse )
    options = options ^ KFind::FindBackwards;

  // make sure our options are used by KFind
  if( d->m_find->options() != options )
    d->m_find->setOptions( options );

  // Changing find direction. Start and end nodes must be switched.
  // Additionally since d->m_findNode points after the last node
  // that was searched, it needs to be "after" it in the opposite direction.
  if( d->m_lastFindState.last_dir != -1
      && bool( d->m_lastFindState.last_dir ) != bool( options & KFind::FindBackwards ))
  {
    qSwap( d->m_findNodeEnd, d->m_findNodeStart );
    qSwap( d->m_findPosEnd, d->m_findPosStart );
    qSwap( d->m_findNode, d->m_findNodePrevious );

    // d->m_findNode now point at the end of the last searched line - advance one node
    khtml::RenderObject* obj = d->m_findNode ? d->m_findNode->renderer() : 0;
    khtml::RenderObject* end = d->m_findNodeEnd ? d->m_findNodeEnd->renderer() : 0;
    if ( obj == end )
      obj = 0L;
    else if ( obj )
    {
      do {
        obj = (options & KFind::FindBackwards) ? obj->objectAbove() : obj->objectBelow();
      } while ( obj && ( !obj->element() || obj->isInlineContinuation() ) );
    }
    if ( obj )
      d->m_findNode = obj->element();
    else {
      // already at end, start again
      (void) initFindNode( options & KFind::SelectedText,
                           options & KFind::FindBackwards,
                           options & KFind::FromCursor );
    }
  }
  d->m_lastFindState.last_dir = ( options & KFind::FindBackwards ) ? 1 : 0;

  int numMatchesOld = m_find->numMatches();
  KFind::Result res = KFind::NoMatch;
  khtml::RenderObject* obj = d->m_findNode ? d->m_findNode->renderer() : 0;
  khtml::RenderObject* end = d->m_findNodeEnd ? d->m_findNodeEnd->renderer() : 0;
  //qDebug() << "obj=" << obj << " end=" << end;
  while( res == KFind::NoMatch )
  {
    if ( d->m_find->needData() )
    {
      if ( !obj ) {
        //qDebug() << "obj=0 -> done";
        break; // we're done
      }
      //qDebug() << " gathering data";
      // First make up the QString for the current 'line' (i.e. up to \n)
      // We also want to remember the DOMNode for every portion of the string.
      // We store this in an index->node list.

      d->m_stringPortions.clear();
      bool newLine = false;
      QString str;
      DOM::NodeImpl* lastNode = d->m_findNode;
      while ( obj && !newLine )
      {
        // Grab text from render object
        QString s;
        if ( obj->renderName() == QLatin1String("RenderTextArea") )
        {
          s = static_cast<khtml::RenderTextArea *>(obj)->text();
          s = s.replace(0xa0, ' ');
        }
        else if ( obj->renderName() ==  QLatin1String("RenderLineEdit") )
        {
          khtml::RenderLineEdit *parentLine= static_cast<khtml::RenderLineEdit *>(obj);
          if (parentLine->widget()->echoMode() == QLineEdit::Normal)
            s = parentLine->widget()->text();
          s = s.replace(0xa0, ' ');
        }
        else if ( obj->isText() )
        {
          bool isLink = false;

          // checks whether the node has a <A> parent
          if ( options & KHTMLPart::FindLinksOnly )
          {
            DOM::NodeImpl *parent = obj->element();
            while ( parent )
            {
              if ( parent->nodeType() == Node::ELEMENT_NODE && parent->id() == ID_A )
              {
                isLink = true;
                break;
              }
              parent = parent->parentNode();
            }
          }
          else
          {
            isLink = true;
          }

          if ( isLink )
          {
            s = static_cast<khtml::RenderText *>(obj)->data().string();
            s = s.replace(0xa0, ' ');
          }
        }
        else if ( KHTMLPart *p = innerPart(obj) )
        {
          if (p->pFindTextNextInThisFrame(reverse))
          {
            numMatchesOld++;
            res = KFind::Match;
            lastNode = obj->element();
            break;
          }

        }
        else if ( obj->isBR() )
          s = '\n';
        else if ( !obj->isInline() && !str.isEmpty() )
          s = '\n';

        if ( lastNode == d->m_findNodeEnd )
          s.truncate( d->m_findPosEnd );
        if ( !s.isEmpty() )
        {
          newLine = s.indexOf( '\n' ) != -1; // did we just get a newline?
          if( !( options & KFind::FindBackwards ))
          {
            //qDebug() << "StringPortion: " << index << "-" << index+s.length()-1 << " -> " << lastNode;
            d->m_stringPortions.append( StringPortion( str.length(), lastNode ) );
            str += s;
          }
          else // KFind itself can search backwards, so str must not be built backwards
          {
            for( QList<StringPortion>::Iterator it = d->m_stringPortions.begin();
                 it != d->m_stringPortions.end();
                 ++it )
                (*it).index += s.length();
            d->m_stringPortions.prepend( StringPortion( 0, lastNode ) );
            str.prepend( s );
          }
        }
        // Compare obj and end _after_ we processed the 'end' node itself
        if ( obj == end )
          obj = 0L;
        else
        {
          // Move on to next object (note: if we found a \n already, then obj (and lastNode)
          // will point to the _next_ object, i.e. they are in advance.
          do {
            // We advance until the next RenderObject that has a NodeImpl as its element().
            // Otherwise (if we keep the 'last node', and it has a '\n') we might be stuck
            // on that object forever...
            obj = (options & KFind::FindBackwards) ? obj->objectAbove() : obj->objectBelow();
          } while ( obj && ( !obj->element() || obj->isInlineContinuation() ) );
        }
        if ( obj )
          lastNode = obj->element();
        else
          lastNode = 0;
      } // end while

      if ( !str.isEmpty() )
      {
        d->m_find->setData( str, d->m_findPos );
      }
      d->m_findPos = -1; // not used during the findnext loops. Only during init.
      d->m_findNodePrevious = d->m_findNode;
      d->m_findNode = lastNode;
    }
    if ( !d->m_find->needData() && !(res == KFind::Match) ) // happens if str was empty
    {
      // Let KFind inspect the text fragment, and emit highlighted if a match is found
      res = d->m_find->find();
    }
  } // end while

  if ( res == KFind::NoMatch ) // i.e. we're done
  {
    // qDebug() << "No more matches.";
    if ( !(options & KHTMLPart::FindNoPopups) && d->m_find->shouldRestart() )
    {
      // qDebug() << "Restarting";
      initFindNode( false, options & KFind::FindBackwards, false );
      d->m_find->resetCounts();
      findTextNext( reverse );
    }
    else // really done
    {
      // qDebug() << "Finishing";
      //delete d->m_find;
      //d->m_find = 0L;
      initFindNode( false, options & KFind::FindBackwards, false );
      d->m_find->resetCounts();
      d->m_part->clearSelection();
    }
    // qDebug() << "Dialog closed.";
  }

  if ( m_findDialog != 0 )
  {
    m_findDialog->setFoundMatch( res == KFind::Match );
    m_findDialog->setAtEnd( m_find->numMatches() < numMatchesOld );
  }

  return res == KFind::Match;
}

void KHTMLFind::slotHighlight( const QString& /*text*/, int index, int length )
{
  //qDebug() << "slotHighlight index=" << index << " length=" << length;
  QList<StringPortion>::Iterator it = d->m_stringPortions.begin();
  const QList<StringPortion>::Iterator itEnd = d->m_stringPortions.end();
  QList<StringPortion>::Iterator prev = it;
  // We stop at the first portion whose index is 'greater than', and then use the previous one
  while ( it != itEnd && (*it).index <= index )
  {
    prev = it;
    ++it;
  }
  Q_ASSERT ( prev != itEnd );
  DOM::NodeImpl* node = (*prev).node;
  Q_ASSERT( node );

  Selection sel(RenderPosition(node, index - (*prev).index).position());

  khtml::RenderObject* obj = node->renderer();
  khtml::RenderTextArea *renderTextArea = 0L;
  khtml::RenderLineEdit *renderLineEdit = 0L;

  Q_ASSERT( obj );
  if ( obj )
  {
    int x = 0, y = 0;

    if ( obj->renderName() == QLatin1String("RenderTextArea") )
      renderTextArea = static_cast<khtml::RenderTextArea *>(obj);
    if ( obj->renderName() == QLatin1String("RenderLineEdit") )
      renderLineEdit = static_cast<khtml::RenderLineEdit *>(obj);
    if ( !renderLineEdit && !renderTextArea )
      //if (static_cast<khtml::RenderText *>(node->renderer())
      //    ->posOfChar(d->m_startOffset, x, y))
      {
        int dummy;
        static_cast<khtml::RenderText *>(node->renderer())
          ->caretPos( RenderPosition::fromDOMPosition(sel.start()).renderedOffset(), false, x, y, dummy, dummy ); // more precise than posOfChar
        //qDebug() << "topleft: " << x << "," << y;
        if ( x != -1 || y != -1 )
        {
          int gox = m_part->view()->contentsX();
          if (x+50 > m_part->view()->contentsX() + m_part->view()->visibleWidth())
              gox = x - m_part->view()->visibleWidth() + 50;
          if (x-10 < m_part->view()->contentsX())
              gox = x - m_part->view()->visibleWidth() - 10;
          if (gox < 0) gox = 0;
          m_part->view()->setContentsPos(gox, y-50);
        }
      }
  }
  // Now look for end node
  it = prev; // no need to start from beginning again
  while ( it != itEnd && (*it).index < index + length )
  {
    prev = it;
    ++it;
  }
  Q_ASSERT ( prev != itEnd );

  sel.moveTo(sel.start(), RenderPosition((*prev).node, index + length - (*prev).index).position());

#if 0
  // qDebug() << "slotHighlight: " << d->m_selectionStart.handle() << "," << d->m_startOffset << " - " <<
    d->m_selectionEnd.handle() << "," << d->m_endOffset << endl;
  it = d->m_stringPortions.begin();
  for ( ; it != d->m_stringPortions.end() ; ++it )
    // qDebug() << "  StringPortion: from index=" << (*it).index << " -> node=" << (*it).node;
#endif
  if ( renderTextArea )
    renderTextArea->highLightWord( length, sel.end().offset()-length );
  else if ( renderLineEdit )
    renderLineEdit->highLightWord( length, sel.end().offset()-length );
  else
  {
    m_part->setCaret( sel );
//    d->m_doc->updateSelection();
    if (sel.end().node()->renderer() )
    {
      int x, y, height, dummy;
      static_cast<khtml::RenderText *>(sel.end().node()->renderer())
          ->caretPos( RenderPosition::fromDOMPosition(sel.end()).renderedOffset(), false, x, y, dummy, height ); // more precise than posOfChar
      //qDebug() << "bottomright: " << x << "," << y+height;
    }
  }
  m_part->emitSelectionChanged();

}

void KHTMLFind::slotSelectionChanged()
{
  if ( d->m_findDialog )
       d->m_findDialog->setHasSelection( m_part->hasSelection() );
}

void KHTMLFind::slotSearchChanged()
{
    createNewKFind( m_findDialog->pattern(), m_findDialog->options(), m_findDialog, 0 );
    findTextNext();
}

void KHTMLFind::slotFindNext()
{
    findTextNext();
}

void KHTMLFind::slotFindPrevious()
{
    findTextNext( true );  // find backwards
}
