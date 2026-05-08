#pragma once

#include <memory>

class ISegmentationMask;

/**
 * @brief Strategy interface for filtering segmented connected components.
 *
 * Responsibilities:
 * - Remove unwanted mask components before mesh extraction.
 * - Keep anatomy-specific component selection in 3D profiles.
 */
class IConnectedComponentStrategy
{
public:
    virtual ~IConnectedComponentStrategy() = default;

    /**
     * @brief Filters a segmentation mask.
     * @param mask Input foreground mask.
     * @return Filtered mask, or null on failure.
     */
    [[nodiscard]] virtual std::shared_ptr<ISegmentationMask> filter(const ISegmentationMask& mask) const = 0;
};
