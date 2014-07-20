#!/usr/bin/env/python2

from __future__ import print_function, division, absolute_import

import xml.etree.cElementTree as ET
from box import *

unit = "mm"

def make_materials():
    materials = ET.Element("materials")
    al = ET.SubElement(materials, 'material')
    al.attrib = {
        "Z":"13",
        "name":"ALUMINUM"
    }
    atom = ET.SubElement(al, "atom")
    atom.attrib = {
        "unit":"g/mole",
        "value":"26.9815385"
        }
    density = ET.SubElement(al, "D")
    density.attrib["value"] = "2.70"

    vacuum = al = ET.SubElement(materials, 'material')
    al.attrib = {
        "Z":"1",
        "name":"VACUUM"
    }
    atom = ET.SubElement(vacuum, "atom")
    atom.attrib = {
        "unit":"g/mole",
        "value":"1.00794"
        }
    density = ET.SubElement(vacuum, "D")
    density.attrib["value"] = "1e-25"

    return materials


def make_defines(things,wbbox):
    defines = ET.Element("define")
    for shapeno,thing in enumerate(things):
        for pno, position in enumerate(thing[0]):
            px = ET.SubElement(defines, "position")
            px.attrib = {
                "name":"{}-{}".format(shapeno, pno),
                "unit":unit,
                "x":str(position[0]),
                "y":str(position[1]),
                "z":str(position[2]),
            }

    px = ET.SubElement(defines, "position")
    center = boxcenter(wbbox)
    px.attrib = {
            "name":"center",
            "unit":unit,
            "x":str(-center[0]),
            "y":str(-center[1]),
            "z":str(-center[2]),
        }

    return defines

def make_solids(things,wbbox):
    solids = ET.Element("solids")
    for shapeno, thing in enumerate(things):
        tess = ET.SubElement(solids, "tessellated")
        tess.attrib["name"] = "T-"+thing[2]
        pre = str(shapeno) + "-"
        for triangle in thing[1]:
            tri = ET.SubElement(tess, "triangular")
            tri.attrib = {
                "vertex1":pre+str(triangle[0]),
                "vertex2":pre+str(triangle[1]),
                "vertex3":pre+str(triangle[2]),
                "type":"ABSOLUTE"
            }

    worldbox = ET.SubElement(solids, "box")
    sizevec = boxsize(wbbox)
    worldbox.attrib = {
        "name":"worldbox",
        "lunit":unit,
        "x":str(sizevec[0]),
        "y":str(sizevec[1]),
        "z":str(sizevec[2]),
        }
    # tessellate, tessellate
    return solids

def make_structure(things):
    structure = ET.Element("structure")
    # list of volumes, the top volume, etc..

    for shapeno, thing in enumerate(things):
        volume = ET.SubElement(structure, "volume")
        matref = ET.SubElement(volume, "materialref")
        solidref = ET.SubElement(volume, "solidref")
        matref.attrib["ref"] = thing[3]
        volume.attrib["name"] = "V-"+thing[2]
        solidref.attrib["ref"] = "T-"+thing[2]

    volume = ET.SubElement(structure, "volume")
    volume.attrib["name"] = "World"
    matref = ET.SubElement(volume, "materialref")
    solidref = ET.SubElement(volume, "solidref")
    matref.attrib["ref"] = "VACUUM"
    solidref.attrib["ref"] = "worldbox"

    for shapeno, thing in enumerate(things):
        physvol = ET.SubElement(volume, "physvol")
        physvol.attrib["name"] = "P-"+thing[2]
        volref = ET.SubElement(physvol, "volumeref")
        volref.attrib["ref"] = "V-"+thing[2]
        positionref = ET.SubElement(physvol, "positionref")
        positionref.attrib["ref"] = "center"

    return structure

def write(filename, deftree,matree,solidtree,structree):
    header = """<?xml version="1.0" encoding="UTF-8" standalone="no" ?><gdml xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="http://service-spi.web.cern.ch/service-spi/app/releases/GDML/schema/gdml.xsd">"""
    footer = """<setup name="Default" version="1.0"><world ref="World"/></setup></gdml>"""
    output = "".join([header]+[ET.tostring(x) for x in (deftree,matree,solidtree,structree)]+[footer])
    import xml.dom.minidom,re
    uglyXml = xml.dom.minidom.parseString(output).toprettyxml(indent='  ')
    text_re = re.compile('>\n\s+([^<>\s].*?)\n\s+</', re.DOTALL)
    prettyXml = text_re.sub('>\g<1></', uglyXml)
    open(filename, "w").write(prettyXml)

def export_to_gdml(filename, things, boundbox):
    """
    filename is string
    boundbox is ((x1,x2),(y1,y2),(z1,z2))
    things is [(verts,faces,name,material)...]
    verts is [(x,y,z)...]
    faces is [(v1,v2,v3)...]
    name is string
    material is string (maybe more complex)

    Incoming units are millimeters
    """

    # things is of form: (verts, faces, name, material)

    superbox = boxgrow(boundbox, 5)

    center = boxcenter(superbox)



    #print(filename, len(things), superbox, center)

    mats = make_materials()
    defs = make_defines(things, superbox)
    sols = make_solids(things, superbox)
    strs = make_structure(things)

    write(filename, defs, mats, sols, strs)
    print("Write complete")
