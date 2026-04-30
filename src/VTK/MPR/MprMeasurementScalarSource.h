#pragma once

#include "ViewerTools/Measurements/IMeasurementScalarSource.h"
#include "VTK/MPR/MprTypes.h"

class IVolumeData;
class VtkMprSceneAdapter;

class MprMeasurementScalarSource final : public IMeasurementScalarSource
{
public:
    MprMeasurementScalarSource(
        const IVolumeData* volume,
        const VtkMprSceneAdapter& sceneAdapter,
        MprSlicePlane plane);

    [[nodiscard]] std::optional<RectangleRoiSamplingBounds> rectangleRoiSamplingBounds(
        const MeasurementAnnotation& measurement) const override;
    [[nodiscard]] bool sampleRectangleRoiValue(
        const RectangleRoiSamplingBounds& bounds,
        int column,
        int row,
        double& value) const override;

private:
    const IVolumeData* m_volume{nullptr};
    const VtkMprSceneAdapter& m_sceneAdapter;
    MprSlicePlane m_plane{MprSlicePlane::Axial};
};
