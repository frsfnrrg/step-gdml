#include "helpdialog.h"
#include <QtGui>
#include <V3d_Viewer.hxx>
#include "viewer.h"

MouseButtonMode::MouseButtonMode() {}
MouseScrollMode::MouseScrollMode() {}

OrientationMapper::OrientationMapper(QObject* source, QObject* target,
                                     V3d_TypeOfOrientation o) : QObject(source)
{
    orientation = o;
    connect(source, SIGNAL(triggered()), SLOT(recieve()));
    connect(this, SIGNAL(send(V3d_TypeOfOrientation)), target,
            SLOT(setOrientation(V3d_TypeOfOrientation)));
}

void OrientationMapper::recieve()
{
    emit send(orientation);
}

class ClsClkNone : public MouseButtonMode
{
public:
    virtual QString getName()
    {
        return QString("ClkNone");
    }
    virtual Qt::CursorShape getCursor()
    {
        return Qt::ForbiddenCursor;
    }
} ClkNone;


class ClsClkHover : public MouseButtonMode
{
    virtual QString getName()
    {
        return QString("ClkHover");
    }
    virtual Qt::CursorShape getCursor()
    {
        return Qt::PointingHandCursor;
    }
    virtual void drag(const ViewActionData& data)
    {
        data.context->MoveTo(data.x, data.y, data.view);
    }
} ClkHover;

class BaseClkRubberBandType : public MouseButtonMode
{
public:
    virtual Qt::CursorShape getCursor()
    {
        return Qt::CrossCursor;
    }
    virtual void click(const ViewActionData& data)
    {
        x1 = data.x, y1 = data.y;
        data.band->setGeometry(x1, y1, 0, 0);
        data.band->show();
    }
    virtual void drag(const ViewActionData& data)
    {
        data.band->setGeometry(QRect(x1, y1, data.x - x1, data.y - y1).normalized());
    }
    virtual void release(const ViewActionData& data)
    {
        data.band->hide();
    }
private:
    int x1, y1;
};

class ClsClkSelect : public BaseClkRubberBandType
{
public:
    virtual QString getName()
    {
        return QString("ClkSelect");
    }
    virtual void release(const ViewActionData& data)
    {
        BaseClkRubberBandType::release(data);
        const QRect& r = data.band->geometry();
        if (r.isEmpty()) {
            data.context->Select();
        } else {
            int x1, y1, x2, y2;
            r.getCoords(&x1, &y1, &x2, &y2);
            data.context->Select(x1, y1, x2, y2, data.view);
        }
    }
} ClkSelect;

class ClsClkSelectToggle : public BaseClkRubberBandType
{
public:
    virtual QString getName()
    {
        return QString("ClkSelectToggle");
    }
    virtual void release(const ViewActionData& data)
    {
        BaseClkRubberBandType::release(data);
        const QRect& r = data.band->geometry();
        if (r.isEmpty()) {
            data.context->ShiftSelect();
        } else {
            QPoint tl = r.topLeft();
            QPoint br = r.bottomRight();
            data.context->ShiftSelect(tl.x(), tl.y(), br.x(), br.y(), data.view);
        }
    }
} ClkSelectToggle;

class ClsClkRotate : public MouseButtonMode
{
public:
    virtual QString getName()
    {
        return QString("ClkRotate");
    }
    virtual Qt::CursorShape getCursor()
    {
        return Qt::ClosedHandCursor;
    }
    virtual void click(const ViewActionData& data)
    {
        data.view->StartRotation(data.x, data.y);
    }
    virtual void drag(const ViewActionData& data)
    {
        data.view->Rotation(data.x, data.y);
        data.view->Redraw();
    }
} ClkRotate;

class ClsClkPan : public MouseButtonMode
{
public:
    virtual QString getName()
    {
        return QString("ClkPan");
    }
    virtual Qt::CursorShape getCursor()
    {
        return Qt::OpenHandCursor;
    }
    virtual void click(const ViewActionData& data)
    {
        x1 = data.x, y1 = data.y;
    }
    virtual void drag(const ViewActionData& data)
    {
        data.view->Pan(data.x - x1, y1 - data.y);
        x1 = data.x, y1 = data.y;
    }
private:
    int x1, y1;
} ClkPan;

class ClsClkPopup: public MouseButtonMode
{
public:
    virtual QString getName()
    {
        return QString("ClkPopup");
    }
    virtual Qt::CursorShape getCursor()
    {
        return Qt::ArrowCursor;
    }
    virtual void click(const ViewActionData& s)
    {
        menu = new QMenu();
        menu->addAction("Reset View", s.viewer, SLOT(resetView()));
        menu->addSeparator();
        addProjectionAction(menu, s.viewer, "Right", V3d_Xpos);

        s.viewer->connect(menu, SIGNAL(aboutToHide()), SLOT(startHover()));
        menu->connect(menu, SIGNAL(aboutToHide()), SLOT(deleteLater()));
        menu->exec(QPoint(s.globalX, s.globalY));
    }
    static void addProjectionAction(QMenu* m, Viewer* v, QString text,
                                    V3d_TypeOfOrientation orientation)
    {
        QAction* action = new QAction(text, m);
        new OrientationMapper(action, v, orientation);
        m->addAction(action);
    }
private:
    QMenu* menu;
} ClkPopup;

