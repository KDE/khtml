/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2006 Matt Broadstone (mbroadst@gmail.com)
 *  Copyright (C) 2007 Maks Orlovich   (maksim@kde.org)
 *  Copyright ©   2007 Fredrik Höglund <fredrik@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "consoledock.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QModelIndex>
#include <QTextLayout>
#include <QPainter>
#include <QLineEdit>
#include <QListWidget>
#include <QDebug>
#include <QAbstractItemDelegate>
#include <QTextLayout>

#include <khistorycombobox.h>
#include <klocalizedstring.h>

namespace KJSDebugger {

class ConsoleItemDelegate : public QAbstractItemDelegate
{
public:
    enum ItemRole { ResultRole = 0x106EE9CA };

    ConsoleItemDelegate(QObject *parent = 0);
    ~ConsoleItemDelegate();
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

private:
    void setLayoutOption(QTextLayout &layout, const QStyleOptionViewItemV3 &option, Qt::Alignment alignment) const;
    QSize layoutText(QTextLayout &layout, const QString &text, const QSize &constraints) const;
    void drawTextLayout(QPainter *painter, const QTextLayout &layout, const QRect &rect) const;
};


ConsoleItemDelegate::ConsoleItemDelegate(QObject *parent)
    : QAbstractItemDelegate(parent)
{
}


ConsoleItemDelegate::~ConsoleItemDelegate()
{
}

void ConsoleItemDelegate::setLayoutOption(QTextLayout &layout, const QStyleOptionViewItemV3 &option, Qt::Alignment alignment) const
{
    QTextOption textoption;
    textoption.setTextDirection(option.direction);
    textoption.setAlignment(QStyle::visualAlignment(option.direction, alignment));
    textoption.setWrapMode(option.features & QStyleOptionViewItemV2::WrapText ? QTextOption::WordWrap
                           : QTextOption::NoWrap);

    layout.setTextOption(textoption);
}

QSize ConsoleItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!index.isValid())
        return QSize();

    QFont normalFont = option.font;
    QFont boldFont = normalFont;
    boldFont.setBold(true);

    // Extract the items we want to measure
    QString input      = index.data(Qt::DisplayRole).toString();
    QString result     = index.data(ResultRole).toString();

    QStyleOptionViewItemV3 opt(option);
    int maxWidth = static_cast<const QListView*>(opt.widget)->viewport()->width();

    QTextLayout inputLayout, resultLayout;
    setLayoutOption(inputLayout, opt, Qt::AlignLeft);
    setLayoutOption(resultLayout, opt, Qt::AlignRight);

    inputLayout.setFont(normalFont);
    resultLayout.setFont(boldFont);

    QSize constraints(maxWidth - 4, 65536);

    QSize inputSize  = layoutText(inputLayout, input, constraints);
    QSize resultSize = layoutText(resultLayout, result, constraints);

    int width = qBound(0, qMax(inputSize.width(), resultSize.width()) + 4, maxWidth);
    int height = inputSize.height() + option.fontMetrics.leading() + resultSize.height();

    return QSize(width, height + 8);
}

QSize ConsoleItemDelegate::layoutText(QTextLayout &layout, const QString &text, const QSize &constraints) const
{
    QFontMetrics metrics(layout.font());
    int leading     = metrics.leading();
    int height      = 0;
    int maxWidth    = constraints.width();
    int widthUsed   = 0;
    QTextLine line;

    layout.setText(text);

    layout.beginLayout();
    while ((line = layout.createLine()).isValid())
    {
        height += leading;

        line.setLineWidth(maxWidth);

        // Make the last line that will fit infinitely long.
        // drawTextLayout() will handle this by fading the line out
        // if it won't fit in the contraints.
        if (height + metrics.height() * 2 + leading >= constraints.height()) {
            line.setLineWidth(INT_MAX);
            line.setLineWidth(line.naturalTextWidth());
        } else
            line.setLineWidth(maxWidth);

        line.setPosition(QPoint(0, height));

        height += int(line.height());
        widthUsed = int(qMax(qreal(widthUsed), line.naturalTextWidth()));
    }
    layout.endLayout();

    return QSize(widthUsed, height);
}

void ConsoleItemDelegate::drawTextLayout(QPainter *painter, const QTextLayout &layout, const QRect &rect) const
{
    if (rect.width() < 1 || rect.height() < 1) {
        return;
    }

    QFontMetrics fm(layout.font());
    bool leftAligned = layout.textOption().alignment() & Qt::AlignLeft;

    // Draw each line in the layout
    for (int i = 0; i < layout.lineCount(); i++)
    {
        QTextLine line = layout.lineAt(i);

        int width = line.naturalTextWidth();
        int x = width > rect.width() && !leftAligned ? rect.width() - width : 0;
        line.draw(painter, QPoint(x + rect.x(), rect.y()));
    }
}

void ConsoleItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!index.isValid())
        return;

    painter->save();

    QFont normalFont = option.font;
    QFont boldFont = normalFont;
    boldFont.setBold(true);

    QString input  = index.data(Qt::DisplayRole).toString();
    QString result = index.data(ResultRole).toString();

    // Draw the background
    bool selected = option.state & QStyle::State_Selected;

    QStyleOptionViewItemV4 opt(option);

    QStyle *style = opt.widget->style();
    style->drawPrimitive(QStyle::PE_PanelItemViewRow, &opt, painter, opt.widget);
    if (selected)
        painter->setPen(option.palette.color(QPalette::HighlightedText));

    // Draw the text
    QTextLayout inputLayout, resultLayout;
    setLayoutOption(inputLayout, opt, Qt::AlignLeft);
    setLayoutOption(resultLayout, opt, Qt::AlignRight);

    inputLayout.setFont(normalFont);
    resultLayout.setFont(boldFont);

    QRect contentRect = option.rect.adjusted(2, 4, -2, -4);

    QSize inputSize = layoutText(inputLayout, input, contentRect.size());
    QSize resultSize = layoutText(resultLayout, result, QSize(contentRect.width(),
                                  contentRect.height() - inputSize.height() - option.fontMetrics.leading()));

    QRect inputRect  = QStyle::alignedRect(option.direction, Qt::AlignLeft | Qt::AlignTop, inputSize, contentRect);
    QRect resultRect = QStyle::alignedRect(option.direction, Qt::AlignRight | Qt::AlignBottom, resultSize, contentRect);

    drawTextLayout(painter, inputLayout, inputRect & option.rect);
    drawTextLayout(painter, resultLayout, resultRect & option.rect);

    painter->restore();
}

//------------------------------------------------------------------------

ConsoleDock::ConsoleDock(QWidget *parent)
    : QDockWidget(i18n("Console"), parent)
{
    setFeatures(DockWidgetMovable | DockWidgetFloatable);

    QWidget* mainFrame = new QWidget(this);

    consoleView = new QListWidget(mainFrame);
    consoleView->setWordWrap(true);
    consoleView->setItemDelegate(new ConsoleItemDelegate(consoleView));
    consoleView->setAlternatingRowColors(true);

    connect(consoleView, SIGNAL(itemClicked(QListWidgetItem*)),
            this, SLOT(slotPasteItem(QListWidgetItem*)));
    connect(consoleView, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
            this, SLOT(slotPasteItem(QListWidgetItem*)));

    consoleInput = new KHistoryComboBox(mainFrame);
    consoleInput->setSizePolicy(QSizePolicy::MinimumExpanding,
                                QSizePolicy::Fixed);

    connect(consoleInput, SIGNAL(returnPressed()),
           this, SLOT(slotUserRequestedEval()));

    consoleInputButton = new QPushButton(i18n("Enter"), mainFrame);
    connect(consoleInputButton, SIGNAL(clicked(bool)),
            this, SLOT(slotUserRequestedEval()));

    QHBoxLayout *bottomLayout = new QHBoxLayout;
    bottomLayout->setSpacing(0);
    bottomLayout->setMargin(0);
    bottomLayout->addWidget(consoleInput);
    bottomLayout->addWidget(consoleInputButton);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setMargin(0);
    mainLayout->addWidget(consoleView);
    mainLayout->addLayout(bottomLayout);
    mainFrame->setLayout(mainLayout);

    setWidget(mainFrame);
}

ConsoleDock::~ConsoleDock()
{
}

void ConsoleDock::reportResult(const QString& src, const QString& msg)
{
    QListWidgetItem* item = new QListWidgetItem(src, consoleView);
    item->setData(ConsoleItemDelegate::ResultRole, msg);
    consoleView->scrollToItem(item);
}

void ConsoleDock::slotPasteItem(QListWidgetItem* item)
{
    consoleInput->lineEdit()->setText(item->data(Qt::DisplayRole).toString());
}

void ConsoleDock::slotUserRequestedEval()
{
    QString src = consoleInput->lineEdit()->text();
    consoleInput->addToHistory(src);
    consoleInput->lineEdit()->clear();

    emit requestEval(src);
}

}
