#pragma once

#include <memory>

class IVolumeData;

struct PreparedVolumeRenderInputs
{
    std::shared_ptr<IVolumeData> baseVolume;
    std::shared_ptr<IVolumeData> boneFocusedVolume;
};

class VolumeRenderPreprocessingService
{
public:
    struct Settings
    {
        short boneLowerThresholdHu{220};
        short outsideValueHu{-1024};
    };

    VolumeRenderPreprocessingService();
    explicit VolumeRenderPreprocessingService(Settings settings);

    [[nodiscard]] PreparedVolumeRenderInputs prepare(const std::shared_ptr<IVolumeData>& diagnosticVolume) const;

private:
    [[nodiscard]] std::shared_ptr<IVolumeData> createLowerThresholdMaskedVolume(const IVolumeData& diagnosticVolume) const;

private:
    Settings m_settings;
};
