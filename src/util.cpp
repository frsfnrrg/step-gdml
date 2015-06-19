#include "util.h"

QAction* mkAction(QObject* parent, const char* text, const char* shortcut,
                  const char* slot)
{
    QAction* k = new QAction(text, parent);
    k->setShortcut(QKeySequence(shortcut));
    parent->connect(k, SIGNAL(triggered()), parent, slot);
    return k;
}
