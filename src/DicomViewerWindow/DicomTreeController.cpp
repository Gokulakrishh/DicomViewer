#include "DicomTreeController.h"

#include "Database/DatabaseService.h"
#include "DicomTreeItemRoles.h"
#include "DicomTreePanel.h"
#include "Model/DicomParameters.h"

#include <QStandardItem>

namespace TreeRoles = DicomTreeItemRoles;

DicomTreeController::DicomTreeController(QObject* parent)
    : QObject(parent)
{
}

void DicomTreeController::setDatabaseService(DatabaseService* databaseService)
{
    m_databaseService = databaseService;
}

void DicomTreeController::bindPanel(DicomTreePanel* treePanel)
{
    m_treePanel = treePanel;
    if (!m_treePanel)
    {
        return;
    }

    connect(m_treePanel, &DicomTreePanel::hierarchyItemActivated, this, &DicomTreeController::onHierarchyItemActivated);
    connect(m_treePanel, &DicomTreePanel::hierarchyItemExpanded, this, &DicomTreeController::onHierarchyItemExpanded);
    connect(m_treePanel, &DicomTreePanel::globalSearchTextChanged, this, &DicomTreeController::onGlobalSearchTextChanged);
    refreshHierarchy();
}

void DicomTreeController::refreshHierarchy()
{
    if (!m_treePanel || !m_databaseService)
    {
        return;
    }

    m_treePanel->refreshHierarchy(m_databaseService->getAllPatients(m_treePanel->globalFilterText()));
}

void DicomTreeController::onHierarchyItemActivated(const QModelIndex& index)
{
    if (!index.isValid() || !m_treePanel)
    {
        return;
    }

    QStandardItem* item = m_treePanel->itemFromIndex(index);
    if (!item)
    {
        return;
    }

    emit patientContextSelected(
        item->data(TreeRoles::PatientName).toString(),
        item->data(TreeRoles::PatientDob).toString(),
        item->data(TreeRoles::DoctorName).toString(),
        item->data(TreeRoles::Modality).toString(),
        item->data(TreeRoles::StudyDate).toString());

    const QString nodeType = item->data(TreeRoles::NodeType).toString();
    if (m_databaseService)
    {
        if (nodeType == TreeRoles::NodeTypePatient)
        {
            m_treePanel->updatePreviewPane(m_databaseService->getPreviewForPatient(item->data(TreeRoles::PatientId).toString()));
        }
        else if (nodeType == TreeRoles::NodeTypeStudy)
        {
            m_treePanel->updatePreviewPane(m_databaseService->getPreviewForStudy(item->data(TreeRoles::StudyInstanceUid).toString()));
        }
        else if (nodeType == TreeRoles::NodeTypeSeries)
        {
            m_treePanel->updatePreviewPane(m_databaseService->getPreviewForSeries(item->data(TreeRoles::SeriesInstanceUid).toString()));
        }
        else
        {
            m_treePanel->updatePreviewPane(QPixmap());
        }
    }

    const QString seriesInstanceUid = item->data(TreeRoles::SeriesInstanceUid).toString();
    if (!seriesInstanceUid.isEmpty())
    {
        emit seriesSelectionRequested(seriesInstanceUid);
        return;
    }

    const QString filePath = item->data(TreeRoles::FilePath).toString();
    if (!filePath.isEmpty())
    {
        emit fileSelectionRequested(filePath);
    }
}

void DicomTreeController::onHierarchyItemExpanded(const QModelIndex& index)
{
    if (!index.isValid() || !m_treePanel || !m_databaseService)
    {
        return;
    }

    QStandardItem* item = m_treePanel->itemFromIndex(index);
    if (!item || item->data(TreeRoles::ChildrenLoaded).toBool())
    {
        return;
    }

    const QString nodeType = item->data(TreeRoles::NodeType).toString();
    if (nodeType == TreeRoles::NodeTypePatient)
    {
        const QString patientId = item->data(TreeRoles::PatientId).toString();
        if (patientId.isEmpty())
        {
            item->removeRows(0, item->rowCount());
            item->setData(true, TreeRoles::ChildrenLoaded);
            return;
        }

        auto patient = std::make_shared<Patient>();
        patient->setPatientId(patientId);
        patient->setPatientName(item->data(TreeRoles::PatientName).toString());
        patient->setDateOfBirth(item->data(TreeRoles::PatientDob).toString());

        item->removeRows(0, item->rowCount());
        const QString globalSearchText = m_treePanel->globalFilterText().toLower();
        const bool patientMatchesGlobalSearch =
            globalSearchText.isEmpty() || item->data(TreeRoles::SearchText).toString().toLower().contains(globalSearchText);
        const QList<DatabaseService::StudyPtr> studies =
            m_databaseService->getStudiesForPatient(patientId, patientMatchesGlobalSearch ? QString() : globalSearchText);
        m_treePanel->appendStudies(item, patient, studies);
        item->setData(true, TreeRoles::ChildrenLoaded);
    }
    else if (nodeType == TreeRoles::NodeTypeStudy)
    {
        const QString patientId = item->data(TreeRoles::PatientId).toString();
        const QString studyInstanceUid = item->data(TreeRoles::StudyInstanceUid).toString();
        if (patientId.isEmpty() || studyInstanceUid.isEmpty())
        {
            item->removeRows(0, item->rowCount());
            item->setData(true, TreeRoles::ChildrenLoaded);
            return;
        }

        auto patient = std::make_shared<Patient>();
        patient->setPatientId(patientId);
        patient->setPatientName(item->data(TreeRoles::PatientName).toString());
        patient->setDateOfBirth(item->data(TreeRoles::PatientDob).toString());

        auto study = std::make_shared<Study>();
        study->setStudyInstanceUid(studyInstanceUid);
        study->setDoctorName(item->data(TreeRoles::DoctorName).toString());
        study->setStudyDate(item->data(TreeRoles::StudyDate).toString());

        item->removeRows(0, item->rowCount());
        const QString globalSearchText = m_treePanel->globalFilterText().toLower();
        const bool studyMatchesGlobalSearch =
            globalSearchText.isEmpty() || item->data(TreeRoles::SearchText).toString().toLower().contains(globalSearchText);
        const QList<DatabaseService::SeriesPtr> seriesList =
            m_databaseService->getSeriesForStudy(studyInstanceUid, studyMatchesGlobalSearch ? QString() : globalSearchText);
        m_treePanel->appendSeries(item, patient, study, seriesList);
        item->setData(true, TreeRoles::ChildrenLoaded);
    }
}

void DicomTreeController::onGlobalSearchTextChanged(const QString& text)
{
    Q_UNUSED(text);
    refreshHierarchy();
}
