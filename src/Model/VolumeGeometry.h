#pragma once

#include <array>

struct VolumeIndex3D
{
    int x{0};
    int y{0};
    int z{0};
};

struct VolumeVector3D
{
    double x{0.0};
    double y{0.0};
    double z{0.0};
};

struct VolumeGeometry
{
    VolumeIndex3D dimensions;
    VolumeVector3D spacing{1.0, 1.0, 1.0};
    VolumeVector3D origin{0.0, 0.0, 0.0};
    std::array<double, 9> direction{
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0};

    [[nodiscard]] bool isValid() const
    {
        return dimensions.x > 0 && dimensions.y > 0 && dimensions.z > 0 &&
               spacing.x > 0.0 && spacing.y > 0.0 && spacing.z > 0.0;
    }

    [[nodiscard]] int voxelCount() const
    {
        return dimensions.x * dimensions.y * dimensions.z;
    }
};
