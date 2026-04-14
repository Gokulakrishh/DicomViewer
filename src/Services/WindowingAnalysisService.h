#pragma once

class DicomImage;

class WindowingAnalysisService
{
public:
    enum class Preset
    {
        None = 0,
        GeneralHeadCt,
        BrainFocused,
        BoneHeavy
    };

    struct WindowingResult
    {
        bool valid{false};
        int lowValue{0};
        int highValue{0};
        int windowLevel{0};
        int windowWidth{1};
    };

    WindowingResult analyzePercentiles(
        const DicomImage& image,
        double lowPercentile,
        double highPercentile) const;

    WindowingResult analyzePreset(const DicomImage& image, Preset preset) const;
};
