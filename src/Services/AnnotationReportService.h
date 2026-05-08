#pragma once

#include "Services/IAnnotationReportService.h"

class DatabaseService;

/**
 * @brief Database-backed annotation report service.
 *
 * Responsibilities:
 * - Translate UI/report use cases into DatabaseService calls.
 * - Group flat annotation rows into slice-level browser groups.
 *
 * Assumptions:
 * - DatabaseService enforces persistence details and soft-delete behavior.
 */
class AnnotationReportService final : public IAnnotationReportService
{
public:
    /**
     * @brief Creates the report service.
     * @param databaseService Database service used for annotation queries.
     */
    explicit AnnotationReportService(DatabaseService& databaseService);

    /** @brief Loads annotation summaries for series rows. */
    [[nodiscard]] AnnotationReportSummaryBySeries loadSeriesSummaries(
        const QList<QString>& seriesInstanceUids) const override;
    /** @brief Loads annotation report rows matching a filter. */
    [[nodiscard]] AnnotationReportRows loadRows(const AnnotationReportFilter& filter) const override;
    /** @brief Loads report rows for one active DICOM slice. */
    [[nodiscard]] AnnotationReportRows loadCurrentSliceRows(
        const QString& seriesInstanceUid,
        const QString& sopInstanceUid,
        int frameIndex = 0) const override;
    /** @brief Loads report rows grouped by DICOM slice. */
    [[nodiscard]] AnnotationSliceGroups loadSliceGroups(const AnnotationReportFilter& filter) const override;
    /** @brief Updates user-editable annotation metadata. */
    [[nodiscard]] bool updateMetadata(
        const QString& annotationId,
        const QString& label,
        const QString& bodyRegion,
        const QString& note = {}) override;
    /** @brief Soft-deletes an annotation by id. */
    [[nodiscard]] bool deleteAnnotation(const QString& annotationId) override;

private:
    DatabaseService& m_databaseService;
};
