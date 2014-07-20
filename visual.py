#!/usr/bin/env python2

from __future__ import print_function, division, absolute_import

import sys
import OCC.Display.SimpleGui as Gui
from OCC.Utils.DataExchange.STEP import STEPImporter
from OCC.Utils.Topology import Topo
from OCC.MSH.Mesh import QuickTriangleMesh

from box import *
from fast_export import export_to_gdml

display = None
shapes = []

def group3(li):
    o = []
    for x in range(0, len(li), 3):
        o.append(tuple(li[x:x+3]))
    return o

def load_file(evt=None):
    global shapes
    name = "test.stp"
    importer = STEPImporter(name)
    importer.read_file()
    lumps = importer.get_shapes()
    shapes = [s for lump in lumps for s in Topo(lump).solids()]

    print("Loading complete. Rendering now.")

    display.EraseAll()
    display.DisplayShape(shapes)
    display.ResetView()

def export_file(evt=None):
    things = []
    bbox = ((0,0),(0,0),(0,0))
    for idx, shape in enumerate(shapes):
        # note: fails on unusual meshes.
        # note: need to set precision appropriately
        qtm = QuickTriangleMesh()
        qtm.set_shape(shape)
        qtm.compute()
        vtx = qtm.get_vertices()
        things.append([vtx, group3(qtm.get_faces()), str(idx), "ALUMINUM"])
        bbox = boxjoin(boundboxv(vtx), bbox)

    print("Meshing complete. Generating now.")

    export_to_gdml("output.gdml", things, bbox)

def named(func, name):
    """ Mutates function name && returns it """
    def k(*x,**y):
        return func(*x, **y)
    k.__name__ = name
    return k


def create_window(args):
    global display

    Gui.set_backend("qt")
    display, start_display, add_menu, add_function_to_menu = \
        Gui.init_display()

    add_menu("File")
    add_function_to_menu("File", named(load_file, "Load File"))
    add_function_to_menu("File", named(export_file, "Export File"))
    add_function_to_menu("File", named(quit, "Quit"))

    print(dir(display))

    display.SetSelectionModeShape()

    start_display()

if __name__ == '__main__':
    # NOTE: we actually should embed OCCViewer in a window reference
    # (use PyQt window). Then we can create a multiframe window.
    # But EXPORT FIRST

    create_window(sys.argv)
