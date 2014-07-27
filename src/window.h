#ifndef WINDOW_H
#define WINDOW_H

#include <QtGui>
#include "util.h"

class AIS_InteractiveContext;
class View;
class Translator;
class IODialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);

signals:

public slots:
    void importSTEP(QString);
    void exportGDML(QString);
    void raiseSTEP();
    void raiseGDML();

private:
    void createMenus();

    View* view;
    AIS_InteractiveContext* context;
    Translator* translate;
    IODialog* stepdialog;
    IODialog* gdmldialog;
};


#endif // WINDOW_H
