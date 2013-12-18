/*
 * This file is part of the DOM implementation for KDE.
 * Copyright (C) 2002 Lars Knoll <knoll@kde.org>
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
 *
 */


#ifndef MISC_ENUM_H
#define MISC_ENUM_H

#include <qglobal.h>

#ifdef Q_CC_MSVC
# define KDE_BF_ENUM(a) unsigned int
# define KDE_CAST_BF_ENUM(a,b) static_cast<a>(b)
#else
# define KDE_BF_ENUM(a) a
# define KDE_CAST_BF_ENUM(a,b) b
#endif

#endif
