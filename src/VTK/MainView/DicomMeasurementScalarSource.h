#pragma once

#include "ViewerTools/Measurements/IMeasurementScalarSource.h"

class DicomImage;
class VtkSliceSceneAdapter;

class DicomMeasurementScalarSource final : public IMeasurementScalarSource
{
public:
    DicomMeasurementScalarSource(const DicomImage* image, const VtkSliceSceneAdapter& sceneAdapter);

    [[nodiscard]] std::optional<RectangleRoiSamplingBounds> rectangleRoiSamplingBounds(
        const MeasurementAnnotation& measurement) const override;
    [[nodiscard]] bool sampleRectangleRoiValue(
        const RectangleRoiSamplingBounds& bounds,
        int column,
        int row,
        double& value) const override;

private:
    const DicomImage* m_image{nullptr};
    const VtkSliceSceneAdapter& m_sceneAdapter;
};
