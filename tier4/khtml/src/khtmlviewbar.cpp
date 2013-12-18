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
#include "khtmlviewbar.h"

#include "khtmlview.h"
#include "khtmlviewbarwidget.h"

#include <QDebug>

#include <QBoxLayout>
#include <QKeyEvent>

KHTMLViewBar::KHTMLViewBar( Position position, KHTMLView *view, QWidget *parent ) :
    QWidget( parent ),
    m_view( view ),
    m_permanentBarWidget( 0 )
{
    const QBoxLayout::Direction direction = ( position == Top ? QBoxLayout::TopToBottom : QBoxLayout::BottomToTop );

    setLayout( new QBoxLayout( direction, this ) );
    layout()->setContentsMargins( 0, 0, 0, 0 );
    layout()->setSpacing( 0 );
}

void KHTMLViewBar::addBarWidget (KHTMLViewBarWidget *newBarWidget)
{
  if (hasWidget(newBarWidget)) {
    // qDebug() << "this bar widget is already added";
    return;
  }
  // add new widget, invisible...
  newBarWidget->hide();
  layout()->addWidget( newBarWidget );
  connect(newBarWidget, SIGNAL(hideMe()), SLOT(hideCurrentBarWidget()));

  // qDebug() << "add barwidget " << newBarWidget;
}

void KHTMLViewBar::addPermanentBarWidget (KHTMLViewBarWidget *barWidget)
{
  // remove old widget from layout (if any)
  if (m_permanentBarWidget) {
    m_permanentBarWidget->hide();
    layout()->removeWidget(m_permanentBarWidget);
  }

  layout()->addWidget(barWidget /*, 0, Qt::AlignBottom*/ ); // FIXME
  m_permanentBarWidget = barWidget;
  m_permanentBarWidget->show();

  setViewBarVisible(true);
}

void KHTMLViewBar::removePermanentBarWidget (KHTMLViewBarWidget *barWidget)
{
  if (m_permanentBarWidget != barWidget) {
    // qDebug() << "no such permanent widget exists in bar";
    return;
  }

  if (!m_permanentBarWidget)
    return;

  m_permanentBarWidget->hide();
  layout()->removeWidget(m_permanentBarWidget);
  m_permanentBarWidget = 0;
}

bool KHTMLViewBar::hasPermanentWidget (KHTMLViewBarWidget *barWidget ) const
{
    return (m_permanentBarWidget == barWidget);
}

void KHTMLViewBar::showBarWidget (KHTMLViewBarWidget *barWidget)
{
  // raise correct widget
// TODO  m_stack->setCurrentWidget (barWidget);
  barWidget->show();

  // if we have any permanent widget, bar is always visible,
  // no need to show it
  if (!m_permanentBarWidget) {
    setViewBarVisible(true);
  }
}

bool KHTMLViewBar::hasWidget(KHTMLViewBarWidget* wid) const
{
    Q_UNUSED(wid);
    return layout()->count() != 0;
}

void KHTMLViewBar::hideCurrentBarWidget ()
{
//  m_stack->hide();

  // if we have any permanent widget, bar is always visible,
  // no need to hide it
  if (!m_permanentBarWidget) {
    setViewBarVisible(false);
  }

  m_view->setFocus();
  // qDebug()<<"hide barwidget";
}

void KHTMLViewBar::setViewBarVisible (bool visible)
{
    setVisible( visible );
}

void KHTMLViewBar::keyPressEvent(QKeyEvent* event)
{
  if (event->key() == Qt::Key_Escape) {
    hideCurrentBarWidget();
    return;
  }
  QWidget::keyPressEvent(event);

}

void KHTMLViewBar::hideEvent(QHideEvent* event)
{
  Q_UNUSED(event);
//   if (!event->spontaneous())
//     m_view->setFocus();
}
