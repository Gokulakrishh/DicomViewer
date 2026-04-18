#pragma once

#include <memory>

class ISegmentationMask;
class IMeshData;

class IMeshExtractionStrategy
{
public:
    virtual ~IMeshExtractionStrategy() = default;

    [[nodiscard]] virtual std::shared_ptr<IMeshData> extract(const ISegmentationMask& mask) const = 0;
};
