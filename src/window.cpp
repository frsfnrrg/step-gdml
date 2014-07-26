#include "window.h"
#include "util.h"
#include <cstdio>
#include <Graphic3d_GraphicDevice.hxx>

Handle(V3d_Viewer) make_viewer(const Standard_CString aDisplay,
                               const Standard_ExtString aName,
                               const Standard_CString aDomain,
                               const Standard_Real ViewSize,
                               const V3d_TypeOfOrientation ViewProj,
                               const Standard_Boolean ComputedMode,
                               const Standard_Boolean aDefaultComputedMode) {
    static Handle(Graphic3d_GraphicDevice) defaultdevice;
    if( defaultdevice.IsNull() )
        defaultdevice = new Graphic3d_GraphicDevice( aDisplay );
    return new V3d_Viewer(defaultdevice,aName,aDomain,ViewSize,ViewProj,
                          Quantity_NOC_GRAY30,V3d_ZBUFFER,V3d_GOURAUD,V3d_WAIT,
                          ComputedMode,aDefaultComputedMode,V3d_TEX_NONE);

}

Window::Window(QWidget *parent) :
    QMainWindow(parent)
{
    setWindowTitle("STEP to GDML");
    createMenus();

    translate = new Translate(this);

    TCollection_ExtendedString a3DName("Visu3D");
    Handle(V3d_Viewer) myViewer = make_viewer( getenv("DISPLAY"), a3DName.ToExtString(), "", 1000.0,
                                               V3d_XposYnegZpos, Standard_True, Standard_True );

    myViewer->Init();
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

void Window::import_file(QString s) {
    printf("Importing file %s\n", s.toLocal8Bit().data());
    bool success = translate->importModel(Translate::FormatSTEP, context);
    printf("Success %c\n", success ? 'Y' : 'N');

    setShadingMode();

    view->fitAll();
}

void Window::setShadingMode() {
    for( context->InitCurrent(); context->MoreCurrent(); context->NextCurrent() ) {
        context->SetDisplayMode( context->Current(), 1, false );
    }
    context->UpdateCurrentViewer();
}

void Window::export_file(QString s) {
    printf("Exporting file %s\n", s.toLocal8Bit().data());
    bool success = translate->exportModel(Translate::FormatGDML, s, translate->getShapes(context));
    printf("Success %c\n", success ? 'Y' : 'N');
}

void Window::createMenus() {
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
