/* This file is part of the KDE project
 *
 * Copyright (C) 2008 Bernhard Beschow <bbeschow AT cs DOT tu-berlin de>
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
#include "khtmlviewbarwidget.h"


#include <QHBoxLayout>
#include <QToolButton>
#include <QResizeEvent>

KHTMLViewBarWidget::KHTMLViewBarWidget( bool addCloseButton, QWidget *parent )
 : QWidget( parent )
{
    QHBoxLayout *layout = new QHBoxLayout;

    // NOTE: Here be cosmetics.
    layout->setMargin( 2 );

    // hide button
    if ( addCloseButton ) {
        QToolButton *hideButton = new QToolButton( this );
        hideButton->setAutoRaise( true );
        hideButton->setIcon( QIcon::fromTheme( "dialog-close" ) );
        connect( hideButton, SIGNAL(clicked()), SIGNAL(hideMe()) );
        layout->addWidget( hideButton );
        layout->setAlignment( hideButton, Qt::AlignLeft | Qt::AlignTop );
    }

    // widget to be used as parent for the real content
    m_centralWidget = new QWidget( this );
    layout->addWidget( m_centralWidget );

    setLayout( layout );
    setFocusProxy( m_centralWidget );
}

void KHTMLViewBarWidget::resizeEvent( QResizeEvent *event )
{
    if ( event->size().width() != width() )
        resize( event->size().width(), minimumSize().height() );
    QWidget::resizeEvent( event );
}
