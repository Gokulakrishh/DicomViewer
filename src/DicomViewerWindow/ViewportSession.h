#pragma once

#include <memory>

class DicomImage;
class Series;

class ViewportSession
{
public:
    void clear();

    std::shared_ptr<Series> currentSeries;
    std::shared_ptr<DicomImage> singleImage;
    int currentImageIndex{-1};
    int currentWindowLevel{0};
    int currentWindowWidth{100};
    int currentPresetIndex{0};
    bool windowStateInitialized{false};
    int seriesGeneration{0};
    int toolIndex{0};
    bool cinePlaying{false};
};
