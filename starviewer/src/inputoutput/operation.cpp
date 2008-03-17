/***************************************************************************
 *   Copyright (C) 2005 by Grup de Gràfics de Girona                       *
 *   http://iiia.udg.es/GGG/index.html?langu=uk                            *
 *                                                                         *
 *   Universitat de Girona                                                 *
 ***************************************************************************/
#include "operation.h"

#include <QString>

#include "const.h"


namespace udg {

Operation::Operation()
{
    m_priority = 9999999;
    m_operation = operationUnknow;
}

Operation::~Operation()
{
}

bool Operation::operator < ( Operation ope ) const
{
    //ordena al reves, perque la prioritat més gran és la 0
    if ( ope.getPriority() < m_priority )
    {
        return true;
    }
    else return false;
}

bool Operation::operator ==(const Operation &operation)
{
    if(    (m_priority == operation.m_priority )
        && (m_operation == operation.m_operation)
        && (m_patientName == operation.m_patientName)
        && (m_patientID == operation.m_patientID)
        && (m_patientName == operation.m_patientName)
        && (m_studyID == operation.m_studyID)
        && (m_studyUID == operation.m_studyUID)
        && (m_pacsParameters == operation.m_pacsParameters)
        && (m_mask == operation.m_mask)
        )
        return true;
    else
        return false;
}

void Operation::setDicomMask( DicomMask mask )
{
    m_mask = mask;
}

void Operation::setPriority( int priority )
{
     if ( priority != operationPriorityHigh &&
          priority != operationPriorityMedium &&
          priority != operationPriorityLow )
     {
         m_priority = operationPriorityLow;
     }
     else m_priority = priority;
}

void Operation::setOperation( int operation )
{

    if ( operation > operationView )
    {
        m_operation = operationUnknow;
    }
    else m_operation = operation;

    // si no s'especifica la prioritat, el setOperation automaticament l'assigna en funció del tipus d'operacio
    if  ( m_priority > operationPriorityLow )
    {
        if ( operation == operationView )
        {
            m_priority = operationPriorityHigh;
        }
        else m_priority = operationPriorityMedium;
    }
}

void Operation::setPacsParameters( PacsParameters parameters )
{
    m_pacsParameters = parameters;
}

DicomMask Operation::getDicomMask()
{
    return m_mask;
}

int Operation::getPriority()
{
    return m_priority;
}

int Operation::getOperation()
{
    return m_operation;
}

PacsParameters Operation::getPacsParameters()
{
    return m_pacsParameters;
}

void Operation::setPatientName( QString patientName )
{
    m_patientName = patientName;
}

void Operation::setPatientID( QString patientID )
{
    m_patientID = patientID;
}

void Operation::setStudyID( QString studyID )
{
    m_studyID = studyID;
}

void Operation::setStudyUID( QString studyUID )
{
    m_studyUID = studyUID;
}

QString Operation::getPatientName()
{
    return m_patientName;
}

QString Operation::getPatientID()
{
    return m_patientID;
}

QString Operation::getStudyUID()
{
    return m_studyUID;
}

QString Operation::getStudyID()
{
    return m_studyID;
}

}
