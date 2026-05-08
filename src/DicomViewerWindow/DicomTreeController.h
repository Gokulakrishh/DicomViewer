#pragma once

#include <QModelIndex>
#include <QObject>

struct DicomPreviewItem;
class DatabaseService;
class DicomTreePanel;
class IAnnotationReportService;
class QStandardItem;

/**
 * @brief Controller for study-browser tree and preview navigation.
 *
 * Responsibilities:
 * - Coordinate lazy loading from DatabaseService into DicomTreePanel.
 * - Keep tree, preview panel, and main viewer navigation synchronized.
 * - Refresh series annotation summaries without loading annotation details.
 *
 * Assumptions:
 * - The tree model contains metadata only; DICOM images are loaded by viewer
 *   workflows when a series or file is selected.
 */
class DicomTreeController : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Creates the tree controller.
     * @param parent Optional Qt parent.
     */
    explicit DicomTreeController(QObject* parent = nullptr);

    /**
     * @brief Sets the persistence service used for hierarchy loading.
     * @param databaseService Non-owning service pointer.
     */
    void setDatabaseService(DatabaseService* databaseService);

    /**
     * @brief Sets the annotation reporting service for summary badges.
     * @param annotationReportService Non-owning report service pointer.
     */
    void setAnnotationReportService(IAnnotationReportService* annotationReportService);

    /**
     * @brief Binds the controller to a tree panel.
     * @param treePanel Non-owning panel pointer.
     */
    void bindPanel(DicomTreePanel* treePanel);

    /**
     * @brief Reloads the visible patient hierarchy.
     */
    void refreshHierarchy();

    /**
     * @brief Refreshes annotation counts for one series row.
     * @param seriesInstanceUid DICOM Series Instance UID.
     */
    void refreshSeriesAnnotationSummary(const QString& seriesInstanceUid);

signals:
    void patientContextSelected(
        const QString& patientName,
        const QString& patientDob,
        const QString& doctorName,
        const QString& modality,
        const QString& studyDate);
    void seriesSelectionRequested(const QString& seriesInstanceUid);
    void fileSelectionRequested(const QString& filePath);

private slots:
    void onHierarchyItemActivated(const QModelIndex& index);
    void onHierarchyItemExpanded(const QModelIndex& index);
    void onPreviewItemDoubleClicked(const DicomPreviewItem& item);
    void onGlobalSearchTextChanged(const QString& text);

private:
    void loadChildrenForItem(QStandardItem* item);
    void activateTreeItem(QStandardItem* item);
    void showSeriesPreviewsForStudy(const QString& studyInstanceUid);
    QStandardItem* ensureStudyItemLoaded(const QString& patientId, const QString& studyInstanceUid);
    QStandardItem* ensureSeriesItemLoaded(const QString& studyInstanceUid, const QString& seriesInstanceUid);

    DatabaseService* m_databaseService{nullptr};
    IAnnotationReportService* m_annotationReportService{nullptr};
    DicomTreePanel* m_treePanel{nullptr};
};
