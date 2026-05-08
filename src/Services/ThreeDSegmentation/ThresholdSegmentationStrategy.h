#pragma once

#include "Services/ISegmentationStrategy.h"

/**
 * @brief Numeric threshold range used for volume segmentation.
 */
struct ThresholdSegmentationParameters
{
    double low{0.0};
    double high{0.0};
    bool inclusiveLower{true};
    bool inclusiveUpper{true};
};

/**
 * @brief Segmentation strategy that thresholds scalar volume values.
 *
 * Responsibilities:
 * - Create a binary mask from a configured scalar range.
 * - Support CT HU-based anatomy profile presets.
 */
class ThresholdSegmentationStrategy final : public ISegmentationStrategy
{
public:
    /**
     * @brief Creates a threshold segmentation strategy.
     * @param parameters Threshold range and inclusivity flags.
     */
    explicit ThresholdSegmentationStrategy(ThresholdSegmentationParameters parameters);

    /**
     * @brief Segments a volume by thresholding scalar values.
     * @param volume Source volume.
     * @return Foreground mask.
     */
    [[nodiscard]] std::shared_ptr<ISegmentationMask> segment(const IVolumeData& volume) const override;
    /** @brief Returns threshold parameters. */
    [[nodiscard]] const ThresholdSegmentationParameters& parameters() const;

private:
    [[nodiscard]] bool isInsideThreshold(double value) const;

private:
    ThresholdSegmentationParameters m_parameters;
};
