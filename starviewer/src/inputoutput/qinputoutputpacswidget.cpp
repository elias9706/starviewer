#include "qinputoutputpacswidget.h"

#include <QMessageBox>
#include <QShortcut>
#include <QMovie>

#include "inputoutputsettings.h"
#include "status.h"
#include "logging.h"
#include "starviewerapplication.h"
#include "dicommask.h"
#include "patient.h"
#include "statswatcher.h"
#include "pacsdevice.h"
#include "querypacs.h"
#include "pacsdevicemanager.h"
#include "study.h"
#include "localdatabasemanager.h"
#include "pacsmanager.h"
#include "harddiskinformation.h"
#include "retrievedicomfilesfrompacsjob.h"
#include "shortcutmanager.h"
#include "querypacsjob.h"

namespace udg {

QInputOutputPacsWidget::QInputOutputPacsWidget(QWidget *parent) : QWidget(parent)
{
    setupUi(this);

    createContextMenuQStudyTreeWidget();

    Settings settings;
    settings.restoreColumnsWidths(InputOutputSettings::PACSStudyListColumnsWidth, m_studyTreeWidget->getQTreeWidget());

    QStudyTreeWidget::ColumnIndex sortByColumn = (QStudyTreeWidget::ColumnIndex) settings.getValue(InputOutputSettings::PACSStudyListSortByColumn).toInt();
    Qt::SortOrder sortOrderColumn = (Qt::SortOrder) settings.getValue(InputOutputSettings::PACSStudyListSortOrder).toInt();
    m_studyTreeWidget->setSortByColumn (sortByColumn, sortOrderColumn);

    m_statsWatcher = new StatsWatcher("QueryInputOutputPacsWidget", this);
    m_statsWatcher->addClicksCounter(m_retrievAndViewButton);
    m_statsWatcher->addClicksCounter(m_retrieveButton);

    // Preparem el QMovie per indicar quan s'estan fent consultes al PACS
    QMovie *operationAnimation = new QMovie(this);
    operationAnimation->setFileName(":/images/loader.gif");
    m_queryAnimationLabel->setMovie(operationAnimation);
    operationAnimation->start();

    setQueryInProgress(false);

    createConnections();
}

QInputOutputPacsWidget::~QInputOutputPacsWidget()
{
    Settings settings;
    settings.saveColumnsWidths(InputOutputSettings::PACSStudyListColumnsWidth, m_studyTreeWidget->getQTreeWidget());

    // Guardem per quin columna està ordenada la llista d'estudis i en quin ordre
    settings.setValue(InputOutputSettings::PACSStudyListSortByColumn, m_studyTreeWidget->getSortColumn());
    settings.setValue(InputOutputSettings::PACSStudyListSortOrder, m_studyTreeWidget->getSortOrderColumn());
}

void QInputOutputPacsWidget::createConnections()
{
    connect(m_studyTreeWidget, SIGNAL(studyExpanded(QString)), SLOT(expandSeriesOfStudy(QString)));
    connect(m_studyTreeWidget, SIGNAL(seriesExpanded(QString, QString)), SLOT(expandImagesOfSeries(QString, QString)));

    connect(m_studyTreeWidget, SIGNAL(studyDoubleClicked()), SLOT(retrieveSelectedStudies()));
    connect(m_studyTreeWidget, SIGNAL(seriesDoubleClicked()), SLOT(retrieveSelectedStudies()));
    connect(m_studyTreeWidget, SIGNAL(imageDoubleClicked()), SLOT(retrieveSelectedStudies()));

    connect(m_retrievAndViewButton, SIGNAL(clicked()), SLOT(retrieveAndViewSelectedStudies()));
    connect(m_retrieveButton, SIGNAL(clicked()), SLOT(retrieveSelectedStudies()));

    connect(m_cancelQueryButton, SIGNAL(clicked()), SLOT(cancelCurrentQueriesToPACS()));
}

void QInputOutputPacsWidget::createContextMenuQStudyTreeWidget()
{
    QAction *action;

    action = m_contextMenuQStudyTreeWidget.addAction(QIcon(":/images/retrieveAndView.png"), tr("Retrieve && &View"), this,
                                                     SLOT(retrieveAndViewSelectedStudies()),
                                                     ShortcutManager::getShortcuts(Shortcuts::RetrieveAndViewSelectedStudies).first());
    (void) new QShortcut(action->shortcut(), this, SLOT(retrieveAndViewSelectedStudies()));

    action = m_contextMenuQStudyTreeWidget.addAction(QIcon(":/images/retrieve.png"), tr("&Retrieve"), this, SLOT(retrieveSelectedStudies()),
                                                     ShortcutManager::getShortcuts(Shortcuts::RetrieveSelectedStudies).first());
    (void) new QShortcut(action->shortcut(), this, SLOT(retrieveSelectedStudies()));

    // Especifiquem que es el menu del dicomdir
    m_studyTreeWidget->setContextMenu(& m_contextMenuQStudyTreeWidget);
}

void QInputOutputPacsWidget::setPacsManager(PacsManager *pacsManager)
{
    m_pacsManager = pacsManager;
}

void QInputOutputPacsWidget::queryStudy(DicomMask queryMask, QList<PacsDevice> pacsToQueryList)
{
    if (pacsToQueryList.count() == 0)
    {
        QMessageBox::information(this, ApplicationNameString, tr("You have to select at least one PACS to query."));
        return;
    }

    if (AreValidQueryParameters(&queryMask, pacsToQueryList))
    {
        cancelCurrentQueriesToPACS();

        m_studyTreeWidget->clear();
        m_hashPacsIDOfStudyInstanceUID.clear();

        foreach (const PacsDevice &pacsDeviceToQuery, pacsToQueryList)
        {
            enqueueQueryPACSJobToPACSManagerAndConnectSignals(new QueryPacsJob(pacsDeviceToQuery, queryMask, QueryPacsJob::study));
        }
    }
}

void QInputOutputPacsWidget::enqueueQueryPACSJobToPACSManagerAndConnectSignals(QueryPacsJob *queryPACSJob)
{
    connect(queryPACSJob, SIGNAL(PACSJobFinished(PACSJob *)), SLOT(queryPACSJobFinished(PACSJob *)));
    connect(queryPACSJob, SIGNAL(PACSJobCancelled(PACSJob *)), SLOT(queryPACSJobCancelled(PACSJob *)));

    m_pacsManager->enqueuePACSJob(queryPACSJob);
    m_queryPACSJobPendingExecuteOrExecuting.insert(queryPACSJob->getPACSJobID(), queryPACSJob);
    setQueryInProgress(true);
}

void QInputOutputPacsWidget::cancelCurrentQueriesToPACS()
{
    foreach (QueryPacsJob *queryPACSJob, m_queryPACSJobPendingExecuteOrExecuting)
    {
        m_pacsManager->requestCancelPACSJob(queryPACSJob);
        m_queryPACSJobPendingExecuteOrExecuting.remove(queryPACSJob->getPACSJobID());
    }

    // Les consultes al PACS poden tarda variis segons a cancel·lar-se, ja que com està documentat hi ha PACS que una vegada un PACS rep l'orde de cancel·lació
    // envien els resultats que havien trobat fins aquell moment i després tanquen la connexió, per fer transparent això a l'usuari, ja que ell no ho notarà en
    // quin moment es cancel·len, ja amaguem el gif indicant que s'ha cancel·lat la consulta, perquè tingui la sensació que s'han cancel·lat immediatament
    setQueryInProgress(false);
}

void QInputOutputPacsWidget::queryPACSJobCancelled(PACSJob *pacsJob)
{
    // Aquest slot també serveix per si alguna altre classe ens cancel·la un PACSJob nostre, d'aquesta manera ens n'assabentem
    QueryPacsJob *queryPACSJob = qobject_cast<QueryPacsJob*>(pacsJob);

    if (queryPACSJob == NULL)
    {
        ERROR_LOG("El PACSJob que s'ha cancel·lat no és un QueryPACSJob");
    }
    else
    {
        m_queryPACSJobPendingExecuteOrExecuting.remove(queryPACSJob->getPACSJobID());
        setQueryInProgress(!m_queryPACSJobPendingExecuteOrExecuting.isEmpty());

        // Fem un deleteLater per si algú més ha capturat el signal de PACSJobFinished per aquest aquest job no es trobi l'objecte destruït
        queryPACSJob->deleteLater();
    }
}

void QInputOutputPacsWidget::queryPACSJobFinished(PACSJob *pacsJob)
{
    QueryPacsJob *queryPACSJob = qobject_cast<QueryPacsJob*>(pacsJob);

    if (queryPACSJob == NULL)
    {
        ERROR_LOG("El PACSJob que ha finalitzat no és un QueryPACSJob");
    }
    else
    {
        if (queryPACSJob->getStatus() != PACSRequestStatus::QueryOk)
        {
            showErrorQueringPACS(queryPACSJob);
        }
        else
        {
            showQueryPACSJobResults(queryPACSJob);
        }

        m_queryPACSJobPendingExecuteOrExecuting.remove(queryPACSJob->getPACSJobID());
        setQueryInProgress(!m_queryPACSJobPendingExecuteOrExecuting.isEmpty());

        // Fem un deleteLater per si algú més ha capturat el signal de PACSJobFinished per aquest aquest job no es trobi l'objecte destruït
        queryPACSJob->deleteLater();
    }
}

void QInputOutputPacsWidget::showQueryPACSJobResults(QueryPacsJob *queryPACSJob)
{
    if (queryPACSJob->getQueryLevel() == QueryPacsJob::study)
    {
        m_studyTreeWidget->insertPatientList(queryPACSJob->getPatientStudyList());
        m_hashPacsIDOfStudyInstanceUID = m_hashPacsIDOfStudyInstanceUID.unite(queryPACSJob->getHashTablePacsIDOfStudyInstanceUID());
    }
    else if (queryPACSJob->getQueryLevel() == QueryPacsJob::series)
    {
        QList<Series*> seriesList = queryPACSJob->getSeriesList();
        QString studyInstanceUID = queryPACSJob->getDicomMask().getStudyInstanceUID();

        if (seriesList.isEmpty())
        {
            QMessageBox::information(this, ApplicationNameString, tr("No series match for this study %1.\n").arg(studyInstanceUID));
        }
        else
        {
            m_studyTreeWidget->insertSeriesList(studyInstanceUID, seriesList);
        }
    }
    else if (queryPACSJob->getQueryLevel() == QueryPacsJob::image)
    {
        QList<Image*> imageList = queryPACSJob->getImageList();
        QString studyInstanceUID = queryPACSJob->getDicomMask().getStudyInstanceUID();
        QString seriesInstanceUID = queryPACSJob->getDicomMask().getSeriesInstanceUID();

        if (imageList.isEmpty())
        {
            QMessageBox::information(this, ApplicationNameString, tr("No images match series %1.\n").arg(seriesInstanceUID));
        }
        else
        {
            m_studyTreeWidget->insertImageList(studyInstanceUID, seriesInstanceUID, imageList);
        }
    }
}

void QInputOutputPacsWidget::showErrorQueringPACS(QueryPacsJob *queryPACSJob)
{
    if (queryPACSJob->getStatus() != PACSRequestStatus::QueryOk && queryPACSJob->getStatus() != PACSRequestStatus::QueryCancelled)
    {
        switch (queryPACSJob->getQueryLevel())
        {
            case QueryPacsJob::study:
                QMessageBox::critical(this, ApplicationNameString, queryPACSJob->getStatusDescription());
                break;
            case QueryPacsJob::series:
            case QueryPacsJob::image:
                QMessageBox::warning(this, ApplicationNameString, queryPACSJob->getStatusDescription());
                break;
        }
    }
}

void QInputOutputPacsWidget::clear()
{
    m_studyTreeWidget->clear();
}

void QInputOutputPacsWidget::expandSeriesOfStudy(QString studyInstanceUID)
{
    PacsDevice pacsDevice = PacsDeviceManager().getPACSDeviceByID(getPacsIDFromQueriedStudies(studyInstanceUID));
    QString pacsDescription = pacsDevice.getAETitle() + " Institució" + pacsDevice.getInstitution() + " IP:" + pacsDevice.getAddress();

    INFO_LOG("Cercant informacio de les series de l'estudi" + studyInstanceUID + " del PACS " + pacsDescription);

    enqueueQueryPACSJobToPACSManagerAndConnectSignals(new QueryPacsJob(pacsDevice, buildSeriesDicomMask(studyInstanceUID), QueryPacsJob::series));
}

void QInputOutputPacsWidget::expandImagesOfSeries(QString studyInstanceUID, QString seriesInstanceUID)
{
    PacsDevice pacsDevice = PacsDeviceManager().getPACSDeviceByID(getPacsIDFromQueriedStudies(studyInstanceUID));
    QString pacsDescription = pacsDevice.getAETitle() + " Institució" + pacsDevice.getInstitution() + " IP:" + pacsDevice.getAddress();

    INFO_LOG("Cercant informacio de les imatges de la serie" + seriesInstanceUID + " de l'estudi" + studyInstanceUID + " del PACS " + pacsDescription);

    enqueueQueryPACSJobToPACSManagerAndConnectSignals(new QueryPacsJob(pacsDevice, buildImageDicomMask(studyInstanceUID, seriesInstanceUID),
                                                                       QueryPacsJob::image));
}

void QInputOutputPacsWidget::retrieveSelectedStudies()
{
    if (m_studyTreeWidget->getSelectedStudiesUID().isEmpty())
    {
        QApplication::restoreOverrideCursor();
        QMessageBox::warning(this, ApplicationNameString, tr("Select a study to retrieve."));
        return;
    }

    foreach (DicomMask dicomMask, m_studyTreeWidget->getDicomMaskOfSelectedItems())
    {
        retrieve(getPacsIDFromQueriedStudies(dicomMask.getStudyInstanceUID()), None, m_studyTreeWidget->getStudy(dicomMask.getStudyInstanceUID()), 
            dicomMask.getSeriesInstanceUID(), dicomMask.getSOPInstanceUID());
    }
}

void QInputOutputPacsWidget::retrieveAndViewSelectedStudies()
{
    if (m_studyTreeWidget->getSelectedStudiesUID().isEmpty())
    {
        QApplication::restoreOverrideCursor();
        QMessageBox::warning(this, ApplicationNameString, tr("Select a study to retrieve and view."));
        return;
    }

    foreach (DicomMask dicomMask, m_studyTreeWidget->getDicomMaskOfSelectedItems())
    {
        retrieve(getPacsIDFromQueriedStudies(dicomMask.getStudyInstanceUID()), View, m_studyTreeWidget->getStudy(dicomMask.getStudyInstanceUID()), 
            dicomMask.getSeriesInstanceUID(), dicomMask.getSOPInstanceUID());
    }
}

void QInputOutputPacsWidget::retrieveDICOMFilesFromPACSJobStarted(PACSJob *pacsJob)
{
    emit studyRetrieveStarted((dynamic_cast<RetrieveDICOMFilesFromPACSJob*> (pacsJob))->getStudyToRetrieveDICOMFiles()->getInstanceUID());
}

void QInputOutputPacsWidget::retrieveDICOMFilesFromPACSJobFinished(PACSJob *pacsJob)
{
    RetrieveDICOMFilesFromPACSJob *retrieveDICOMFilesFromPACSJob = dynamic_cast<RetrieveDICOMFilesFromPACSJob*> (pacsJob);

    if (retrieveDICOMFilesFromPACSJob->getStatus() != PACSRequestStatus::RetrieveOk)
    {
        if (retrieveDICOMFilesFromPACSJob->getStatus() == PACSRequestStatus::RetrieveSomeDICOMFilesFailed)
        {
            QMessageBox::warning(this, ApplicationNameString, retrieveDICOMFilesFromPACSJob->getStatusDescription());
        }
        else
        {
            QMessageBox::critical(this, ApplicationNameString, retrieveDICOMFilesFromPACSJob->getStatusDescription());
            emit studyRetrieveFailed(retrieveDICOMFilesFromPACSJob->getStudyToRetrieveDICOMFiles()->getInstanceUID());
            return;
        }
    }

    emit studyRetrieveFinished(retrieveDICOMFilesFromPACSJob->getStudyToRetrieveDICOMFiles()->getInstanceUID());

    switch (m_actionsWhenRetrieveJobFinished.take(retrieveDICOMFilesFromPACSJob->getPACSJobID()))
    {
        case Load:
            emit loadRetrievedStudy(retrieveDICOMFilesFromPACSJob->getStudyToRetrieveDICOMFiles()->getInstanceUID());;
            break;
        case View:
            emit viewRetrievedStudy(retrieveDICOMFilesFromPACSJob->getStudyToRetrieveDICOMFiles()->getInstanceUID());
            break;
        default:
            break;
    }

    // Com que l'objecte és un punter altres classes poden haver capturat el Signal per això li fem un deleteLater() en comptes d'un delete, per evitar
    // que quan responguin al signal es trobin que l'objecte ja no existeix. L'objecte serà destruït per Qt quan es retorni el eventLoop
    pacsJob->deleteLater();
}

void QInputOutputPacsWidget::retrieve(QString pacsIDToRetrieve, ActionsAfterRetrieve actionAfterRetrieve, Study *studyToRetrieve,
    const QString &seriesInstanceUIDToRetrieve, const QString &sopInstanceUIDToRetrieve)
{
    PacsDevice pacsDevice = PacsDeviceManager().getPACSDeviceByID(pacsIDToRetrieve);
    RetrieveDICOMFilesFromPACSJob::RetrievePriorityJob retrievePriorityJob = actionAfterRetrieve == View ? RetrieveDICOMFilesFromPACSJob::High
        : RetrieveDICOMFilesFromPACSJob::Medium;

    RetrieveDICOMFilesFromPACSJob *retrieveDICOMFilesFromPACSJob = new RetrieveDICOMFilesFromPACSJob(pacsDevice, retrievePriorityJob, studyToRetrieve,
        seriesInstanceUIDToRetrieve, sopInstanceUIDToRetrieve);

    m_pacsManager->enqueuePACSJob(retrieveDICOMFilesFromPACSJob);
    connect(retrieveDICOMFilesFromPACSJob, SIGNAL(PACSJobStarted(PACSJob *)), SLOT(retrieveDICOMFilesFromPACSJobStarted(PACSJob *)));
    connect(retrieveDICOMFilesFromPACSJob, SIGNAL(PACSJobFinished(PACSJob *)), SLOT(retrieveDICOMFilesFromPACSJobFinished(PACSJob *)));

    m_actionsWhenRetrieveJobFinished.insert(retrieveDICOMFilesFromPACSJob->getPACSJobID(), actionAfterRetrieve);
}

bool QInputOutputPacsWidget::AreValidQueryParameters(DicomMask *maskToQuery, QList<PacsDevice> pacsToQuery)
{
    // Es comprova que hi hagi pacs seleccionats
    if (pacsToQuery.isEmpty())
    {
        QMessageBox::warning(this, ApplicationNameString, tr("Please select a PACS to query."));
        return false;
    }

    if (maskToQuery->isEmpty())
    {
        QMessageBox::StandardButton response;
        response = QMessageBox::question(this, ApplicationNameString, tr("No search fields were filled.") + "\n" +
            tr("The query can take a long time.\nDo you want continue?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        return (response == QMessageBox::Yes);
    }
    else
    {
        return true;
    }
}

QString QInputOutputPacsWidget::getPacsIDFromQueriedStudies(QString studyInstanceUID)
{
    // TODO Tenir en compte que podem tenir un studyUID repetit en dos PACS, ara mateix no ho tenim contemplat a la QHash
    if (!m_hashPacsIDOfStudyInstanceUID.contains(studyInstanceUID))
    {
        ERROR_LOG(QString("No s'ha trobat a quin PACS pertany l'estudi %1 a la QHash").arg(studyInstanceUID));
        return "";
    }
    else
    {
        return m_hashPacsIDOfStudyInstanceUID[studyInstanceUID];
    }
}

DicomMask QInputOutputPacsWidget::buildSeriesDicomMask(QString studyInstanceUID)
{
    DicomMask mask;

    mask.setStudyInstanceUID(studyInstanceUID);
    mask.setSeriesDate(QDate(), QDate());
    mask.setSeriesTime(QTime(), QTime());
    mask.setSeriesModality("");
    mask.setSeriesNumber("");
    mask.setSeriesInstanceUID("");
    mask.setPPSStartDate(QDate(), QDate());
    mask.setPPStartTime(QTime(), QTime());
    mask.setRequestAttributeSequence("", "");

    return mask;
}

DicomMask QInputOutputPacsWidget::buildImageDicomMask(QString studyInstanceUID, QString seriesInstanceUID)
{
    DicomMask mask;

    mask.setStudyInstanceUID(studyInstanceUID);
    mask.setSeriesInstanceUID(seriesInstanceUID);
    mask.setImageNumber("");
    mask.setSOPInstanceUID("");

    return mask;
}

void QInputOutputPacsWidget::setQueryInProgress(bool queryInProgress)
{
    m_queryAnimationLabel->setVisible(queryInProgress);
    m_queryInProgressLabel->setVisible(queryInProgress);
    m_cancelQueryButton->setEnabled(queryInProgress);
}
};
