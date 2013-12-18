/* This file is part of the KDE project
 *
 * Copyright (C) 2008 Bernhard Beschow <bbeschow cs tu berlin de>
 *           (C) 2008 Germain Garand <germain@ebooksfrance.org>
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

#include "khtmlfindbar.h"

#include "khtml_part.h"

#include <kfind.h>
#include <kcolorscheme.h>

#include <QMenu>
#include <QLineEdit>

#define d this

KHTMLFindBar::KHTMLFindBar( QWidget *parent ) :
    KHTMLViewBarWidget( true, parent ),
    m_enabled( KFind::WholeWordsOnly | KFind::FromCursor | KFind::SelectedText | KFind::CaseSensitive | KFind::FindBackwards | KFind::RegularExpression | KHTMLPart::FindLinksOnly )
{
    setupUi( centralWidget() );

    m_next->setIcon( QIcon::fromTheme( "go-down-search" ) );
    m_previous->setIcon( QIcon::fromTheme( "go-up-search" ) );
    m_next->setDisabled( true );
    m_previous->setDisabled( true );

    // Fill options menu
    m_incMenu = new QMenu();
    m_options->setMenu(m_incMenu);
    m_caseSensitive = m_incMenu->addAction(i18n("C&ase sensitive"));
    m_caseSensitive->setCheckable(true);
    m_wholeWordsOnly = m_incMenu->addAction(i18n("&Whole words only"));
    m_wholeWordsOnly->setCheckable(true);
    m_fromCursor = m_incMenu->addAction(i18n("From c&ursor"));
    m_fromCursor->setCheckable(true);
    m_selectedText = m_incMenu->addAction(i18n("&Selected text"));
    m_selectedText->setCheckable(true);
    m_regExp = m_incMenu->addAction(i18n("Regular e&xpression"));
    m_regExp->setCheckable(true);
    m_findLinksOnly = m_incMenu->addAction(i18n("Find &links only"));
    m_findLinksOnly->setCheckable(true);

    m_atEnd = false;

    m_find->setDuplicatesEnabled( false );
    centralWidget()->setFocusProxy( m_find );

    connect( m_selectedText, SIGNAL(toggled(bool)), this, SLOT(slotSelectedTextToggled(bool)) );
    connect( m_find, SIGNAL(editTextChanged(QString)), this, SIGNAL(searchChanged()) );
    connect( m_find->lineEdit(), SIGNAL(clearButtonClicked()), this, SLOT(slotAddPatternToHistory()) );
    connect( this, SIGNAL(hideMe()), this, SLOT(slotAddPatternToHistory()) );
    connect( this, SIGNAL(searchChanged()), this, SLOT(slotSearchChanged()) );
    connect( m_next, SIGNAL(clicked()), this, SIGNAL(findNextClicked()) );
    connect( m_previous, SIGNAL(clicked()), this, SIGNAL(findPreviousClicked()) );
    connect( m_caseSensitive, SIGNAL(changed()), this, SIGNAL(searchChanged()) );
    connect( m_wholeWordsOnly, SIGNAL(changed()), this, SIGNAL(searchChanged()) );
    connect( m_fromCursor, SIGNAL(changed()), this, SIGNAL(searchChanged()) );
    connect( m_regExp, SIGNAL(changed()), this, SIGNAL(searchChanged()) );
    connect( m_findLinksOnly, SIGNAL(changed()), this, SIGNAL(searchChanged()) );

    m_find->setFocus();
}

QStringList KHTMLFindBar::findHistory() const
{
    return d->m_find->historyItems();
}

long KHTMLFindBar::options() const
{
    long options = 0;

    if (d->m_caseSensitive->isChecked())
        options |= KFind::CaseSensitive;
    if (d->m_wholeWordsOnly->isChecked())
        options |= KFind::WholeWordsOnly;
    if (d->m_fromCursor->isChecked())
        options |= KFind::FromCursor;
    if (d->m_selectedText->isChecked())
        options |= KFind::SelectedText;
    if (d->m_regExp->isChecked())
        options |= KFind::RegularExpression;
    if (d->m_findLinksOnly->isChecked())
        options |= KHTMLPart::FindLinksOnly;
    return options | KHTMLPart::FindNoPopups /* | KFind::FindIncremental */;
}

QString KHTMLFindBar::pattern() const
{
    return m_find->currentText();
}

void KHTMLFindBar::slotSearchChanged()
{
   // reset background color of the combo box
   if (pattern().isEmpty()) {
       d->m_find->setPalette(QPalette());
       m_next->setDisabled( true );
       m_previous->setDisabled( true );
       m_statusLabel->clear();
   } else {
       m_prevPattern = pattern();
       m_next->setDisabled( false );
       m_previous->setDisabled( false );
   }
}

bool KHTMLFindBar::restoreLastPatternFromHistory()
{
    if (d->m_find->historyItems().isEmpty())
        return false;
    d->m_find->lineEdit()->setText( d->m_find->historyItems().first() );
    return true;
}

