#include "viewer.h"
#include "helpdialog.h"

#include <Standard.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>

#include <QMouseEvent>
#include <QResizeEvent>
#include <QTimer>

#include <Standard_Version.hxx>
#if OCC_VERSION_HEX >= 0x060503
#define WITH_DRIVER 1
#define DEGENERATE_MODE 0
#else
#define WITH_DRIVER 0
#define DEGENERATE_MODE 1
#endif

#if WITH_DRIVER
#include <Aspect_DisplayConnection.hxx>
#include <Graphic3d_ExportFormat.hxx>
#include <Graphic3d_GraphicDriver.hxx>
#include <Graphic3d_TextureEnv.hxx>
#include <OpenGl_GraphicDriver.hxx>
#else
#include <Graphic3d_GraphicDevice.hxx>
#endif

#include <Xw_Window.hxx>

Viewer::Viewer(Handle(AIS_InteractiveContext) v, QWidget* parent) :
    QWidget(parent)
{
    context = v;
    if (context->CurrentViewer().IsNull()) {
        qFatal("Context provided does not have a viewer");
    }

    this->setMinimumSize(150, 150);
    this->setFocusPolicy(Qt::NoFocus);// or Qt::StrongFocus
    this->setMouseTracking(true);
    // Render to widget outside of Qt
    this->setAttribute(Qt::WA_PaintOnScreen);
    // Don't redraw widget background
    this->setAttribute(Qt::WA_NoSystemBackground);

    view = Handle(V3d_View)();

    buttonMode = NULL;
    scrollMode = NULL;
    lastEvt = NULL;
    readyForInteraction = false;

    QTimer::singleShot(50, this, SLOT(init()));
}

Viewer::~Viewer()
{
    rubberBand->hide();
    delete rubberBand;
    rubberBand = 0;
}

V3d_Viewer* Viewer::makeViewer()
{
#if WITH_DRIVER
    static Handle(Graphic3d_GraphicDriver) graphics;
    static Handle(Aspect_DisplayConnection) connection;
    if (graphics.IsNull()) {
        // Default constructor for Aspect_DisplayConnection
        // uses DISPLAY env. variable
        connection = new Aspect_DisplayConnection();
        graphics = new OpenGl_GraphicDriver(connection);
    }
#else
    static Handle(Graphic3d_GraphicDevice) graphics;
    if (graphics.IsNull()) {
        graphics = new Graphic3d_GraphicDevice(getenv("DISPLAY"));
    }
#endif

    V3d_Viewer* viewer = new V3d_Viewer(graphics);
#if !WITH_DRIVER
    viewer->Init();
#endif
    return viewer;
}

void Viewer::init()
{
    Handle(V3d_Viewer) viewer = context->CurrentViewer();
    view = viewer->CreateView();
#if WITH_DRIVER
    Handle(Aspect_DisplayConnection) dispCon =
        viewer->Driver()->GetDisplayConnection();
    Handle(Xw_Window) window = new Xw_Window(dispCon, (Window)winId());
#else
    short lo = (short) winId();
    short hi = (short)(winId() >> 16);
    Handle(Graphic3d_GraphicDevice) dev = Handle(Graphic3d_GraphicDevice)::DownCast(
            viewer->Device());
    Handle(Xw_Window) window = new Xw_Window(dev, (int) hi, (int) lo,
            Xw_WQ_SAMEQUALITY);
#endif
    view->SetWindow(window);
    if (! window->IsMapped()) {
        window->Map();
    }
    view->SetBgGradientColors(Quantity_NOC_ALICEBLUE, Quantity_NOC_BLUEVIOLET,
                              Aspect_GFM_VER, false);
    view->SetLightOn();
    view->TriedronDisplay(Aspect_TOTP_LEFT_LOWER, Quantity_NOC_BLACK, 0.1,
                          V3d_WIREFRAME);
    view->MustBeResized();

    readyForInteraction = true;
    emit readyToUse();
    rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
    rubberBand->hide();

    mustResize = false;
}

void Viewer::setOrientation(V3d_TypeOfOrientation orientation)
{
    view->SetProj(orientation);
}

void Viewer::resetView()
{
    setOrientation(V3d_XposYnegZpos);
    view->ZFitAll(1.5);
#if OCC_VERSION_HEX > 0x060701
    view->FitAll(0.01, Standard_False);
#else
    view->FitAll(0.01, Standard_False, Standard_False);
#endif
}

void Viewer::paintEvent(QPaintEvent*)
{
    if (!readyForInteraction) {
        return;
    }

    if (mustResize) {
        view->MustBeResized();
        mustResize = false;
    } else {
        view->Redraw();
    }
}
void Viewer::resizeEvent(QResizeEvent*)
{
    mustResize = true;
}

void Viewer::mousePressEvent(QMouseEvent* evt)
{
    if (!readyForInteraction) {
        return;
    }

    lastEvt = evt;
    ViewActionData data = getViewActionData(evt);
    if (buttonMode) {
        buttonMode->release(data);
    }
    buttonMode = getButtonAction(evt->button(), evt->modifiers());
    setCursor(buttonMode->getCursor());
    buttonMode->click(data);
    emit selectionMightBeChanged();
}

void Viewer::mouseReleaseEvent(QMouseEvent* evt)
{
    if (!readyForInteraction) {
        return;
    }

    lastEvt = evt;
    if (buttonMode) {
        buttonMode->release(getViewActionData(evt));
        emit selectionMightBeChanged();
    }
    buttonMode = getButtonAction(Qt::NoButton, evt->modifiers());
    setCursor(buttonMode->getCursor());
    buttonMode->click(getViewActionData(evt));
}
void Viewer::mouseMoveEvent(QMouseEvent* evt)
{
    if (!readyForInteraction) {
        return;
    }

    lastEvt = evt;
    if (buttonMode) {
        buttonMode->drag(getViewActionData(evt));
        emit selectionMightBeChanged();
    } else {
        buttonMode = getButtonAction(Qt::NoButton, evt->modifiers());
        setCursor(buttonMode->getCursor());
        buttonMode->click(getViewActionData(evt));
    }
}
void Viewer::mouseDoubleClickEvent(QMouseEvent*)
{
}
void Viewer::wheelEvent(QWheelEvent* evt)
{
    if (!readyForInteraction) {
        return;
    }

    scrollMode = getScrollAction(evt->modifiers());
    scrollMode->scroll(&(*view), evt->delta());
}

ViewActionData Viewer::getViewActionData(QMouseEvent* evt)
{
    ViewActionData d;
    d.context = &(*context);
    d.view = &(*view);
    d.x = evt->x();
    d.y = evt->y();
    d.band = rubberBand;
    d.globalX = evt->globalX();
    d.globalY = evt->globalY();
    d.viewer = this;
    this->view->Window()->Size(d.sizeX, d.sizeY);
    return d;
}

void Viewer::startHover()
{
    if (!readyForInteraction) {
        return;
    }

    if (!lastEvt || !buttonMode) {
        qFatal("Must have a prior click/buttonMode before startHover is called.");
        return;
    }
    buttonMode->release(getViewActionData(lastEvt));
    buttonMode = getButtonAction(Qt::NoButton, lastEvt->modifiers());
    setCursor(buttonMode->getCursor());
    buttonMode->click(getViewActionData(lastEvt));
}
