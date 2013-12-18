/*
    This file is part of the KDE libraries

    The Original Code is WOFF font packaging code.
    Copyright (C) 2009 Mozilla Corporation
 
    Contributor(s):
      Jonathan Kew <jfkthame@gmail.com>
      Germain Garand <germain@ebooksfrance.org>

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

#ifndef WOFF_PRIVATE_H_
#define WOFF_PRIVATE_H_

#include "woff.h"
#include <QtGlobal>

/* private definitions used in the WOFF encoder/decoder functions */

/* create an OT tag from 4 characters */
#define TAG(a,b,c,d) ((a)<<24 | (b)<<16 | (c)<<8 | (d))

#define WOFF_SIGNATURE    TAG('w','O','F','F')

#define SFNT_VERSION_CFF  TAG('O','T','T','O')
#define SFNT_VERSION_TT   0x00010000
#define SFNT_VERSION_true TAG('t','r','u','e')

#define TABLE_TAG_DSIG    TAG('D','S','I','G')
#define TABLE_TAG_head    TAG('h','e','a','d')
#define TABLE_TAG_bhed    TAG('b','h','e','d')

#define SFNT_CHECKSUM_CALC_CONST  0xB1B0AFBAU /* from the TT/OT spec */

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
#  define READ16BE(x) (x)
#  define READ32BE(x) (x)
#else
#  define READ16BE(x) ((((x) & 0xff) << 8) | (((x) >> 8) & 0xff))
#  define READ32BE(x) ((READ16BE((x) & 0xffff) << 16) | (READ16BE((x) >> 16)))
# endif

#if defined(__SUNPRO_CC)
#pragma pack(1) /* no pragma stack in Sun Studio */
#else
#pragma pack(push,1) /* assume this is GCC compatible */
#endif

typedef struct {
  quint32 version;
  quint16 numTables;
  quint16 searchRange;
  quint16 entrySelector;
  quint16 rangeShift;
} sfntHeader;

typedef struct {
  quint32 tag;
  quint32 checksum;
  quint32 offset;
  quint32 length;
} sfntDirEntry;

typedef struct {
  quint32 signature;
  quint32 flavor;
  quint32 length;
  quint16 numTables;
  quint16 reserved;
  quint32 totalSfntSize;
  quint16 majorVersion;
  quint16 minorVersion;
  quint32 metaOffset;
  quint32 metaCompLen;
  quint32 metaOrigLen;
  quint32 privOffset;
  quint32 privLen;
} woffHeader;

typedef struct {
  quint32 tag;
  quint32 offset;
  quint32 compLen;
  quint32 origLen;
  quint32 checksum;
} woffDirEntry;

typedef struct {
  quint32 version;
  quint32 fontRevision;
  quint32 checkSumAdjustment;
  quint32 magicNumber;
  quint16 flags;
  quint16 unitsPerEm;
  quint32 created[2];
  quint32 modified[2];
  qint16 xMin;
  qint16 yMin;
  qint16 xMax;
  qint16 yMax;
  quint16 macStyle;
  quint16 lowestRecPpem;
  qint16 fontDirectionHint;
  qint16 indexToLocFormat;
  qint16 glyphDataFormat;
} sfntHeadTable;

#define HEAD_TABLE_SIZE 54 /* sizeof(sfntHeadTable) may report 56 because of alignment */

typedef struct {
  quint32 offset;
  quint16 oldIndex;
  quint16 newIndex;
} tableOrderRec;

#if defined(__SUNPRO_CC)
#pragma pack()
#else
#pragma pack(pop)
#endif

#endif
