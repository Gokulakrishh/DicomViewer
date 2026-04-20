#pragma once

class vtkCamera;
class vtkRenderer;
class vtkRenderWindow;
class vtkVolume;

class VtkVolumeCameraController
{
public:
    void captureInitialState(vtkRenderer& renderer);
    void applyView(
        vtkRenderer& renderer,
        vtkRenderWindow& renderWindow,
        vtkVolume& volume,
        int rotateXDegrees,
        int rotateYDegrees,
        int rotateZDegrees,
        int zoomPercent) const;

private:
    double m_initialCameraPosition[3]{0.0, 0.0, 1.0};
    double m_initialCameraFocalPoint[3]{0.0, 0.0, 0.0};
    double m_initialCameraViewUp[3]{0.0, 1.0, 0.0};
    bool m_hasInitialState{false};
};
