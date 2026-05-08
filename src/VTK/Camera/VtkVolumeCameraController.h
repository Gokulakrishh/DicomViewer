#pragma once

class vtkCamera;
class vtkRenderer;
class vtkRenderWindow;
class vtkVolume;

/**
 * @brief Maintains and applies volume viewer camera state.
 */
class VtkVolumeCameraController
{
public:
    /** @brief Captures the renderer's initial camera state. */
    void captureInitialState(vtkRenderer& renderer);
    /** @brief Applies rotation and zoom controls relative to the captured state. */
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
