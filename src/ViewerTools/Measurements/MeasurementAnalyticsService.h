#pragma once

#include "ViewerTools/Measurements/IMeasurementScalarSource.h"

#include <optional>

/**
 * @brief Stateless analytics helpers for measurements and ROI annotations.
 *
 * Responsibilities:
 * - Compute ROI statistics from a scalar source.
 * - Produce display labels for measurement overlays.
 */
class MeasurementAnalyticsService
{
public:
    /** @brief Computes rectangular ROI statistics. */
    [[nodiscard]] static std::optional<RoiStatistics> rectangleRoiStatistics(
        const MeasurementAnnotation& measurement,
        const IMeasurementScalarSource& scalarSource);
    /** @brief Formats a rectangular ROI label. */
    [[nodiscard]] static QString rectangleRoiLabel(
        const MeasurementAnnotation& measurement,
        const IMeasurementScalarSource& scalarSource);
};
