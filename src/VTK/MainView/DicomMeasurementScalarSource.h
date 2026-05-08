#pragma once

#include "ViewerTools/Measurements/IMeasurementScalarSource.h"

class DicomImage;
class VtkSliceSceneAdapter;

/**
 * @brief ROI scalar source for the main diagnostic slice view.
 */
class DicomMeasurementScalarSource final : public IMeasurementScalarSource
{
public:
    /** @brief Creates the source for one displayed DICOM image. */
    DicomMeasurementScalarSource(const DicomImage* image, const VtkSliceSceneAdapter& sceneAdapter);

    /** @brief Computes rectangle ROI sampling bounds. */
    [[nodiscard]] std::optional<RectangleRoiSamplingBounds> rectangleRoiSamplingBounds(
        const MeasurementAnnotation& measurement) const override;
    /** @brief Samples one scalar value inside ROI bounds. */
    [[nodiscard]] bool sampleRectangleRoiValue(
        const RectangleRoiSamplingBounds& bounds,
        int column,
        int row,
        double& value) const override;

private:
    const DicomImage* m_image{nullptr};
    const VtkSliceSceneAdapter& m_sceneAdapter;
};
