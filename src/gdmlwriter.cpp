#include "gdmlwriter.h"

#include <QSet>
#include <QMap>

#include <BRepBndLib.hxx>
#include <StlAPI_Writer.hxx>
#include <Standard_Version.hxx>
#include <Poly_CoherentTriangulation.hxx> // Since 0x060300 ?

QString GdmlWriter::defaultMaterial()
{
    return QString("VACUUM");
}

GdmlWriter::GdmlWriter(QString filename)
{
    f = fopen(filename.toUtf8().data(), "w");
    if (!f) {
        throw "FAIL";
    }

    bounds = Bnd_Box();
}

static QByteArray convName(const QString& name) {
    // Strip unicode & get rid of quotes, brackets.
    QByteArray b;
    for (QChar c : name) {
        if (c <= 127 && c != QChar('"') && c != QChar('[') && c != QChar(']')) {
            b.push_back(c.toLatin1());
        } else {
            b.push_back('?');
        }
    }
    return b;
}

GdmlWriter::~GdmlWriter()
{
    fclose(f);
}

#define _(...) fprintf (f, __VA_ARGS__)

void GdmlWriter::writeMaterials()
{
    _("  <materials>\n");
    _("    <material Z=\"13\" name=\"ALUMINUM\">\n");
    _("      <atom unit=\"g/mole\" value=\"26.9815385\"/>\n");
    _("      <D value=\"2.70\"/>\n");
    _("    </material>\n");
    _("    <material Z=\"1\" name=\"VACUUM\">\n");
    _("      <atom unit=\"g/mole\" value=\"1.00794\"/>\n");
    _("      <D value=\"1e-25\"/>\n");
    _("    </material>\n");
    _("  </materials>\n");
}

void GdmlWriter::writeIntro()
{
    _("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
    _("<gdml xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"http://service-spi.web.cern.ch/service-spi/app/releases/GDML/GDML_3_0_0/schema/gdml.xsd\" >");

    writeMaterials();
}

bool operator <(const gp_XYZ& a, const gp_XYZ& b)
{
    if (a.X() == b.X()) {
        if (a.Y() == b.Y()) {
            return a.Z() < b.Z();
        }
        return a.Y() < b.Y();
    }
    return a.X() < b.X();
}

Handle_Poly_CoherentTriangulation triangulateShape(TopoDS_Shape shape);
void GdmlWriter::addSolid(TopoDS_Shape shape, QString name, QString material)
{
    Handle_Poly_CoherentTriangulation aMesh = triangulateShape(shape);

    _("  <define>\n");
    for (int i = 0; i < aMesh->NNodes(); i++) {
        const Poly_CoherentNode& vert = aMesh->Node(i);
        _("    <position name=\"%d\" x=\"%f\" y=\"%f\" z=\"%f\" unit=\"mm\"/>\n",
          i, vert.X(), vert.Y(), vert.Z());
    }
    _("  </define>\n");

    _("  <solids>\n");
    _("    <tessellated name=\"T-%s\">\n", convName(name).data());
    for (int i=0;i<aMesh->NTriangles();i++) {
        const Poly_CoherentTriangle& tri = aMesh->Triangle(i);
         _("      <triangular vertex1=\"%d\" vertex2=\"%d\" vertex3=\"%d\" type=\"ABSOLUTE\"/>\n",
              tri.Node(0), tri.Node(1), tri.Node(2));
    }
    _("    </tessellated>\n");
    _("  </solids>\n");

    names.append(name);
    materials.append(material);
    BRepBndLib::Add(shape, bounds);

    printf("% 6d vertices, % 6d triangles <- %s\n", aMesh->NNodes(), aMesh->NTriangles(),
           convName(name).data());
}

void GdmlWriter::writeWorldBox()
{
    const Standard_Real buffer = 5.0;
    Standard_Real xMin, xMax, yMin, yMax, zMin, zMax;
    bounds.Get(xMin, yMin, zMin, xMax, yMax, zMax);
    xMin -= buffer;
    yMin -= buffer;
    zMin -= buffer;
    xMax += buffer;
    yMax += buffer;
    zMax += buffer;

    double cx, cy, cz, sx, sy, sz;
    cx = (xMin + xMax) / 2;
    cy = (yMin + yMax) / 2;
    cz = (zMin + zMax) / 2;
    sx = (xMax - xMin);
    sy = (yMax - yMin);
    sz = (zMax - zMin);

    _("  <define>\n");
    _("    <position name=\"center\" x=\"%f\" y=\"%f\" z=\"%f\" unit=\"mm\"/>\n",
      -cx, -cy, -cz);
    _("  </define>\n");

    _("  <solids>\n");
    _("    <box name=\"worldbox\" x=\"%f\" y=\"%f\" z=\"%f\" lunit=\"mm\"/>\n", sx,
      sy, sz);
    _("  </solids>\n");
}

void GdmlWriter::writeStructures()
{
    _("  <structure>\n");

    int sz = names.size();
    for (int i = 0; i < sz; i++) {
        _("    <volume name=\"V-%s\">\n", convName(names[i]).data());
        _("      <materialref ref=\"%s\"/>\n", materials[i].toUtf8().data());
        _("      <solidref ref=\"T-%s\"/>\n", convName(names[i]).data());
        _("    </volume>\n");
    }

    _("    <volume name=\"World\">\n");
    _("      <materialref ref=\"VACUUM\"/>\n");
    _("      <solidref ref=\"worldbox\"/>\n");

    for (int i = 0; i < sz; i++) {
        _("      <physvol name=\"P-%s\">\n", convName(names[i]).data());
        _("        <volumeref ref=\"V-%s\"/>\n", convName(names[i]).data());
        _("        <positionref ref=\"center\"/>\n");
        _("      </physvol>\n");
    }

    _("    </volume>\n");

    _("  </structure>\n");
}

void GdmlWriter::writeSetup()
{
    _("  <setup name=\"Default\" version=\"1.0\">\n");
    _("    <world ref=\"World\"/>\n");
    _("  </setup>\n");
}

void GdmlWriter::writeExtro()
{
    writeWorldBox();
    writeStructures();
    writeSetup();
    _("</gdml>\n");
}

#undef _
