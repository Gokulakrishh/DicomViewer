#pragma once

#include <memory>

class IVolumeData;
class ISegmentationMask;

/**
 * @brief Strategy interface for segmenting scalar volume data.
 *
 * Responsibilities:
 * - Convert a diagnostic volume into a foreground mask.
 * - Keep 3D pipeline profiles independent of concrete segmentation algorithms.
 */
class ISegmentationStrategy
{
public:
    virtual ~ISegmentationStrategy() = default;

    /**
     * @brief Segments a volume.
     * @param volume Source scalar volume.
     * @return Foreground mask, or null on failure.
     */
    [[nodiscard]] virtual std::shared_ptr<ISegmentationMask> segment(const IVolumeData& volume) const = 0;
};
