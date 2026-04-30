#pragma once

#include "Audit/AuditEvent.h"

#include <QStringList>

class Series;

struct DicomMetadataAuditResult
{
    bool distanceMeasurementAvailable{false};
    bool volumeGeometryAvailable{false};
    bool calibratedPixelValuesAvailable{false};
    QStringList warnings;
    QStringList blockingIssues;

    [[nodiscard]] bool hasBlockingIssues() const
    {
        return !blockingIssues.isEmpty();
    }
};

class DicomMetadataAudit
{
public:
    [[nodiscard]] DicomMetadataAuditResult evaluateSeries(const Series& series) const;
    [[nodiscard]] AuditEvent toAuditEvent(const DicomMetadataAuditResult& result, const QString& seriesInstanceUid) const;
};
