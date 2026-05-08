#include "DicomTreeController.h"

#include "Database/DatabaseService.h"
#include "DicomTreeItemRoles.h"
#include "DicomTreePanel.h"
#include "Model/DicomParameters.h"
#include "Services/IAnnotationReportService.h"

#include <QStandardItem>

namespace TreeRoles = DicomTreeItemRoles;

namespace
{
QString countTitle(int count, const QString& singular, const QString& plural)
{
    return QString("%1 %2").arg(count).arg(count == 1 ? singular : plural);
}
}

DicomTreeController::DicomTreeController(QObject* parent)
    : QObject(parent)
{
}

void DicomTreeController::setDatabaseService(DatabaseService* databaseService)
{
    m_databaseService = databaseService;
}

void DicomTreeController::setAnnotationReportService(IAnnotationReportService* annotationReportService)
{
    m_annotationReportService = annotationReportService;
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
    connect(m_treePanel, &DicomTreePanel::previewItemDoubleClicked, this, &DicomTreeController::onPreviewItemDoubleClicked);
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

void DicomTreeController::refreshSeriesAnnotationSummary(const QString& seriesInstanceUid)
{
    if (!m_treePanel || !m_annotationReportService || seriesInstanceUid.trimmed().isEmpty())
    {
        return;
    }

    const AnnotationReportSummaryBySeries summaries =
        m_annotationReportService->loadSeriesSummaries({seriesInstanceUid});
    m_treePanel->updateSeriesAnnotationSummary(seriesInstanceUid, summaries.value(seriesInstanceUid));
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
            const DicomPreviewItems items =
                m_databaseService->getStudyPreviewItemsForPatient(item->data(TreeRoles::PatientId).toString());
            m_treePanel->updatePreviewItems(countTitle(items.size(), "study", "studies"), items, "No studies found");
        }
        else if (nodeType == TreeRoles::NodeTypeStudy)
        {
            showSeriesPreviewsForStudy(item->data(TreeRoles::StudyInstanceUid).toString());
        }
        else if (nodeType == TreeRoles::NodeTypeSeries)
        {
            showSeriesPreviewsForStudy(item->data(TreeRoles::StudyInstanceUid).toString());
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
    if (!item)
    {
        return;
    }

    loadChildrenForItem(item);
}

void DicomTreeController::onPreviewItemDoubleClicked(const DicomPreviewItem& item)
{
    if (!m_treePanel || !m_databaseService)
    {
        return;
    }

    switch (item.targetType)
    {
    case DicomPreviewTargetType::Study:
        activateTreeItem(ensureStudyItemLoaded(item.parentId, item.targetId));
        return;
    case DicomPreviewTargetType::Series:
        activateTreeItem(ensureSeriesItemLoaded(item.parentId, item.targetId));
        return;
    case DicomPreviewTargetType::None:
        return;
    }
}

void DicomTreeController::loadChildrenForItem(QStandardItem* item)
{
    if (!item || !m_treePanel || !m_databaseService || item->data(TreeRoles::ChildrenLoaded).toBool())
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

        QList<QString> seriesInstanceUids;
        seriesInstanceUids.reserve(seriesList.size());
        for (const auto& series : seriesList)
        {
            if (series)
            {
                seriesInstanceUids.append(series->seriesInstanceUid());
            }
        }

        const AnnotationReportSummaryBySeries annotationSummaries =
            m_annotationReportService ? m_annotationReportService->loadSeriesSummaries(seriesInstanceUids)
                                      : AnnotationReportSummaryBySeries{};
        m_treePanel->appendSeries(item, patient, study, seriesList, annotationSummaries);
        item->setData(true, TreeRoles::ChildrenLoaded);
    }
}

void DicomTreeController::activateTreeItem(QStandardItem* item)
{
    if (!item || !m_treePanel)
    {
        return;
    }

    loadChildrenForItem(item);
    m_treePanel->selectAndRevealItem(item);
    onHierarchyItemActivated(item->index());
}

void DicomTreeController::showSeriesPreviewsForStudy(const QString& studyInstanceUid)
{
    if (!m_treePanel || !m_databaseService || studyInstanceUid.trimmed().isEmpty())
    {
        return;
    }

    const DicomPreviewItems items = m_databaseService->getSeriesPreviewItemsForStudy(studyInstanceUid);
    m_treePanel->updatePreviewItems(countTitle(items.size(), "series", "series"), items, "No series found");
}

QStandardItem* DicomTreeController::ensureStudyItemLoaded(const QString& patientId, const QString& studyInstanceUid)
{
    if (!m_treePanel || studyInstanceUid.trimmed().isEmpty())
    {
        return nullptr;
    }

    if (QStandardItem* existingStudyItem = m_treePanel->findStudyItem(studyInstanceUid))
    {
        return existingStudyItem;
    }

    QStandardItem* patientItem = m_treePanel->findPatientItem(patientId);
    loadChildrenForItem(patientItem);
    return m_treePanel->findStudyItem(studyInstanceUid);
}

QStandardItem* DicomTreeController::ensureSeriesItemLoaded(const QString& studyInstanceUid, const QString& seriesInstanceUid)
{
    if (!m_treePanel || seriesInstanceUid.trimmed().isEmpty())
    {
        return nullptr;
    }

    if (QStandardItem* existingSeriesItem = m_treePanel->findSeriesItem(seriesInstanceUid))
    {
        return existingSeriesItem;
    }

    QStandardItem* studyItem = m_treePanel->findStudyItem(studyInstanceUid);
    loadChildrenForItem(studyItem);
    return m_treePanel->findSeriesItem(seriesInstanceUid);
}

void DicomTreeController::onGlobalSearchTextChanged(const QString& text)
{
    Q_UNUSED(text);
    refreshHierarchy();
}
