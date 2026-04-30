#include "ViewerTools/Measurements/MeasurementAnalyticsService.h"

#include "ViewerTools/Measurements/MeasurementService.h"

#include <algorithm>
#include <cmath>
#include <limits>

std::optional<RoiStatistics> MeasurementAnalyticsService::rectangleRoiStatistics(
    const MeasurementAnnotation& measurement,
    const IMeasurementScalarSource& scalarSource)
{
    if (measurement.type != MeasurementType::RectangleRoi)
    {
        return std::nullopt;
    }

    const auto bounds = scalarSource.rectangleRoiSamplingBounds(measurement);
    if (!bounds)
    {
        return std::nullopt;
    }

    RoiStatistics stats;
    stats.minimum = std::numeric_limits<double>::max();
    stats.maximum = std::numeric_limits<double>::lowest();

    double sum = 0.0;
    double sumSquares = 0.0;
    for (int row = bounds->minRow; row <= bounds->maxRow; ++row)
    {
        for (int column = bounds->minColumn; column <= bounds->maxColumn; ++column)
        {
            double value = 0.0;
            if (!scalarSource.sampleRectangleRoiValue(*bounds, column, row, value))
            {
                continue;
            }

            sum += value;
            sumSquares += value * value;
            stats.minimum = std::min(stats.minimum, value);
            stats.maximum = std::max(stats.maximum, value);
            ++stats.sampleCount;
        }
    }

    if (stats.sampleCount <= 0)
    {
        return std::nullopt;
    }

    stats.valid = true;
    stats.mean = sum / static_cast<double>(stats.sampleCount);
    const double variance = std::max(
        0.0,
        (sumSquares / static_cast<double>(stats.sampleCount)) - (stats.mean * stats.mean));
    stats.standardDeviation = std::sqrt(variance);
    stats.areaMm2 = bounds->sampleAreaMm2 * static_cast<double>(stats.sampleCount);
    return stats;
}

QString MeasurementAnalyticsService::rectangleRoiLabel(
    const MeasurementAnnotation& measurement,
    const IMeasurementScalarSource& scalarSource)
{
    const auto stats = rectangleRoiStatistics(measurement, scalarSource);
    if (!stats || !stats->valid)
    {
        return {};
    }

    return QString("%1\nMean %2  SD %3\nMin %4  Max %5")
        .arg(MeasurementService::formattedArea(stats->areaMm2))
        .arg(stats->mean, 0, 'f', 1)
        .arg(stats->standardDeviation, 0, 'f', 1)
        .arg(stats->minimum, 0, 'f', 0)
        .arg(stats->maximum, 0, 'f', 0);
}
