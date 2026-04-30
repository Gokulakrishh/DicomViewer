#pragma once

#include "ViewerTools/Measurements/MeasurementTypes.h"

#include <optional>

struct RectangleRoiSamplingBounds
{
    int minColumn{0};
    int maxColumn{0};
    int minRow{0};
    int maxRow{0};
    int fixedSliceIndex{0};
    double sampleAreaMm2{0.0};
};

class IMeasurementScalarSource
{
public:
    virtual ~IMeasurementScalarSource() = default;

    [[nodiscard]] virtual std::optional<RectangleRoiSamplingBounds> rectangleRoiSamplingBounds(
        const MeasurementAnnotation& measurement) const = 0;
    [[nodiscard]] virtual bool sampleRectangleRoiValue(
        const RectangleRoiSamplingBounds& bounds,
        int column,
        int row,
        double& value) const = 0;
};
