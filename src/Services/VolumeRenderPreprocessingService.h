#pragma once

#include <memory>

class IVolumeData;

/**
 * @brief Volumes prepared for VTK volume rendering profiles.
 */
struct PreparedVolumeRenderInputs
{
    std::shared_ptr<IVolumeData> baseVolume;
    std::shared_ptr<IVolumeData> boneFocusedVolume;
};

/**
 * @brief Prepares diagnostic volumes for volume rendering.
 *
 * Responsibilities:
 * - Preserve the base diagnostic volume.
 * - Create derived thresholded volumes for focused rendering presets.
 */
class VolumeRenderPreprocessingService
{
public:
    /**
     * @brief HU threshold settings used during preprocessing.
     */
    struct Settings
    {
        short boneLowerThresholdHu{220};
        short outsideValueHu{-1024};
    };

    /** @brief Creates the preprocessing service with default settings. */
    VolumeRenderPreprocessingService();

    /**
     * @brief Creates the preprocessing service.
     * @param settings Thresholding settings.
     */
    explicit VolumeRenderPreprocessingService(Settings settings);

    /**
     * @brief Prepares rendering inputs for a diagnostic volume.
     * @param diagnosticVolume Source volume.
     * @return Prepared base and derived volumes.
     */
    [[nodiscard]] PreparedVolumeRenderInputs prepare(const std::shared_ptr<IVolumeData>& diagnosticVolume) const;

private:
    [[nodiscard]] std::shared_ptr<IVolumeData> createLowerThresholdMaskedVolume(const IVolumeData& diagnosticVolume) const;

private:
    Settings m_settings;
};
