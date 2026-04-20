#pragma once

#include "Services/ThreeDProfiles/ThreeDProfileSelection.h"
#include "VTK/Camera/VtkVolumeCameraController.h"
#include "VTK/Presets/VtkVolumePresetLibrary.h"

#include <QMainWindow>
#include <vtkSmartPointer.h>
#include <memory>

class IVolumeData;
class QComboBox;
class QSlider;
class QVTKOpenGLNativeWidget;
class vtkCamera;
class vtkColorTransferFunction;
class vtkGenericOpenGLRenderWindow;
class vtkImageData;
class vtkPiecewiseFunction;
class vtkRenderer;
class vtkSmartVolumeMapper;
class vtkVolume;
class vtkVolumeProperty;

class VtkVolumeViewerWindow : public QMainWindow
{
public:
    explicit VtkVolumeViewerWindow(
        std::shared_ptr<IVolumeData> diagnosticVolume,
        ThreeDProfileSelection profileSelection,
        QWidget* parent = nullptr);
    ~VtkVolumeViewerWindow() override;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void setupUi();
    void setupRenderer();
    void applyRenderPreset(VtkVolumePresetLibrary::Mode mode);
    void assignVolumeInput(VtkVolumeInputKind inputKind);
    void applyViewControls();

private:
    std::shared_ptr<IVolumeData> m_diagnosticVolume;
    ThreeDProfileSelection m_autoProfileSelection;
    QComboBox* m_profileComboBox{nullptr};
    QSlider* m_zoomSlider{nullptr};
    QSlider* m_rotateXSlider{nullptr};
    QSlider* m_rotateYSlider{nullptr};
    QSlider* m_rotateZSlider{nullptr};
    QVTKOpenGLNativeWidget* m_vtkWidget{nullptr};
    vtkSmartPointer<vtkImageData> m_baseVolumeData;
    vtkSmartPointer<vtkImageData> m_boneFocusedVolumeData;
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
    vtkSmartPointer<vtkRenderer> m_renderer;
    vtkSmartPointer<vtkSmartVolumeMapper> m_volumeMapper;
    vtkSmartPointer<vtkVolume> m_volumeActor;
    vtkSmartPointer<vtkColorTransferFunction> m_colorTransferFunction;
    vtkSmartPointer<vtkPiecewiseFunction> m_opacityTransferFunction;
    vtkSmartPointer<vtkPiecewiseFunction> m_gradientOpacityTransferFunction;
    vtkSmartPointer<vtkVolumeProperty> m_volumeProperty;
    VtkVolumeCameraController m_cameraController;
};
