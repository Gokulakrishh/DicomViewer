#include "VolumeResampleService.h"

#include "Model/VolumeData.h"

#include <QDebug>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

namespace
{
VolumeVector3D add(const VolumeVector3D& left, const VolumeVector3D& right)
{
    return {left.x + right.x, left.y + right.y, left.z + right.z};
}

VolumeVector3D subtract(const VolumeVector3D& left, const VolumeVector3D& right)
{
    return {left.x - right.x, left.y - right.y, left.z - right.z};
}

VolumeVector3D multiplyDirection(
    const std::array<double, 9>& direction,
    const VolumeVector3D& vector)
{
    return {
        (direction[0] * vector.x) + (direction[1] * vector.y) + (direction[2] * vector.z),
        (direction[3] * vector.x) + (direction[4] * vector.y) + (direction[5] * vector.z),
        (direction[6] * vector.x) + (direction[7] * vector.y) + (direction[8] * vector.z)};
}

VolumeVector3D multiplyDirectionTranspose(
    const std::array<double, 9>& direction,
    const VolumeVector3D& vector)
{
    return {
        (direction[0] * vector.x) + (direction[3] * vector.y) + (direction[6] * vector.z),
        (direction[1] * vector.x) + (direction[4] * vector.y) + (direction[7] * vector.z),
        (direction[2] * vector.x) + (direction[5] * vector.y) + (direction[8] * vector.z)};
}

VolumeVector3D sourceWorldCoordinate(const VolumeGeometry& geometry, int x, int y, int z)
{
    const VolumeVector3D local{
        static_cast<double>(x) * geometry.spacing.x,
        static_cast<double>(y) * geometry.spacing.y,
        static_cast<double>(z) * geometry.spacing.z};
    return add(geometry.origin, multiplyDirection(geometry.direction, local));
}
}

std::shared_ptr<IVolumeData> VolumeResampleService::resampleIsotropic(
    const IVolumeData& sourceVolume,
    double targetSpacing) const
{
    const VolumeGeometry& sourceGeometry = sourceVolume.geometry();
    if (!sourceGeometry.isValid())
    {
        return {};
    }

    if (targetSpacing <= 0.0)
    {
        targetSpacing = std::min({sourceGeometry.spacing.x, sourceGeometry.spacing.y, sourceGeometry.spacing.z});
    }

    qDebug().nospace()
        << "VolumeResampleService source geometry:"
        << " dims=(" << sourceGeometry.dimensions.x << ", " << sourceGeometry.dimensions.y << ", " << sourceGeometry.dimensions.z << ")"
        << " spacing=(" << sourceGeometry.spacing.x << ", " << sourceGeometry.spacing.y << ", " << sourceGeometry.spacing.z << ")"
        << " origin=(" << sourceGeometry.origin.x << ", " << sourceGeometry.origin.y << ", " << sourceGeometry.origin.z << ")"
        << " direction=["
        << sourceGeometry.direction[0] << ", " << sourceGeometry.direction[1] << ", " << sourceGeometry.direction[2] << "; "
        << sourceGeometry.direction[3] << ", " << sourceGeometry.direction[4] << ", " << sourceGeometry.direction[5] << "; "
        << sourceGeometry.direction[6] << ", " << sourceGeometry.direction[7] << ", " << sourceGeometry.direction[8] << "]"
        << " targetSpacing=" << targetSpacing;

    if (targetSpacing <= 0.0)
    {
        throw std::invalid_argument("Target spacing must be positive");
    }

    const std::array<int, 2> xCorners{0, std::max(0, sourceGeometry.dimensions.x - 1)};
    const std::array<int, 2> yCorners{0, std::max(0, sourceGeometry.dimensions.y - 1)};
    const std::array<int, 2> zCorners{0, std::max(0, sourceGeometry.dimensions.z - 1)};

    VolumeVector3D minimumCenterWorld{
        std::numeric_limits<double>::max(),
        std::numeric_limits<double>::max(),
        std::numeric_limits<double>::max()};
    VolumeVector3D maximumCenterWorld{
        std::numeric_limits<double>::lowest(),
        std::numeric_limits<double>::lowest(),
        std::numeric_limits<double>::lowest()};

    for (const int x : xCorners)
    {
        for (const int y : yCorners)
        {
            for (const int z : zCorners)
            {
                const VolumeVector3D world = sourceWorldCoordinate(sourceGeometry, x, y, z);
                minimumCenterWorld.x = std::min(minimumCenterWorld.x, world.x);
                minimumCenterWorld.y = std::min(minimumCenterWorld.y, world.y);
                minimumCenterWorld.z = std::min(minimumCenterWorld.z, world.z);
                maximumCenterWorld.x = std::max(maximumCenterWorld.x, world.x);
                maximumCenterWorld.y = std::max(maximumCenterWorld.y, world.y);
                maximumCenterWorld.z = std::max(maximumCenterWorld.z, world.z);
            }
        }
    }

    const auto computeDimension = [targetSpacing](double minimumCoordinate, double maximumCoordinate) {
        const double centerExtent = std::max(0.0, maximumCoordinate - minimumCoordinate);
        return std::max(1, static_cast<int>(std::lround(centerExtent / targetSpacing)) + 1);
    };

    VolumeGeometry resampledGeometry;
    resampledGeometry.dimensions = {
        computeDimension(minimumCenterWorld.x, maximumCenterWorld.x),
        computeDimension(minimumCenterWorld.y, maximumCenterWorld.y),
        computeDimension(minimumCenterWorld.z, maximumCenterWorld.z)};
    resampledGeometry.spacing = {targetSpacing, targetSpacing, targetSpacing};
    resampledGeometry.origin = minimumCenterWorld;
    resampledGeometry.direction = {
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0};

    qDebug().nospace()
        << "VolumeResampleService resampled geometry:"
        << " centerWorldMin=(" << minimumCenterWorld.x << ", " << minimumCenterWorld.y << ", " << minimumCenterWorld.z << ")"
        << " centerWorldMax=(" << maximumCenterWorld.x << ", " << maximumCenterWorld.y << ", " << maximumCenterWorld.z << ")"
        << " dims=(" << resampledGeometry.dimensions.x << ", " << resampledGeometry.dimensions.y << ", " << resampledGeometry.dimensions.z << ")"
        << " spacing=(" << resampledGeometry.spacing.x << ", " << resampledGeometry.spacing.y << ", " << resampledGeometry.spacing.z << ")";

    std::vector<int16_t> voxels;
    voxels.reserve(resampledGeometry.voxelCount());

    for (int z = 0; z < resampledGeometry.dimensions.z; ++z)
    {
        for (int y = 0; y < resampledGeometry.dimensions.y; ++y)
        {
            for (int x = 0; x < resampledGeometry.dimensions.x; ++x)
            {
                const VolumeVector3D targetWorld{
                    minimumCenterWorld.x + (static_cast<double>(x) * targetSpacing),
                    minimumCenterWorld.y + (static_cast<double>(y) * targetSpacing),
                    minimumCenterWorld.z + (static_cast<double>(z) * targetSpacing)};
                const VolumeVector3D deltaWorld = subtract(targetWorld, sourceGeometry.origin);
                const VolumeVector3D sourceLocal = multiplyDirectionTranspose(sourceGeometry.direction, deltaWorld);
                const double sourceX = sourceLocal.x / sourceGeometry.spacing.x;
                const double sourceY = sourceLocal.y / sourceGeometry.spacing.y;
                const double sourceZ = sourceLocal.z / sourceGeometry.spacing.z;
                const double sampledValue = sampleTrilinear(sourceVolume, sourceX, sourceY, sourceZ);
                voxels.push_back(static_cast<int16_t>(std::clamp(
                    static_cast<int>(std::lround(sampledValue)),
                    -32768,
                    32767)));
            }
        }
    }

    return std::make_shared<VolumeData<int16_t>>(std::move(resampledGeometry), std::move(voxels));
}

