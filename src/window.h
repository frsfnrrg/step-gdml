#ifndef WINDOW_H
#define WINDOW_H

#include "metadata.h"

#include <QSplitter>
#include <QListWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSlider>
#include <QPushButton>
#include <QMainWindow>
#include <QSettings>

class AIS_InteractiveContext;
class AIS_InteractiveObject;
class Viewer;
class Translator;
class HelpDialog;

class GDMLNameValidator : public QValidator
{
    Q_OBJECT
public:
    GDMLNameValidator(QObject* parent, const QSet<QString>& enames);
    virtual State validate(QString& text, int&) const;
    virtual void fixup(QString& text) const;
signals:
    void textAcceptable();
    void textIntermediate();
private:
    const QSet<QString>& names;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QString openFile);
    virtual void closeEvent(QCloseEvent* event);

signals:
    void enableObjectEditor(bool enabled);

public slots:
    void importSTEP(QString);
    void exportGDML(QString);

    void raiseSTEP();
    void raiseGDML();
    void raiseHelp();

private slots:
    void onViewSelectionChanged();
    void onListSelectionChanged();

    void changeCurrentObject(int);
    void currentObjectUpdated();

    void getColor();
private:
    void loadSettings();
    void createInterface();
    void createMenus();
    SolidMetadata& currentMetadata();

    Viewer* view;
    AIS_InteractiveContext* context;
    Translator* translate;
    HelpDialog* helpdialog;

    GDMLNameValidator* validator;
    QSplitter* splitter;
    QListWidget* namesList;
    QLineEdit* objName;
    QComboBox* objMaterial;
    QSlider* objTransparency;
    QPushButton* objColor;

    QVector<SolidMetadata> metadata;
    QMap<QListWidgetItem*, int> itemsToIndices;
    QMap<AIS_InteractiveObject*, int> objectsToIndices;
    QSet<QString> names;
    int current_object;
};


#endif // WINDOW_H
