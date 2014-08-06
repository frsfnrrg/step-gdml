#ifndef HELPDIALOG_H
#define HELPDIALOG_H

#include <QDialog>
#include <QRubberBand>
#include <V3d_TypeOfOrientation.hxx>

class V3d_View;
class ViewActionData;

class MouseButtonMode
{
public:
    MouseButtonMode();
    virtual Qt::CursorShape getCursor() = 0;
    virtual QString getName() = 0;
    virtual void click(const ViewActionData&) {}
    virtual void drag(const ViewActionData&) {}
    virtual void release(const ViewActionData&) {}
private:
    MouseButtonMode(const MouseButtonMode&);
    void operator=(const MouseButtonMode&);
};

class MouseScrollMode
{
public:
    MouseScrollMode();
    virtual QString getName() = 0;
    virtual void scroll(V3d_View*, int) {}
private:
    MouseScrollMode(const MouseScrollMode&);
    void operator=(const MouseScrollMode&);
};

class OrientationMapper : public QObject {
    Q_OBJECT
public:
    OrientationMapper(QObject*, QObject*, V3d_TypeOfOrientation);
signals:
    void send(V3d_TypeOfOrientation);
public slots:
    void recieve();
private:
    V3d_TypeOfOrientation orientation;
};

MouseButtonMode* getButtonAction(Qt::MouseButton button,
                          Qt::KeyboardModifiers modifiers);
MouseScrollMode* getScrollAction(Qt::KeyboardModifiers modifiers);


class HelpDialog : public QDialog
{
    Q_OBJECT
public:
    explicit HelpDialog(QWidget* parent = 0);


signals:

public slots:

private:
};

#endif // HELPDIALOG_H
