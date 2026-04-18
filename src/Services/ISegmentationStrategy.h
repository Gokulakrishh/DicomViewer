#pragma once

#include <memory>

class IVolumeData;
class ISegmentationMask;

class ISegmentationStrategy
{
public:
    virtual ~ISegmentationStrategy() = default;

    [[nodiscard]] virtual std::shared_ptr<ISegmentationMask> segment(const IVolumeData& volume) const = 0;
};