double VolumeResampleService::sampleTrilinear(const IVolumeData& volume, double x, double y, double z) const
{
    const VolumeGeometry& geometry = volume.geometry();
    const auto clampCoordinate = [](double value, int extent) {
        return std::clamp(value, 0.0, static_cast<double>(std::max(0, extent - 1)));
    };

    x = clampCoordinate(x, geometry.dimensions.x);
    y = clampCoordinate(y, geometry.dimensions.y);
    z = clampCoordinate(z, geometry.dimensions.z);

    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));
    const int z0 = static_cast<int>(std::floor(z));
    const int x1 = std::min(x0 + 1, geometry.dimensions.x - 1);
    const int y1 = std::min(y0 + 1, geometry.dimensions.y - 1);
    const int z1 = std::min(z0 + 1, geometry.dimensions.z - 1);

    const double fx = x - static_cast<double>(x0);
    const double fy = y - static_cast<double>(y0);
    const double fz = z - static_cast<double>(z0);

    const double c000 = volume.scalarAt(x0, y0, z0);
    const double c100 = volume.scalarAt(x1, y0, z0);
    const double c010 = volume.scalarAt(x0, y1, z0);
    const double c110 = volume.scalarAt(x1, y1, z0);
    const double c001 = volume.scalarAt(x0, y0, z1);
    const double c101 = volume.scalarAt(x1, y0, z1);
    const double c011 = volume.scalarAt(x0, y1, z1);
    const double c111 = volume.scalarAt(x1, y1, z1);

    const double c00 = c000 + ((c100 - c000) * fx);
    const double c10 = c010 + ((c110 - c010) * fx);
    const double c01 = c001 + ((c101 - c001) * fx);
    const double c11 = c011 + ((c111 - c011) * fx);
    const double c0 = c00 + ((c10 - c00) * fy);
    const double c1 = c01 + ((c11 - c01) * fy);
    return c0 + ((c1 - c0) * fz);
}
