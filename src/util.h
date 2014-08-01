#ifndef UTIL_H_
#define UTIL_H_

#include <QtGui>
#include <QtCore>

QAction* mkAction(QObject* parent, const char* text, const char* shortcut, const char* slot);

#endif
