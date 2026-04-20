#include "VTK/Camera/VtkVolumeCameraController.h"

#include <algorithm>
#include <vtkCamera.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkVolume.h>

void VtkVolumeCameraController::captureInitialState(vtkRenderer& renderer)
{
    if (vtkCamera* camera = renderer.GetActiveCamera())
    {
        camera->GetPosition(m_initialCameraPosition);
        camera->GetFocalPoint(m_initialCameraFocalPoint);
        camera->GetViewUp(m_initialCameraViewUp);
        m_hasInitialState = true;
    }
}

void VtkVolumeCameraController::applyView(
    vtkRenderer& renderer,
    vtkRenderWindow& renderWindow,
    vtkVolume& volume,
    int rotateXDegrees,
    int rotateYDegrees,
    int rotateZDegrees,
    int zoomPercent) const
{
    volume.SetOrientation(
        static_cast<double>(rotateXDegrees),
        static_cast<double>(rotateYDegrees),
        static_cast<double>(rotateZDegrees));

    if (m_hasInitialState)
    {
        if (vtkCamera* camera = renderer.GetActiveCamera())
        {
            const double zoomFactor = static_cast<double>(zoomPercent) / 100.0;
            const double inverseZoom = 1.0 / std::max(zoomFactor, 0.01);
            const double direction[3] = {
                m_initialCameraPosition[0] - m_initialCameraFocalPoint[0],
                m_initialCameraPosition[1] - m_initialCameraFocalPoint[1],
                m_initialCameraPosition[2] - m_initialCameraFocalPoint[2]
            };

            camera->SetFocalPoint(m_initialCameraFocalPoint);
            camera->SetViewUp(m_initialCameraViewUp);
            camera->SetPosition(
                m_initialCameraFocalPoint[0] + direction[0] * inverseZoom,
                m_initialCameraFocalPoint[1] + direction[1] * inverseZoom,
                m_initialCameraFocalPoint[2] + direction[2] * inverseZoom);
        }
    }

    renderer.ResetCameraClippingRange();
    renderWindow.Render();
}
