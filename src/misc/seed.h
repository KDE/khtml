/*
 * A simple hash seed chooser.
 *
 * Copyright (C) 2003 Germain Garand <germain@ebooksfrance.org>
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

#ifndef html_seed_h
#define html_seed_h

#define khtml_MaxSeed 47963

namespace khtml {

static const int primes_t[] =
{
    31,    61,   107,   233,   353,   541,
   821,  1237,  1861,  2797,  4201,  6311,
  9467, 14207, 21313, 31973, 47963,  0
};

static inline int nextSeed(int curSize) {
    for (int i = 0 ; primes_t[i] ; i++)
        if (primes_t[i] > curSize)
            return primes_t[i];
    return curSize;
}

} // namespace

#endif
