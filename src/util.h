#ifndef UTIL_H_
#define UTIL_H_

#include <QtGui>
#include <QtCore>

QAction* mkAction(QObject* parent, const char* text, const char* shortcut, const char* slot);

//class QDialogHandle : QObject {
//public:
//    QDialogHandle(QObject* parent, QString title, QString filters);
//signals:
//    void gotAFile(QString);
//public slots:
//    void liftit();
//private:
//    QDialog * dialog;
//};

#endif
