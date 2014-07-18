#!/usr/bin/env python2

from __future__ import division, print_function, absolute_import

import sys,os,time
if len(sys.argv) != 3:
    print("Usage: step-gdml path/to/FreeCAD/lib filename.stp")
    quit()

sys.path.append(sys.argv[1])
filename = sys.argv[2]

import FreeCAD
import Part

class block():
    def __init__(self, text):
        self.text = text
    def __enter__(self):
        print("\n\n      ENTER {}\n\n".format(self.text))
        self.time = time.time()
    def __exit__(self, x,y,z):
        print("\n\n      LEAVE {}; t={}\n\n".format(self.text,time.time()-self.time))


with block("READ"):
    Part.open(filename)


def boxjoin(a, b):
    return FreeCAD.BoundBox(min(a.XMin, b.XMin),min(a.YMin, b.YMin), min(a.ZMin, b.ZMin),
        max(a.XMax, b.XMax),max(a.YMax, b.YMax),max(a.ZMax, b.ZMax))


# Precision is a float in (0,1]
# 1 is fast, 10^-8 will OOM you.
# Does NOT improve triangle quality
precision = 1

unit = "mm" # FreeCAD internal length. We don't care about mass/time
superbox = FreeCAD.BoundBox(0,0,0,0,0,0)
with block("UNPACK"):
    objs = FreeCAD.getDocument("Unnamed").findObjects()

    things = []

    for obj in objs:
        verts, tris = obj.Shape.tessellate(precision)
        material = "ALUMINUM" # by default
        things.append((verts, tris, material))

        bbox = obj.Shape.BoundBox
        superbox = boxjoin(superbox, bbox)

        # obj.Shape.Placement gives loc, Yaw-pitch-roll.
        # should be zero, if not must counter it :-{

def boxgrow(a,r):
    return FreeCAD.BoundBox(a.XMin-r,a.YMin-r,a.ZMin-r,a.XMax+r,a.YMax+r,a.ZMax+r)

worldbox = boxgrow(superbox, 5) # 5 mm grow for safety

import xml.etree.cElementTree as ET

# python still hogs memory when exporting.
# need to find a really lightweight method

def make_defines(things,wbbox):
    defines = ET.Element("define")
    for shapeno,thing in enumerate(things):
        for pno, position in enumerate(thing[0]):
            px = ET.SubElement(defines, "position")
            px.attrib = {
                "name":"{}-{}".format(shapeno, pno),
                "unit":unit,
                "x":str(position.x),
                "y":str(position.y),
                "z":str(position.z),
            }

    px = ET.SubElement(defines, "position")
    center = wbbox.Center
    px.attrib = {
            "name":"center",
            "unit":unit,
            "x":str(-center.x),
            "y":str(-center.y),
            "z":str(-center.z),
        }

    return defines

def make_solids(things,wbbox):
    solids = ET.Element("solids")
    for shapeno, thing in enumerate(things):
        tess = ET.SubElement(solids, "tessellated")
        tess.attrib["name"] = "T-"+str(shapeno)
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
    worldbox.attrib = {
        "name":"worldbox",
        "lunit":unit,
        "x":str(wbbox.XLength),
        "y":str(wbbox.YLength),
        "z":str(wbbox.ZLength),
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
        matref.attrib["ref"] = thing[2]
        volume.attrib["name"] = "V-"+str(shapeno)
        solidref.attrib["ref"] = "T-"+str(shapeno)

    volume = ET.SubElement(structure, "volume")
    volume.attrib["name"] = "World"
    matref = ET.SubElement(volume, "materialref")
    solidref = ET.SubElement(volume, "solidref")
    matref.attrib["ref"] = "VACUUM"
    solidref.attrib["ref"] = "worldbox"

    for shapeno, thing in enumerate(things):
        physvol = ET.SubElement(volume, "physvol")
        physvol.attrib["name"] = "P-"+str(shapeno)
        volref = ET.SubElement(physvol, "volumeref")
        volref.attrib["ref"] = "V-"+str(shapeno)
        positionref = ET.SubElement(physvol, "positionref")
        positionref.attrib["ref"] = "center"

    return structure

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

header = """<?xml version="1.0" encoding="UTF-8" standalone="no" ?><gdml xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="http://service-spi.web.cern.ch/service-spi/app/releases/GDML/schema/gdml.xsd">"""
footer = """<setup name="Default" version="1.0"><world ref="World"/></setup></gdml>"""


with block("GENERATE"):
    content = [make_defines(things,worldbox), make_materials(), make_solids(things,worldbox), make_structure(things)]

    output = "".join([header]+[ET.tostring(x) for x in content]+[footer])

def write(output):
    # should really write it properly in the first place
    # feels like an O(N^2) algorithm

    import xml.dom.minidom,re
    # SO 749796!
    uglyXml = xml.dom.minidom.parseString(output).toprettyxml(indent='  ')
    text_re = re.compile('>\n\s+([^<>\s].*?)\n\s+</', re.DOTALL)
    prettyXml = text_re.sub('>\g<1></', uglyXml)
    open("output.gdml", "w").write(prettyXml)

with block("WRITE"):
    write(output)
