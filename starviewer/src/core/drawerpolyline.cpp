/***************************************************************************
 *   Copyright (C) 2005-2007 by Grup de Gràfics de Girona                  *
 *   http://iiia.udg.es/GGG/index.html?langu=uk                            *
 *                                                                         *
 *   Universitat de Girona                                                 *
 ***************************************************************************/
#include "drawerpolyline.h"
#include "logging.h"
#include "q2dviewer.h"
#include "mathtools.h"
// vtk
#include <vtkPolyData.h>
#include <vtkCellArray.h>
#include <vtkActor2D.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkProperty2D.h>
#include <vtkLine.h>
// qt
#include <QVector>

namespace udg {

DrawerPolyline::DrawerPolyline(QObject *parent)
 : DrawerPrimitive(parent), m_vtkPolydata(0), m_vtkPoints(0), m_vtkCellArray(0), m_vtkActor(0), m_vtkMapper(0)
{
    m_vtkActor = NULL;
}

DrawerPolyline::~DrawerPolyline()
{
    emit dying(this);

    if ( m_vtkPolydata )
        m_vtkPolydata->Delete();

    if ( m_vtkPoints )
        m_vtkPoints->Delete();

    if ( m_vtkCellArray )
        m_vtkCellArray->Delete();
}

void DrawerPolyline::addPoint( double point[3] )
{
    double *array = new double[3];
    for( int i = 0; i<3; i++ )
        array[i] = point[i];

    m_pointsList << array;
    emit changed();
}

void DrawerPolyline::setPoint( int i, double point[3] )
{
    if( i >= m_pointsList.count() || i < 0 )
        addPoint( point );
    else
    {
        double *array = new double[3];
        array = m_pointsList.takeAt(i);
        for( int j=0; j < 3; j++ )
            array[j] = point[j];

        m_pointsList.insert(i,array);
        emit changed();
    }
}

double* DrawerPolyline::getPoint( int position )
{
    if( position >= m_pointsList.count() )
    {
        double *array = new double[3];
        return array;
    }
   else
    {
        return m_pointsList.at( position );
    }
}

void DrawerPolyline::removePoint( int i )
{
    m_pointsList.removeAt( i );
    emit changed();
}

void DrawerPolyline::deleteAllPoints()
{
    m_pointsList.clear();
}

vtkProp *DrawerPolyline::getAsVtkProp()
{
    if( !m_vtkActor )
    {
        buildVtkPoints();
        // creem el pipeline de l'm_vtkActor
        m_vtkActor = vtkActor2D::New();
        m_vtkMapper = vtkPolyDataMapper2D::New();

        m_vtkActor->SetMapper( m_vtkMapper );
        m_vtkMapper->SetInput( m_vtkPolydata );
        // li donem els atributs
        updateVtkActorProperties();
    }
    return m_vtkActor;
}

double* DrawerPolyline::getPolylineBounds()
{
    return m_vtkPolydata->GetBounds();
}

void DrawerPolyline::update( int representation )
{
    switch( representation )
    {
    case VTKRepresentation:
        updateVtkProp();
    break;

    case OpenGLRepresentation:
    break;

    }
}

void DrawerPolyline::updateVtkProp()
{
    if( m_vtkActor )
    {
        m_vtkPolydata->Reset();
        buildVtkPoints();
        updateVtkActorProperties();
        this->setModified(false);
    }
    else
    {
        DEBUG_LOG("No es pot actualitzar la polilínia, ja que encara no s'ha creat!");
    }
}

void DrawerPolyline::buildVtkPoints()
{
    if( !m_vtkPolydata )
    {
        m_vtkPolydata = vtkPolyData::New();
        m_vtkPoints = vtkPoints::New();
        m_vtkCellArray = vtkCellArray::New();
    }

    //especifiquem el nombre de vèrtexs que té la polilinia
    int numberOfVertices = m_pointsList.count();
    m_vtkCellArray->InsertNextCell( numberOfVertices );
    m_vtkPoints->SetNumberOfPoints( numberOfVertices );

    //donem els punts
    int i = 0;
    foreach( double *vertix, m_pointsList )
    {
        m_vtkPoints->InsertPoint( i, vertix );
        m_vtkCellArray->InsertCellPoint( i );
        i++;
    }

    //assignem els punts al polydata
    m_vtkPolydata->SetPoints( m_vtkPoints );

    m_vtkPolydata->SetLines( m_vtkCellArray );
}

void DrawerPolyline::updateVtkActorProperties()
{
    // sistema de coordenades
    m_vtkMapper->SetTransformCoordinate( this->getVtkCoordinateObject() );
    // estil de la línia
    m_vtkActor->GetProperty()->SetLineStipplePattern( m_linePattern );
    //Assignem gruix de la línia
    m_vtkActor->GetProperty()->SetLineWidth( m_lineWidth );
    //Assignem opacitat de la línia
    m_vtkActor->GetProperty()->SetOpacity( m_opacity );
    //mirem la visibilitat de l'm_vtkActor
    m_vtkActor->SetVisibility( this->isVisible() );
    //Assignem color
    QColor color = this->getColor();
    m_vtkActor->GetProperty()->SetColor( color.redF(), color.greenF(), color.blueF() );
}

int DrawerPolyline::getNumberOfPoints()
{
    return m_pointsList.count();
}

void DrawerPolyline::swap()
{
    for ( int i = 0; i < (int)(m_pointsList.count()/2); i++ )
        m_pointsList.swap( i, (m_pointsList.count()-1)-i );
}

double DrawerPolyline::getDistanceToPoint( double *point3D )
{
    double minDistanceLine = VTK_DOUBLE_MAX;
    double distance;
    bool found = false;

    if ( !m_pointsList.isEmpty() )
    {
        //mirem si el polígon conté com a últim punt el primer punt, és a dir, si està tancat o no.
        //ens cal que sigui tancat per a dibuixar tots els segments reals que el formen.
        QList< double* > auxList;
        auxList += m_pointsList;

        if ( auxList.first()[0] != auxList.last()[0] || auxList.first()[1] != auxList.last()[1] || auxList.first()[2] != auxList.last()[2] )
        {
            //si el primer i últim punt no són iguals, dupliquem el primer punt.
            auxList << auxList.first();
        }

        for ( int i = 0; ( i < auxList.count() - 1 ) && !found ; i++ )
        {
            if ( isPointIncludedInLineBounds( point3D, auxList[i], auxList[i+1] ) )
            {
                minDistanceLine = 0.0;
                found = true;
            }
            else
            {
                distance = vtkLine::DistanceToLine( point3D , auxList[i] , auxList[i+1] );

                if ( minDistanceLine == VTK_DOUBLE_MAX )
                    minDistanceLine = distance;
                else if ( distance < minDistanceLine )
                        minDistanceLine = distance;
            }
        }
    }
    return minDistanceLine;
}

bool DrawerPolyline::isPointIncludedInLineBounds( double point[3], double *lineP1, double *lineP2 )
{
    double range = 10.0;

    return ( MathTools::getDistance3D( point, lineP1 ) <= range || MathTools::getDistance3D( point, lineP2 ) <= range );
}

bool DrawerPolyline::isInsideOfBounds( double p1[3], double p2[3], int view )
{
    double minX, maxX, minY, maxY;

    int numberOfPoints = m_pointsList.count();
    bool allPointsAreInside = true;

    //determinem x i y màximes i mínimes segons la vista
    switch( view )
    {
        case Q2DViewer::AxialPlane:
            if ( p1[0] < p2[0] )
            {
                minX = p1[0];
                maxX = p2[0];
            }
            else
            {
                maxX = p1[0];
                minX = p2[0];
            }

            if ( p1[1] < p2[1] )
            {
                minY = p1[1];
                maxY = p2[1];
            }
            else
            {
                maxY = p1[1];
                minY = p2[1];
            }

            for (int i = 0; i < numberOfPoints && allPointsAreInside; i++ )
            {
                double *currentPoint = m_pointsList.at( i );
                if ( currentPoint[0] >= maxX || currentPoint[0] <= minX || currentPoint[1] >= maxY || currentPoint[1] <= minY )
                    allPointsAreInside = false;
            }
            break;
        case Q2DViewer::SagitalPlane:
            if ( p1[2] < p2[2] )
            {
                minX = p1[2];
                maxX = p2[2];
            }
            else
            {
                maxX = p1[2];
                minX = p2[2];
            }

            if ( p1[1] < p2[1] )
            {
                minY = p1[1];
                maxY = p2[1];
            }
            else
            {
                maxY = p1[1];
                minY = p2[1];
            }

            for (int i = 0; i < numberOfPoints && allPointsAreInside; i++ )
            {
                double *currentPoint = m_pointsList.at( i );
                if ( currentPoint[2] >= maxX || currentPoint[2] <= minX || currentPoint[1] >= maxY || currentPoint[1] <= minY )
                    allPointsAreInside = false;
            }
            break;
        case Q2DViewer::CoronalPlane:
            if ( p1[0] < p2[0] )
            {
                minX = p1[0];
                maxX = p2[0];
            }
            else
            {
                maxX = p1[0];
                minX = p2[0];
            }

            if ( p1[2] < p2[2] )
            {
                minY = p1[2];
                maxY = p2[2];
            }
            else
            {
                maxY = p1[2];
                minY = p2[2];
            }
            for (int i = 0; i < numberOfPoints && allPointsAreInside; i++ )
            {
                double *currentPoint = m_pointsList.at( i );
                if ( currentPoint[0] >= maxX || currentPoint[0] <= minX || currentPoint[2] >= maxY || currentPoint[2] <= minY )
                    allPointsAreInside = false;
            }
            break;
    }

    return ( allPointsAreInside );
}

double DrawerPolyline::computeArea( int view, const double * spacing )
{
    double area = 0.0;
    double actualPoint[3];
    double followPoint[3];
    double * point;
    for ( int j = 0; j < m_pointsList.count()-1; j++ )
    {
        point = m_pointsList.at( j );
        actualPoint[0] = point[0];
        actualPoint[1] = point[1];
        actualPoint[2] = point[2];

        point = m_pointsList.at( j+1 );
        followPoint[0] = point[0];
        followPoint[1] = point[1];
        followPoint[2] = point[2];

        if ( spacing != NULL )
        {
            actualPoint[0] = MathTools::trunc( actualPoint[0]/spacing[0] );
            actualPoint[1] = MathTools::trunc( actualPoint[1]/spacing[1] );
            actualPoint[2] = MathTools::trunc( actualPoint[2]/spacing[2] );
            followPoint[0] = MathTools::trunc( followPoint[0]/spacing[0] );
            followPoint[1] = MathTools::trunc( followPoint[1]/spacing[1] );
            followPoint[2] = MathTools::trunc( followPoint[2]/spacing[2] );
        }
        switch( view )
        {
            case Q2DViewer::Axial:
                area += ( ( followPoint[0]-actualPoint[0] )*(followPoint[1] + actualPoint[1] ) )/2.0;
                break;

            case Q2DViewer::Sagital:
                area += ( ( followPoint[2]-actualPoint[2] )*(followPoint[1] + actualPoint[1] ) )/2.0;
                break;

            case Q2DViewer::Coronal:
                area += ( ( followPoint[0]-actualPoint[0] )*(followPoint[2] + actualPoint[2] ) )/2.0;
                break;
        }
    }

     //en el cas de que l'àrea de la polilínia ens doni negativa, vol dir que hem anotat els punts en sentit antihorari,
     //per això cal girar-los per tenir una disposició correcta. Cal girar-ho del vtkPoints i de la QList de la ROI
     if ( area < 0 )
     {
        //donem el resultat el valor absolut
        area *= -1;

        //intercanviem els punts de la QList
        swap();
    }
    return area;
}

}
