/***************************************************************************
 *   Copyright (C) 2005-2007 by Grup de Gràfics de Girona                  *
 *   http://iiia.udg.es/GGG/index.html?langu=uk                            *
 *                                                                         *
 *   Universitat de Girona                                                 *
 ***************************************************************************/
#ifndef UDGDISTANCETOOL_H
#define UDGDISTANCETOOL_H

#include "tool.h"
#include <QPointer>

namespace udg {

class Q2DViewer;
class DrawerLine;

/**
    Eina de distancies.

	@author Grup de Gràfics de Girona  ( GGG ) <vismed@ima.udg.es>
*/
class DistanceTool : public Tool {
Q_OBJECT
public:
    DistanceTool( QViewer *viewer, QObject *parent = 0 );
    ~DistanceTool();

    /// Funcio manejadora dels events passats.
    void handleEvent( long unsigned eventID );

private:
    /// Ens permet anotar el seguent punt de la polilinia. Si la primitiva no ha sigut creada, abans d'afegir el nou punt, la crea.
    void annotateNewPoint();

    /// Simula la linia quan es mou el ratolí
    void simulateLine();

private slots:
    /// Inicialitza l'estat de la tool
    void initialize();

private:
    /// Estat de la línia
    enum { NoPointFixed, FirstPointFixed };

    /// Viewer 2D sobre el qual treballem
    Q2DViewer *m_2DViewer;

    /// Línia que es dibuixa
    QPointer<DrawerLine> m_line;

    /// Estat de la línia
    int m_lineState;

};
}

#endif
