#ifndef VIEWER_H
#define VIEWER_H

#include <QWidget>
#include <QRubberBand>
#include <QPaintEngine>

#include <Standard.hxx>
#include <AIS_InteractiveContext.hxx>
#include <V3d_TypeOfOrientation.hxx>
#include <V3d_View.hxx>

class V3d_Viewer;
class MouseButtonMode;
class MouseScrollMode;
class Viewer;

class ViewActionData {
public:
    V3d_View* view;
    AIS_InteractiveContext* context;
    int x;
    int y;
    int globalX;
    int globalY;
    int sizeX;
    int sizeY;
    QRubberBand* band;
    Viewer* viewer;
};

class Viewer : public QWidget
{
    Q_OBJECT
public:
    explicit Viewer(Handle(AIS_InteractiveContext), QWidget *parent = 0);
    virtual ~Viewer();

    static V3d_Viewer* makeViewer();

signals:
    void readyToUse();
    void selectionMightBeChanged();

public slots:
    void setOrientation(V3d_TypeOfOrientation);
    void resetView();

    void startHover();
private:
    void init();

    virtual QPaintEngine* paintEngine() const {return 0;}

    virtual void paintEvent(QPaintEvent*);
    virtual void resizeEvent(QResizeEvent*);

    virtual void mousePressEvent(QMouseEvent*);
    virtual void mouseReleaseEvent(QMouseEvent*);
    virtual void mouseMoveEvent(QMouseEvent*);
    virtual void mouseDoubleClickEvent(QMouseEvent*);
    virtual void wheelEvent(QWheelEvent*);

    ViewActionData getViewActionData(QMouseEvent*);

    Handle(V3d_View) view;
    Handle(AIS_InteractiveContext) context;
    MouseButtonMode* buttonMode;
    MouseScrollMode* scrollMode;
    QRubberBand* rubberBand;
    QMouseEvent* lastEvt;
};

#endif // VIEWER_H