void KHTMLFindBar::setFindHistory(const QStringList &strings)
{
    if (strings.count() > 0)
    {
        d->m_find->setHistoryItems(strings, true);
        //d->m_find->lineEdit()->setText( strings.first() );
        //d->m_find->lineEdit()->selectAll();
    }
    else
        d->m_find->clearHistory();
}

void KHTMLFindBar::setHasSelection(bool hasSelection)
{
    if (hasSelection) d->m_enabled |= KFind::SelectedText;
    else d->m_enabled &= ~KFind::SelectedText;
    d->m_selectedText->setEnabled( hasSelection );
    if ( !hasSelection )
    {
        d->m_selectedText->setChecked( false );
        slotSelectedTextToggled( hasSelection );
    }
}

void KHTMLFindBar::slotAddPatternToHistory()
{
    bool patternIsEmpty = pattern().isEmpty();
    if (!patternIsEmpty || !m_prevPattern.isEmpty()) {
        d->m_find->addToHistory(pattern().isEmpty() ? m_prevPattern : pattern());
        if (patternIsEmpty && !pattern().isEmpty()) {
            // ### Hack - addToHistory sometimes undo the clearing of the lineEdit
            // with clear button. Clear it again.
            bool sb = d->m_find->blockSignals(true);
            d->m_find->lineEdit()->setText(QString());
            d->m_find->blockSignals(sb);
        }
        m_prevPattern.clear();
    }
}

void KHTMLFindBar::slotSelectedTextToggled(bool selec)
{
    // From cursor doesn't make sense if we have a selection
    m_fromCursor->setEnabled( !selec && (m_enabled & KFind::FromCursor) );
    if ( selec ) // uncheck if disabled
        m_fromCursor->setChecked( false );
}

void KHTMLFindBar::setHasCursor(bool hasCursor)
{
    if (hasCursor) d->m_enabled |= KFind::FromCursor;
    else d->m_enabled &= ~KFind::FromCursor;
    d->m_fromCursor->setEnabled( hasCursor );
    d->m_fromCursor->setChecked( hasCursor && (options() & KFind::FromCursor) );
}

void KHTMLFindBar::setOptions(long options)
{
    d->m_caseSensitive->setChecked((d->m_enabled & KFind::CaseSensitive) && (options & KFind::CaseSensitive));
    d->m_wholeWordsOnly->setChecked((d->m_enabled & KFind::WholeWordsOnly) && (options & KFind::WholeWordsOnly));
    d->m_fromCursor->setChecked((d->m_enabled & KFind::FromCursor) && (options & KFind::FromCursor));
    d->m_selectedText->setChecked((d->m_enabled & KFind::SelectedText) && (options & KFind::SelectedText));
    d->m_regExp->setChecked((d->m_enabled & KFind::RegularExpression) && (options & KFind::RegularExpression));
    d->m_findLinksOnly->setChecked((d->m_enabled & KHTMLPart::FindLinksOnly) && (options & KHTMLPart::FindLinksOnly));
}

void KHTMLFindBar::setFoundMatch( bool match )
{
    if ( pattern().isEmpty() ) {
        m_find->setPalette(QPalette());
        m_next->setDisabled( true );
        m_previous->setDisabled( true );
        m_statusLabel->clear();
    } else if ( !match ) {
        QPalette newPal( m_find->palette() );
        KColorScheme::adjustBackground(newPal, KColorScheme::NegativeBackground);
        m_find->setPalette(newPal);
        m_statusLabel->setText(i18n("Not found"));
    } else {
        QPalette newPal( m_find->palette() );
        KColorScheme::adjustBackground(newPal, KColorScheme::PositiveBackground);
        m_find->setPalette(newPal);
        m_statusLabel->clear();
    }
}

void KHTMLFindBar::setAtEnd( bool atEnd )
{
    if (atEnd == m_atEnd)
        return;
    if ( atEnd ) {
        m_statusLabel->setText( i18n( "No more matches for this search direction." ) );
    } else {
        m_statusLabel->clear();
    }
    m_atEnd = atEnd;
}

void KHTMLFindBar::setVisible( bool visible )
{
    KHTMLViewBarWidget::setVisible( visible );

    if ( visible ) {
        m_find->setFocus( Qt::ActiveWindowFocusReason );
        m_find->lineEdit()->selectAll();
    }
}

bool KHTMLFindBar::event(QEvent* e)
{
    // Close the bar when pressing Escape.
    // Not using a QShortcut for this because it could conflict with
    // window-global actions (e.g. Emil Sedgh binds Esc to "close tab").
    // With a shortcut override we can catch this before it gets to kactions.
    if (e->type() == QEvent::ShortcutOverride) {
        QKeyEvent* kev = static_cast<QKeyEvent* >(e);
        if (kev->key() == Qt::Key_Escape) {
            e->accept();
            emit hideMe();
            return true;
        }
    }
    return KHTMLViewBarWidget::event(e);
}
