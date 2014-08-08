#ifndef GDMLWRITER_H
#define GDMLWRITER_H

#include <Standard.hxx>
#include <TopoDS.hxx>
#include "stdio.h"
#include <QString>
#include <QList>
#include <Bnd_Box.hxx>

class GdmlWriter
{
public:
    static QString defaultMaterial();

    GdmlWriter(QString);
    ~GdmlWriter();
    void writeIntro();
    void addSolid(TopoDS_Shape, QString, QString);
    void writeExtro();
private:
    void writeMaterials();
    void writeSetup();
    void writeStructures();
    void writeWorldBox();

    FILE* f = NULL;
    QList<QString> names;
    QList<QString> materials;
    Bnd_Box bounds;
};

#endif // GDMLWRITER_H
