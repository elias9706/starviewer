# Fitxer generat pel gestor de qmake de kdevelop. 
# ------------------------------------------- 
# Subdirectori relatiu al directori principal del projecte: ./src/extensions/segmentationframework
# L'objectiu és una biblioteca:  

FORMS += qsegmentationframeworkextensionbase.ui 
HEADERS += qsegmentationframeworkextension.h \
           segmentationframeworkextensionmediator.h  \
           connectedthreshold.h \
           confidenceconnected.h \
           isolatedconnected.h \
           neighborhoodconnected.h \
           volumcalculator.h \
           contourntool.h \
           determinatecontour.h \
           areaspline.h \
           llescacontorn.h \
           llenca.h
SOURCES += qsegmentationframeworkextension.cpp \
           segmentationframeworkextensionmediator.cpp  \
           connectedthreshold.cpp \
           confidenceconnected.cpp \
           isolatedconnected.cpp \
           neighborhoodconnected.cpp \
           volumcalculator.cpp \
           contourntool.cpp \
           determinatecontour.cpp \
           areaspline.cpp \
           llescacontorn.cpp
TARGETDEPS += ../../../core/libcore.a
LIBS += ../../../core/libcore.a
INCLUDEPATH += ../../../core
MOC_DIR = ../../../../tmp/moc
UI_DIR = ../../../../tmp/ui
OBJECTS_DIR = ../../../../tmp/obj
QMAKE_CXXFLAGS_RELEASE += -Wno-deprecated
QMAKE_CXXFLAGS_DEBUG += -Wno-deprecated
CONFIG += warn_on \
qt \
opengl \
thread \
x11 \
staticlib \
exceptions \
stl
TEMPLATE = lib
include(../../../vtk.inc)
include(../../../itk.inc)
include(../../../dcmtk.inc)
include(../../../compilationtype.inc)
