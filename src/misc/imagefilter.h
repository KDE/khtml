/*
    This file is a part of the KDE project

    Copyright © 2006 Zack Rusin <zack@kde.org>
    Copyright © 2006-2007, 2008 Fredrik Höglund <fredrik@kde.org>

    The stack blur algorithm was invented by Mario Klingemann <mario@quasimondo.com>

    This implementation is based on the version in Anti-Grain Geometry Version 2.4,
    Copyright © 2002-2005 Maxim Shemanarev (http://www.antigrain.com)

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

namespace khtml {


class ImageFilter
{
public:
    // Blurs the alpha channel of the image and recolors it to the specified color.
    // The image must have transparent padding on all sides, or the shadow will be clipped.
    static void shadowBlur(QImage &image, float radius, const QColor &color);
};

}

