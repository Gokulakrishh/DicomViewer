#pragma once

#include <memory>

class ISegmentationMask;

class IConnectedComponentStrategy
{
public:
    virtual ~IConnectedComponentStrategy() = default;

    [[nodiscard]] virtual std::shared_ptr<ISegmentationMask> filter(const ISegmentationMask& mask) const = 0;
};
