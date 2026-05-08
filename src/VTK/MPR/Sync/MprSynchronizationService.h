#pragma once

#include "VTK/MPR/MprTypes.h"

#include <QPointF>

class vtkImageData;

/**
 * @brief Stateless coordinate conversion helpers for synchronized MPR panes.
 */
class MprSynchronizationService
{
public:
    /** @brief Computes slice index for a plane from world cursor position. */
    [[nodiscard]] static int sliceIndexForPlane(
        vtkImageData& imageData,
        const MprCursorPositionWorld& cursorPosition,
        MprSlicePlane plane);
    /** @brief Computes normalized pane position for a world cursor position. */
    [[nodiscard]] static QPointF normalizedPositionForPlane(
        vtkImageData& imageData,
        const MprCursorPositionWorld& cursorPosition,
        MprSlicePlane plane);
    /** @brief Computes world cursor position from normalized pane coordinates. */
    [[nodiscard]] static MprCursorPositionWorld cursorPositionFromNormalizedPosition(
        vtkImageData& imageData,
        const MprCursorPositionWorld& currentCursorPosition,
        MprSlicePlane plane,
        const QPointF& normalizedPosition);
};
