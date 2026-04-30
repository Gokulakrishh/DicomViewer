#include "VTK/MainView/DicomMeasurementScalarSource.h"

#include "Model/DicomImage.h"
#include "VTK/MainView/VtkSliceSceneAdapter.h"

#include <algorithm>
#include <cmath>

DicomMeasurementScalarSource::DicomMeasurementScalarSource(
    const DicomImage* image,
    const VtkSliceSceneAdapter& sceneAdapter)
    : m_image(image),
      m_sceneAdapter(sceneAdapter)
{
}

std::optional<RectangleRoiSamplingBounds> DicomMeasurementScalarSource::rectangleRoiSamplingBounds(
    const MeasurementAnnotation& measurement) const
{
    if (!m_image || !m_image->hasRawPixels() || measurement.points.size() < 2)
    {
        return std::nullopt;
    }

    const QPointF first = m_sceneAdapter.imageIndexForMeasurementPoint(measurement.points[0]);
    const QPointF second = m_sceneAdapter.imageIndexForMeasurementPoint(measurement.points[1]);

    RectangleRoiSamplingBounds bounds;
    bounds.minColumn = std::max(0, static_cast<int>(std::floor(std::min(first.x(), second.x()))));
    bounds.maxColumn = std::min(m_image->width() - 1, static_cast<int>(std::ceil(std::max(first.x(), second.x()))));
    bounds.minRow = std::max(0, static_cast<int>(std::floor(std::min(first.y(), second.y()))));
    bounds.maxRow = std::min(m_image->height() - 1, static_cast<int>(std::ceil(std::max(first.y(), second.y()))));
    if (bounds.maxColumn < bounds.minColumn || bounds.maxRow < bounds.minRow)
    {
        return std::nullopt;
    }

    const double pixelSpacingX = m_image->hasPixelSpacing() ? m_image->pixelSpacingX() : 1.0;
    const double pixelSpacingY = m_image->hasPixelSpacing() ? m_image->pixelSpacingY() : 1.0;
    bounds.sampleAreaMm2 = pixelSpacingX * pixelSpacingY;
    return bounds;
}

bool DicomMeasurementScalarSource::sampleRectangleRoiValue(
    const RectangleRoiSamplingBounds& bounds,
    int column,
    int row,
    double& value) const
{
    if (!m_image ||
        column < bounds.minColumn ||
        column > bounds.maxColumn ||
        row < bounds.minRow ||
        row > bounds.maxRow)
    {
        return false;
    }

    value = static_cast<double>(m_image->rawPixelValueAt(column, row));
    return true;
}
