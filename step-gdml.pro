TEMPLATE = app
CONFIG += debug_and_release qt

TARGET = step-gdml

HEADERS = src/util.h \
    src/window.h \
    src/view.h \
    src/translate.h \
    src/gdmlwriter.h
SOURCES = src/main.cpp \
    src/util.cpp \
    src/window.cpp \
    src/view.cpp \
    src/translate.cpp \
    src/gdmlwriter.cpp

DEFINES = CSFDB

CASROOT = /opt/OpenCASCADE

INCLUDEPATH = $$CASROOT $$CASROOT/inc $(QTDIR)/include/QtCore \
              $(QTDIR)/include/QtGui $(QTDIR)/include

DESTDIR = ./build/bin
OBJECTS_DIR = ./build/obj
MOC_DIR = ./build/moc

INCLUDEPATH += $$QMAKE_INCDIR_X11 $$QMAKE_INCDIR_OPENGL $$QMAKE_INCDIR_THREAD
DEFINES += LIN LININTEL OCC_CONVERT_SIGNALS HAVE_CONFIG_H HAVE_WOK_CONFIG_H QT_NO_STL
LIBS = -L$$CASROOT/lib -L$$QMAKE_LIBDIR_X11 $$QMAKE_LIBS_X11 -L$$QMAKE_LIBDIR_OPENGL $$QMAKE_LIBS_OPENGL $$QMAKE_LIBS_THREAD

FREEIMAGE_DIR = $$(FREEIMAGEDIR)
exists($$FREEIMAGE_DIR) {
    LIBS += -L$(FREEIMAGEDIR)/lib -lfreeimageplus
}
TBB_LIB = $$(TBBLIB)
exists($$TBB_LIB) {
    LIBS += -L$(TBBLIB) -ltbb -ltbbmalloc
}

LIBS += -lTKernel -lPTKernel -lTKMath -lTKService -lTKV3d -lTKV2d \
        -lTKBRep -lTKIGES -lTKSTL -lTKVRML -lTKSTEP -lTKSTEPAttr -lTKSTEP209 \
        -lTKSTEPBase -lTKShapeSchema -lTKGeomBase -lTKGeomAlgo -lTKG3d -lTKG2d \
        -lTKXSBase -lTKPShape -lTKShHealing -lTKHLR -lTKTopAlgo -lTKMesh -lTKPrim \
        -lTKCDF -lTKBool -lTKBO -lTKFillet -lTKOffset \
    -L$(QTDIR)/lib -lQtCore -lQtGui
