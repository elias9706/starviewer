/***************************************************************************
 *   Copyright (C) 2005-2006 by Grup de Gràfics de Girona                  *
 *   http://iiia.udg.es/GGG/index.html?langu=uk                            *
 *                                                                         *
 *   Universitat de Girona                                                 *
 ***************************************************************************/
#ifndef UDGROTATE3DTOOL_H
#define UDGROTATE3DTOOL_H

#include "tool.h"

class vtkInteractorStyle;

namespace udg {

class QViewer;

/**
Eina per rotacions tridimensionals ( pensat per visors 3D )

	@author Grup de Gràfics de Girona  ( GGG ) <vismed@ima.udg.es>
*/
class Rotate3DTool : public Tool
{
Q_OBJECT
public:

    enum { None , Rotating, Spinning };
    Rotate3DTool( QViewer *viewer, QObject *parent = 0 );

    ~Rotate3DTool();

    void handleEvent( unsigned long eventID );

/// \TODO potser aquests mètodes slots passen a ser públics
private slots:
    /// Comença el translate
    void startRotate3D();

    /// Calcula el nou translate
    void doRotate3D();

    /// Atura l'estat de translate
    void endRotate3D();
    
private:
    /// interactor style que omplirem en el constructor depenent del visor
    vtkInteractorStyle *m_interactorStyle;
    
    ///estat de la tool
    int m_state;
};

}

#endif
