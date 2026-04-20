#include "VTK/Viewer/VtkVolumeViewerWindow.h"

#include "VTK/Adapters/VtkVolumeAdapter.h"
#include "Model/IVolumeData.h"
#include "Services/VolumeRenderPreprocessingService.h"

#include <algorithm>
#include <QComboBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QSurfaceFormat>
#include <QVBoxLayout>
#include <QWidget>
#include <QVTKOpenGLNativeWidget.h>
#include <vtkColorTransferFunction.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkPiecewiseFunction.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartPointer.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>

namespace
{
QSlider* createViewSlider(int minimum, int maximum, int value, QWidget* parent)
{
    auto* slider = new QSlider(Qt::Horizontal, parent);
    slider->setRange(minimum, maximum);
    slider->setValue(value);
    slider->setTracking(false);
    slider->setFixedWidth(140);
    return slider;
}
}

VtkVolumeViewerWindow::VtkVolumeViewerWindow(
    std::shared_ptr<IVolumeData> diagnosticVolume,
    ThreeDProfileSelection profileSelection,
    QWidget* parent)
    : QMainWindow(parent),
      m_diagnosticVolume(std::move(diagnosticVolume)),
      m_autoProfileSelection(std::move(profileSelection))
{
    setupUi();
    setupRenderer();
}

VtkVolumeViewerWindow::~VtkVolumeViewerWindow() = default;

