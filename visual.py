#!/usr/bin/env python2

from __future__ import print_function, division, absolute_import

from PyQt4 import QtCore, QtGui

import sys,os,argparse

display = None
shapes = []

def group3(li):
    o = []
    for x in range(0, len(li), 3):
        o.append(tuple(li[x:x+3]))
    return o

def import_file(filename):
    print("Importing modules...")

    from OCC.Utils.DataExchange.STEP import STEPImporter
    from OCC.Utils.Topology import Topo

    print("Importing complete. Loading now...")

    global shapes
    importer = STEPImporter(filename)
    importer.read_file()
    lumps = importer.get_shapes()
    shapes = [s for lump in lumps for s in Topo(lump).solids()]

    print("Loading complete.")

def load_file(filename):
    import_file(filename)
    print("Rendering now...")

    display.EraseAll()
    display.DisplayShape(shapes)
    display.ResetView()

    print("Rendering complete.")

def export_file(filename):
    # TODO: eventually, reduce Mesh to core components, since QTM is O(n**2)
    print("Importing modules...")

    from OCC.MSH.Mesh import QuickTriangleMesh
    from box import boxjoin, boundboxv
    from fast_export import export_to_gdml

    print("Import complete. Meshing now...")

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

    print("Meshing complete. Generating now...")

    export_to_gdml(filename, things, bbox)

    print("Generating complete.")

def named(func, name):
    """ Mutates function name && returns it """
    def k(*x,**y):
        return func(*x, **y)
    k.__name__ = name
    return k

class MainWindow(QtGui.QMainWindow):
    def __init__(self):
        apply(QtGui.QMainWindow.__init__, (self,))
        self.setWindowTitle("Step to GDML Converter")

        # delayed import
        from OCC.Display.qtDisplay import qtViewer3d
        self.viewer = qtViewer3d(self)
        # set background image?

        self.resize(1024, 768)

        self.create_menus()

        slayout = QtGui.QVBoxLayout()
        slayout.addWidget(QtGui.QLabel("WOO"))
        slayout.addWidget(QtGui.QLabel("YEAH"))

        layout = QtGui.QHBoxLayout()
        layout.addLayout(slayout, 0)
        layout.addSpacing(12)
        layout.addWidget(self.viewer, 3)

        mainwidget = QtGui.QWidget()
        mainwidget.setLayout(layout)
        self.setCentralWidget(mainwidget)

        self.load_dialog = None
        self.export_dialog = None

    def quit(self,evt=None):
        self.setVisible(False)
        sys.exit(0)

    def load_file_dialog(self, evt=None):
        def on_accept(evt=None):
            filenames = self.load_dialog.selectedFiles()
            if len(filenames) != 1:
                print("Odd file choice")
                return
            name = str(filenames[0])
            load_file(name)

        # make a global dialog registry:
        # call load_file_dialog("Name", *params, lambda file: ..)
        if self.load_dialog:
            ld.raise_()
        else:
            ld = QtGui.QFileDialog(self, "Load STEP File",
                                   QtCore.QDir.home().path(),
                                   "STEP file (*.stp *.step)")

            ld.setModal(True)

            self.connect(ld, QtCore.SIGNAL("accepted()"), on_accept)
            ld.show()
            self.load_dialog = ld

    def export_file_dialog(self, evt=None):
        def on_accept(evt=None):
            filenames = self.export_dialog.selectedFiles()
            if len(filenames) != 1:
                print("Odd file choice")
                return
            name = str(filenames[0])
            export_file(name)

        # make a global dialog registry:
        # call load_file_dialog("Name", *params, lambda file: ..)
        # but we only have two dialogs!
        if self.export_dialog:
            ld.raise_()
        else:
            ld = QtGui.QFileDialog(self, "Export GDML File",
                                   QtCore.QDir.home().path(),
                                   "GDML file (*.gdml)")

            ld.setAcceptMode(QtGui.QFileDialog.AcceptSave)
            ld.setModal(True)

            self.connect(ld, QtCore.SIGNAL("accepted()"), on_accept)
            ld.show()
            self.export_dialog = ld

    def create_menus(self):
        self.menu_bar = self.menuBar()
        menu = self.menu_bar.addMenu("&File")

        # add some declarative helpers
        # menu = [["File", ["Name", "Ctrl-C", looga]]...]

        def make_action(desc, shortcut, func):
            action = QtGui.QAction(desc, self)
            action.setShortcut(shortcut)
            self.connect(action, QtCore.SIGNAL("triggered()"), func)
            return action

        menu.addAction(make_action("Open STEP File...","Ctrl+O",self.load_file_dialog))
        menu.addAction(make_action("Export to GDML...","Ctrl+E",self.export_file_dialog))
        qaction = make_action("Quit!","Ctrl+Q",self.quit)
        qaction.setMenuRole(QtGui.QAction.QuitRole)
        menu.addAction(qaction)

        menu = self.menu_bar.addMenu("&Help")
        action = QtGui.QAction("Sorry! :-(", self)
        action.setDisabled(True)
        menu.addAction(action)

    def get_display(self):
        return self.viewer._display

def create_window(filename):
    # evaluate arguments!!

    def get_bg_filename():
        pack = sys.modules["OCC"]
        return os.path.join(pack.__path__[0], "Display", "default_background.bmp")

    global display
    app = QtGui.QApplication([])
    window = MainWindow()
    window.show()
    window.viewer.InitDriver()

    display = window.get_display()
    display.SetBackgroundImage(get_bg_filename())

    # maybe put it in a callback
    if filename:
        load_file(filename)

    app.exec_()

if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description='Convert STEP files to GDML!')
    parser.add_argument('inputfile', nargs='?', metavar="STEP-INPUT", help='input filename',)
    parser.add_argument('outputfile', nargs='?', metavar="GDML-OUTPUT", help="don't show display & output to file")
    args = parser.parse_args()

    if args.outputfile and args.inputfile:
        import_file(args.inputfile)
        export_file(args.outputfile)
    else:
        create_window(args.inputfile)
