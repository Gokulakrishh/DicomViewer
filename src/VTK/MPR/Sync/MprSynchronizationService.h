#pragma once

#include "VTK/MPR/MprTypes.h"

#include <QPointF>

class vtkImageData;

class MprSynchronizationService
{
public:
    [[nodiscard]] static int sliceIndexForPlane(
        vtkImageData& imageData,
        const MprCursorPositionWorld& cursorPosition,
        MprSlicePlane plane);
    [[nodiscard]] static QPointF normalizedPositionForPlane(
        vtkImageData& imageData,
        const MprCursorPositionWorld& cursorPosition,
        MprSlicePlane plane);
    [[nodiscard]] static MprCursorPositionWorld cursorPositionFromNormalizedPosition(
        vtkImageData& imageData,
        const MprCursorPositionWorld& currentCursorPosition,
        MprSlicePlane plane,
        const QPointF& normalizedPosition);
};
