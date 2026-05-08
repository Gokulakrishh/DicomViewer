#pragma once

#include "Model/AnnotationReportSummary.h"

#include <QList>
#include <QString>

/**
 * @brief Read/write service boundary for annotation reporting workflows.
 *
 * Responsibilities:
 * - Provide bounded annotation report queries for UI panels.
 * - Update user-editable annotation metadata.
 * - Delete annotations without exposing database implementation details.
 *
 * Assumptions:
 * - Annotations are persisted outside source DICOM files.
 * - Slice-level queries use SOP Instance UID for precise scoping.
 */
class IAnnotationReportService
{
public:
    virtual ~IAnnotationReportService() = default;

    /** @brief Loads annotation summaries for series rows. */
    [[nodiscard]] virtual AnnotationReportSummaryBySeries loadSeriesSummaries(
        const QList<QString>& seriesInstanceUids) const = 0;
    /** @brief Loads annotation report rows matching a filter. */
    [[nodiscard]] virtual AnnotationReportRows loadRows(const AnnotationReportFilter& filter) const = 0;
    /** @brief Loads report rows for one active DICOM slice. */
    [[nodiscard]] virtual AnnotationReportRows loadCurrentSliceRows(
        const QString& seriesInstanceUid,
        const QString& sopInstanceUid,
        int frameIndex = 0) const = 0;
    /** @brief Loads report rows grouped by DICOM slice. */
    [[nodiscard]] virtual AnnotationSliceGroups loadSliceGroups(const AnnotationReportFilter& filter) const = 0;
    /** @brief Updates user-editable annotation metadata. */
    [[nodiscard]] virtual bool updateMetadata(
        const QString& annotationId,
        const QString& label,
        const QString& bodyRegion,
        const QString& note = {}) = 0;
    /** @brief Soft-deletes an annotation by id. */
    [[nodiscard]] virtual bool deleteAnnotation(const QString& annotationId) = 0;
};
