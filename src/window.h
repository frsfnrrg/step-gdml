#ifndef WINDOW_H
#define WINDOW_H

#include <QtGui>
#include <QtCore>
#include "util.h"

class AIS_InteractiveContext;
class AIS_InteractiveObject;
class View;
class Translator;
class IODialog;



class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QString openFile);

signals:

public slots:
    void importSTEP(QString);
    void exportGDML(QString);
    void raiseSTEP();
    void raiseGDML();

private slots:
    void onViewSelectionChanged();
    void onListSelectionChanged();

private:
    void createMenus();

    typedef struct {
        AIS_InteractiveObject* object;
        QListWidgetItem* item;
        QString name;
        QString material;
    } SolidMetadata;

    View* view;
    AIS_InteractiveContext* context;
    Translator* translate;
    IODialog* gdmldialog;
    IODialog* stepdialog;
    QListWidget* namesList;

    QList<SolidMetadata> metadata;
    QMap<QListWidgetItem*,int> itemsToIndices;
    QMap<AIS_InteractiveObject*,int> objectsToIndices;
};


#endif // WINDOW_H
