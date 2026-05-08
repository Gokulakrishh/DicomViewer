#pragma once

#include "Model/IVolumeData.h"

#include <memory>

/**
 * @brief Resamples scalar volumes for isotropic 3D workflows.
 *
 * Responsibilities:
 * - Create isotropic volume data from anisotropic DICOM spacing.
 * - Use interpolation without modifying the source volume.
 */
class VolumeResampleService
{
public:
    /**
     * @brief Resamples a volume to isotropic spacing.
     * @param sourceVolume Source scalar volume.
     * @param targetSpacing Target spacing; 0 selects an automatic spacing.
     * @return Resampled volume.
     */
    std::shared_ptr<IVolumeData> resampleIsotropic(
        const IVolumeData& sourceVolume,
        double targetSpacing = 0.0) const;

private:
    double sampleTrilinear(const IVolumeData& volume, double x, double y, double z) const;
};
