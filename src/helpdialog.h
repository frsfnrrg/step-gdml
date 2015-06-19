#ifndef HELPDIALOG_H
#define HELPDIALOG_H

#include <QDialog>
#include <QRubberBand>
#include <QTableWidget>
#include <QComboBox>
#include <QList>

#include <V3d_TypeOfOrientation.hxx>

class V3d_View;
class ViewActionData;

class MouseMode
{
public:
    MouseMode() {}
    virtual QString getName() = 0;
    virtual Qt::GlobalColor getColor() = 0;
};

class MouseButtonMode : public MouseMode
{
public:
    MouseButtonMode() {}
    virtual Qt::CursorShape getCursor() = 0;
    virtual void click(const ViewActionData&) {}
    virtual void drag(const ViewActionData&) {}
    virtual void release(const ViewActionData&) {}
private:
    MouseButtonMode(const MouseButtonMode&);
    void operator=(const MouseButtonMode&);
};

class MouseScrollMode : public MouseMode
{
public:
    MouseScrollMode() {}
    virtual void scroll(V3d_View*, int) {}
private:
    MouseScrollMode(const MouseScrollMode&);
    void operator=(const MouseScrollMode&);
};

class OrientationMapper : public QObject
{
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

class ModeComboBox : public QComboBox
{
    Q_OBJECT
public:
    ModeComboBox();

    void addModeItem(MouseMode* m);
    virtual void hidePopup();
    virtual void showPopup();
    MouseMode* getSelectedPointer();
signals:
    void hidePopupSignal();
    void showPopupSignal();
private:
    QList<MouseMode*> list;
};

class HelpDialog : public QDialog
{
    Q_OBJECT
public:
    explicit HelpDialog(QWidget* parent = 0);

    static void loadConfig();
    static void saveConfig();

public slots:
    void resetConfig();
    void editCell(int, int);
    void changeItem();
    void clearPopup();

private:
    QTableWidget* table;
    ModeComboBox* popup;
    int lastRow, lastCol;
};

#endif // HELPDIALOG_H
