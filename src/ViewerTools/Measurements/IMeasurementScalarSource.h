#pragma once

#include "ViewerTools/Measurements/MeasurementTypes.h"

#include <optional>

/**
 * @brief Pixel sampling bounds for rectangular ROI statistics.
 */
struct RectangleRoiSamplingBounds
{
    int minColumn{0};
    int maxColumn{0};
    int minRow{0};
    int maxRow{0};
    int fixedSliceIndex{0};
    double sampleAreaMm2{0.0};
};

/**
 * @brief Scalar sampling source for measurement analytics.
 *
 * Responsibilities:
 * - Convert ROI geometry into pixel sampling bounds.
 * - Return calibrated scalar values used for ROI statistics.
 */
class IMeasurementScalarSource
{
public:
    virtual ~IMeasurementScalarSource() = default;

    /** @brief Computes sampling bounds for a rectangle ROI. */
    [[nodiscard]] virtual std::optional<RectangleRoiSamplingBounds> rectangleRoiSamplingBounds(
        const MeasurementAnnotation& measurement) const = 0;
    /** @brief Samples one scalar value inside ROI bounds. */
    [[nodiscard]] virtual bool sampleRectangleRoiValue(
        const RectangleRoiSamplingBounds& bounds,
        int column,
        int row,
        double& value) const = 0;
};