bool VtkVolumeViewerWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_vtkWidget)
    {
        switch (event->type())
        {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseMove:
        case QEvent::Wheel:
        case QEvent::ContextMenu:
            return true;
        default:
            break;
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void VtkVolumeViewerWindow::setupUi()
{
    auto* centralWidget = new QWidget(this);
    auto* rootLayout = new QVBoxLayout(centralWidget);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto* topBar = new QWidget(centralWidget);
    auto* topBarLayout = new QHBoxLayout(topBar);
    topBarLayout->setContentsMargins(12, 8, 12, 8);
    topBarLayout->setSpacing(10);
    topBarLayout->addWidget(new QLabel("Volume Preset", topBar));

    m_profileComboBox = new QComboBox(topBar);
    m_profileComboBox->addItem("Auto", static_cast<int>(VtkVolumePresetLibrary::Mode::Auto));
    m_profileComboBox->addItem("Bone", static_cast<int>(VtkVolumePresetLibrary::Mode::Bone));
    m_profileComboBox->addItem("Lung", static_cast<int>(VtkVolumePresetLibrary::Mode::Lung));
    topBarLayout->addWidget(m_profileComboBox);

    topBarLayout->addSpacing(12);
    topBarLayout->addWidget(new QLabel("Zoom", topBar));
    m_zoomSlider = createViewSlider(25, 250, 100, topBar);
    topBarLayout->addWidget(m_zoomSlider);

    topBarLayout->addWidget(new QLabel("X", topBar));
    m_rotateXSlider = createViewSlider(-180, 180, 0, topBar);
    topBarLayout->addWidget(m_rotateXSlider);

    topBarLayout->addWidget(new QLabel("Y", topBar));
    m_rotateYSlider = createViewSlider(-180, 180, 0, topBar);
    topBarLayout->addWidget(m_rotateYSlider);

    topBarLayout->addWidget(new QLabel("Z", topBar));
    m_rotateZSlider = createViewSlider(-180, 180, 0, topBar);
    topBarLayout->addWidget(m_rotateZSlider);

    topBarLayout->addStretch();
    rootLayout->addWidget(topBar);

    QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());
    m_vtkWidget = new QVTKOpenGLNativeWidget(centralWidget);
    m_vtkWidget->installEventFilter(this);
    rootLayout->addWidget(m_vtkWidget, 1);
    setCentralWidget(centralWidget);
    resize(1200, 820);

    connect(m_profileComboBox, &QComboBox::currentIndexChanged, this, [this](int index) {
        applyRenderPreset(static_cast<VtkVolumePresetLibrary::Mode>(m_profileComboBox->itemData(index).toInt()));
        if (m_renderWindow)
        {
            m_renderWindow->Render();
        }
    });

    const auto connectSlider = [this](QSlider* slider) {
        connect(slider, &QSlider::valueChanged, this, [this]() { applyViewControls(); });
    };
    connectSlider(m_zoomSlider);
    connectSlider(m_rotateXSlider);
    connectSlider(m_rotateYSlider);
    connectSlider(m_rotateZSlider);
}

void VtkVolumeViewerWindow::setupRenderer()
{
    m_renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    m_renderer = vtkSmartPointer<vtkRenderer>::New();
    m_renderer->SetBackground(0.06, 0.08, 0.11);
    m_renderWindow->AddRenderer(m_renderer);
    m_vtkWidget->setRenderWindow(m_renderWindow);

    const VolumeRenderPreprocessingService preprocessingService;
    const PreparedVolumeRenderInputs preparedInputs = preprocessingService.prepare(m_diagnosticVolume);
    m_baseVolumeData = VtkVolumeAdapter::createImageData(*preparedInputs.baseVolume);
    m_boneFocusedVolumeData = VtkVolumeAdapter::createImageData(*preparedInputs.boneFocusedVolume);

    m_volumeMapper = vtkSmartPointer<vtkSmartVolumeMapper>::New();
    m_volumeMapper->SetRequestedRenderModeToGPU();
    m_volumeMapper->SetBlendModeToComposite();
    m_volumeMapper->SetInteractiveUpdateRate(1.0e10);
    m_volumeMapper->InteractiveAdjustSampleDistancesOff();
    m_volumeMapper->AutoAdjustSampleDistancesOff();

    const VolumeGeometry& geometry = m_diagnosticVolume->geometry();
    const float minSpacing = static_cast<float>(
        std::min({geometry.spacing.x, geometry.spacing.y, geometry.spacing.z}));
    m_volumeMapper->SetSampleDistance(std::max(0.2f, minSpacing * 0.5f));

    m_volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
    m_volumeProperty->ShadeOn();
    m_volumeProperty->SetInterpolationTypeToLinear();
    m_volumeProperty->SetAmbient(0.22);
    m_volumeProperty->SetDiffuse(0.82);
    m_volumeProperty->SetSpecular(0.18);
    m_volumeProperty->SetSpecularPower(18.0);

    m_colorTransferFunction = vtkSmartPointer<vtkColorTransferFunction>::New();
    m_opacityTransferFunction = vtkSmartPointer<vtkPiecewiseFunction>::New();
    m_gradientOpacityTransferFunction = vtkSmartPointer<vtkPiecewiseFunction>::New();
    m_volumeProperty->SetColor(m_colorTransferFunction);
    m_volumeProperty->SetScalarOpacity(m_opacityTransferFunction);
    m_volumeProperty->SetGradientOpacity(m_gradientOpacityTransferFunction);

    m_volumeActor = vtkSmartPointer<vtkVolume>::New();
    m_volumeActor->SetMapper(m_volumeMapper);
    m_volumeActor->SetProperty(m_volumeProperty);
    m_renderer->AddVolume(m_volumeActor);

    if (vtkRenderWindowInteractor* interactor = m_vtkWidget->interactor())
    {
        interactor->SetDesiredUpdateRate(0.05);
        interactor->SetStillUpdateRate(0.05);
    }

    m_profileComboBox->setCurrentIndex(
        m_autoProfileSelection.visualStyle.anatomyKind == ThreeDAnatomyKind::Lung ? 2 : 0);

    applyRenderPreset(static_cast<VtkVolumePresetLibrary::Mode>(m_profileComboBox->currentData().toInt()));
    m_renderer->ResetCamera();
    m_cameraController.captureInitialState(*m_renderer);
    applyViewControls();
    m_renderWindow->Render();
}

void VtkVolumeViewerWindow::applyRenderPreset(VtkVolumePresetLibrary::Mode mode)
{
    const VtkVolumeRenderPreset preset = VtkVolumePresetLibrary::createPreset(mode, m_autoProfileSelection);
    assignVolumeInput(preset.inputKind);
    applyColorPoints(*m_colorTransferFunction, preset.colorPoints);
    applyScalarPoints(*m_opacityTransferFunction, preset.scalarOpacityPoints);
    applyScalarPoints(*m_gradientOpacityTransferFunction, preset.gradientOpacityPoints);
    m_renderer->ResetCameraClippingRange();
}

void VtkVolumeViewerWindow::assignVolumeInput(VtkVolumeInputKind inputKind)
{
    m_volumeMapper->SetInputData(
        inputKind == VtkVolumeInputKind::BoneFocusedVolume ? m_boneFocusedVolumeData : m_baseVolumeData);
}

void VtkVolumeViewerWindow::applyViewControls()
{
    if (!m_volumeActor || !m_renderer || !m_renderWindow)
    {
        return;
    }

    m_cameraController.applyView(
        *m_renderer,
        *m_renderWindow,
        *m_volumeActor,
        m_rotateXSlider ? m_rotateXSlider->value() : 0,
        m_rotateYSlider ? m_rotateYSlider->value() : 0,
        m_rotateZSlider ? m_rotateZSlider->value() : 0,
        m_zoomSlider ? m_zoomSlider->value() : 100);
}
