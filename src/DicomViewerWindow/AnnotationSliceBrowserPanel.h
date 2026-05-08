#pragma once

#include "Model/AnnotationReportSummary.h"

#include <QSet>
#include <QString>
#include <QWidget>

class QComboBox;
class QLineEdit;
class QVBoxLayout;

/**
 * @brief Searchable browser for annotated DICOM slices.
 *
 * Responsibilities:
 * - Maintain annotation search/filter UI state.
 * - Render one compact row per annotated slice with optional expanded details.
 * - Emit navigation requests without loading image data itself.
 *
 * Assumptions:
 * - Data is supplied as bounded report groups to avoid loading all annotations
 *   for very large studies into memory.
 */
class AnnotationSliceBrowserPanel final : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Creates the annotated-slice browser panel.
     * @param parent Optional Qt parent.
     */
    explicit AnnotationSliceBrowserPanel(QWidget* parent = nullptr);

    /**
     * @brief Returns the current search and type/body-region filters.
     * @return Annotation report filter.
     */
    [[nodiscard]] AnnotationReportFilter currentFilter() const;

    /**
     * @brief Replaces the visible annotated-slice groups.
     * @param groups Slice groups returned by the annotation report service.
     */
    void setGroups(const AnnotationSliceGroups& groups);

signals:
    /**
     * @brief Emitted when the search or filter state changes.
     */
    void filterChanged();

    /**
     * @brief Requests navigation to a slice containing annotations.
     * @param seriesInstanceUid DICOM Series Instance UID.
     * @param sopInstanceUid DICOM SOP Instance UID.
     * @param frameIndex Zero-based frame index for multi-frame instances.
     * @param annotationId Representative annotation id for the target slice.
     */
    void goToSliceRequested(
        const QString& seriesInstanceUid,
        const QString& sopInstanceUid,
        int frameIndex,
        const QString& annotationId);

private:
    void buildUi();
    void clearGroups();
    void appendGroupCard(const AnnotationSliceGroup& group);

private:
    QLineEdit* m_searchLineEdit{nullptr};
    QComboBox* m_regionFilterComboBox{nullptr};
    QComboBox* m_typeFilterComboBox{nullptr};
    QWidget* m_groupsWidget{nullptr};
    QVBoxLayout* m_groupsLayout{nullptr};
    QSet<QString> m_expandedGroupKeys;
};
