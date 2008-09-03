/***************************************************************************
 *   Copyright (C) 2005 by Grup de Gràfics de Girona                       *
 *   http://iiia.udg.es/GGG/index.html?langu=uk                            *
 *                                                                         *
 *   Universitat de Girona                                                 *
 ***************************************************************************/
#include "extensionhandler.h"

// qt
#include <QFileInfo>
#include <QDir>
#include <QProgressDialog>
#include <QMessageBox>
// recursos
#include "logging.h"
#include "extensionworkspace.h"
#include "qapplicationmainwindow.h"
#include "volumerepository.h" // per esborrar els volums
#include "extensionmediatorfactory.h"
#include "extensionfactory.h"
#include "extensioncontext.h"
#include "singleton.h"

// Espai reservat pels include de les mini-apps
#include "appimportfile.h"

// Fi de l'espai reservat pels include de les mini-apps

// PACS --------------------------------------------
#include "queryscreen.h"
#include "patientfiller.h"
#include "patientfillerinput.h"

namespace udg {

typedef SingletonPointer<QueryScreen> QueryScreenSingleton;

ExtensionHandler::ExtensionHandler( QApplicationMainWindow *mainApp , QObject *parent, QString name)
 : QObject(parent )
{
    this->setObjectName( name );
    m_mainApp = mainApp;

    // Aquí en principi només farem l'inicialització
    m_importFileApp = new AppImportFile;

    createConnections();
}

ExtensionHandler::~ExtensionHandler()
{
    // Si és la última finestra oberta, hem de tancar la queryscreen
    if (m_mainApp->getCountQApplicationMainWindow() == 1)
    {
        QueryScreenSingleton::instance()->close();
    }
}

void ExtensionHandler::request( int who )
{
    // \TODO: crear l'extensió amb el factory ::createExtension, no com està ara
    // \TODO la numeració és completament temporal!!! s'haurà de canviar aquest sistema
    STAT_LOG("Activated open " + QString::number(who));
    switch( who )
    {
        case 1:
            m_importFileApp->open();
            break;

        case 6:
            m_importFileApp->openDirectory();
            break;

        case 7:
            // HACK degut a que la QueryScreen és un singleton, això provoca efectes colaterals quan teníem
            // dues finestres ( mirar ticket #542 ). Fem aquest petit hack perquè això no passi.
            // Queda pendent resoldre-ho de la forma adequada
#ifdef NEW_PACS
            disconnect(QueryScreenSingleton::instance(), SIGNAL( selectedPatient(Patient*, QString) ), 0,0 );
            QueryScreenSingleton::instance()->bringToFront();
            connect(QueryScreenSingleton::instance(), SIGNAL( selectedPatient(Patient*, QString) ), SLOT(processInput(Patient*, QString)));
#else
            disconnect( QueryScreenSingleton::instance(), SIGNAL(processFiles(QStringList,QString,QString,QString) ), 0,0 );
            QueryScreenSingleton::instance()->bringToFront();
            connect( QueryScreenSingleton::instance(), SIGNAL(processFiles(QStringList,QString,QString,QString)), SLOT(processInput(QStringList,QString,QString,QString)) );
#endif
            break;

        case 8:
            // HACK degut a que la QueryScreen és un singleton, això provoca efectes colaterals quan teníem
            // dues finestres ( mirar ticket #542 ). Fem aquest petit hack perquè això no passi.
            // Queda pendent resoldre-ho de la forma adequada
            disconnect( QueryScreenSingleton::instance(), SIGNAL(processFiles(QStringList,QString,QString,QString) ), 0,0 );
            QueryScreenSingleton::instance()->openDicomdir();
            connect( QueryScreenSingleton::instance(), SIGNAL(processFiles(QStringList,QString,QString,QString)), SLOT(processInput(QStringList,QString,QString,QString)) );
            break;
    }
}

void ExtensionHandler::request( const QString &who )
{
    QWidget *extension = ExtensionFactory::instance()->create(who);
    ExtensionMediator* mediator = ExtensionMediatorFactory::instance()->create(who);

    if (mediator && extension)
    {
        STAT_LOG("Activated extension " + who);
        mediator->initializeExtension(extension, m_extensionContext );
        m_mainApp->getExtensionWorkspace()->addApplication(extension, mediator->getExtensionID().getLabel() );
    }
    else
    {
        DEBUG_LOG( "Error carregant " + who );
    }
}

void ExtensionHandler::killBill()
{
    // eliminem totes les extensions
    m_mainApp->getExtensionWorkspace()->killThemAll();
    // TODO descarregar tots els volums que tingui el pacient en aquesta finestra
    // quan ens destruim alliberem tots els volums que hi hagi a memòria
    if (m_mainApp->getCurrentPatient() != NULL)
    {
        foreach( Study *study, m_mainApp->getCurrentPatient()->getStudies() )
        {
            foreach( Series *series, study->getSeries() )
            {
                foreach( Identifier id, series->getVolumesIDList()  )
                {
                    VolumeRepository::getRepository()->removeVolume( id );
                }
            }
        }
    }
}

void ExtensionHandler::setContext( const ExtensionContext &context )
{
    m_extensionContext = context;
}

ExtensionContext &ExtensionHandler::getContext()
{
    return m_extensionContext;
}

void ExtensionHandler::updateConfiguration(const QString &configuration)
{
    QueryScreenSingleton::instance()->updateConfiguration(configuration);
}

void ExtensionHandler::createConnections()
{
    connect( m_importFileApp,SIGNAL( selectedFiles(QStringList) ), SLOT(processInput(QStringList) ) );
}

void ExtensionHandler::processInput( QStringList inputFiles, QString defaultStudyUID, QString defaultSeriesUID, QString defaultImageInstance )
{
    Q_UNUSED(defaultImageInstance);
    PatientFillerInput *fillerInput = new PatientFillerInput;
    fillerInput->setFilesList( inputFiles );

    QProgressDialog progressDialog( m_mainApp );
    progressDialog.setModal( true );
    progressDialog.setRange(0, 0);
    progressDialog.setMinimumDuration( 0 );
    progressDialog.setWindowTitle( tr("Patient loading") );
    progressDialog.setLabelText( tr("Loading, please wait...") );
    progressDialog.setCancelButton( 0 );

    PatientFiller patientFiller;
    connect(&patientFiller, SIGNAL( progress(int) ), &progressDialog, SLOT( setValue(int) ));

    qApp->processEvents();

    patientFiller.fill( fillerInput );

    progressDialog.close();

    unsigned int numberOfPatients = fillerInput->getNumberOfPatients();

    if (numberOfPatients == 0)
    {
        QMessageBox::critical(0, tr("Starviewer"), tr("Sorry, it seems that there is no patient data we can load."));
        ERROR_LOG("Error fent el fill de patientFiller. Ha retornat 0 pacients.");
        return;
    }

    DEBUG_LOG( "Labels: " + fillerInput->getLabels().join("; "));
    DEBUG_LOG( QString("getNumberOfPatients: %1").arg( numberOfPatients ) );

    QList<int> correctlyLoadedPatients;

    for(unsigned int i = 0; i < numberOfPatients; i++ )
    {
        DEBUG_LOG( QString("Patient #%1\n %2").arg(i).arg( fillerInput->getPatient(i)->toString() ) );

        // marquem les series seleccionades
        Study *study = NULL;
        study = fillerInput->getPatient(i)->getStudy( defaultStudyUID );
        // Si no ens diuen un study seleccionat en seleccionem un nosaltres. Això pot ser perquè l'uid d'estudi sigui d'un altre pacient o perquè l'uid està buit
        if(!study)
        {
            QList<Study *> studyList = fillerInput->getPatient(i)->getStudies();
            if ( !studyList.isEmpty() )
            {
                study = studyList.first();
            }
        }

        bool error = true;
        if( study )
        {
            Series *series = NULL;
            series = fillerInput->getPatient(i)->getSeries( defaultSeriesUID );
            // si aquest ens "falla" és perquè possiblement l'UID és d'un altre estudi
            // o perquè l'uid és buit, per tant no tenim cap predilecció i escollim el primer
            if( !series ) // No tenim cap serie seleccionada, seleccionem per defecte la primera
            {
                QList<Series *> seriesList = study->getSeries();
                if( !seriesList.isEmpty() )
                {
                    series = seriesList.first();
                }
            }

            if (series)
            {
                series->select();
                error = false;
            }
            else
            {
                ERROR_LOG(fillerInput->getPatient(i)->toString());
                ERROR_LOG("Error carregant aquest pacient. La serie retornada és null. defaultSeriesUID: " + defaultSeriesUID);
            }
        }
        else
        {
            ERROR_LOG(fillerInput->getPatient(i)->toString());
            ERROR_LOG("Error carregant aquest pacient. L'study retornat és null. defaultStudyUID: " + defaultStudyUID);
        }

        if (!error)
        {
            correctlyLoadedPatients << i;
        }
    }

    QString patientsWithError;
    if (correctlyLoadedPatients.count() != numberOfPatients)
    {
        for (unsigned int i = 0; i < numberOfPatients; i++ )
        {
            if (!correctlyLoadedPatients.contains(i))
            {
                patientsWithError += "- " + fillerInput->getPatient(i)->getFullName() + "; ID: " + fillerInput->getPatient(i)->getID() + "<br>";
            }
        }
        QMessageBox::critical(0, tr("Starviewer"), tr("Sorry, an error ocurred while loading the data of patients:<br> %1").arg(patientsWithError) );
    }

    // Si de tots els pacients que es carreguen intentem carregar-ne un d'igual al que ja tenim carregat, el mantenim
    bool canReplaceActualPatient = true;
    if (m_mainApp->getCurrentPatient())
    {
        foreach (int i, correctlyLoadedPatients)
        {
            if (m_mainApp->getCurrentPatient()->compareTo( fillerInput->getPatient(i) ) == Patient::SamePatients )
            {
                canReplaceActualPatient = false;
                break;
            }
        }
    }

    // Afegim els pacients carregats correctament
    foreach (int i, correctlyLoadedPatients)
    {
        this->addPatientToWindow( fillerInput->getPatient(i), canReplaceActualPatient );
        canReplaceActualPatient = false; //Un cop carregat un pacient, ja no el podem reemplaçar
    }
}

void ExtensionHandler::processInput(Patient *patient, const QString &defaultSeriesUID)
{
    foreach(Study *study, patient->getStudies() )
    {
        foreach(Series *series, study->getSeries() )
        {
            // TODO ara el que fem és que 1 Series equival a 1 Volume, més endavant es podrien fer un tracte més elaborat
            Volume *volume = new Volume;
            volume->setImages( series->getImages() );
            volume->setNumberOfPhases( series->getNumberOfPhases() );
            series->addVolume(volume);
        }
    }
    DEBUG_LOG( QString("Patient:\n%1").arg( patient->toString() ));

    // Marquem les series seleccionades
    Series *selectedSeries = patient->getSeries(defaultSeriesUID);
    if ( selectedSeries )
    {
        DEBUG_LOG("Marquem com a seleccionada");
        selectedSeries->select();
    }
    else
    {
        QList<Study *> studyList = patient->getStudies();
        if (!studyList.isEmpty())
        {
            QList<Series *> seriesList = studyList.first()->getSeries();
            if( !seriesList.isEmpty() )
            {
                seriesList.first()->select();
            }
        }
    }

    // Si de tots els pacients que es carreguen intentem carregar-ne un d'igual al que ja tenim carregat, el mantenim
    bool canReplaceActualPatient = !(m_mainApp->getCurrentPatient() && m_mainApp->getCurrentPatient()->compareTo( patient ) == Patient::SamePatients );

    this->addPatientToWindow(patient, canReplaceActualPatient);
}

void ExtensionHandler::addPatientToWindow(Patient *patient, bool canReplaceActualPatient)
{
    if( !m_mainApp->getCurrentPatient() )
    {
        m_mainApp->setPatient(patient);
        DEBUG_LOG("No tenim dades de cap pacient. Obrim en la finestra actual");
    }
    else if( ( m_mainApp->getCurrentPatient()->compareTo( patient ) == Patient::SamePatients ))
    {
        *(m_mainApp->getCurrentPatient()) += *patient;
        DEBUG_LOG("Ja teníem dades d'aquest pacient. Fusionem informació");

        //mirem si hi ha alguna extensió oberta, sinó obrim la de per defecte
        if ( m_mainApp->getExtensionWorkspace()->count() == 0 )
            openDefaultExtension();
    }
    else //Són diferents o no sabem diferenciar
    {
        if (canReplaceActualPatient)
        {
            m_mainApp->setPatient(patient);
            DEBUG_LOG("Tenim pacient i el substituim");
        }
        else
        {
            m_mainApp->setPatientInNewWindow(patient);
            DEBUG_LOG("Tenim pacient i no ens deixen substituir-lo. L'obrim en una finestra nova.");
        }
    }
}

void ExtensionHandler::openDefaultExtension()
{
    if( m_mainApp->getCurrentPatient() )
    {
        // TODO de moment simplement cridem el Q2DViewerExtension
        request("Q2DViewerExtension");
    }
    else
    {
        DEBUG_LOG("No hi ha dades de pacient!");
    }
}

};  // end namespace udg

