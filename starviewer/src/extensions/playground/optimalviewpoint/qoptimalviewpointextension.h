/***************************************************************************
 *   Copyright (C) 2007 by Grup de Gràfics de Girona                       *
 *   http://iiia.udg.edu/GGG/index.html                                    *
 *                                                                         *
 *   Universitat de Girona                                                 *
 ***************************************************************************/


#ifndef UDGQOPTIMALVIEWPOINTEXTENSION_H
#define UDGQOPTIMALVIEWPOINTEXTENSION_H


#include "ui_qoptimalviewpointextensionbase.h"


namespace udg {


class OptimalViewpoint;
class OptimalViewpointParameters;

class Volume;


/**
 * Extensió OptimalViewpoint.
 *
 * Aquesta extensió permet aplicar diversos càlculs de mesures de la teoria de
 * la informació sobre el volum. Algunes d'aquestes mesures (entropy rate i
 * excess entropy) poden ajudar a trobar el millor punt de vista del volum
 * (d'aquí ve el nom de l'extensió), però l'extensió simplement calcula les
 * mesures i en mostra els resultats a l'usuari.
 *
 * Hi ha altres operacions que es poden aplicar sobre un volum des d'un punt de
 * vista seleccionat, com ara reconstruir-lo en llesques en aquella direcció i
 * agrupar les llesques semblants.
 *
 * Addicionalment, es visualitza el volum en 3D i es poden configurar diferents
 * efectes de visualització i la funció de transferència per a obtenir la
 * visualització desitjada.
 *
 * \author Grup de Gràfics de Girona (GGG) <vismed@ima.udg.edu>
*/
class QOptimalViewpointExtension
    : public QWidget, private ::Ui::QOptimalViewpointExtensionBase {

    Q_OBJECT

public:

    QOptimalViewpointExtension( QWidget * parent = 0 );
    virtual ~QOptimalViewpointExtension();

    /// Assigna el volum amb el qual treballarà l'extensió.
    void setInput( Volume * input );

public slots:

    /// Fa la segmentació segons els paràmetres actuals.
    void doSegmentation();

    void doVisualization();

    void doViewpointSelection();
    void computeViewpointEntropies();

    ///Aplica el mètode segons els paràmetres actuals.
    void execute();

    /// Assinge el rang escalar als editors de la funció de transferència.
    void setScalarRange( unsigned char rangeMin, unsigned char rangeMax );

    /**
     * Slot que ens serveix per indicar que hem d'actualitzar el paràmetre que
     * ens diguin mitjançant un identificador (que, en realitat, serà un enum).
     * Serveix per canviar els valors a partir d'una classe Parameters.
     */
    void readParameter( int index );

    /**
     * Escriu tots els valors de paràmetres que té actualment al Parameters
     * associat.
     */
    void writeAllParameters();

private slots:

    void openSegmentationFile();
    void toggleSegmentationParameters();
    void computeObscurances();
    void computeSaliency();
    void renderPlane();
    void setNumberOfPlanes( const QString & numberOfPlanes );

private:

    void createConnections();

    /// Paràmetres del mètode.
    OptimalViewpointParameters * m_parameters;
    /// Mètode.
    OptimalViewpoint * m_method;
    /// Serà cert quan l'usuari hagi triat el fitxer de segmentació.
    bool m_segmentationFileChosen;

};


}


#endif
