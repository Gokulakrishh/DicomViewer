#pragma once

#include "VTK/MPR/MprTypes.h"

class QPointF;
class MprScene;
class vtkImageData;

class MprController
{
public:
    explicit MprController(MprScene& scene);

    void setImageData(vtkImageData* imageData);
    void setSlice(MprSlicePlane plane, int sliceValue);
    void incrementSlice(MprSlicePlane plane, int stepCount);
    void setCursorFromNormalizedPosition(MprSlicePlane plane, const QPointF& normalizedPosition);
    void adjustWindowLevelWidth(const QPointF& normalizedDelta);
    void setWindowLevelWidth(int level, int width);
    void setActiveTool(MprToolType toolType);

private:
    MprScene& m_scene;
    vtkImageData* m_imageData{nullptr};
};
