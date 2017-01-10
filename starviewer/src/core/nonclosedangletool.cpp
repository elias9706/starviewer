/*************************************************************************************
  Copyright (C) 2014 Laboratori de Gràfics i Imatge, Universitat de Girona &
  Institut de Diagnòstic per la Imatge.
  Girona 2014. All rights reserved.
  http://starviewer.udg.edu

  This file is part of the Starviewer (Medical Imaging Software) open source project.
  It is subject to the license terms in the LICENSE file found in the top-level
  directory of this distribution and at http://starviewer.udg.edu/license. No part of
  the Starviewer (Medical Imaging Software) open source project, including this file,
  may be copied, modified, propagated, or distributed except according to the
  terms contained in the LICENSE file.
 *************************************************************************************/

#include "nonclosedangletool.h"
#include "q2dviewer.h"
#include "drawer.h"
#include "drawerline.h"
#include "drawertext.h"
#include "mathtools.h"
// VTK
#include <vtkRenderWindowInteractor.h>
#include <vtkCommand.h>

namespace udg {

NonClosedAngleTool::NonClosedAngleTool(QViewer *viewer, QObject *parent)
 : Tool(viewer, parent), m_firstLine(0), m_secondLine(0), m_state(None), m_lineState(NoPoints)
{
    m_toolName = "NonClosedAngleTool";
    m_hasSharedData = false;

    m_2DViewer = Q2DViewer::castFromQViewer(viewer);

    connect(m_2DViewer, SIGNAL(volumeChanged(Volume*)), SLOT(initialize()));
}

NonClosedAngleTool::~NonClosedAngleTool()
{
    deleteTemporalRepresentation();
}

void NonClosedAngleTool::deleteTemporalRepresentation()
{
    bool hasToRefresh = false;
    // Cal decrementar el reference count perquè
    // l'annotació s'esborri si "matem" l'eina
    if (m_firstLine)
    {
        m_firstLine->decreaseReferenceCount();
        delete m_firstLine;
        hasToRefresh = true;
    }

    if (m_secondLine)
    {
        m_secondLine->decreaseReferenceCount();
        delete m_secondLine;
        hasToRefresh = true;
    }

    if (hasToRefresh)
    {
        m_2DViewer->render();
    }

    m_state = None;
    m_lineState = NoPoints;
}

void NonClosedAngleTool::handleEvent(long unsigned eventID)
{
    switch (eventID)
    {
        case vtkCommand::LeftButtonPressEvent:
            handlePointAddition();
            break;

        case vtkCommand::MouseMoveEvent:
            handleLineDrawing();
            break;
        case vtkCommand::KeyPressEvent:
            int keyCode = m_2DViewer->getInteractor()->GetKeyCode();
            if (keyCode == 27) // ESC
            {
                deleteTemporalRepresentation();
            }
            break;
    }
}

void NonClosedAngleTool::handlePointAddition()
{
    if (m_2DViewer->hasInput())
    {
        if (m_2DViewer->getInteractor()->GetRepeatCount() == 0)
        {
            this->annotateLinePoints();

            if (m_state == SecondLineFixed)
            {
                computeAngle();
                // Restaurem m_state
                m_state = None;
            }
        }
    }
}

void NonClosedAngleTool::annotateLinePoints()
{
    DrawerLine *line;

    // Creem primera o segona línies
    if ((m_state == None && m_lineState == NoPoints) || (m_state == FirstLineFixed && m_lineState == NoPoints))
    {
        line = new DrawerLine;
        // Així evitem que la primitiva pugui ser esborrada durant l'edició per events externs
        line->increaseReferenceCount();
    }
    else if (m_state == None)
    {
        line = m_firstLine;
    }
    else
    {
        line = m_secondLine;
    }

    Vector3 clickedWorldPoint = m_2DViewer->getEventWorldCoordinate();

    // Afegim el punt
    if (m_lineState == NoPoints)
    {
        line->setFirstPoint(clickedWorldPoint);
        line->setSecondPoint(clickedWorldPoint);
        m_lineState = FirstPoint;

        if (m_state == None)
        {
            m_firstLine = line;
        }
        else
        {
            m_secondLine = line;
        }

        m_2DViewer->getDrawer()->draw(line);
    }
    else
    {
        line->setSecondPoint(clickedWorldPoint);
        line->update();

        m_lineState = NoPoints;

        if (m_state == None)
        {
            m_state = FirstLineFixed;
        }
        else
        {
            m_state = SecondLineFixed;
        }
    }
}

void NonClosedAngleTool::handleLineDrawing()
{
    if (m_firstLine && m_state == None)
    {
        this->simulateLine(m_firstLine);
    }
    else if (m_secondLine && m_state == FirstLineFixed)
    {
        this->simulateLine(m_secondLine);
    }
}

void NonClosedAngleTool::simulateLine(DrawerLine *line)
{
    Vector3 clickedWorldPoint = m_2DViewer->getEventWorldCoordinate();
    line->setSecondPoint(clickedWorldPoint);
    // Actualitzem viewer
    line->update();
    m_2DViewer->render();
}

void NonClosedAngleTool::computeAngle()
{
    equalizeDepth();

    if (!m_middleLine)
    {
        m_middleLine = new DrawerLine;
        // Així evitem que la primitiva pugui ser esborrada durant l'edició per events externs
        m_middleLine->increaseReferenceCount();
    }
    m_middleLine->setColor(QColor(191, 147, 64));
    m_middleLine->setOpacity(0.5);

    auto point1 = m_firstLine->getFirstPoint();
    auto point2 = m_firstLine->getSecondPoint();
    auto point3 = m_secondLine->getFirstPoint();
    auto point4 = m_secondLine->getSecondPoint();

    int state;

    Vector3 intersection = MathTools::infiniteLinesIntersection(point1, point2, point3, point4, state);

    double distance1 = MathTools::getDistance3D(intersection, point1);
    double distance2 = MathTools::getDistance3D(intersection, point2);
    double distance3 = MathTools::getDistance3D(intersection, point3);
    double distance4 = MathTools::getDistance3D(intersection, point4);

    Vector3 directorVector1;
    Vector3 directorVector2;
    // Per calcular el vectors directors farem servir la intersecció i el punt
    // més llunyà a la intersecció de cada recta ja que si per alguna casualitat
    // l'usuari fa coincidir un dels punts de cada recta, la distància seria de 0
    // i com a conseqüència l'angle calculat ens sortiria Nan
    if (distance1 <= distance2)
    {
        if (distance3 <= distance4)
        {
            directorVector1 = MathTools::directorVector(point2, intersection);
            directorVector2 = MathTools::directorVector(point4, intersection);
            m_middleLine->setFirstPoint(point1);
            m_middleLine->setSecondPoint(point3);
        }
        else
        {
            directorVector1 = MathTools::directorVector(point2, intersection);
            directorVector2 = MathTools::directorVector(point3, intersection);
            m_middleLine->setFirstPoint(point1);
            m_middleLine->setSecondPoint(point4);
        }
    }
    else
    {
        if (distance3 <= distance4)
        {
            directorVector1 = MathTools::directorVector(point1, intersection);
            directorVector2 = MathTools::directorVector(point4, intersection);
            m_middleLine->setFirstPoint(point2);
            m_middleLine->setSecondPoint(point3);
        }
        else
        {
            directorVector1 = MathTools::directorVector(point1, intersection);
            directorVector2 = MathTools::directorVector(point3, intersection);
            m_middleLine->setFirstPoint(point2);
            m_middleLine->setSecondPoint(point4);
        }
    }

    // Alliberem les primitives perquè puguin ser esborrades
    m_firstLine->decreaseReferenceCount();
    m_secondLine->decreaseReferenceCount();
    m_middleLine->decreaseReferenceCount();
    
    // Col·loquem cada línia al pla que toca
    m_2DViewer->getDrawer()->erasePrimitive(m_firstLine);
    m_2DViewer->getDrawer()->erasePrimitive(m_secondLine);
    m_2DViewer->getDrawer()->draw(m_firstLine, m_2DViewer->getView(), m_2DViewer->getCurrentSlice());
    m_2DViewer->getDrawer()->draw(m_secondLine, m_2DViewer->getView(), m_2DViewer->getCurrentSlice());
    m_2DViewer->getDrawer()->draw(m_middleLine, m_2DViewer->getView(), m_2DViewer->getCurrentSlice());

    if (fabs(directorVector1.x) < 0.0001)
    {
        directorVector1.x = 0.0;
    }
    if (fabs(directorVector1.y) < 0.0001)
    {
        directorVector1.y = 0.0;
    }
    if (fabs(directorVector1.z) < 0.0001)
    {
        directorVector1.z = 0.0;
    }

    if (fabs(directorVector2.x) < 0.0001)
    {
        directorVector2.x = 0.0;
    }
    if (fabs(directorVector2.y) < 0.0001)
    {
        directorVector2.y = 0.0;
    }
    if (fabs(directorVector2.z) < 0.0001)
    {
        directorVector2.z = 0.0;
    }

    double angle = MathTools::angleInDegrees(directorVector1, directorVector2);

    DrawerText *text = new DrawerText;

    if (state == MathTools::ParallelLines)
    {
        text->setText(tr("0.0 degrees"));
    }
    // Won't occur
    else if (state == MathTools::SkewIntersection)
    {
        text->setText(tr("Skew lines"));
    }
    else
    {
        text->setText(tr("%1 degrees").arg(angle, 0, 'f', 1));
    }

    placeText(m_middleLine->getFirstPoint().toArray().data(), m_middleLine->getSecondPoint().toArray().data(), text);
    m_2DViewer->getDrawer()->draw(text, m_2DViewer->getView(), m_2DViewer->getCurrentSlice());

    // Re-iniciem els punters
    m_firstLine = NULL;
    m_secondLine = NULL;
    m_middleLine = NULL;
}

void NonClosedAngleTool::placeText(const Vector3 &firstLineVertex, const Vector3 &secondLineVertex, DrawerText *angleText)
{
    auto position = (firstLineVertex + secondLineVertex) / 2.0;
    angleText->setAttachmentPoint(position);
}

void NonClosedAngleTool::initialize()
{
    // Alliberem les primitives perquè puguin ser esborrades
    if (m_firstLine)
    {
        m_firstLine->decreaseReferenceCount();
        delete m_firstLine;
    }

    if (m_secondLine)
    {
        m_secondLine->decreaseReferenceCount();
        delete m_secondLine;
    }

    m_firstLine = NULL;
    m_secondLine = NULL;
    m_state = None;
    m_lineState = NoPoints;
}

void NonClosedAngleTool::equalizeDepth()
{
    auto point = m_firstLine->getFirstPoint();
    auto pointDisplay = m_2DViewer->computeWorldToDisplay(point);
    pointDisplay.z = 0.0;
    point = m_2DViewer->computeDisplayToWorld(pointDisplay);
    m_firstLine->setFirstPoint(point);

    point = m_firstLine->getSecondPoint();
    pointDisplay = m_2DViewer->computeWorldToDisplay(point);
    pointDisplay.z = 0.0;
    point = m_2DViewer->computeDisplayToWorld(pointDisplay);
    m_firstLine->setSecondPoint(point);

    m_firstLine->update();

    point = m_secondLine->getFirstPoint();
    pointDisplay = m_2DViewer->computeWorldToDisplay(point);
    pointDisplay.z = 0.0;
    point = m_2DViewer->computeDisplayToWorld(pointDisplay);
    m_secondLine->setFirstPoint(point);

    point = m_secondLine->getSecondPoint();
    pointDisplay = m_2DViewer->computeWorldToDisplay(point);
    pointDisplay.z = 0.0;
    point = m_2DViewer->computeDisplayToWorld(pointDisplay);
    m_secondLine->setSecondPoint(point);

    m_secondLine->update();
}

}
