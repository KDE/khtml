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

#include "woff-private.h"
#include <string.h>
#include <stdlib.h>
#include <zlib.h>

using namespace WOFF;

/* on errors, each function sets a status variable and jumps to failure: */
#undef FAIL
#define FAIL(err) do { status |= err; goto failure; } while (0)

/* adjust an offset for longword alignment */
#define LONGALIGN(x) (((x) + 3) & ~3)

static int compareOffsets(const void * lhs, const void * rhs)
{
  const tableOrderRec * a = (const tableOrderRec *) lhs;
  const tableOrderRec * b = (const tableOrderRec *) rhs;
  /* don't simply return a->offset - b->offset because these are unsigned
     offset values; could convert to int, but possible integer overflow */
  return a->offset > b->offset ? 1 :
         a->offset < b->offset ? -1 :
         0;
}

static int sanityCheck(const char* woffData, int _woffLen)
{
  quint32 woffLen = _woffLen;
  const woffHeader * header;
  quint16 numTables, i;
  const woffDirEntry * dirEntry;
  quint32 tableTotal = 0;

  if (!woffData || !woffLen) {
    return eWOFF_bad_parameter;
  }

  if (woffLen < sizeof(woffHeader)) {
    return eWOFF_invalid;
  }

  header = (const woffHeader *) (woffData);
  if (READ32BE(header->signature) != WOFF_SIGNATURE) {
    return eWOFF_bad_signature;
  }

  if (READ32BE(header->length) != woffLen || header->reserved != 0) {
    return eWOFF_invalid;
  }

  numTables = READ16BE(header->numTables);
  if (woffLen < sizeof(woffHeader) + numTables * sizeof(woffDirEntry)) {
    return eWOFF_invalid;
  }

  dirEntry = (const woffDirEntry *) (woffData + sizeof(woffHeader));
  for (i = 0; i < numTables; ++i) {
    quint32 offs = READ32BE(dirEntry->offset);
    quint32 orig = READ32BE(dirEntry->origLen);
    quint32 comp = READ32BE(dirEntry->compLen);
    if (comp > orig || comp > woffLen || offs > woffLen - comp) {
      return eWOFF_invalid;
    }
    orig = (orig + 3) & ~3;
    if (tableTotal > 0xffffffffU - orig) {
      return eWOFF_invalid;
    }
    tableTotal += orig;
    ++dirEntry;
  }

  if (tableTotal > 0xffffffffU - sizeof(sfntHeader) -
                                 numTables * sizeof(sfntDirEntry) ||
      READ32BE(header->totalSfntSize) !=
        tableTotal + sizeof(sfntHeader) + numTables * sizeof(sfntDirEntry)) {
    return eWOFF_invalid;
  }

  return eWOFF_ok;
}

int WOFF::getDecodedSize(const char* woffData, int woffLen, int* pStatus)
{
  int status = eWOFF_ok;
  quint32 totalLen = 0;

  if (pStatus && WOFF_FAILURE(*pStatus)) {
    return 0;
  }

  status = sanityCheck(woffData, woffLen);
  if (WOFF_FAILURE(status)) {
    FAIL(status);
  }

  totalLen = READ32BE(((const woffHeader *) (woffData))->totalSfntSize);
  /* totalLen must be correctly rounded up to 4-byte alignment, otherwise
     sanityCheck would have failed */

failure:
  if (pStatus) {
    *pStatus = status;
  }
  return totalLen;
}

