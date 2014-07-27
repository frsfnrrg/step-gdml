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

#if WITH_DRIVER
#include <OpenGl_GraphicDriver.hxx>
#include <Aspect_DisplayConnection.hxx>
#else
#include <Graphic3d_GraphicDevice.hxx>
#endif

Handle(V3d_Viewer) make_viewer(const Standard_CString aDisplay,
                               const Standard_ExtString aName,
                               const Standard_CString aDomain,
                               const Standard_Real ViewSize,
                               const V3d_TypeOfOrientation ViewProj,
                               const Standard_Boolean ComputedMode,
                               const Standard_Boolean aDefaultComputedMode) {
#if WITH_DRIVER
    static Handle(Graphic3d_GraphicDriver) defaultdevice;
    static Handle(Aspect_DisplayConnection) connection;
    if( defaultdevice.IsNull() ) {
        connection = new Aspect_DisplayConnection(aDisplay);
        defaultdevice = new OpenGl_GraphicDriver(connection);
    }
#else
    static Handle(Graphic3d_GraphicDevice) defaultdevice;
    if( defaultdevice.IsNull() )
        defaultdevice = new Graphic3d_GraphicDevice( aDisplay );
#endif
    return new V3d_Viewer(defaultdevice,aName,aDomain,ViewSize,ViewProj,
                          Quantity_NOC_GRAY30,V3d_ZBUFFER,V3d_GOURAUD,V3d_WAIT,
                          ComputedMode,aDefaultComputedMode,V3d_TEX_NONE);

}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    setWindowTitle("STEP to GDML");
    createMenus();

    translate = new Translate(this);

    TCollection_ExtendedString a3DName("Visu3D");
    Handle(V3d_Viewer) myViewer = make_viewer( getenv("DISPLAY"), a3DName.ToExtString(), "", 1000.0,
                                               V3d_XposYnegZpos, Standard_True, Standard_True );

#if OCC_VERSION_HEX < 0x060503
    myViewer->Init();
#endif
    myViewer->SetDefaultLights();
    myViewer->SetLightOn();

    context = new AIS_InteractiveContext(myViewer);
    view = new View(context, this);

    QHBoxLayout* layout = new QHBoxLayout();
    layout->addWidget(new QLabel("WEE"), 1);
    layout->addWidget(view, 3);
    QWidget* e = new QWidget();
    e->setLayout(layout);
    this->setCentralWidget(e);
}

void MainWindow::import_file(QString s) {
    printf("Importing file %s\n", s.toLocal8Bit().data());
    bool success = translate->importModel(Translate::FormatSTEP, context);
    printf("Success %c\n", success ? 'Y' : 'N');

    setShadingMode();

    view->fitAll();
}

void MainWindow::setShadingMode() {
    for( context->InitCurrent(); context->MoreCurrent(); context->NextCurrent() ) {
        context->SetDisplayMode( context->Current(), 1, false );
    }
    context->UpdateCurrentViewer();
}

void MainWindow::export_file(QString s) {
    printf("Exporting file %s\n", s.toLocal8Bit().data());
    bool success = translate->exportModel(Translate::FormatGDML, s, translate->getShapes(context));
    printf("Success %c\n", success ? 'Y' : 'N');
}

void MainWindow::createMenus() {
    QAction* quit = mkAction(this, "Quit", "Ctrl+Q");
    connect(quit, SIGNAL(triggered()), this, SLOT(close()));

    QSignalMapper* loadmap = new QSignalMapper(this);
    QAction* load = mkAction(this, "Load STEP file...", "Ctrl+O");
    connect(load, SIGNAL(triggered()), loadmap, SLOT(map()));
    loadmap->setMapping(load, QString("wing.stp"));
    connect(loadmap, SIGNAL(mapped(QString)), this, SLOT(import_file(QString)));

    QSignalMapper* expomap = new QSignalMapper(this);
    QAction* expo = mkAction(this, "Export GDML file", "Ctrl+E");
    connect(expo, SIGNAL(triggered()), expomap, SLOT(map()));
    expomap->setMapping(expo, QString("output.gdml"));
    connect(expomap, SIGNAL(mapped(QString)), this, SLOT(export_file(QString)));

    QMenu* fileMenu = new QMenu("File", this);
    fileMenu->addAction(load);
    fileMenu->addAction(expo);
    fileMenu->addSeparator();
    fileMenu->addAction(quit);

    this->menuBar()->addMenu(fileMenu);
}
