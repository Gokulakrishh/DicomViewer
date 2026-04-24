#include "VTK/MPR/Sync/MprSynchronizationService.h"

#include <vtkImageData.h>

#include <algorithm>
#include <cmath>

namespace
{
double normalizedCoordinate(double value, int minExtent, int maxExtent)
{
    if (maxExtent <= minExtent)
    {
        return 0.5;
    }

    return std::clamp(
        (value - static_cast<double>(minExtent)) /
            static_cast<double>(maxExtent - minExtent),
        0.0,
        1.0);
}

double extentCoordinate(double normalizedValue, int minExtent, int maxExtent)
{
    if (maxExtent <= minExtent)
    {
        return static_cast<double>(minExtent);
    }

    const double clamped = std::clamp(normalizedValue, 0.0, 1.0);
    return static_cast<double>(minExtent) +
        clamped * static_cast<double>(maxExtent - minExtent);
}
}

int MprSynchronizationService::sliceIndexForPlane(
    vtkImageData& imageData,
    const MprCursorPositionWorld& cursorPosition,
    MprSlicePlane plane)
{
    double continuousIndex[3];
    imageData.TransformPhysicalPointToContinuousIndex(
        cursorPosition.x,
        cursorPosition.y,
        cursorPosition.z,
        continuousIndex);

    switch (plane)
    {
    case MprSlicePlane::Sagittal:
        return static_cast<int>(std::lround(continuousIndex[0]));
    case MprSlicePlane::Coronal:
        return static_cast<int>(std::lround(continuousIndex[1]));
    case MprSlicePlane::Axial:
        return static_cast<int>(std::lround(continuousIndex[2]));
    }

    return 0;
}

QPointF MprSynchronizationService::normalizedPositionForPlane(
    vtkImageData& imageData,
    const MprCursorPositionWorld& cursorPosition,
    MprSlicePlane plane)
{
    double continuousIndex[3];
    imageData.TransformPhysicalPointToContinuousIndex(
        cursorPosition.x,
        cursorPosition.y,
        cursorPosition.z,
        continuousIndex);

    int extent[6];
    imageData.GetExtent(extent);

    switch (plane)
    {
    case MprSlicePlane::Axial:
        return {
            normalizedCoordinate(continuousIndex[0], extent[0], extent[1]),
            1.0 - normalizedCoordinate(continuousIndex[1], extent[2], extent[3])};
    case MprSlicePlane::Coronal:
        return {
            normalizedCoordinate(continuousIndex[0], extent[0], extent[1]),
            1.0 - normalizedCoordinate(continuousIndex[2], extent[4], extent[5])};
    case MprSlicePlane::Sagittal:
        return {
            normalizedCoordinate(continuousIndex[1], extent[2], extent[3]),
            1.0 - normalizedCoordinate(continuousIndex[2], extent[4], extent[5])};
    }

    return {0.5, 0.5};
}

MprCursorPositionWorld MprSynchronizationService::cursorPositionFromNormalizedPosition(
    vtkImageData& imageData,
    const MprCursorPositionWorld& currentCursorPosition,
    MprSlicePlane plane,
    const QPointF& normalizedPosition)
{
    double continuousIndex[3];
    imageData.TransformPhysicalPointToContinuousIndex(
        currentCursorPosition.x,
        currentCursorPosition.y,
        currentCursorPosition.z,
        continuousIndex);

    int extent[6];
    imageData.GetExtent(extent);

    switch (plane)
    {
    case MprSlicePlane::Axial:
        continuousIndex[0] = extentCoordinate(normalizedPosition.x(), extent[0], extent[1]);
        continuousIndex[1] = extentCoordinate(1.0 - normalizedPosition.y(), extent[2], extent[3]);
        break;
    case MprSlicePlane::Coronal:
        continuousIndex[0] = extentCoordinate(normalizedPosition.x(), extent[0], extent[1]);
        continuousIndex[2] = extentCoordinate(1.0 - normalizedPosition.y(), extent[4], extent[5]);
        break;
    case MprSlicePlane::Sagittal:
        continuousIndex[1] = extentCoordinate(normalizedPosition.x(), extent[2], extent[3]);
        continuousIndex[2] = extentCoordinate(1.0 - normalizedPosition.y(), extent[4], extent[5]);
        break;
    }

    double worldPoint[3];
    imageData.TransformContinuousIndexToPhysicalPoint(
        continuousIndex[0],
        continuousIndex[1],
        continuousIndex[2],
        worldPoint);
    return {worldPoint[0], worldPoint[1], worldPoint[2]};
}
