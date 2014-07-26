
from OCC.MSH.Mesh import QuickTriangleMesh

# For OCC Triangle Mesh
from OCC.BRepMesh import *
from OCC.BRep import *
from OCC.BRepTools import *
from OCC.TopLoc import *
from OCC.Poly import *
from OCC.BRepBuilderAPI import *
from OCC.StdPrs import *
from OCC.TColgp import *
from OCC.Poly import *
from OCC.Utils.Topology import *
from OCC.TopoDS import *
from OCC.TopAbs import *
from OCC.Bnd import *
from OCC.BRepBndLib import *
from OCC.gp import *

import time

def group3(li):
    o = []
    for x in range(0, len(li), 3):
        o.append(tuple(li[x:x+3]))
    return o

# possibilities
#  BRepMesh_FastDiscretFace
#  BRepMesh_Delaun
# Brep_Tool (from OCC.Brep import BRep_Tool;
#Handle (Poly_Triangulation) facing = BRep_Tool::Triangulation(F,L);
#const Poly_Array1OfTriangle & triangles = facing->Triangles();
#const TColgp_Array1OfPnt & nodes = facing->Nodes();
#for ( int i=triangles.Upper(); i >= triangles.Lower()+1; --i ) // (indeksy 1...N)
#{
    #Poly_Triangle triangle = triangles(i);

    #Standard_Integer node1,node2,node3;
    #triangle.Get(node1, node2, node3);

    #gp_Pnt v1 = nodes(node1);
    #gp_Pnt v2 = nodes(node2);
    #gp_Pnt v3 = nodes(node3);
    #// now we need to transform vertices
    #// don't forget about face orientation :)
#}

def conv_shape_leaky(shape):

    # QTM is a memory leak :-(
    qtm = QuickTriangleMesh()
    qtm.set_shape(shape)

    qtm.compute()

    print(qtm.get_precision())
    vtx = qtm.get_vertices()
    fcs = group3(qtm.get_faces())
    del qtm
    return vtx,fcs

def triangle_is_valid(P1,P2,P3):
    ''' check whether a triangle is or not valid
    '''
    V1 = gp_Vec(P1,P2)
    V2 = gp_Vec(P2,P3)
    V3 = gp_Vec(P3,P1)
    if V1.SquareMagnitude()>1e-10 and V2.SquareMagnitude()>1e-10 and V3.SquareMagnitude()>1e-10:
        V1.Cross(V2)
        if V1.SquareMagnitude()>1e-10:
            return True
        else:
            print 'Not valid!'
            return False
    else:
        print 'Not valid!'
        return False

def conv_shape_raw(shape, precision=10, DISPLAY_LIST=False):
    # still leaky
    init_time = time.time()
    BRepMesh_Mesh(shape,precision)
    points = []
    faces = []
    normals = []
    uv = []
    faces_iterator = Topo(shape).faces()

    for F in faces_iterator:
        face_location = F.Location()
        facing = BRep_Tool_Triangulation(F,face_location).GetObject()
        tab = facing.Nodes()
        tri = facing.Triangles()
        # Compute normal
        the_normal = TColgp_Array1OfDir(tab.Lower(), tab.Upper())
        StdPrs_ToolShadedShape_Normal(F, Poly_Connect(facing.GetHandle()), the_normal)
        for i in range(1,facing.NbTriangles()+1):
            trian = tri.Value(i)
            if F.Orientation() == TopAbs_REVERSED:
                index1, index3, index2 = trian.Get()
            else:
                index1, index2, index3 = trian.Get()
            # Transform points
            P1 = tab.Value(index1).Transformed(face_location.Transformation())
            P2 = tab.Value(index2).Transformed(face_location.Transformation())
            P3 = tab.Value(index3).Transformed(face_location.Transformation())

            p1_coord = P1.XYZ().Coord()
            p2_coord = P2.XYZ().Coord()
            p3_coord = P3.XYZ().Coord()
            if triangle_is_valid(P1, P2, P3):
                if not DISPLAY_LIST:
                    points.append(p1_coord)
                    points.append(p2_coord)
                    points.append(p3_coord)
                else:
                    if not (p1_coord in points):
                        points.append(p1_coord)
                    if not (p2_coord in points):
                        points.append(p2_coord)
                    if not (p3_coord in points):
                        points.append(p3_coord)
                    faces.append(points.index(p1_coord))
                    faces.append(points.index(p2_coord))
                    faces.append(points.index(p3_coord))
    vertices = points
    if not DISPLAY_LIST:
        faces = range(len(vertices))
    else:
        faces = faces
    print 'Finished mesh computation in %fs'%(time.time()-init_time)
    return vertices, group3(faces)

def conv_shape_woop(shape, precision=10):
    bbox = Bnd_Box()
    BRepBndLib_Add(shape, bbox)
    deflection = 0.01
    angle = 0.01
    mesh = BRepMesh_FastDiscret(deflection, angle, bbox)
    mesh.Perform(shape)
    numtris = mesh.NbTriangles()
    # index from 1 to numtris (inc).

    # take the triangles, find the edges, find verts
    # (Edge.FirstNode())
    #

    # build dict!

def copy_shape(shape):
    # separates them, doesn't delete nicely
    BRepTools.Write(shape, "tmp.txt")
    new = TopoDS_Shape()
    builder = BRep_Builder()
    BRepTools.Read(new, "tmp.txt", builder)
    return new


conv_shape = conv_shape_raw
