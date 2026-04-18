#pragma once

#include <memory>
#include <string_view>

class ISegmentationStrategy;
class IConnectedComponentStrategy;
class IMeshExtractionStrategy;
class IMeshPostProcessor;

class I3dPipelineProfile
{
public:
    virtual ~I3dPipelineProfile() = default;

    [[nodiscard]] virtual std::string_view name() const = 0;
    [[nodiscard]] virtual std::shared_ptr<ISegmentationStrategy> createSegmentationStrategy() const = 0;
    [[nodiscard]] virtual std::shared_ptr<IConnectedComponentStrategy> createConnectedComponentStrategy() const = 0;
    [[nodiscard]] virtual std::shared_ptr<IMeshExtractionStrategy> createMeshExtractionStrategy() const = 0;
    [[nodiscard]] virtual std::shared_ptr<IMeshPostProcessor> createMeshPostProcessor() const = 0;
};
