#!/usr/bin/env/python2

from __future__ import print_function, division, absolute_import

from lxml import etree as ET
from box import *

unit = "mm"

def mksub(_parent, _name, **attribs):
    """
        First two args have underscores to prevent collisions.
    """
    x = ET.SubElement(_parent, _name)
    for k,v in attribs.items():
        x.set(k,str(v))
    return x

def make_materials():
    materials = ET.Element("materials")
    al = mksub(materials, "material",Z=13,name="ALUMINUM")

    atom = mksub(al, "atom", unit="g/mole", value=26.9815385)
    density = mksub(al, "D", value="2.70")

    vacuum = mksub(materials, "material", Z=1, name="VACUUM")
    atom = mksub(vacuum, "atom",  unit="g/mole", value=1.00794)
    density = mksub(vacuum, "D", value=1e-25)

    return materials

def make_defines(things,wbbox):
    defines = ET.Element("define")
    for shapeno,thing in enumerate(things):
        for pno, position in enumerate(thing[0]):
            mksub(defines, "position", name="{}-{}".format(shapeno, pno), unit=unit,x=position[0],y=position[1],z=position[2])

    center = boxcenter(wbbox)
    mksub(defines, "position", name="center", unit=unit, x=-center[0],y=-center[1],z=-center[2])

    return defines

def make_solids(things,wbbox):
    solids = ET.Element("solids")
    for shapeno, thing in enumerate(things):
        tess = mksub(solids, "tessellated", name="T-"+thing[2])
        pre = str(shapeno) + "-"
        for triangle in thing[1]:
            mksub(tess, "triangular",
                vertex1=pre+str(triangle[0]),
                vertex2=pre+str(triangle[1]),
                vertex3=pre+str(triangle[2]),
                type="ABSOLUTE")

    sizevec = boxsize(wbbox)
    mksub(solids, "box", name="worldbox", lunit=unit, x=sizevec[0],y=sizevec[1],z=sizevec[2])

    return solids


def make_structure(things):
    structure = ET.Element("structure")

    for shapeno, thing in enumerate(things):
        vol = mksub(structure, "volume", name="V-"+thing[2])
        mksub(vol, "materialref", ref=thing[3])
        mksub(vol, "solidref", ref="T-"+thing[2])

    vol = mksub(structure, "volume", name="World")
    mksub(vol, "materialref", ref="VACUUM")
    mksub(vol, "solidref", ref="worldbox")

    for shapeno, thing in enumerate(things):
        physvol = mksub(vol, "physvol", name="P-"+thing[2])
        mksub(physvol, "volumeref", ref="V-"+thing[2])
        mksub(physvol, "positionref", ref="center")

    return structure

def make_setup():
    setup = ET.Element("setup")
    setup.set("name", "Default")
    setup.set("version","1.0")
    mksub(setup, "world", ref="World")
    return setup

def write(filename, tree):
    output = ET.tostring(tree,pretty_print=True)

    with open(filename, "w") as f:
        f.write('<?xml version="1.0" encoding="UTF-8" standalone="no" ?>\n')
        f.write(output)

def export_to_gdml(filename, things, boundbox):
    """
    filename is string
    boundbox is ((x1,x2),(y1,y2),(z1,z2))
    things is [(verts,faces,name,material)...]
    verts is [(x,y,z)...]
    faces is [(v1,v2,v3)...]
    name is string
    material is string (maybe more complex).

    Remember: Passing numpy arrays as arguments
    is not only supported, but desired.

    Incoming units are millimeters
    """

    superbox = boxgrow(boundbox, 5)
    center = boxcenter(superbox)

    gdml = ET.Element("gdml")
    #gdml.set("xmlns:xsi","http://www.w3.org/2001/XMLSchema-instance")
    #gdml.set("xsi:noNamespaceSchemaLocation","http://service-spi.web.cern.ch/service-spi/app/releases/GDML/schema/gdml.xsd")

    # there may be a large-tree append cost in lxml
    gdml.append(make_defines(things, superbox))
    gdml.append(make_materials())
    gdml.append(make_solids(things, superbox))
    gdml.append(make_structure(things))
    gdml.append(make_setup())

    write(filename, gdml)
