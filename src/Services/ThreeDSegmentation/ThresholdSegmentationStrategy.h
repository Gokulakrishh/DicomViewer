#pragma once

#include "Services/ISegmentationStrategy.h"

struct ThresholdSegmentationParameters
{
    double low{0.0};
    double high{0.0};
    bool inclusiveLower{true};
    bool inclusiveUpper{true};
};

class ThresholdSegmentationStrategy final : public ISegmentationStrategy
{
public:
    explicit ThresholdSegmentationStrategy(ThresholdSegmentationParameters parameters);

    [[nodiscard]] std::shared_ptr<ISegmentationMask> segment(const IVolumeData& volume) const override;
    [[nodiscard]] const ThresholdSegmentationParameters& parameters() const;

private:
    [[nodiscard]] bool isInsideThreshold(double value) const;

private:
    ThresholdSegmentationParameters m_parameters;
};
