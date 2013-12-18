/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2013 Bernd Buschinski <b.buschinski@googlemail.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "ecma/kjs_arraybufferview.h"

namespace KJS {

/* Source for ArrayBufferViewProtoTable.

@begin ArrayBufferViewProtoTable 2
    subarray  ArrayBufferViewBase::Subarray     Function|DontDelete     1
    set       ArrayBufferViewBase::Set          Function|DontDelete     1
@end

*/

/* Source for ArrayBufferViewConstTable.

@begin ArrayBufferViewConstTable 1
    BYTES_PER_ELEMENT      ArrayBufferViewBase::BytesPerElement     ReadOnly|DontDelete
@end

*/

/* Source for ArrayBufferViewTable.

@begin ArrayBufferViewTable 4
    byteLength      ArrayBufferViewBase::ByteLength     ReadOnly|DontDelete
    byteOffset      ArrayBufferViewBase::ByteOffset     ReadOnly|DontDelete
    buffer          ArrayBufferViewBase::Buffer         ReadOnly|DontDelete
    length          ArrayBufferViewBase::Length         ReadOnly|DontDelete
@end

*/

}
