# Fitxer generat pel gestor de qmake de kdevelop. 
# ------------------------------------------- 
# Subdirectori relatiu al directori principal del projecte: ./src/extensions/playground/multiq2dviewer
# L'objectiu �s una biblioteca:  

HEADERS += multiq2dviewerextensionmediator.h \
           patientitemmodel.h \
           patientitem.h \
           interactivewidget.h
SOURCES += multiq2dviewerextensionmediator.cpp \
           patientitemmodel.cpp \
           patientitem.cpp \
           interactivewidget.cpp
TARGETDEPS += ../../../core/libcore.a
LIBS += ../../../core/libcore.a
INCLUDEPATH += ../../../core
MOC_DIR = ../../../../tmp/moc
UI_DIR = ../../../../tmp/ui
OBJECTS_DIR = ../../../../tmp/obj
QMAKE_CXXFLAGS_RELEASE += -Wno-deprecated
QMAKE_CXXFLAGS_DEBUG += -Wno-deprecated
CONFIG += release \
warn_on \
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
