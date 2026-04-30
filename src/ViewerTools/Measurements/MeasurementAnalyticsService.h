#pragma once

#include "ViewerTools/Measurements/IMeasurementScalarSource.h"

#include <optional>

class MeasurementAnalyticsService
{
public:
    [[nodiscard]] static std::optional<RoiStatistics> rectangleRoiStatistics(
        const MeasurementAnnotation& measurement,
        const IMeasurementScalarSource& scalarSource);
    [[nodiscard]] static QString rectangleRoiLabel(
        const MeasurementAnnotation& measurement,
        const IMeasurementScalarSource& scalarSource);
};
