#include "VTK/MPR/Controllers/MprController.h"

#include "VTK/MPR/State/MprScene.h"
#include "VTK/MPR/Sync/MprSynchronizationService.h"

#include <QPointF>
#include <algorithm>
#include <vtkImageData.h>

MprController::MprController(MprScene& scene)
    : m_scene(scene)
{
}

void MprController::setImageData(vtkImageData* imageData)
{
    m_imageData = imageData;
}

void MprController::setSlice(MprSlicePlane plane, int sliceValue)
{
    auto cursor = m_scene.cursorPosition();

    if (m_imageData)
    {
        double continuousIndex[3];
        m_imageData->TransformPhysicalPointToContinuousIndex(
            cursor.x,
            cursor.y,
            cursor.z,
            continuousIndex);

        switch (plane)
        {
        case MprSlicePlane::Axial:
            continuousIndex[2] = sliceValue;
            break;
        case MprSlicePlane::Coronal:
            continuousIndex[1] = sliceValue;
            break;
        case MprSlicePlane::Sagittal:
            continuousIndex[0] = sliceValue;
            break;
        }

        double worldPoint[3];
        m_imageData->TransformContinuousIndexToPhysicalPoint(
            continuousIndex[0],
            continuousIndex[1],
            continuousIndex[2],
            worldPoint);
        cursor = {worldPoint[0], worldPoint[1], worldPoint[2]};
    }
    else
    {
        switch (plane)
        {
        case MprSlicePlane::Axial:
            cursor.z = static_cast<double>(sliceValue);
            break;
        case MprSlicePlane::Coronal:
            cursor.y = static_cast<double>(sliceValue);
            break;
        case MprSlicePlane::Sagittal:
            cursor.x = static_cast<double>(sliceValue);
            break;
        }
    }

    m_scene.setCursorPosition(cursor);
}

void MprController::incrementSlice(MprSlicePlane plane, int stepCount)
{
    if (!m_imageData || stepCount == 0)
    {
        return;
    }

    double continuousIndex[3];
    auto cursor = m_scene.cursorPosition();
    m_imageData->TransformPhysicalPointToContinuousIndex(
        cursor.x,
        cursor.y,
        cursor.z,
        continuousIndex);

    int extent[6];
    m_imageData->GetExtent(extent);

    switch (plane)
    {
    case MprSlicePlane::Axial:
        continuousIndex[2] = std::clamp(
            continuousIndex[2] + static_cast<double>(stepCount),
            static_cast<double>(extent[4]),
            static_cast<double>(extent[5]));
        break;
    case MprSlicePlane::Coronal:
        continuousIndex[1] = std::clamp(
            continuousIndex[1] + static_cast<double>(stepCount),
            static_cast<double>(extent[2]),
            static_cast<double>(extent[3]));
        break;
    case MprSlicePlane::Sagittal:
        continuousIndex[0] = std::clamp(
            continuousIndex[0] + static_cast<double>(stepCount),
            static_cast<double>(extent[0]),
            static_cast<double>(extent[1]));
        break;
    }

    double worldPoint[3];
    m_imageData->TransformContinuousIndexToPhysicalPoint(
        continuousIndex[0],
        continuousIndex[1],
        continuousIndex[2],
        worldPoint);
    m_scene.setCursorPosition({worldPoint[0], worldPoint[1], worldPoint[2]});
}

void MprController::setCursorFromNormalizedPosition(MprSlicePlane plane, const QPointF& normalizedPosition)
{
    if (!m_imageData)
    {
        return;
    }

    const auto cursor = MprSynchronizationService::cursorPositionFromNormalizedPosition(
        *m_imageData,
        m_scene.cursorPosition(),
        plane,
        normalizedPosition);
    m_scene.setCursorPosition(cursor);
}

void MprController::adjustWindowLevelWidth(const QPointF& normalizedDelta)
{
    const int newLevel = m_scene.windowLevel() - static_cast<int>(normalizedDelta.y() * 512.0);
    const int newWidth = std::max(1, m_scene.windowWidth() + static_cast<int>(normalizedDelta.x() * 512.0));
    m_scene.setWindowLevelWidth(newLevel, newWidth);
}

void MprController::setWindowLevelWidth(int level, int width)
{
    m_scene.setWindowLevelWidth(level, width);
}

void MprController::setActiveTool(MprToolType toolType)
{
    m_scene.setActiveTool(toolType);
}
