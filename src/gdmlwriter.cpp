#include "gdmlwriter.h"

#include <QSet>
#include <QMap>

#include <BRepBndLib.hxx>
#include <StlTransfer.hxx>
#include <StlMesh_Mesh.hxx>
#include <StlMesh_SequenceOfMeshTriangle.hxx>
#include <StlMesh_MeshTriangle.hxx>
#include <StlMesh_MeshExplorer.hxx>
#include <Standard_Version.hxx>


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

void GdmlWriter::addSolid(TopoDS_Shape shape, QString name, QString material)
{
    Handle_StlMesh_Mesh aMesh = new StlMesh_Mesh();
    int index = names.size();

    StlTransfer::BuildIncrementalMesh(shape, 1.0,
#if OCC_VERSION_HEX >= 0x060503
                                      Standard_True,
#endif
                                      aMesh);

    typedef struct {
        Standard_Integer V1;
        Standard_Integer V2;
        Standard_Integer V3;
    } Triangle;

    QList<gp_XYZ> verts;
    QMap<gp_XYZ, int> dedup;
    QList<int> steps;

    QList<int> triconv;

    int idx = 0;
    for (int i = 1; i <= aMesh->NbDomains(); i++) {
        const TColgp_SequenceOfXYZ& seq = aMesh->Vertices(i);
        for (int j = 1; j <= seq.Length(); j++) {
            const gp_XYZ& pt = seq.Value(j);

            int val = dedup.value(pt, -1);
            if (val == -1) {
                verts.append(pt);
                triconv.append(idx);
                dedup[pt] = idx;
                idx += 1;
            } else {
                triconv.append(val);
            }
        }
        steps.append(seq.Length());
    }

    dedup.clear();

    _("  <define>\n");
    int numverts = verts.size();
    for (int i = 0; i < numverts; i++) {
        _("    <position name=\"%d-%d\" x=\"%f\" y=\"%f\" z=\"%f\" unit=\"mm\"/>\n",
          index, i, verts[i].X(), verts[i].Y(), verts[i].Z());
    }
    _("  </define>\n");

    int num_verts = verts.size();
    verts.clear();

    _("  <solids>\n");
    _("    <tessellated name=\"T-%s\">\n", name.toUtf8().data());

    idx = 0;
    int num_tris = 0;
    for (int i = 1; i <= aMesh->NbDomains(); i++) {
        const StlMesh_SequenceOfMeshTriangle& x = aMesh->Triangles(i);
        for (int j = 1; j <= x.Length(); j++) {
            int V1, V2, V3;
            x.Value(j)->GetVertex(V1, V2, V3);

            int R1, R2, R3;
            R1 = triconv[V1 + idx - 1];
            R2 = triconv[V2 + idx - 1];
            R3 = triconv[V3 + idx - 1];

            _("      <triangular vertex1=\"%d-%d\" vertex2=\"%d-%d\" vertex3=\"%d-%d\" type=\"ABSOLUTE\"/>\n",
              index, R1, index, R2, index, R3);

        }
        num_tris += x.Length();
        idx += steps[i - 1];
    }
    _("    </tessellated>\n");
    _("  </solids>\n");

    names.append(name);
    materials.append(material);
    BRepBndLib::Add(shape, bounds);

    printf("% 6d vertices, % 6d triangles <- %s\n", num_verts, num_tris,
           name.toUtf8().data());
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
        _("    <volume name=\"V-%s\">\n", names[i].toUtf8().data());
        _("      <materialref ref=\"%s\"/>\n", materials[i].toUtf8().data());
        _("      <solidref ref=\"T-%s\"/>\n", names[i].toUtf8().data());
        _("    </volume>\n");
    }

    _("    <volume name=\"World\">\n");
    _("      <materialref ref=\"VACUUM\"/>\n");
    _("      <solidref ref=\"worldbox\"/>\n");

    for (int i = 0; i < sz; i++) {
        _("      <physvol name=\"P-%s\">\n", names[i].toUtf8().data());
        _("        <volumeref ref=\"V-%s\"/>\n", names[i].toUtf8().data());
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
