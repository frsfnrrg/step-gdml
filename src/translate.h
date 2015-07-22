#ifndef TRANSLATE_H
#define TRANSLATE_H

#include "metadata.h"

#include <QString>
#include <QVector>
#include <QPair>
#include <QColor>

#include <Standard.hxx>
#include <AIS_InteractiveContext.hxx>
#include <TopTools_HSequenceOfShape.hxx>

class GdmlWriter;
class Graphic3d_MaterialAspect;

class Translator
{
public:
    Translator(const Handle(AIS_InteractiveContext) context);
    QList<AIS_InteractiveObject*> importSTEP(QString,
            QList<QPair<QString, QColor> >&);
    bool exportGDML(QString, const QVector<SolidMetadata>&);

    static bool importSTEP(QString, const Handle(TopTools_HSequenceOfShape)&,
                           QList<QPair<QString, QColor> >&);
    static bool exportGDML(QString, const Handle(TopTools_HSequenceOfShape)&,
                           const QVector<SolidMetadata>&);
    static QList<AIS_InteractiveObject*> displayShapes(const Handle(
                AIS_InteractiveContext)&, const Handle(TopTools_HSequenceOfShape)&);
    static bool findAllShapes(const Handle(AIS_InteractiveContext)&,
                              const Handle(TopTools_HSequenceOfShape)&);
    static QList<AIS_InteractiveObject*> getInteractiveObjects(const Handle(
                AIS_InteractiveContext)&);
private:
    static GdmlWriter* gdmlWriter;
    const Handle(AIS_InteractiveContext) context;
};

#endif

