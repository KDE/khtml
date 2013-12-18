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
#ifndef _khtml_viewbarwidget_h_
#define _khtml_viewbarwidget_h_

#include <QWidget>

class KHTMLViewBarWidget : public QWidget
{
    Q_OBJECT
public:
    explicit KHTMLViewBarWidget(bool addCloseButton, QWidget* parent = 0);

protected:
    /**
     * @return widget that should be used to add controls to bar widget
     */
    QWidget *centralWidget() { return m_centralWidget; }

    void resizeEvent(QResizeEvent *event);

Q_SIGNALS:
    void hideMe();

private:
    QWidget *m_centralWidget;
};

#endif
