#pragma once

#include "Model/AnnotationReportSummary.h"

#include <QWidget>

class AnnotationSliceBrowserPanel;
class CurrentSliceAnnotationPanel;

/**
 * @brief Composite dock widget for annotation review and navigation.
 *
 * Responsibilities:
 * - Present current-slice annotation controls.
 * - Present a searchable, grouped list of annotated slices.
 * - Forward UI intents without owning annotation persistence.
 *
 * Assumptions:
 * - Measurement records are stored outside the dock through annotation services.
 * - The dock is shared by the main viewer workflow; MPR-specific behavior can be
 *   added through the same presentation boundary later.
 */
class AnnotationReportDock final : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Creates the annotation report dock content widget.
     * @param parent Optional Qt parent.
     */
    explicit AnnotationReportDock(QWidget* parent = nullptr);

    /**
     * @brief Returns the current browser filter state.
     * @return Annotation report filter selected in the browser panel.
     */
    [[nodiscard]] AnnotationReportFilter currentFilter() const;

    /**
     * @brief Updates active slice context shown in the current-slice panel.
     * @param context Active viewer slice metadata.
     */
    void setCurrentSliceContext(const AnnotationCurrentSliceContext& context);

    /**
     * @brief Updates annotations displayed for the active slice.
     * @param rows Current-slice annotation report rows.
     */
    void setCurrentSliceRows(const AnnotationReportRows& rows);

    /**
     * @brief Updates grouped annotated-slice browser rows.
     * @param groups Annotation rows grouped by DICOM slice.
     */
    void setSliceGroups(const AnnotationSliceGroups& groups);

signals:
    void filterChanged();
    void goToSliceRequested(
        const QString& seriesInstanceUid,
        const QString& sopInstanceUid,
        int frameIndex,
        const QString& annotationId);
    void metadataChanged(
        const QString& annotationId,
        const QString& label,
        const QString& bodyRegion,
        const QString& seriesInstanceUid);
    void deleteRequested(
        const QString& annotationId,
        const QString& seriesInstanceUid,
        const QString& sopInstanceUid,
        int frameIndex);

private:
    void buildUi();

private:
    CurrentSliceAnnotationPanel* m_currentSlicePanel{nullptr};
    AnnotationSliceBrowserPanel* m_sliceBrowserPanel{nullptr};
};
