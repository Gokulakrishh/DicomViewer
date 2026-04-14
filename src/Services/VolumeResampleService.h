#pragma once

#include "Model/IVolumeData.h"

#include <memory>

class VolumeResampleService
{
public:
    std::shared_ptr<IVolumeData> resampleIsotropic(
        const IVolumeData& sourceVolume,
        double targetSpacing = 0.0) const;

private:
    double sampleTrilinear(const IVolumeData& volume, double x, double y, double z) const;
};
