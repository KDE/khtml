/*
 * This file is part of the KDE libraries
 *
 * Copyright 2000-2003 Shiro Kawai <shiro@acm.org>, All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  3. Neither the name of the authors nor the names of its contributors
 *     may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/*
 * original code is here.
 * http://cvs.sourceforge.net/viewcvs.py/gauche/Gauche/ext/charconv/guess.c?view=markup
 */
#ifndef GUESS_JA_H
#define GUESS_JA_H

#include <qglobal.h>
#ifdef Q_OS_WIN
#undef UNICODE
#endif 
#ifdef SOLARIS
#undef UNICODE
#endif 
namespace khtml {
    class guess_arc {
    public:
        unsigned int next;          /* next state */
        double score;               /* score */
    };
}

using namespace khtml;

typedef signed char dfa_table[256];

/* DFA tables declared in guess_ja.cpp */
extern const dfa_table guess_eucj_st[];
extern guess_arc guess_eucj_ar[7];
extern const dfa_table guess_sjis_st[];
extern guess_arc guess_sjis_ar[6];
extern const dfa_table guess_utf8_st[];
extern guess_arc guess_utf8_ar[11];

namespace khtml {

    class guess_dfa {
    public:
        const dfa_table *states;
        const guess_arc *arcs;
        int state;
        double score;

        guess_dfa (const dfa_table stable[], const guess_arc *atable) :
            states(stable), arcs(atable)
        {
            state = 0;
            score = 1.0;
        }
    };

    class JapaneseCode
    {
    public:
        enum Type {ASCII, JIS, EUC, SJIS, UNICODE, UTF8 };
        enum Type guess_jp(const char* buf, int buflen);

        JapaneseCode () {
            eucj = new guess_dfa(guess_eucj_st, guess_eucj_ar);
            sjis = new guess_dfa(guess_sjis_st, guess_sjis_ar);
            utf8 = new guess_dfa(guess_utf8_st, guess_utf8_ar);
            last_JIS_escape = false;
        }

        ~JapaneseCode () {
            delete eucj;
            delete sjis;
            delete utf8;
        }

    protected:
        guess_dfa *eucj;
        guess_dfa *sjis;
        guess_dfa *utf8;

        bool last_JIS_escape;
    };
}

#define DFA_NEXT(dfa, ch)                               \
    do {                                                \
        int arc__;                                      \
        if (dfa->state >= 0) {                          \
            arc__ = dfa->states[dfa->state][ch];        \
            if (arc__ < 0) {                            \
                dfa->state = -1;                        \
            } else {                                    \
                dfa->state = dfa->arcs[arc__].next;     \
                dfa->score *= dfa->arcs[arc__].score;   \
            }                                           \
        }                                               \
    } while (0)

#define DFA_ALIVE(dfa)  (dfa->state >= 0)

#endif  /* GUESS_JA_H */
