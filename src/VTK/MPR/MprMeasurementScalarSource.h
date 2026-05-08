#pragma once

#include "ViewerTools/Measurements/IMeasurementScalarSource.h"
#include "VTK/MPR/MprTypes.h"

class IVolumeData;
class VtkMprSceneAdapter;

/**
 * @brief ROI scalar source for MPR measurement analytics.
 *
 * Responsibilities:
 * - Map MPR measurement geometry to volume voxel sampling bounds.
 * - Sample scalar values from the diagnostic volume.
 */
class MprMeasurementScalarSource final : public IMeasurementScalarSource
{
public:
    /** @brief Creates the scalar source for one MPR plane. */
    MprMeasurementScalarSource(
        const IVolumeData* volume,
        const VtkMprSceneAdapter& sceneAdapter,
        MprSlicePlane plane);

    /** @brief Computes ROI sampling bounds for the active MPR plane. */
    [[nodiscard]] std::optional<RectangleRoiSamplingBounds> rectangleRoiSamplingBounds(
        const MeasurementAnnotation& measurement) const override;
    /** @brief Samples one ROI scalar value. */
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
