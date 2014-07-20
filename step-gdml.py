#!/usr/bin/env python2

from __future__ import division, print_function, absolute_import

import sys
if len(sys.argv) != 3:
    print("Usage: step-gdml path/to/FreeCAD/lib filename.stp")
    quit()

sys.path.append(sys.argv[1])
filename = sys.argv[2]

import os,time
import FreeCAD
import Part
from box import *
from fast_export import export_to_gdml
import resource

def print_memusage():
    print("MEMORY: "+str(resource.getrusage(resource.RUSAGE_SELF).ru_maxrss/1000.0))

class block():
    def __init__(self, text):
        self.text = text
    def __enter__(self):
        print("\n\n      ENTER {}\n\n".format(self.text))
        self.time = time.time()
    def __exit__(self, x,y,z):
        print("\n\n      LEAVE {}; t={}\n\n".format(self.text,time.time()-self.time))

print_memusage()

with block("READ"):
    Part.open(filename)

print_memusage()

# Precision is a float in (0,1]
# 1 is fast, 10^-8 will OOM you.
# Does NOT improve triangle quality
precision = 1

unit = "mm" # FreeCAD internal length. We don't care about mass/time
bounds = ((0,0),(0,0),(0,0))
with block("UNPACK"):
    doc = FreeCAD.getDocument("Unnamed")
    objs = doc.findObjects()

    things = []
    for idx, obj in enumerate(objs):
        verts, tris = obj.Shape.tessellate(precision)
        # NOTE: make verts/tris numpy arrays
        # for much lower memory usage

        things.append(([(v.x,v.y,v.z) for v in verts], tris, str(idx), "ALUMINUM"))
        b = obj.Shape.BoundBox
        bbox = ((b.XMin, b.XMax),
                (b.YMin, b.YMax),
                (b.ZMin, b.ZMax))

        bounds = boxjoin(bounds, bbox)
        doc.removeObject(obj.Name) # no longer needed

        #print_memusage()

FreeCAD.closeDocument("Unnamed")

# clean out FreeCAD

with block("EXPORT"):
    export_to_gdml("output.gdml", things, bounds)

print_memusage()
