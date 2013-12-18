#ifndef BREAK_LINES_H
#define BREAK_LINES_H

#include <QtCore/QString>

namespace khtml {
    bool isBreakableThai( const QChar *string, const int pos, const int len);
    void cleanup_thaibreaks();

    bool isBreakable( const QChar *str, const int pos, int len );
}

#endif
