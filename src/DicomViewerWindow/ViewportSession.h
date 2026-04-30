#pragma once

#include <memory>

class DicomImage;
class Series;

enum class ViewportWindowPreset
{
    Custom = 0,
    Brain,
    SoftTissue,
    Bone,
    Lung
};

class ViewportSession
{
public:
    void clear();

    std::shared_ptr<Series> currentSeries;
    std::shared_ptr<DicomImage> singleImage;
    int currentImageIndex{-1};
    int currentWindowLevel{0};
    int currentWindowWidth{100};
    ViewportWindowPreset currentPreset{ViewportWindowPreset::Custom};
    int currentDicomWindowPresetIndex{-1};
    bool windowStateInitialized{false};
    int seriesGeneration{0};
    bool cinePlaying{false};
};