static void woffDecodeToBufferInternal(const char* woffData, char* sfntData, int* pActualSfntLen, int* pStatus)
{
  /* this is only called after sanityCheck has verified that
     (a) basic header fields are ok
     (b) all the WOFF table offset/length pairs are valid (within the data)
     (c) the sum of original sizes + header/directory matches totalSfntSize
     so we don't have to re-check those overflow conditions here */
  tableOrderRec * tableOrder = NULL;
  const woffHeader * header;
  quint16 numTables;
  quint16 tableIndex;
  quint16 order;
  const woffDirEntry * woffDir;
  quint32 totalLen;
  sfntHeader * newHeader;
  quint16 searchRange, rangeShift, entrySelector;
  quint32 offset;
  sfntDirEntry * sfntDir;
  quint32 headOffset = 0, headLength = 0;
  sfntHeadTable * head;
  quint32 csum = 0;
  const quint32 * csumPtr;
  quint32 oldCheckSumAdjustment;
  quint32 status = eWOFF_ok;

  if (pStatus && WOFF_FAILURE(*pStatus)) {
    return;
  }

  /* check basic header fields */
  header = (const woffHeader *) (woffData);
  if (READ32BE(header->flavor) != SFNT_VERSION_TT &&
      READ32BE(header->flavor) != SFNT_VERSION_CFF &&
      READ32BE(header->flavor) != SFNT_VERSION_true) {
    status |= eWOFF_warn_unknown_version;
  }

  numTables = READ16BE(header->numTables);
  woffDir = (const woffDirEntry *) (woffData + sizeof(woffHeader));

  totalLen = READ32BE(header->totalSfntSize);

  /* construct the sfnt header */
  newHeader = (sfntHeader *) (sfntData);
  newHeader->version = header->flavor;
  newHeader->numTables = READ16BE(numTables);
  
  /* calculate header fields for binary search */
  searchRange = numTables;
  searchRange |= (searchRange >> 1);
  searchRange |= (searchRange >> 2);
  searchRange |= (searchRange >> 4);
  searchRange |= (searchRange >> 8);
  searchRange &= ~(searchRange >> 1);
  searchRange *= 16;
  newHeader->searchRange = READ16BE(searchRange);
  rangeShift = numTables * 16 - searchRange;
  newHeader->rangeShift = READ16BE(rangeShift);
  entrySelector = 0;
  while (searchRange > 16) {
    ++entrySelector;
    searchRange >>= 1;
  }
  newHeader->entrySelector = READ16BE(entrySelector);

  tableOrder = (tableOrderRec *) malloc(numTables * sizeof(tableOrderRec));
  if (!tableOrder) {
    FAIL(eWOFF_out_of_memory);
  }
  for (tableIndex = 0; tableIndex < numTables; ++tableIndex) {
    tableOrder[tableIndex].offset = READ32BE(woffDir[tableIndex].offset);
    tableOrder[tableIndex].oldIndex = tableIndex;
  }
  qsort(tableOrder, numTables, sizeof(tableOrderRec), compareOffsets);

  /* process each table, filling in the sfnt directory */
  offset = sizeof(sfntHeader) + numTables * sizeof(sfntDirEntry);
  sfntDir = (sfntDirEntry *) (sfntData + sizeof(sfntHeader));
  for (order = 0; order < numTables; ++order) {
    quint32 origLen, compLen, tag, sourceOffset;
    tableIndex = tableOrder[order].oldIndex;

    /* validity of these was confirmed by sanityCheck */
    origLen = READ32BE(woffDir[tableIndex].origLen);
    compLen = READ32BE(woffDir[tableIndex].compLen);
    sourceOffset = READ32BE(woffDir[tableIndex].offset);

    sfntDir[tableIndex].tag = woffDir[tableIndex].tag;
    sfntDir[tableIndex].offset = READ32BE(offset);
    sfntDir[tableIndex].length = woffDir[tableIndex].origLen;
    sfntDir[tableIndex].checksum = woffDir[tableIndex].checksum;
    csum += READ32BE(sfntDir[tableIndex].checksum);

    if (compLen < origLen) {
      uLongf destLen = origLen;
      if (uncompress((Bytef *)(sfntData + offset), &destLen,
                     (const Bytef *)(woffData + sourceOffset),
                     compLen) != Z_OK || destLen != origLen) {
        FAIL(eWOFF_compression_failure);
      }
    } else {
      memcpy(sfntData + offset, woffData + sourceOffset, origLen);
    }

    /* note that old Mac bitmap-only fonts have no 'head' table
       (eg NISC18030.ttf) but a 'bhed' table instead */
    tag = READ32BE(sfntDir[tableIndex].tag);
    if (tag == TABLE_TAG_head || tag == TABLE_TAG_bhed) {
      headOffset = offset;
      headLength = origLen;
    }

    offset += origLen;

    while (offset < totalLen && (offset & 3) != 0) {
      sfntData[offset++] = 0;
    }
  }

  if (headOffset > 0) {
    /* the font checksum in the 'head' table depends on all the individual
       table checksums (collected above), plus the header and directory
       which are added in here */
    if (headLength < HEAD_TABLE_SIZE) {
      FAIL(eWOFF_invalid);
    }
    head = (sfntHeadTable *)(sfntData + headOffset);
    oldCheckSumAdjustment = READ32BE(head->checkSumAdjustment);
    head->checkSumAdjustment = 0;
    csumPtr = (const quint32 *)sfntData;
    while (csumPtr < (const quint32 *)(sfntData + sizeof(sfntHeader) +
                                        numTables * sizeof(sfntDirEntry))) {
      csum += READ32BE(*csumPtr);
      csumPtr++;
    }
    csum = SFNT_CHECKSUM_CALC_CONST - csum;

    if (oldCheckSumAdjustment != csum) {
      /* if the checksum doesn't match, we fix it; but this will invalidate
         any DSIG that may be present */
      status |= eWOFF_warn_checksum_mismatch;
    }
    head->checkSumAdjustment = READ32BE(csum);
  }

  if (pActualSfntLen) {
    *pActualSfntLen = totalLen;
  }
  if (pStatus) {
    *pStatus |= status;
  }
  free(tableOrder);
  return;

failure:
  if (tableOrder) {
    free(tableOrder);
  }
  if (pActualSfntLen) {
    *pActualSfntLen = 0;
  }
  if (pStatus) {
    *pStatus = status;
  }
}

void WOFF::decodeToBuffer(const char* woffData, int woffLen, char* sfntData, int bufferLen,
                   int* pActualSfntLen, int* pStatus)
{
  quint32 status = eWOFF_ok;
  quint32 totalLen;

  if (pStatus && WOFF_FAILURE(*pStatus)) {
    return;
  }

  status = sanityCheck(woffData, woffLen);
  if (WOFF_FAILURE(status)) {
    FAIL(status);
  }

  if (!sfntData) {
    FAIL(eWOFF_bad_parameter);
  }

  totalLen = READ32BE(((const woffHeader *) (woffData))->totalSfntSize);
  if ((unsigned)bufferLen < totalLen) {
    FAIL(eWOFF_buffer_too_small);
  }

  woffDecodeToBufferInternal(woffData, sfntData, pActualSfntLen, pStatus);
  return;

failure:
  if (pActualSfntLen) {
    *pActualSfntLen = 0;
  }
  if (pStatus) {
    *pStatus = status;
  }
}