class ClsClkZoomInBox : public BaseClkRubberBandType
{
public:
    virtual QString getName()
    {
        return QString("ClkZoomInBox");
    }
    virtual void release(const ViewActionData& data)
    {
        BaseClkRubberBandType::release(data);
        const QRect& r = data.band->geometry();
        if (r.isEmpty()) {
            qDebug("Zoom to point: (%d, %d) ignoring.", r.x(), r.y());
        } else {
            int x1, y1, x2, y2;
            r.getCoords(&x1, &y1, &x2, &y2);
            QPoint c = r.center();
            double x_ratio = double(x2 - x1) / double(data.sizeX);
            double y_ratio = double(y2 - y1) / double(data.sizeY);
            double scale = data.view->Scale() / qMax(x_ratio, y_ratio);
            data.view->Place(c.x(), c.y(), scale);
        }
    }
} ClkZoomInBox;

class ClsClkZoomOutBox : public BaseClkRubberBandType
{
public:
    virtual QString getName()
    {
        return QString("ClkZoomOutBox");
    }
    virtual void release(const ViewActionData& data)
    {
        BaseClkRubberBandType::release(data);
        const QRect& r = data.band->geometry();
        if (r.isEmpty()) {
            qDebug("Zoom to point: (%d, %d) ignoring.", r.x(), r.y());
        } else {
            int x1, y1, x2, y2;
            r.getCoords(&x1, &y1, &x2, &y2);
            QPoint c = r.center();
            double x_ratio = double(x2 - x1) / double(data.sizeX);
            double y_ratio = double(y2 - y1) / double(data.sizeY);
            double scale = data.view->Scale() * qMax(x_ratio, y_ratio);
            data.view->Place(c.x(), c.y(), scale);
        }
    }
} ClkZoomOutBox;


void doScroll(const Handle(V3d_View)& v, double step)
{
    double factor;
    if (step < 0) {
        factor = (1.0 + step);
    } else {
        factor = 1.0 / (1.0 - step);
    }
    v->SetZoom(factor, true);
}

class ClsScrZoom : public MouseScrollMode
{
public:
    virtual QString getName()
    {
        return QString("ScrZoom");
    }
    virtual void scroll(V3d_View* v, int delta)
    {
        doScroll(v, delta / 360.0);
    }
} ScrZoom;

class ClsScrZoomInv : public MouseScrollMode
{
public:
    virtual QString getName()
    {
        return QString("ScrZoomInv");
    }
    virtual void scroll(V3d_View* v, int delta)
    {
        doScroll(v, -delta / 360.0);
    }
} ScrZoomInv;

class ClsScrNone : public MouseScrollMode
{
public:
    virtual QString getName()
    {
        return QString("ScrNone");
    }
} ScrNone;

class ClsScrPanStepV : public MouseScrollMode
{
public:
    virtual QString getName()
    {
        return QString("ScrPanStepV");
    }
    virtual void scroll(V3d_View* v, int delta)
    {
        v->Pan(0, delta / 2);
    }
} ScrPanStepV;

class ClsScrPanStepH : public MouseScrollMode
{
public:
    virtual QString getName()
    {
        return QString("ScrPanStepH");
    }
    virtual void scroll(V3d_View* v, int delta)
    {
        v->Pan(delta / 2, 0);
    }
} ScrPanStepH;

MouseButtonMode* buttonModes[4][4] = {
    {
        &ClkSelect,
        &ClkSelectToggle,
        &ClkSelect,
        &ClkSelectToggle
    },
    {
        &ClkPopup,
        &ClkZoomInBox,
        &ClkRotate,
        &ClkZoomOutBox,
    },
    {
        &ClkPan,
        &ClkRotate,
        &ClkPan,
        &ClkNone
    },
    {
        &ClkHover,
        &ClkHover,
        &ClkHover,
        &ClkHover
    }
};

MouseScrollMode* scrollModes[4] = {
    &ScrZoom,
    &ScrPanStepV,
    &ScrPanStepH,
    &ScrNone
};


MouseButtonMode* getButtonAction(Qt::MouseButton button,
                                 Qt::KeyboardModifiers modifiers)
{
    int idx = 0;
    if (modifiers & Qt::ShiftModifier) {
        idx += 1;
    }
    if (modifiers & Qt::ControlModifier) {
        idx += 2;
    }

    if (button == Qt::MiddleButton) {
        return buttonModes[2][idx];
    } else if (button == Qt::RightButton) {
        return buttonModes[1][idx];
    } else if (button == Qt::LeftButton) {
        return buttonModes[0][idx];
    } else {
        return buttonModes[3][idx];
    }
}

MouseScrollMode* getScrollAction(Qt::KeyboardModifiers modifiers)
{
    int idx = 0;
    if (modifiers & Qt::ShiftModifier) {
        idx += 1;
    }
    if (modifiers & Qt::ControlModifier) {
        idx += 2;
    }

    return scrollModes[idx];
}

HelpDialog::HelpDialog(QWidget* parent) :
    QDialog(parent)
{
    QTableWidget* table = new QTableWidget(this);
    table->setRowCount(5);
    table->setColumnCount(4);
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            QWidget* w = new QLabel(buttonModes[y][x]->getName());
            table->setCellWidget(y, x, w);
        }
    }
    {
        int y = 4;
        for (int x = 0; x < 4; x++) {
            QWidget* w = new QLabel(scrollModes[x]->getName());
            table->setCellWidget(y, x, w);
        }
    }
    table->setHorizontalHeaderLabels(QStringList() << "Plain" << "Shift" << "Ctrl"
                                     << "Ctrl+Shift");
    table->setVerticalHeaderLabels(QStringList() << "Left" << "Right" << "Middle" <<
                                   "Hover" << "Scroll");
    table->setAlternatingRowColors(true);
    table->setShowGrid(false);
    int xw = table->horizontalHeader()->length() +
             table->verticalHeader()->length();
    int yw = table->horizontalHeader()->height() +
             table->verticalHeader()->height();
    table->setMinimumSize(xw, yw);

    setWindowTitle("Step-Gdml Help");

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(table);
    setLayout(layout);
    resize(sizeHint());
}
