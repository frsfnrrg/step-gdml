#ifndef UTIL_H_
#define UTIL_H_

#include <QAction>

QAction* mkAction(QObject* parent, const char* text, const char* shortcut,
                  const char* slot);

#endif
