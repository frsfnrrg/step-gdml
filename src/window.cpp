#include <Standard_Version.hxx>
#if OCC_VERSION_HEX >= 0x060503
#define WITH_DRIVER 1
#else
#define WITH_DRIVER 0
#endif

#include "window.h"
#include "view.h"
#include "translate.h"
#include "util.h"
#include <cstdio>
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    setWindowTitle("STEP to GDML");
    createMenus();

    Handle(V3d_Viewer) myViewer = View::makeViewer();

    myViewer->SetDefaultLights();
    myViewer->SetLightOn();

    context = new AIS_InteractiveContext(myViewer);
    view = new View(context, this);
    translate = new Translator(context);


    QHBoxLayout* layout = new QHBoxLayout();
    layout->addWidget(new QLabel("WEE"), 1);
    layout->addWidget(view, 3);
    QWidget* e = new QWidget();
    e->setLayout(layout);
    this->setCentralWidget(e);

    stepdialog = NULL;
    gdmldialog = NULL;
}

void MainWindow::importSTEP(QString path) {
    printf("Importing file %s\n", path.toLocal8Bit().data());
    bool success = translate->importSTEP(path);
    printf("Success %c\n", success ? 'Y' : 'N');
    view->fitAll();
}


void MainWindow::exportGDML(QString path) {
    printf("Exporting file %s\n", path.toLocal8Bit().data());
    bool success = translate->exportGDML(path);
    printf("Success %c\n", success ? 'Y' : 'N');
}

void MainWindow::raiseSTEP() {
    if (!stepdialog) {
        stepdialog = new IODialog(this, QFileDialog::AcceptOpen, QStringList("Step Files (*.stp *.step)"), "stp");
        stepdialog->hook(this, SLOT(importSTEP(QString)));
    }
    stepdialog->display();
}

void MainWindow::raiseGDML() {
    if (!gdmldialog) {
        gdmldialog = new IODialog(this, QFileDialog::AcceptSave, QStringList("GDML Files (*.gdml)"), "gdml");
        gdmldialog->hook(this, SLOT(exportGDML(QString)));
    }
    gdmldialog->display();
}

void MainWindow::createMenus() {
    QAction* quit = mkAction(this, "Quit", "Ctrl+Q", SLOT(close()));

    QAction* load = mkAction(this, "Load STEP file...", "Ctrl+O", SLOT(raiseSTEP()));

    QAction* expo = mkAction(this, "Export GDML file", "Ctrl+E", SLOT(raiseGDML()));

    QMenu* fileMenu = new QMenu("File", this);
    fileMenu->addAction(load);
    fileMenu->addAction(expo);
    fileMenu->addSeparator();
    fileMenu->addAction(quit);

    this->menuBar()->addMenu(fileMenu);
}
