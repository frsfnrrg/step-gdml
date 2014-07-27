TEMPLATE = app
CONFIG += debug_and_release qt

#################
#### TARGETS ####
#################

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

DESTDIR = .
OBJECTS_DIR = ./build/obj
MOC_DIR = ./build/moc
RCC_DIR = ./build/rcc

DEFINES = CSFDB QT_NO_DEPRECATED

##################
#### INCLUDES ####
##################

CASROOT = $$(CASROOT)
isEmpty (CASROOT) {
    CASROOT = /opt/OpenCASCADE
}
message (CASROOT is $$CASROOT)

INCLUDEPATH = $$CASROOT $$CASROOT/inc $(QTDIR)/include/QtCore \
              $(QTDIR)/include/QtGui $(QTDIR)/include
INCLUDEPATH += $$QMAKE_INCDIR_X11 $$QMAKE_INCDIR_OPENGL $$QMAKE_INCDIR_THREAD
DEFINES += LIN LININTEL OCC_CONVERT_SIGNALS HAVE_CONFIG_H HAVE_WOK_CONFIG_H

##############
#### LIBS ####
##############

# To place CASROOT before -L/usr/lib in case we override it
QMAKE_LFLAGS += -L$$CASROOT/lib

LIBS += -lTKernel -lPTKernel -lTKMath -lTKService -lTKV3d -lTKOpenGl \
        -lTKBRep -lTKIGES -lTKSTL -lTKVRML -lTKSTEP -lTKSTEPAttr -lTKSTEP209 \
        -lTKSTEPBase -lTKShapeSchema -lTKGeomBase -lTKGeomAlgo -lTKG3d -lTKG2d \
        -lTKXSBase -lTKPShape -lTKShHealing -lTKHLR -lTKTopAlgo -lTKMesh -lTKPrim \
        -lTKCDF -lTKBool -lTKBO -lTKFillet -lTKOffset
LIBS += -L/usr/lib -lQtCore -lQtGui
LIBS += -L$$QMAKE_LIBDIR_X11 $$QMAKE_LIBS_X11
LIBS += -L$$QMAKE_LIBDIR_OPENGL $$QMAKE_LIBS_OPENGL $$QMAKE_LIBS_THREAD

