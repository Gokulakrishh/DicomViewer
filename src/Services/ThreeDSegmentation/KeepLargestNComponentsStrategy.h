#pragma once

#include "Services/IConnectedComponentStrategy.h"

/**
 * @brief Preset for choosing which connected components to keep.
 */
enum class ConnectedComponentKeepPreset
{
    LargestOne,
    LargestTwo,
    LargestThree,
    CustomCount,
};

/**
 * @brief Parameters for connected-component filtering.
 */
struct ConnectedComponentSelectionParameters
{
    ConnectedComponentKeepPreset preset{ConnectedComponentKeepPreset::LargestOne};
    int customCount{1};
    bool excludeBorderTouchingComponents{false};
};

/**
 * @brief Connected-component filter that keeps the largest foreground components.
 *
 * Responsibilities:
 * - Remove small disconnected structures after segmentation.
 * - Support anatomy profile presets and custom counts.
 */
class KeepLargestNComponentsStrategy final : public IConnectedComponentStrategy
{
public:
    /**
     * @brief Creates the component filter.
     * @param parameters Component selection parameters.
     */
    explicit KeepLargestNComponentsStrategy(
        ConnectedComponentSelectionParameters parameters = {});

    /**
     * @brief Filters a segmentation mask by connected component size.
     * @param mask Input mask.
     * @return Filtered mask.
     */
    [[nodiscard]] std::shared_ptr<ISegmentationMask> filter(const ISegmentationMask& mask) const override;
    /** @brief Returns component selection parameters. */
    [[nodiscard]] const ConnectedComponentSelectionParameters& parameters() const;

private:
    [[nodiscard]] int componentCountToKeep() const;

private:
    ConnectedComponentSelectionParameters m_parameters;
};
