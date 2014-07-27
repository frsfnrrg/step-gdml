#ifndef TRANSLATE_H
#define TRANSLATE_H

#include <QtGui>
#include "gdmlwriter.h"

#include <AIS_InteractiveContext.hxx>
#include <TopTools_HSequenceOfShape.hxx>

class IODialog : public QObject {
    Q_OBJECT
public:
    IODialog(QWidget* parent, QFileDialog::AcceptMode, QStringList filters, QString suffix);
    void addOption(QString label, QWidget*);

    void hook(QObject* target, const char* slot);

public slots:
    void display();

signals:
    void chosen(QString filename);
private slots:
    void onComplete();
private:
    QFileDialog* fd;
};


class Translator: public QObject {
    Q_OBJECT
public:
    Translator(const Handle(AIS_InteractiveContext) context);

    bool importSTEP(QString, const Handle(TopTools_HSequenceOfShape)& );
    bool exportGDML(QString, const Handle(TopTools_HSequenceOfShape)& );
    bool importSTEP(QString);
    bool exportGDML(QString);
    static bool displayShapes(const Handle(AIS_InteractiveContext)&, const Handle(TopTools_HSequenceOfShape) &);
    static bool findAllShapes(const Handle(AIS_InteractiveContext)&, const Handle(TopTools_HSequenceOfShape) &);
    static QList<AIS_InteractiveObject*> getInteractiveObjects(const Handle(AIS_InteractiveContext)&);
private:

    GdmlWriter* gdmlWriter;
    const Handle(AIS_InteractiveContext) context;
};

#endif

