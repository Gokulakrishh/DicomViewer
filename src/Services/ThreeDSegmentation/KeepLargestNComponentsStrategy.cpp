#include "Services/ThreeDSegmentation/KeepLargestNComponentsStrategy.h"

#include "Model/ISegmentationMask.h"
#include "Model/SegmentationMaskData.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <deque>
#include <memory>
#include <stdexcept>
#include <vector>

namespace
{
struct Index3D
{
    int x{0};
    int y{0};
    int z{0};
};
}

KeepLargestNComponentsStrategy::KeepLargestNComponentsStrategy(
    ConnectedComponentSelectionParameters parameters)
    : m_parameters(parameters)
{
    if (componentCountToKeep() <= 0)
    {
        throw std::invalid_argument("Connected component count to keep must be positive");
    }
}

std::shared_ptr<ISegmentationMask> KeepLargestNComponentsStrategy::filter(const ISegmentationMask& mask) const
{
    const VolumeGeometry& geometry = mask.geometry();
    const int voxelCount = geometry.voxelCount();
    std::vector<std::uint8_t> visited(static_cast<std::size_t>(voxelCount), std::uint8_t{0});
    std::vector<std::uint8_t> output(static_cast<std::size_t>(voxelCount), std::uint8_t{0});
    std::vector<std::vector<int>> largestComponents;
    const int keepCount = componentCountToKeep();
    largestComponents.reserve(static_cast<std::size_t>(keepCount));

    const auto flatIndex = [&geometry](int x, int y, int z) {
        return (z * geometry.dimensions.y * geometry.dimensions.x) +
               (y * geometry.dimensions.x) +
               x;
    };

    const std::array<Index3D, 6> neighbors{
        Index3D{1, 0, 0},
        Index3D{-1, 0, 0},
        Index3D{0, 1, 0},
        Index3D{0, -1, 0},
        Index3D{0, 0, 1},
        Index3D{0, 0, -1}};

    for (int z = 0; z < geometry.dimensions.z; ++z)
    {
        for (int y = 0; y < geometry.dimensions.y; ++y)
        {
            for (int x = 0; x < geometry.dimensions.x; ++x)
            {
                if (!mask.isForeground(x, y, z))
                {
                    continue;
                }

                const int seedIndex = flatIndex(x, y, z);
                if (visited[static_cast<std::size_t>(seedIndex)] != 0)
                {
                    continue;
                }

                std::vector<int> currentComponent;
                std::deque<Index3D> queue;
                bool touchesBorder = false;
                queue.push_back({x, y, z});
                visited[static_cast<std::size_t>(seedIndex)] = 1;

                while (!queue.empty())
                {
                    const Index3D current = queue.front();
                    queue.pop_front();

                    if (current.x == 0 || current.y == 0 || current.z == 0 ||
                        current.x == geometry.dimensions.x - 1 ||
                        current.y == geometry.dimensions.y - 1 ||
                        current.z == geometry.dimensions.z - 1)
                    {
                        touchesBorder = true;
                    }

                    const int currentFlatIndex = flatIndex(current.x, current.y, current.z);
                    currentComponent.push_back(currentFlatIndex);

                    for (const Index3D& neighborOffset : neighbors)
                    {
                        const int nx = current.x + neighborOffset.x;
                        const int ny = current.y + neighborOffset.y;
                        const int nz = current.z + neighborOffset.z;
                        if (!mask.isValidIndex(nx, ny, nz) || !mask.isForeground(nx, ny, nz))
                        {
                            continue;
                        }

                        const int neighborFlatIndex = flatIndex(nx, ny, nz);
                        if (visited[static_cast<std::size_t>(neighborFlatIndex)] != 0)
                        {
                            continue;
                        }

                        visited[static_cast<std::size_t>(neighborFlatIndex)] = 1;
                        queue.push_back({nx, ny, nz});
                    }
                }

                if (m_parameters.excludeBorderTouchingComponents && touchesBorder)
                {
                    continue;
                }

                largestComponents.push_back(std::move(currentComponent));
                std::sort(
                    largestComponents.begin(),
                    largestComponents.end(),
                    [](const std::vector<int>& left, const std::vector<int>& right) {
                        return left.size() > right.size();
                    });

                if (static_cast<int>(largestComponents.size()) > keepCount)
                {
                    largestComponents.resize(static_cast<std::size_t>(keepCount));
                }
            }
        }
    }

    for (const std::vector<int>& component : largestComponents)
    {
        for (const int index : component)
        {
            output[static_cast<std::size_t>(index)] = 1;
        }
    }

    return std::make_shared<SegmentationMaskData<std::uint8_t>>(geometry, std::move(output));
}

int KeepLargestNComponentsStrategy::componentCountToKeep() const
{
    switch (m_parameters.preset)
    {
    case ConnectedComponentKeepPreset::LargestOne:
        return 1;
    case ConnectedComponentKeepPreset::LargestTwo:
        return 2;
    case ConnectedComponentKeepPreset::LargestThree:
        return 3;
    case ConnectedComponentKeepPreset::CustomCount:
        return m_parameters.customCount;
    }

    return 1;
}

const ConnectedComponentSelectionParameters& KeepLargestNComponentsStrategy::parameters() const
{
    return m_parameters;
}
