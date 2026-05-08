#pragma once

#include <QModelIndex>
#include <QPixmap>
#include <QHash>
#include <QWidget>
#include <memory>

#include "Model/AnnotationReportSummary.h"
#include "Model/DicomPreviewItem.h"

class DicomPreviewPanel;
class Patient;
class QLineEdit;
class QSplitter;
class QStandardItem;
class QStandardItemModel;
class QTreeView;
class Series;
class Study;

/**
 * @brief Study-browser widget combining DICOM hierarchy and preview tiles.
 *
 * Responsibilities:
 * - Render patient, study, and series rows with metadata columns.
 * - Support local/global search and lazy child loading.
 * - Display bounded thumbnail previews for studies and series.
 *
 * Assumptions:
 * - The panel stores lightweight hierarchy metadata only.
 * - Series-level rows may include annotation summary counts, not annotation
 *   geometry.
 */
class DicomTreePanel : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Creates the DICOM tree panel.
     * @param parent Optional Qt parent.
     */
    explicit DicomTreePanel(QWidget* parent = nullptr);

    /**
     * @brief Resolves a model index to its primary item.
     * @param index Tree model index.
     * @return Primary item, or null for invalid indexes.
     */
    QStandardItem* itemFromIndex(const QModelIndex& index) const;

    /**
     * @brief Returns the local tree filter text.
     * @return Current local filter text.
     */
    QString localFilterText() const;

    /**
     * @brief Returns the global hierarchy search text.
     * @return Current global search text.
     */
    QString globalFilterText() const;

    /**
     * @brief Finds a patient row by patient id.
     * @param patientId DICOM patient identifier.
     * @return Matching item, or null when not present.
     */
    QStandardItem* findPatientItem(const QString& patientId) const;

    /**
     * @brief Finds a study row by Study Instance UID.
     * @param studyInstanceUid DICOM Study Instance UID.
     * @return Matching item, or null when not present.
     */
    QStandardItem* findStudyItem(const QString& studyInstanceUid) const;

    /**
     * @brief Finds a series row by Series Instance UID.
     * @param seriesInstanceUid DICOM Series Instance UID.
     * @return Matching item, or null when not present.
     */
    QStandardItem* findSeriesItem(const QString& seriesInstanceUid) const;

    /**
     * @brief Selects a tree item and scrolls it into view.
     * @param item Item to reveal.
     */
    void selectAndRevealItem(QStandardItem* item);

    /**
     * @brief Replaces the top-level patient hierarchy.
     * @param patients Patients to show.
     */
    void refreshHierarchy(const QList<std::shared_ptr<Patient>>& patients);

    /**
     * @brief Appends study rows under a patient.
     * @param patientItem Parent patient item.
     * @param patient Patient metadata.
     * @param studies Studies to append.
     */
    void appendStudies(QStandardItem* patientItem, const std::shared_ptr<Patient>& patient, const QList<std::shared_ptr<Study>>& studies);

    /**
     * @brief Appends series rows under a study.
     * @param studyItem Parent study item.
     * @param patient Patient metadata for inherited columns.
     * @param study Study metadata.
     * @param seriesList Series to append.
     * @param annotationSummaries Annotation summary counts keyed by series.
     */
    void appendSeries(
        QStandardItem* studyItem,
        const std::shared_ptr<Patient>& patient,
        const std::shared_ptr<Study>& study,
        const QList<std::shared_ptr<Series>>& seriesList,
        const AnnotationReportSummaryBySeries& annotationSummaries);

    /**
     * @brief Updates one series row with current annotation summary counts.
     * @param seriesInstanceUid DICOM Series Instance UID.
     * @param annotationSummary Summary counts for the series.
     */
    void updateSeriesAnnotationSummary(
        const QString& seriesInstanceUid,
        const AnnotationReportSummary& annotationSummary);

    /**
     * @brief Displays a single preview image.
     * @param pixmap Preview pixmap.
     */
    void updatePreviewPane(const QPixmap& pixmap);

    /**
     * @brief Displays preview/navigation items.
     * @param title Preview panel title.
     * @param items Preview items.
     * @param emptyText Text shown when no preview is available.
     */
    void updatePreviewItems(
        const QString& title,
        const DicomPreviewItems& items,
        const QString& emptyText = "No preview");

signals:
    void hierarchyItemActivated(const QModelIndex& index);
    void hierarchyItemExpanded(const QModelIndex& index);
    void previewItemDoubleClicked(const DicomPreviewItem& item);
    void localSearchTextChanged(const QString& text);
    void globalSearchTextChanged(const QString& text);

private:
    void buildUi();
    void applyTreeFilter(const QString& filterText);
    bool updateItemVisibility(QStandardItem* item, const QString& filterText);
    bool updateSeriesAnnotationSummaryRecursive(
        QStandardItem* item,
        const QString& seriesInstanceUid,
        const AnnotationReportSummary& annotationSummary);
    QStandardItem* findItemByRole(
        int role,
        const QString& value,
        const QString& requiredNodeType = {}) const;
    QStandardItem* findItemByRoleRecursive(
        QStandardItem* item,
        int role,
        const QString& value,
        const QString& requiredNodeType) const;
    QList<QStandardItem*> makeRow(QStandardItem* primaryItem, const QStringList& columnValues) const;
    void addPatientToTree(const std::shared_ptr<Patient>& patient);
    void addStudyToTree(QStandardItem* patientItem, const std::shared_ptr<Patient>& patient, const std::shared_ptr<Study>& study);
    void addSeriesToTree(
        QStandardItem* studyItem,
        const std::shared_ptr<Patient>& patient,
        const std::shared_ptr<Study>& study,
        const std::shared_ptr<Series>& series,
        const AnnotationReportSummary& annotationSummary);

private:
    QSplitter* m_splitter{nullptr};
    QTreeView* m_treeView{nullptr};
    QStandardItemModel* m_treeModel{nullptr};
    QLineEdit* m_searchLineEdit{nullptr};
    QLineEdit* m_globalSearchLineEdit{nullptr};
    DicomPreviewPanel* m_previewPanel{nullptr};
};
