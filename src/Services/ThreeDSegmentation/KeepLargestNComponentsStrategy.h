#pragma once

#include "Services/IConnectedComponentStrategy.h"

enum class ConnectedComponentKeepPreset
{
    LargestOne,
    LargestTwo,
    LargestThree,
    CustomCount,
};

struct ConnectedComponentSelectionParameters
{
    ConnectedComponentKeepPreset preset{ConnectedComponentKeepPreset::LargestOne};
    int customCount{1};
    bool excludeBorderTouchingComponents{false};
};

class KeepLargestNComponentsStrategy final : public IConnectedComponentStrategy
{
public:
    explicit KeepLargestNComponentsStrategy(
        ConnectedComponentSelectionParameters parameters = {});

    [[nodiscard]] std::shared_ptr<ISegmentationMask> filter(const ISegmentationMask& mask) const override;
    [[nodiscard]] const ConnectedComponentSelectionParameters& parameters() const;

private:
    [[nodiscard]] int componentCountToKeep() const;

private:
    ConnectedComponentSelectionParameters m_parameters;
};
