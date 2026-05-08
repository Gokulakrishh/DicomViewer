#pragma once

#include "Errors/AppResult.h"
#include "Utilities/VolumeValidationSettings.h"
#include "Model/IVolumeData.h"

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

class DicomImage;
class Series;

/**
 * @brief Constructs scalar volume data from a loaded DICOM series.
 *
 * Responsibilities:
 * - Validate slice dimensions, orientation, pixel spacing, and slice spacing.
 * - Sort slices by patient-space geometry.
 * - Build a contiguous voxel buffer for MPR and 3D workflows.
 *
 * Assumptions:
 * - Input images already contain raw pixel data.
 * - Geometry validation failures should block derived volume construction.
 */
class VolumeBuilder
{
public:
    /**
     * @brief Creates a volume builder.
     * @param validationSettings Geometry validation tolerances.
     */
    explicit VolumeBuilder(VolumeValidationSettings validationSettings = {});

    /**
     * @brief Builds a volume from a diagnostic DICOM series.
     * @param diagnosticSeries Series with loaded raw pixels.
     * @return Volume data or structured validation error.
     */
    AppResult<std::shared_ptr<IVolumeData>> buildFromDiagnosticSeries(const Series& diagnosticSeries) const;

private:
    struct SliceBasis
    {
        std::array<double, 3> row{1.0, 0.0, 0.0};
        std::array<double, 3> column{0.0, 1.0, 0.0};
        std::array<double, 3> normal{0.0, 0.0, 1.0};
    };

    struct VolumeInput
    {
        const DicomImage* firstImage{nullptr};
        int width{0};
        int height{0};
        int depth{0};
        std::vector<const DicomImage*> orderedImages;
    };

    static VolumeInput collectVolumeInput(const Series& series);
    static std::array<double, 3> normalizeVector(const std::array<double, 3>& vector);
    static double dotProduct(const std::array<double, 3>& left, const std::array<double, 3>& right);
    static std::array<double, 3> crossProduct(
        const std::array<double, 3>& left,
        const std::array<double, 3>& right);
    static SliceBasis deriveSliceBasis(const DicomImage& firstImage);
    static void validateSliceGeometry(
        const VolumeInput& input,
        const SliceBasis& basis,
        const VolumeValidationSettings& settings);
    static void validateImageOrientation(
        const DicomImage& image,
        const SliceBasis& referenceBasis,
        const VolumeValidationSettings& settings);
    static void validateImageSpacing(
        const DicomImage& image,
        const DicomImage& referenceImage,
        const VolumeValidationSettings& settings);
    static void validateSliceSpacing(
        const VolumeInput& input,
        const SliceBasis& basis,
        const VolumeValidationSettings& settings);
    static double sliceCoordinate(const DicomImage& image, const SliceBasis& basis);
    static void sortSlicesByGeometry(std::vector<const DicomImage*>& orderedImages, const SliceBasis& basis);
    static VolumeGeometry buildGeometry(const VolumeInput& input, const SliceBasis& basis);
    static std::vector<int16_t> buildVoxelBuffer(const VolumeInput& input);

private:
    VolumeValidationSettings m_validationSettings;
};
