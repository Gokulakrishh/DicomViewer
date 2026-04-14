#pragma once

#include "VolumeGeometry.h"

class IVolumeData
{
public:
    virtual ~IVolumeData() = default;

    [[nodiscard]] virtual const VolumeGeometry& geometry() const = 0;
    [[nodiscard]] virtual double scalarAt(int x, int y, int z) const = 0;
    [[nodiscard]] virtual bool isValidIndex(int x, int y, int z) const = 0;
};
