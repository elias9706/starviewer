include(defaultdirectories.pri)

INCLUDEPATH += $${VTKINCLUDEDIR}

LIBS += -L$${VTKLIBDIR} \
        -lvtkCommon \
        -lvtkGenericFiltering \
        -lvtkGraphics \
        -lvtkHybrid \
        -lvtkImaging \
        -lvtkRendering \
        -lvtkVolumeRendering \
        -lvtksys \
        -lQVTK \
        -lvtkWidgets \
        -lvtkFiltering \
        -lvtkIO
 