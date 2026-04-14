#include "Services/WindowingAnalysisService.h"

#include <QVector>
#include <algorithm>
#include <cmath>
#include <vector>

#include "Model/DicomImage.h"

namespace
{
int percentileValue(const std::vector<int>& values, double percentile)
{
    if (values.empty())
    {
        return 0;
    }

    const double clampedPercentile = std::clamp(percentile, 0.0, 100.0);
    const double scaledIndex = (clampedPercentile / 100.0) * static_cast<double>(values.size() - 1);
    const auto index = static_cast<std::size_t>(std::llround(scaledIndex));
    return values[index];
}
}

WindowingAnalysisService::WindowingResult WindowingAnalysisService::analyzePercentiles(
    const DicomImage& image,
    double lowPercentile,
    double highPercentile) const
{
    WindowingResult result;
    if (!image.hasRawPixels() || image.width() <= 0 || image.height() <= 0)
    {
        return result;
    }

    std::vector<int> values;
    values.reserve(static_cast<std::size_t>(image.width() * image.height()));
    for (int y = 0; y < image.height(); ++y)
    {
        for (int x = 0; x < image.width(); ++x)
        {
            values.push_back(image.rawPixelValueAt(x, y));
        }
    }

    if (values.empty())
    {
        return result;
    }

    std::sort(values.begin(), values.end());

    const int lowValue = percentileValue(values, lowPercentile);
    const int highValue = percentileValue(values, highPercentile);
    const int lowerBound = std::min(lowValue, highValue);
    const int upperBound = std::max(lowValue, highValue);

    result.valid = true;
    result.lowValue = lowerBound;
    result.highValue = upperBound;
    result.windowLevel = static_cast<int>(std::lround((static_cast<double>(lowerBound) + static_cast<double>(upperBound)) / 2.0));
    result.windowWidth = std::max(1, upperBound - lowerBound);
    return result;
}

WindowingAnalysisService::WindowingResult WindowingAnalysisService::analyzePreset(
    const DicomImage& image,
    Preset preset) const
{
    switch (preset)
    {
    case Preset::GeneralHeadCt:
        return analyzePercentiles(image, 1.0, 99.0);
    case Preset::BrainFocused:
        return analyzePercentiles(image, 5.0, 95.0);
    case Preset::BoneHeavy:
        return analyzePercentiles(image, 2.0, 98.0);
    case Preset::None:
    default:
        return {};
    }
}
