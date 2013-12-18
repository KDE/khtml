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
#ifndef __khtml_findbar_h__
#define __khtml_findbar_h__

#include "khtmlviewbarwidget.h"
#include "ui_khtmlfindbar_base.h"

class QMenu;
class QAction;

class KHTMLFindBar : public KHTMLViewBarWidget, private Ui::KHTMLFindBarBase
{
    Q_OBJECT
public:
    KHTMLFindBar( QWidget *parent = 0 );

    /**
     * Provide the list of @p strings to be displayed as the history
     * of find strings. @p strings might get truncated if it is
     * too long.
     *
     * @param history The find history.
     * @see findHistory
     */
    void setFindHistory( const QStringList &history );

    /**
     * Returns the list of history items.
     *
     * @see setFindHistory
     */
    QStringList findHistory() const;

    /*
     *  makes the current pattern be the first entry of the find history.
     *  Return false if the history was empty.
     *
     */
    bool restoreLastPatternFromHistory();

    /**
     * Enable/disable the 'search in selection' option, depending
     * on whether there actually is a selection.
     *
     * @param hasSelection true if a selection exists
     */
    void setHasSelection( bool hasSelection );

    /**
     * Hide/show the 'from cursor' option, depending
     * on whether the application implements a cursor.
     *
     * @param hasCursor true if the application features a cursor
     * This is assumed to be the case by default.
     */
    void setHasCursor( bool hasCursor );

    /**
     * Set the options which are checked.
     *
     * @param options The setting of the Options.
     *
     * @see options()
     * @see KFind::Options
     */
    void setOptions( long options );

    /**
     * Returns the state of the options. Disabled options may be returned in
     * an indeterminate state.
     *
     * @see setOptions()
     * @see KFind::Options
     */
    long options() const;

    /**
     * Returns the pattern to find.
     */
    QString pattern() const;

    void setFoundMatch( bool match );
    void setAtEnd( bool atEnd );
    virtual void setVisible( bool visible );

protected:
    virtual bool event(QEvent* e);

private Q_SLOTS:
    void slotSelectedTextToggled(bool selec);
    void slotSearchChanged();
    void slotAddPatternToHistory();

Q_SIGNALS:
    void searchChanged();
    void findNextClicked();
    void findPreviousClicked();

private:
    long m_enabled;
    QMenu *m_incMenu;
    QAction *m_caseSensitive;
    QAction *m_wholeWordsOnly;
    QAction *m_fromCursor;
    QAction *m_selectedText;
    QAction *m_regExp;
    QAction *m_findLinksOnly;
    QString m_prevPattern;
    bool m_atEnd;
};

#endif
