#!/usr/bin/env/python2

from __future__ import print_function, division, absolute_import

from box import *
from time import time

unit = "mm"

def write_intro(X):
    X('<?xml version="1.0" encoding="UTF-8" ?>')
    X('<gdml xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="http://service-spi.web.cern.ch/service-spi/app/releases/GDML/GDML_3_0_0/schema/gdml.xsd" >')

def write_definitions(X, things, center):
    X('  <define>')

    for thingno, thing in enumerate(things):
        for idx, vtx in enumerate(thing[0]):
            X('    <position name="{}-{}" x="{}" y="{}" z="{}" unit="{}"/>'.format(thingno, idx, vtx[0],vtx[1],vtx[2],unit))

    X('    <position name="center" x="{}" y="{}" z="{}" unit="{}"/>'.format(-center[0],-center[1],-center[2],unit))

    X('  </define>')

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

def write_solids(X, things, size):
    X('  <solids>')
    for thingno, thing in enumerate(things):
        X('    <tessellated name="T-{}">'.format(thing[2]))
        pre = str(thingno) + "-"
        for face in thing[1]:
            X('      <triangular vertex1="{}" vertex2="{}" vertex3="{}" type="ABSOLUTE"/>'.format(pre+str(face[0]),pre+str(face[1]),pre+str(face[2])))

        X('    </tessellated>')

    X('    <box name="worldbox" x="{}" y="{}" z="{}" lunit="{}"/>'.format(size[0],size[1],size[2],unit))

    X('  </solids>')

def write_structures(X, things):
    X('  <structure>')

    for thing in things:
        X('    <volume name="V-{}">'.format(thing[2]))
        X('      <materialref ref="{}"/>'.format(thing[3]))
        X('      <solidref ref="T-{}"/>'.format(thing[2]))
        X('    </volume>')

    X('    <volume name="World">')
    X('      <materialref ref="VACUUM"/>')
    X('      <solidref ref="worldbox"/>')

    for thing in things:
        X('      <physvol name="P-{}">'.format(thing[2]))
        X('        <volumeref ref="V-{}"/>'.format(thing[2]))
        X('        <positionref ref="center"/>')
        X('      </physvol>')

    X('    </volume>')

    X('  </structure>')

def write_extro(X):
    X('  <setup name="Default" version="1.0">')
    X('    <world ref="World"/>')
    X('  </setup>')
    X('</gdml>')

def export_to_gdml(filename, things, boundbox):
    """
    Numpy arrays are better.
    Note: quotes in names will ruin stuff (be slow). Don't use them!
    """
    superbox = boxgrow(boundbox, 5)
    center = boxcenter(superbox)
    size = boxsize(superbox)

    t = time()
    with open(filename, "w") as f:
        X = lambda line: f.write(line + "\n")

        write_intro(X)
        write_definitions(X, things, center)
        write_materials(X)
        write_solids(X, things, size)
        write_structures(X, things)
        write_extro(X)
    print("Writing took: {: 3.4f} seconds".format(time() -t))



