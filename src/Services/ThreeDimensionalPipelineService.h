#pragma once

#include "ThreeDimensionalPipelineResult.h"

class I3dPipelineProfile;
class IVolumeData;

class ThreeDimensionalPipelineService
{
public:
    [[nodiscard]] ThreeDimensionalPipelineResult buildMesh(
        const IVolumeData& volume,
        const I3dPipelineProfile& profile) const;

private:
    [[nodiscard]] static int countForegroundVoxels(const ISegmentationMask& mask);
};
