#pragma once

#include "VolumeGeometry.h"

class ISegmentationMask
{
public:
    virtual ~ISegmentationMask() = default;

    [[nodiscard]] virtual const VolumeGeometry& geometry() const = 0;
    [[nodiscard]] virtual bool isValidIndex(int x, int y, int z) const = 0;
    [[nodiscard]] virtual bool isForeground(int x, int y, int z) const = 0;
};
