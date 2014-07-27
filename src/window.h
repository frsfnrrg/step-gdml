#ifndef WINDOW_H
#define WINDOW_H

#include <QtGui>
#include "util.h"

class AIS_InteractiveContext;
class View;
class Translate;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);

signals:

public slots:
    void import_file(QString);
    void export_file(QString);

private:
    void createMenus();
    void setShadingMode();

    View* view;
    AIS_InteractiveContext* context;
    Translate* translate;
};

#endif // WINDOW_H
