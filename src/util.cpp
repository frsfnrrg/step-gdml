#include "util.h"
#include "QtGui"

QAction* mkAction(QObject* parent, const char* text, const char* shortcut) {
    QAction* k = new QAction(text, parent);
    k->setShortcut(QKeySequence(shortcut));
    return k;
}
