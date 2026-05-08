#pragma once

#include "ThreeDimensionalPipelineResult.h"

class I3dPipelineProfile;
class IVolumeData;

/**
 * @brief Orchestrates segmentation, component filtering, mesh extraction, and post-processing.
 *
 * Responsibilities:
 * - Execute the concrete strategies provided by a 3D pipeline profile.
 * - Return intermediate and final outputs for rendering and diagnostics.
 */
class ThreeDimensionalPipelineService
{
public:
    /**
     * @brief Builds a mesh from a diagnostic volume using a profile.
     * @param volume Source scalar volume.
     * @param profile Pipeline profile providing concrete strategies.
     * @return Pipeline result with masks, mesh, and diagnostics.
     */
    [[nodiscard]] ThreeDimensionalPipelineResult buildMesh(
        const IVolumeData& volume,
        const I3dPipelineProfile& profile) const;

private:
    [[nodiscard]] static int countForegroundVoxels(const ISegmentationMask& mask);
};
