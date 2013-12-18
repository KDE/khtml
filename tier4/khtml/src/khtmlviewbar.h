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
#ifndef _khtml_viewbar_h_
#define _khtml_viewbar_h_

#include <QWidget>

class KHTMLView;
class KHTMLViewBarWidget;

class KHTMLViewBar : public QWidget
{
    Q_OBJECT
public:
    enum Position {
        Top,
        Bottom
    };

    KHTMLViewBar(Position position, KHTMLView *view, QWidget *parent);

    /**
     * Adds a widget to this viewbar.
     * Widget is initially invisible, you should call showBarWidget, to show it.
     * Several widgets can be added to the bar, but only one can be visible
     */
    void addBarWidget(KHTMLViewBarWidget *newBarWidget);

    /**
     * Shows barWidget that was previously added with addBarWidget.
     * @see hideCurrentBarWidget
     */
    void showBarWidget(KHTMLViewBarWidget *barWidget);

    /**
     * Adds widget that will be always shown in the viewbar.
     * After adding permanent widget viewbar is immediately shown.
     * ViewBar with permanent widget won't hide itself
     * until permanent widget is removed.
     * OTOH showing/hiding regular barWidgets will work as usual
     * (they will be shown above permanent widget)
     *
     * If permanent widget already exists, new one replaces old one
     * Old widget is not deleted, caller can do it if it wishes
     */
    void addPermanentBarWidget(KHTMLViewBarWidget *barWidget);

    /**
     * Removes permanent bar widget from viewbar.
     * If no other viewbar widgets are shown, viewbar gets hidden.
     *
     * barWidget is not deleted, caller must do it if it wishes
     */
    void removePermanentBarWidget(KHTMLViewBarWidget *barWidget);

    /**
     * @return if viewbar has permanent widget @p barWidget
     */
    bool hasPermanentWidget(KHTMLViewBarWidget *barWidget) const;

public Q_SLOTS:
    /**
     * Hides currently shown bar widget
     */
    void hideCurrentBarWidget();

protected:
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void hideEvent(QHideEvent* event);

private:
    bool hasWidget(KHTMLViewBarWidget*) const;

    /**
     * Shows or hides whole viewbar
     */
    void setViewBarVisible(bool visible);

private:
    KHTMLView *m_view;
    KHTMLViewBarWidget *m_permanentBarWidget;
};

#endif
