#pragma once

#include "VTK/MPR/MprTypes.h"

class QPointF;
class MprScene;
class vtkImageData;

/**
 * @brief Controller for MPR scene state changes.
 *
 * Responsibilities:
 * - Update crosshair, slice, WL/WW, and active tool state.
 * - Keep UI event handling separate from MprScene mutation logic.
 */
class MprController
{
public:
    /** @brief Creates the controller bound to an MPR scene. */
    explicit MprController(MprScene& scene);

    /** @brief Sets the VTK image data used for bounds/synchronization. */
    void setImageData(vtkImageData* imageData);
    /** @brief Sets a slice index for one plane. */
    void setSlice(MprSlicePlane plane, int sliceValue);
    /** @brief Increments a plane slice index. */
    void incrementSlice(MprSlicePlane plane, int stepCount);
    /** @brief Updates cursor from normalized pane coordinates. */
    void setCursorFromNormalizedPosition(MprSlicePlane plane, const QPointF& normalizedPosition);
    /** @brief Adjusts WL/WW from normalized drag delta. */
    void adjustWindowLevelWidth(const QPointF& normalizedDelta);
    /** @brief Sets absolute WL/WW values. */
    void setWindowLevelWidth(int level, int width);
    /** @brief Sets active MPR tool. */
    void setActiveTool(MprToolType toolType);

private:
    MprScene& m_scene;
    vtkImageData* m_imageData{nullptr};
};
