#!/usr/bin/env/python2

from __future__ import print_function, division, absolute_import

from box import *
from time import time

unit = "mm"

def write_intro(X):
    X('<?xml version="1.0" encoding="UTF-8" ?>')
    X('<gdml xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="http://service-spi.web.cern.ch/service-spi/app/releases/GDML/GDML_3_0_0/schema/gdml.xsd" >')

def write_materials(X):
    X('  <materials>')
    X('    <material Z="13" name="ALUMINUM">')
    X('      <atom unit="g/mole" value="26.9815385"/>')
    X('      <D value="2.70"/>')
    X('    </material>')
    X('    <material Z="1" name="VACUUM">')
    X('      <atom unit="g/mole" value="1.00794"/>')
    X('      <D value="1e-25"/>')
    X('    </material>')
    X('  </materials>')

def write_thing(X, vtx, fcs, thingno, name):
    X('  <define>')
    for idx, vtx in enumerate(vtx):
        X('    <position name="{}-{}" x="{}" y="{}" z="{}" unit="{}"/>'.format(thingno, idx, vtx[0],vtx[1],vtx[2],unit))
    X('  </define>')

    X('  <solids>')
    X('    <tessellated name="T-{}">'.format(name))
    pre = str(thingno) + "-"
    for face in fcs:
        X('      <triangular vertex1="{}" vertex2="{}" vertex3="{}" type="ABSOLUTE"/>'.format(pre+str(face[0]),pre+str(face[1]),pre+str(face[2])))
    X('    </tessellated>')
    X('  </solids>')

def write_worldbox(X, bbox):
    center = boxcenter(bbox)
    X('  <define>')
    X('    <position name="center" x="{}" y="{}" z="{}" unit="{}"/>'.format(-center[0],-center[1],-center[2],unit))
    X('  </define>')

    size = boxsize(bbox)
    X('  <solids>')
    X('    <box name="worldbox" x="{}" y="{}" z="{}" lunit="{}"/>'.format(size[0],size[1],size[2],unit))
    X('  </solids>')


def write_structures(X, nms):
    X('  <structure>')

    for name, mat in nms:
        X('    <volume name="V-{}">'.format(name))
        X('      <materialref ref="{}"/>'.format(mat))
        X('      <solidref ref="T-{}"/>'.format(name))
        X('    </volume>')

    X('    <volume name="World">')
    X('      <materialref ref="VACUUM"/>')
    X('      <solidref ref="worldbox"/>')

    for name, mat in nms:
        X('      <physvol name="P-{}">'.format(name))
        X('        <volumeref ref="V-{}"/>'.format(name))
        X('        <positionref ref="center"/>')
        X('      </physvol>')

    X('    </volume>')

    X('  </structure>')

def write_setup(X):
    X('  <setup name="Default" version="1.0">')
    X('    <world ref="World"/>')
    X('  </setup>')

def write_extro(X):
    X('</gdml>')

def export_to_gdml(filename, things_generator):
    """
    Numpy arrays are better.
    Note: quotes in names will ruin stuff. Don't use them!
    """

    t = time()

    with open(filename, "w") as f:
        X = lambda line: f.write(line + "\n")
        write_intro(X)
        write_materials(X)

        names_mats = []
        bbox = ((0,0),(0,0),(0,0))
        for idx, thing in enumerate(things_generator):
            vtx,fcs,name,mat = thing

            names_mats.append((name,mat))

            write_thing(X,vtx,fcs,idx,name)

            bbox = boxjoin(boundboxv(vtx), bbox)

            print(len(thing[0]),len(thing[1]),thing[2],thing[3])

        write_worldbox(X, bbox)

        write_structures(X, names_mats)
        write_setup(X)
        write_extro(X)

    print("Writing took: {: 3.4f} seconds".format(time() -t))



