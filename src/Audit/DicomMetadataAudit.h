#pragma once

#include "Audit/AuditEvent.h"

#include <QStringList>

class Series;

/**
 * @brief Result of checking whether a DICOM series has metadata needed by tools.
 *
 * Responsibilities:
 * - Summarize metadata availability for measurement, volume, and pixel-value
 *   workflows.
 * - Separate warnings from blocking issues so the UI can decide whether to
 *   continue.
 */
struct DicomMetadataAuditResult
{
    bool distanceMeasurementAvailable{false};
    bool volumeGeometryAvailable{false};
    bool calibratedPixelValuesAvailable{false};
    QStringList warnings;
    QStringList blockingIssues;

    /**
     * @brief Reports whether the series has blocking metadata issues.
     * @return True when at least one blocking issue was detected.
     */
    [[nodiscard]] bool hasBlockingIssues() const
    {
        return !blockingIssues.isEmpty();
    }
};

/**
 * @brief Evaluates DICOM metadata required by diagnostic viewer features.
 *
 * Responsibilities:
 * - Inspect a lightweight series model for measurement and volume prerequisites.
 * - Convert findings into an auditable event.
 *
 * Assumptions:
 * - The audit checks metadata completeness, not clinical correctness.
 * - Missing metadata may limit tools such as distance measurement, MPR, and 3D.
 */
class DicomMetadataAudit
{
public:
    /**
     * @brief Evaluates metadata availability for one series.
     * @param series Series hierarchy to inspect.
     * @return Metadata audit result with warnings and blocking issues.
     */
    [[nodiscard]] DicomMetadataAuditResult evaluateSeries(const Series& series) const;

    /**
     * @brief Converts a metadata audit result into a structured audit event.
     * @param result Result returned by evaluateSeries().
     * @param seriesInstanceUid Series identifier used as the audit subject.
     * @return Audit event suitable for configured audit sinks.
     */
    [[nodiscard]] AuditEvent toAuditEvent(const DicomMetadataAuditResult& result, const QString& seriesInstanceUid) const;
};
