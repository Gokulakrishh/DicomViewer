#include "VTK/MPR/MprMeasurementScalarSource.h"

#include "Model/IVolumeData.h"
#include "VTK/MPR/Adapters/VtkMprSceneAdapter.h"

#include <algorithm>
#include <cmath>

MprMeasurementScalarSource::MprMeasurementScalarSource(
    const IVolumeData* volume,
    const VtkMprSceneAdapter& sceneAdapter,
    MprSlicePlane plane)
    : m_volume(volume),
      m_sceneAdapter(sceneAdapter),
      m_plane(plane)
{
}

std::optional<RectangleRoiSamplingBounds> MprMeasurementScalarSource::rectangleRoiSamplingBounds(
    const MeasurementAnnotation& measurement) const
{
    if (!m_volume || measurement.points.size() < 2)
    {
        return std::nullopt;
    }

    const auto first = m_sceneAdapter.continuousIndexFromWorldPosition(
        {measurement.points[0].x, measurement.points[0].y, measurement.points[0].z});
    const auto second = m_sceneAdapter.continuousIndexFromWorldPosition(
        {measurement.points[1].x, measurement.points[1].y, measurement.points[1].z});
    const auto& spacing = m_volume->geometry().spacing;

    RectangleRoiSamplingBounds bounds;
    switch (m_plane)
    {
    case MprSlicePlane::Axial:
        bounds.minColumn = static_cast<int>(std::floor(std::min(first[0], second[0])));
        bounds.maxColumn = static_cast<int>(std::ceil(std::max(first[0], second[0])));
        bounds.minRow = static_cast<int>(std::floor(std::min(first[1], second[1])));
        bounds.maxRow = static_cast<int>(std::ceil(std::max(first[1], second[1])));
        bounds.fixedSliceIndex = static_cast<int>(std::lround(first[2]));
        bounds.sampleAreaMm2 = spacing.x * spacing.y;
        break;
    case MprSlicePlane::Coronal:
        bounds.minColumn = static_cast<int>(std::floor(std::min(first[0], second[0])));
        bounds.maxColumn = static_cast<int>(std::ceil(std::max(first[0], second[0])));
        bounds.minRow = static_cast<int>(std::floor(std::min(first[2], second[2])));
        bounds.maxRow = static_cast<int>(std::ceil(std::max(first[2], second[2])));
        bounds.fixedSliceIndex = static_cast<int>(std::lround(first[1]));
        bounds.sampleAreaMm2 = spacing.x * spacing.z;
        break;
    case MprSlicePlane::Sagittal:
        bounds.minColumn = static_cast<int>(std::floor(std::min(first[1], second[1])));
        bounds.maxColumn = static_cast<int>(std::ceil(std::max(first[1], second[1])));
        bounds.minRow = static_cast<int>(std::floor(std::min(first[2], second[2])));
        bounds.maxRow = static_cast<int>(std::ceil(std::max(first[2], second[2])));
        bounds.fixedSliceIndex = static_cast<int>(std::lround(first[0]));
        bounds.sampleAreaMm2 = spacing.y * spacing.z;
        break;
    }

    if (bounds.maxColumn < bounds.minColumn || bounds.maxRow < bounds.minRow)
    {
        return std::nullopt;
    }

    return bounds;
}

bool MprMeasurementScalarSource::sampleRectangleRoiValue(
    const RectangleRoiSamplingBounds& bounds,
    int column,
    int row,
    double& value) const
{
    if (!m_volume ||
        column < bounds.minColumn ||
        column > bounds.maxColumn ||
        row < bounds.minRow ||
        row > bounds.maxRow)
    {
        return false;
    }

    int x = 0;
    int y = 0;
    int z = 0;
    switch (m_plane)
    {
    case MprSlicePlane::Axial:
        x = column;
        y = row;
        z = bounds.fixedSliceIndex;
        break;
    case MprSlicePlane::Coronal:
        x = column;
        y = bounds.fixedSliceIndex;
        z = row;
        break;
    case MprSlicePlane::Sagittal:
        x = bounds.fixedSliceIndex;
        y = column;
        z = row;
        break;
    }

    if (!m_volume->isValidIndex(x, y, z))
    {
        return false;
    }

    value = m_volume->scalarAt(x, y, z);
    return true;
}
