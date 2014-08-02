#ifndef WINDOW_H
#define WINDOW_H

#include <QtGui>
#include <QtCore>
#include "util.h"
#include "metadata.h"

class AIS_InteractiveContext;
class AIS_InteractiveObject;
class View;
class Translator;
class IODialog;


class ObjectListView : public QListView {
public:
    ObjectListView();
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QString openFile);

signals:
    void enableObjectEditor(bool enabled);

public slots:
    void importSTEP(QString);
    void exportGDML(QString);
    void raiseSTEP();
    void raiseGDML();

private slots:
    void onViewSelectionChanged();
    void onListSelectionChanged();

    void changeCurrentObject(int);
    void currentObjectUpdated();

    void getColor();
private:



    void createInterface();
    void createMenus();
    SolidMetadata& currentMetadata();

    View* view;
    AIS_InteractiveContext* context;
    Translator* translate;
    IODialog* gdmldialog;
    IODialog* stepdialog;

    QListWidget* namesList;
    QLineEdit* objName;
    QComboBox* objMaterial;
    QSlider* objTransparency;
    QPushButton* objColor;

    QVector<SolidMetadata> metadata;
    QMap<QListWidgetItem*,int> itemsToIndices;
    QMap<AIS_InteractiveObject*,int> objectsToIndices;
};


#endif // WINDOW_H
