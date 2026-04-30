#pragma once

#include "ViewerTools/Measurements/MeasurementTypes.h"

#include <QDateTime>
#include <optional>

struct SliceMeasurementAnnotationRecord
{
    QString seriesInstanceUid;
    QString sopInstanceUid;
    MeasurementAnnotation measurement;
    std::optional<double> angleDegrees;
    std::optional<RoiStatistics> roiStatistics;
    QDateTime createdAtUtc;
    QDateTime updatedAtUtc;
    bool deleted{false};
};
