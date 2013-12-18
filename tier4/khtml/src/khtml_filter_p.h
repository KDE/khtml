/* This file is part of the KDE project

   Copyright (C) 2005 Ivor Hewitt     <ivor@kde.org>
   Copyright (C) 2008 Maksim Orlovich <maksim@kde.org>
   Copyright (C) 2008 Vyacheslav Tokarev <tsjoker@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <QString>
#include <QRegExp>
#include <QVector>
#include <wtf/HashMap.h>
#include <QHash>
#include <QBitArray>

namespace khtml {

// Updateable Multi-String Matcher based on Rabin-Karp's algorithm
class StringsMatcher {
public:
    // add filter to matching set
    void addString(const QString& pattern);

    // check if string matches at least one string from matching set,
    // optionally return the matching string or filter
    bool isMatched(const QString& str, QString *by = 0) const;

    // add filter to matching set with wildcards (*,?) in it
    void addWildedString(const QString& prefix, const QRegExp& rx);

    void clear();

private:
    QVector<QString> stringFilters;
    QVector<QString> shortStringFilters;
    QVector<QRegExp> reFilters;
    QVector<QString> rePrefixes;
    QBitArray fastLookUp;

    WTF::HashMap<int, QVector<int> > stringFiltersHash;
};

// This represents a set of filters that may match URLs.
// Currently it supports a subset of AddBlock Plus functionality.
class FilterSet {
public:
    // Parses and registers a filter. This will also strip @@ for exclusion rules, skip comments, etc.
    // The user does have to split black and white lists into separate sets, however
    void addFilter(const QString& filter);

    bool isUrlMatched(const QString& url);
    QString urlMatchedBy(const QString& url);

    void clear();

private:
    QVector<QRegExp> reFilters;
    StringsMatcher stringFiltersMatcher;
};

}

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
