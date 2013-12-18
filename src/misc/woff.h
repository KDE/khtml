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

#ifndef WOFF_H_
#define WOFF_H_

/* API for the decoding of WOFF (Web Open Font Format) font files */

typedef enum {
  /* Success */
  eWOFF_ok = 0,

  /* Errors: no valid result returned */
  eWOFF_out_of_memory = 1,       /* malloc or realloc failed */
  eWOFF_invalid = 2,             /* invalid input file (e.g., bad offset) */
  eWOFF_compression_failure = 3, /* error in zlib call */
  eWOFF_bad_signature = 4,       /* unrecognized file signature */
  eWOFF_buffer_too_small = 5,    /* the provided buffer is too small */
  eWOFF_bad_parameter = 6,       /* bad parameter (e.g., null source ptr) */
  eWOFF_illegal_order = 7,       /* improperly ordered chunks in WOFF font */

  /* Warnings: call succeeded but something odd was noticed.
     Multiple warnings may be OR'd together. */
  eWOFF_warn_unknown_version = 0x0100,   /* unrecognized version of sfnt,
                                            not standard TrueType or CFF */
  eWOFF_warn_checksum_mismatch = 0x0200, /* bad checksum, use with caution;
                                            any DSIG will be invalid */
  eWOFF_warn_misaligned_table = 0x0400,  /* table not long-aligned; fixing,
                                            but DSIG will be invalid */
  eWOFF_warn_trailing_data = 0x0800,     /* trailing junk discarded,
                                            any DSIG may be invalid */
  eWOFF_warn_unpadded_table = 0x1000,    /* sfnt not correctly padded,
                                            any DSIG may be invalid */
  eWOFF_warn_removed_DSIG = 0x2000       /* removed digital signature
                                            while fixing checksum errors */
} WOFFStatus;

/* Note: status parameters must be initialized to eWOFF_ok before calling
   WOFF functions. If the status parameter contains an error code,
   functions will return immediately. */

#define WOFF_SUCCESS(status) (((status) & 0xff) == eWOFF_ok)
#define WOFF_FAILURE(status) (!WOFF_SUCCESS(status))
#define WOFF_WARNING(status) ((status) & ~0xff)

namespace WOFF {

/*****************************************************************************
 * Returns the size of buffer needed to decode the font (or zero on error).
 */
int getDecodedSize(const char* woffData, int woffLen, int* pStatus);

/*****************************************************************************
 * Decodes WOFF font to a caller-supplied buffer of size bufferLen.
 * Returns the actual size of the decoded sfnt data in pActualSfntLen
 * (must be <= bufferLen, otherwise an error will be returned).
 */
void decodeToBuffer(const char* woffData, int woffLen, char * sfntData, int bufferLen,
                        int* pActualSfntLen, int* pStatus);

}

#endif
