#include "VTK/MPR/Adapters/VtkMprSceneAdapter.h"

#include <QVTKOpenGLNativeWidget.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <vtkActor.h>
#include <vtkAxesActor.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkCommand.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageData.h>
#include <vtkImageReslice.h>
#include <vtkImageSlabReslice.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkInteractorStyleUser.h>
#include <vtkLineSource.h>
#include <vtkObjectFactory.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkOutlineFilter.h>
#include <vtkPlane.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkResliceCursor.h>
#include <vtkResliceCursorActor.h>
#include <vtkResliceCursorLineRepresentation.h>
#include <vtkResliceCursorPolyDataAlgorithm.h>
#include <vtkResliceCursorRepresentation.h>
#include <vtkResliceCursorWidget.h>
#include <vtkResliceImageViewer.h>
#include <vtkSphereSource.h>

namespace
{
double clampedNormalizedCoordinate(double value, int minExtent, int maxExtent)
{
    if (maxExtent <= minExtent)
    {
        return 0.5;
    }

    return std::clamp(
        (value - static_cast<double>(minExtent)) /
            static_cast<double>(maxExtent - minExtent),
        0.0,
        1.0);
}

double clampedIndexCoordinate(double value, int minExtent, int maxExtent)
{
    return std::clamp(
        value,
        static_cast<double>(minExtent),
        static_cast<double>(maxExtent));
}

QSize rendererDisplaySize(vtkRenderer& renderer, const QSize& fallbackSize)
{
    const int* size = renderer.GetSize();
    if (!size || size[0] <= 0 || size[1] <= 0)
    {
        return fallbackSize;
    }

    return {size[0], size[1]};
}

class ReferenceNavigatorInteractorStyle final : public vtkInteractorStyleTrackballCamera
{
public:
    static ReferenceNavigatorInteractorStyle* New();
    vtkTypeMacro(ReferenceNavigatorInteractorStyle, vtkInteractorStyleTrackballCamera);

    void OnMiddleButtonDown() override {}
    void OnMiddleButtonUp() override {}
    void OnRightButtonDown() override {}
    void OnRightButtonUp() override {}
    void OnMouseWheelForward() override {}
    void OnMouseWheelBackward() override {}
};

vtkStandardNewMacro(ReferenceNavigatorInteractorStyle);

class SlicePaneInteractorStyle final : public vtkInteractorStyleUser
{
public:
    static SlicePaneInteractorStyle* New();
    vtkTypeMacro(SlicePaneInteractorStyle, vtkInteractorStyleUser);

    void OnLeftButtonDown() override {}
    void OnLeftButtonUp() override {}
    void OnMiddleButtonDown() override {}
    void OnMiddleButtonUp() override {}
    void OnRightButtonDown() override {}
    void OnRightButtonUp() override {}
    void OnMouseMove() override {}
    void OnMouseWheelForward() override {}
    void OnMouseWheelBackward() override {}
    void OnChar() override {}
    void OnKeyPress() override {}
    void OnKeyRelease() override {}
};

vtkStandardNewMacro(SlicePaneInteractorStyle);

void swallowVtkSliceInput(vtkObject*, unsigned long, void*, void*)
{
}

int vtkSlabModeForMprMode(MprSlabMode mode)
{
    switch (mode)
    {
    case MprSlabMode::MaximumIntensity:
        return VTK_IMAGE_SLAB_MAX;
    case MprSlabMode::MinimumIntensity:
        return VTK_IMAGE_SLAB_MIN;
    case MprSlabMode::Average:
    case MprSlabMode::Thin:
        return VTK_IMAGE_SLAB_MEAN;
    }

    return VTK_IMAGE_SLAB_MEAN;
}

constexpr double kPi = 3.14159265358979323846;

void resetSliceCameraToFit(vtkResliceImageViewer& viewer)
{
    auto* renderer = viewer.GetRenderer();
    if (!renderer)
    {
        return;
    }

    renderer->ResetCamera();
    renderer->ResetCameraClippingRange();
}

using Vector3 = std::array<double, 3>;

Vector3 rotatedAroundAxis(const Vector3& vector, const Vector3& axis, double angleRadians)
{
    const double cosAngle = std::cos(angleRadians);
    const double sinAngle = std::sin(angleRadians);
    const double dot =
        vector[0] * axis[0] +
        vector[1] * axis[1] +
        vector[2] * axis[2];

    const Vector3 cross = {
        axis[1] * vector[2] - axis[2] * vector[1],
        axis[2] * vector[0] - axis[0] * vector[2],
        axis[0] * vector[1] - axis[1] * vector[0]};

    return {
        vector[0] * cosAngle + cross[0] * sinAngle + axis[0] * dot * (1.0 - cosAngle),
        vector[1] * cosAngle + cross[1] * sinAngle + axis[1] * dot * (1.0 - cosAngle),
        vector[2] * cosAngle + cross[2] * sinAngle + axis[2] * dot * (1.0 - cosAngle)};
}

std::array<Vector3, 3> canonicalPlaneNormals()
{
    return {
        Vector3{1.0, 0.0, 0.0},
        Vector3{0.0, -1.0, 0.0},
        Vector3{0.0, 0.0, 1.0}};
}

Vector3 obliqueRotationAxisForPlane(MprSlicePlane plane)
{
    switch (plane)
    {
    case MprSlicePlane::Axial:
    case MprSlicePlane::Coronal:
        return {1.0, 0.0, 0.0};
    case MprSlicePlane::Sagittal:
        return {0.0, 1.0, 0.0};
    }

    return {1.0, 0.0, 0.0};
}

void setCursorPlaneNormals(vtkResliceCursor& cursor, const std::array<Vector3, 3>& normals)
{
    for (int index = 0; index < 3; ++index)
    {
        if (auto* plane = cursor.GetPlane(index))
        {
            plane->SetNormal(normals[index].data());
        }
    }
    cursor.Update();
}

void disableResliceWidgetInteraction(vtkResliceImageViewer& viewer)
{
    auto* widget = viewer.GetResliceCursorWidget();
    if (!widget)
    {
        return;
    }

    widget->ProcessEventsOff();
    widget->ManagesCursorOff();
}

void setViewerReslicePlaneNormal(vtkResliceImageViewer& viewer, int planeIndex)
{
    auto* widget = viewer.GetResliceCursorWidget();
    auto* representation = widget
        ? vtkResliceCursorLineRepresentation::SafeDownCast(widget->GetRepresentation())
        : nullptr;
    auto* algorithm = representation
        ? representation->GetResliceCursorActor()->GetCursorAlgorithm()
        : nullptr;
    if (algorithm)
    {
        algorithm->SetReslicePlaneNormal(planeIndex);
    }
}

void installSliceInputSwallower(
    vtkResliceImageViewer& viewer,
    vtkInteractorStyleUser& interactorStyle,
    vtkSmartPointer<vtkCallbackCommand>& callback)
{
    disableResliceWidgetInteraction(viewer);

    auto* renderWindow = viewer.GetRenderWindow();
    auto* interactor = renderWindow ? renderWindow->GetInteractor() : nullptr;
    if (!interactor)
    {
        return;
    }

    if (interactor->GetInteractorStyle() != &interactorStyle)
    {
        interactor->SetInteractorStyle(&interactorStyle);
    }

    if (callback)
    {
        return;
    }

    callback = vtkSmartPointer<vtkCallbackCommand>::New();
    callback->SetCallback(swallowVtkSliceInput);
    callback->AbortFlagOnExecuteOn();

    constexpr double priority = 1.0;
    interactor->AddObserver(vtkCommand::LeftButtonPressEvent, callback, priority);
    interactor->AddObserver(vtkCommand::LeftButtonReleaseEvent, callback, priority);
    interactor->AddObserver(vtkCommand::MiddleButtonPressEvent, callback, priority);
    interactor->AddObserver(vtkCommand::MiddleButtonReleaseEvent, callback, priority);
    interactor->AddObserver(vtkCommand::RightButtonPressEvent, callback, priority);
    interactor->AddObserver(vtkCommand::RightButtonReleaseEvent, callback, priority);
    interactor->AddObserver(vtkCommand::MouseMoveEvent, callback, priority);
    interactor->AddObserver(vtkCommand::MouseWheelForwardEvent, callback, priority);
    interactor->AddObserver(vtkCommand::MouseWheelBackwardEvent, callback, priority);
}
}

VtkMprSceneAdapter::VtkMprSceneAdapter()
{
    for (int index = 0; index < 3; ++index)
    {
        m_sliceViewers[index] = vtkSmartPointer<vtkResliceImageViewer>::New();
        auto renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
        m_sliceViewers[index]->SetRenderWindow(renderWindow);
    }
}

VtkMprSceneAdapter::~VtkMprSceneAdapter() = default;

void VtkMprSceneAdapter::attachPane(MprSlicePlane plane, QVTKOpenGLNativeWidget& widget)
{
    const int index = planeIndex(plane);
    widget.setRenderWindow(m_sliceViewers[index]->GetRenderWindow());
    m_sliceViewers[index]->SetupInteractor(widget.renderWindow()->GetInteractor());
    m_neutralInteractorStyles[index] = vtkSmartPointer<SlicePaneInteractorStyle>::New();
    installSliceInputSwallower(
        *m_sliceViewers[index],
        *m_neutralInteractorStyles[index],
        m_sliceInputSwallowCallbacks[index]);
}

void VtkMprSceneAdapter::attachReferencePane(QVTKOpenGLNativeWidget& widget)
{
    m_referenceRenderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    m_referenceRenderer = vtkSmartPointer<vtkRenderer>::New();
    m_referenceRenderWindow->AddRenderer(m_referenceRenderer);
    widget.setRenderWindow(m_referenceRenderWindow);

    m_referenceInteractorStyle = vtkSmartPointer<ReferenceNavigatorInteractorStyle>::New();
    m_referenceInteractorStyle->SetMotionFactor(3.0);
    widget.renderWindow()->GetInteractor()->SetInteractorStyle(m_referenceInteractorStyle);
}

void VtkMprSceneAdapter::initialize(vtkImageData& imageData, int windowLevel, int windowWidth)
{
    m_imageData = &imageData;

    for (int index = 0; index < 3; ++index)
    {
        m_sliceViewers[index]->SetInputData(&imageData);
        m_sliceViewers[index]->SetSliceOrientation(index);
        setViewerReslicePlaneNormal(*m_sliceViewers[index], index);
        m_sliceViewers[index]->SetResliceModeToAxisAligned();
        m_sliceViewers[index]->SetColorLevel(windowLevel);
        m_sliceViewers[index]->SetColorWindow(windowWidth);
        m_sliceViewers[index]->SetSliceScrollOnMouseWheel(false);
        m_sliceViewers[index]->SetThickMode(0);
        if (m_neutralInteractorStyles[index])
        {
            installSliceInputSwallower(
                *m_sliceViewers[index],
                *m_neutralInteractorStyles[index],
                m_sliceInputSwallowCallbacks[index]);
        }
        m_sliceViewers[index]->GetRenderer()->ResetCamera();
        if (auto* camera = m_sliceViewers[index]->GetRenderer()->GetActiveCamera())
        {
            m_initialParallelScales[index] = std::max(camera->GetParallelScale(), 1e-6);
        }
    }

    applySlabSettings(m_slabSettings);
    initializeReferenceScene();
}

void VtkMprSceneAdapter::applyCursorPositionWorld(const MprCursorPositionWorld& cursorPosition)
{
    if (!m_imageData)
    {
        return;
    }

    double continuousIndex[3];
    m_imageData->TransformPhysicalPointToContinuousIndex(
        cursorPosition.x,
        cursorPosition.y,
        cursorPosition.z,
        continuousIndex);

    for (int index = 0; index < 3; ++index)
    {
        auto* viewer = m_sliceViewers[index].GetPointer();
        const int clampedSlice = std::clamp(
            static_cast<int>(std::lround(continuousIndex[index])),
            viewer->GetSliceMin(),
            viewer->GetSliceMax());
        viewer->SetSlice(clampedSlice);
        if (auto* cursor = viewer->GetResliceCursor())
        {
            cursor->SetCenter(cursorPosition.x, cursorPosition.y, cursorPosition.z);
        }
    }

    updateReferencePlanes(cursorPosition);
}

void VtkMprSceneAdapter::applyWindowLevelWidth(int level, int width)
{
    for (int index = 0; index < 3; ++index)
    {
        m_sliceViewers[index]->SetColorLevel(level);
        m_sliceViewers[index]->SetColorWindow(width);
    }
}

void VtkMprSceneAdapter::applySlabSettings(const MprSlabSettings& settings)
{
    m_slabSettings = settings;
    const bool thickMode = settings.mode != MprSlabMode::Thin;
    const double thicknessMm = std::max(1.0, settings.thicknessMm);
    const int slabMode = vtkSlabModeForMprMode(settings.mode);

    for (int index = 0; index < 3; ++index)
    {
        auto* viewer = m_sliceViewers[index].GetPointer();
        if (!viewer)
        {
            continue;
        }

        viewer->SetResliceMode(thickMode
            ? vtkResliceImageViewer::RESLICE_OBLIQUE
            : vtkResliceImageViewer::RESLICE_AXIS_ALIGNED);
        viewer->SetThickMode(thickMode ? 1 : 0);
        setViewerReslicePlaneNormal(*viewer, index);
        if (m_neutralInteractorStyles[index])
        {
            installSliceInputSwallower(
                *viewer,
                *m_neutralInteractorStyles[index],
                m_sliceInputSwallowCallbacks[index]);
        }
        else
        {
            disableResliceWidgetInteraction(*viewer);
        }
        if (auto* cursor = viewer->GetResliceCursor())
        {
            cursor->SetThickMode(thickMode ? 1 : 0);
            cursor->SetThickness(thicknessMm, thicknessMm, thicknessMm);
            cursor->SetCenter(
                m_referenceCursorPosition.x,
                m_referenceCursorPosition.y,
                m_referenceCursorPosition.z);
        }

        if (auto* widget = viewer->GetResliceCursorWidget())
        {
            if (auto* representation = widget->GetResliceCursorRepresentation())
            {
                if (auto* slabReslice = vtkImageSlabReslice::SafeDownCast(representation->GetReslice()))
                {
                    slabReslice->SetBlendMode(slabMode);
                    slabReslice->SetSlabThickness(thicknessMm);
                    slabReslice->SetSlabResolution(std::max(0.1, thicknessMm / 10.0));
                }
                else if (auto* imageReslice = vtkImageReslice::SafeDownCast(representation->GetReslice()))
                {
                    imageReslice->SetSlabMode(slabMode);
                    imageReslice->SetSlabNumberOfSlices(thickMode ? std::max(2, static_cast<int>(std::lround(thicknessMm))) : 1);
                }
            }
        }
        resetSliceCameraToFit(*viewer);
        viewer->Render();
        if (m_neutralInteractorStyles[index])
        {
            installSliceInputSwallower(
                *viewer,
                *m_neutralInteractorStyles[index],
                m_sliceInputSwallowCallbacks[index]);
        }
    }

    applyObliqueSettings(m_obliqueSettings);
}

void VtkMprSceneAdapter::applyObliqueSettings(const MprObliqueSettings& settings)
{
    m_obliqueSettings = settings;
    const int obliqueIndex = planeIndex(settings.basePlane);
    const double angleRadians = settings.angleDegrees * kPi / 180.0;
    const Vector3 rotationAxis = obliqueRotationAxisForPlane(settings.basePlane);

    for (int index = 0; index < 3; ++index)
    {
        auto* viewer = m_sliceViewers[index].GetPointer();
        if (!viewer)
        {
            continue;
        }

        const bool selectedObliquePane = settings.enabled && index == obliqueIndex;
        setViewerReslicePlaneNormal(*viewer, index);
        if (m_slabSettings.mode == MprSlabMode::Thin)
        {
            viewer->SetResliceMode(selectedObliquePane
                ? vtkResliceImageViewer::RESLICE_OBLIQUE
                : vtkResliceImageViewer::RESLICE_AXIS_ALIGNED);
        }

        if (auto* cursor = viewer->GetResliceCursor())
        {
            auto normals = canonicalPlaneNormals();
            if (selectedObliquePane)
            {
                normals[index] = rotatedAroundAxis(normals[index], rotationAxis, angleRadians);
            }
            setCursorPlaneNormals(*cursor, normals);
            cursor->SetCenter(
                m_referenceCursorPosition.x,
                m_referenceCursorPosition.y,
                m_referenceCursorPosition.z);
        }

        if (m_neutralInteractorStyles[index])
        {
            installSliceInputSwallower(
                *viewer,
                *m_neutralInteractorStyles[index],
                m_sliceInputSwallowCallbacks[index]);
        }
        else
        {
            disableResliceWidgetInteraction(*viewer);
        }

        resetSliceCameraToFit(*viewer);
        viewer->Render();
    }
}

void VtkMprSceneAdapter::zoom(MprSlicePlane plane, const QPointF& normalizedDelta)
{
    auto* viewer = m_sliceViewers[planeIndex(plane)].GetPointer();
    auto* renderer = viewer->GetRenderer();
    auto* camera = renderer->GetActiveCamera();
    if (!camera)
    {
        return;
    }

    const double factor = std::exp(-normalizedDelta.y() * 2.0);
    camera->Zoom(factor);
    viewer->Render();
}

void VtkMprSceneAdapter::pan(MprSlicePlane plane, const QPointF& displayDelta, const QSize& widgetSize)
{
    auto* viewer = m_sliceViewers[planeIndex(plane)].GetPointer();
    auto* renderer = viewer ? viewer->GetRenderer() : nullptr;
    auto* camera = renderer ? renderer->GetActiveCamera() : nullptr;
    if (!renderer || !camera || widgetSize.width() <= 0 || widgetSize.height() <= 0)
    {
        return;
    }

    const QSize displaySize = rendererDisplaySize(*renderer, widgetSize);

    renderer->SetWorldPoint(
        camera->GetFocalPoint()[0],
        camera->GetFocalPoint()[1],
        camera->GetFocalPoint()[2],
        1.0);
    renderer->WorldToDisplay();

    double focalDisplay[3];
    renderer->GetDisplayPoint(focalDisplay);

    const double dx = displayDelta.x() * static_cast<double>(displaySize.width()) /
        static_cast<double>(widgetSize.width());
    const double dy = displayDelta.y() * static_cast<double>(displaySize.height()) /
        static_cast<double>(widgetSize.height());

    renderer->SetDisplayPoint(focalDisplay[0], focalDisplay[1], focalDisplay[2]);
    renderer->DisplayToWorld();
    double beforeWorld[4];
    renderer->GetWorldPoint(beforeWorld);

    renderer->SetDisplayPoint(focalDisplay[0] - dx, focalDisplay[1] + dy, focalDisplay[2]);
    renderer->DisplayToWorld();
    double afterWorld[4];
    renderer->GetWorldPoint(afterWorld);

    if (std::abs(beforeWorld[3]) <= 1e-12 || std::abs(afterWorld[3]) <= 1e-12)
    {
        return;
    }

    const double worldDelta[3] = {
        afterWorld[0] / afterWorld[3] - beforeWorld[0] / beforeWorld[3],
        afterWorld[1] / afterWorld[3] - beforeWorld[1] / beforeWorld[3],
        afterWorld[2] / afterWorld[3] - beforeWorld[2] / beforeWorld[3]};

    const double* focalPoint = camera->GetFocalPoint();
    const double* position = camera->GetPosition();
    camera->SetFocalPoint(
        focalPoint[0] + worldDelta[0],
        focalPoint[1] + worldDelta[1],
        focalPoint[2] + worldDelta[2]);
    camera->SetPosition(
        position[0] + worldDelta[0],
        position[1] + worldDelta[1],
        position[2] + worldDelta[2]);
    renderer->ResetCameraClippingRange();
    viewer->Render();
}

QPointF VtkMprSceneAdapter::normalizedImagePositionFromDisplayPosition(
    MprSlicePlane plane,
    const QPointF& widgetPosition,
    const QSize& widgetSize,
    const MprCursorPositionWorld& currentCursorPosition) const
{
    if (!m_imageData)
    {
        return {0.5, 0.5};
    }

    double pickedIndex[3];
    const MprCursorPositionWorld pickedPosition = worldPositionFromDisplayPosition(
        plane,
        widgetPosition,
        widgetSize,
        currentCursorPosition);
    m_imageData->TransformPhysicalPointToContinuousIndex(
        pickedPosition.x,
        pickedPosition.y,
        pickedPosition.z,
        pickedIndex);

    double currentIndex[3];
    m_imageData->TransformPhysicalPointToContinuousIndex(
        currentCursorPosition.x,
        currentCursorPosition.y,
        currentCursorPosition.z,
        currentIndex);

    int extent[6];
    m_imageData->GetExtent(extent);

    switch (plane)
    {
    case MprSlicePlane::Axial:
        pickedIndex[2] = currentIndex[2];
        break;
    case MprSlicePlane::Coronal:
        pickedIndex[1] = currentIndex[1];
        break;
    case MprSlicePlane::Sagittal:
        pickedIndex[0] = currentIndex[0];
        break;
    }

    pickedIndex[0] = clampedIndexCoordinate(pickedIndex[0], extent[0], extent[1]);
    pickedIndex[1] = clampedIndexCoordinate(pickedIndex[1], extent[2], extent[3]);
    pickedIndex[2] = clampedIndexCoordinate(pickedIndex[2], extent[4], extent[5]);

    switch (plane)
    {
    case MprSlicePlane::Axial:
        return {
            clampedNormalizedCoordinate(pickedIndex[0], extent[0], extent[1]),
            1.0 - clampedNormalizedCoordinate(pickedIndex[1], extent[2], extent[3])};
    case MprSlicePlane::Coronal:
        return {
            clampedNormalizedCoordinate(pickedIndex[0], extent[0], extent[1]),
            1.0 - clampedNormalizedCoordinate(pickedIndex[2], extent[4], extent[5])};
    case MprSlicePlane::Sagittal:
        return {
            clampedNormalizedCoordinate(pickedIndex[1], extent[2], extent[3]),
            1.0 - clampedNormalizedCoordinate(pickedIndex[2], extent[4], extent[5])};
    }

    return {0.5, 0.5};
}

MprCursorPositionWorld VtkMprSceneAdapter::worldPositionFromDisplayPosition(
    MprSlicePlane plane,
    const QPointF& widgetPosition,
    const QSize& widgetSize,
    const MprCursorPositionWorld& currentCursorPosition) const
{
    if (!m_imageData || widgetSize.width() <= 0 || widgetSize.height() <= 0)
    {
        return currentCursorPosition;
    }

    auto* viewer = m_sliceViewers[planeIndex(plane)].GetPointer();
    auto* renderer = viewer ? viewer->GetRenderer() : nullptr;
    if (!renderer)
    {
        return currentCursorPosition;
    }
    const QSize displaySize = rendererDisplaySize(*renderer, widgetSize);

    renderer->SetWorldPoint(
        currentCursorPosition.x,
        currentCursorPosition.y,
        currentCursorPosition.z,
        1.0);
    renderer->WorldToDisplay();

    double cursorDisplay[3];
    renderer->GetDisplayPoint(cursorDisplay);

    renderer->SetDisplayPoint(
        widgetPosition.x() * static_cast<double>(displaySize.width()) /
            static_cast<double>(widgetSize.width()),
        static_cast<double>(displaySize.height()) -
            widgetPosition.y() * static_cast<double>(displaySize.height()) /
                static_cast<double>(widgetSize.height()),
        cursorDisplay[2]);
    renderer->DisplayToWorld();

    double worldPoint[4];
    renderer->GetWorldPoint(worldPoint);
    if (std::abs(worldPoint[3]) <= 1e-12)
    {
        return currentCursorPosition;
    }

    double pickedIndex[3];
    m_imageData->TransformPhysicalPointToContinuousIndex(
        worldPoint[0] / worldPoint[3],
        worldPoint[1] / worldPoint[3],
        worldPoint[2] / worldPoint[3],
        pickedIndex);

    double currentIndex[3];
    m_imageData->TransformPhysicalPointToContinuousIndex(
        currentCursorPosition.x,
        currentCursorPosition.y,
        currentCursorPosition.z,
        currentIndex);

    int extent[6];
    m_imageData->GetExtent(extent);

    switch (plane)
    {
    case MprSlicePlane::Axial:
        pickedIndex[2] = currentIndex[2];
        break;
    case MprSlicePlane::Coronal:
        pickedIndex[1] = currentIndex[1];
        break;
    case MprSlicePlane::Sagittal:
        pickedIndex[0] = currentIndex[0];
        break;
    }

    pickedIndex[0] = clampedIndexCoordinate(pickedIndex[0], extent[0], extent[1]);
    pickedIndex[1] = clampedIndexCoordinate(pickedIndex[1], extent[2], extent[3]);
    pickedIndex[2] = clampedIndexCoordinate(pickedIndex[2], extent[4], extent[5]);

    double physicalPoint[3];
    m_imageData->TransformContinuousIndexToPhysicalPoint(
        pickedIndex[0],
        pickedIndex[1],
        pickedIndex[2],
        physicalPoint);
    return {physicalPoint[0], physicalPoint[1], physicalPoint[2]};
}

QPointF VtkMprSceneAdapter::normalizedDisplayPositionForCursorWorld(
    MprSlicePlane plane,
    const MprCursorPositionWorld& cursorPosition,
    const QSize& widgetSize) const
{
    if (widgetSize.width() <= 0 || widgetSize.height() <= 0)
    {
        return {0.5, 0.5};
    }

    auto* viewer = m_sliceViewers[planeIndex(plane)].GetPointer();
    auto* renderer = viewer ? viewer->GetRenderer() : nullptr;
    if (!renderer)
    {
        return {0.5, 0.5};
    }
    const QSize displaySize = rendererDisplaySize(*renderer, widgetSize);

    renderer->SetWorldPoint(cursorPosition.x, cursorPosition.y, cursorPosition.z, 1.0);
    renderer->WorldToDisplay();

    double displayPoint[3];
    renderer->GetDisplayPoint(displayPoint);

    return {
        displayPoint[0] / static_cast<double>(displaySize.width()),
        (static_cast<double>(displaySize.height()) - displayPoint[1]) /
            static_cast<double>(displaySize.height())};
}

std::array<double, 3> VtkMprSceneAdapter::continuousIndexFromWorldPosition(
    const MprCursorPositionWorld& worldPosition) const
{
    std::array<double, 3> continuousIndex{0.0, 0.0, 0.0};
    if (!m_imageData)
    {
        return continuousIndex;
    }

    m_imageData->TransformPhysicalPointToContinuousIndex(
        worldPosition.x,
        worldPosition.y,
        worldPosition.z,
        continuousIndex.data());
    return continuousIndex;
}

MprCursorPositionWorld VtkMprSceneAdapter::centeredCursorPositionWorld() const
{
    if (!m_imageData)
    {
        return {};
    }

    int extent[6];
    m_imageData->GetExtent(extent);

    const double centerIndex[3] = {
        0.5 * static_cast<double>(extent[0] + extent[1]),
        0.5 * static_cast<double>(extent[2] + extent[3]),
        0.5 * static_cast<double>(extent[4] + extent[5])};

    double worldPoint[3];
    m_imageData->TransformContinuousIndexToPhysicalPoint(
        centerIndex[0],
        centerIndex[1],
        centerIndex[2],
        worldPoint);

    return {worldPoint[0], worldPoint[1], worldPoint[2]};
}

int VtkMprSceneAdapter::sliceMin(MprSlicePlane plane) const
{
    return m_sliceViewers[planeIndex(plane)]->GetSliceMin();
}

int VtkMprSceneAdapter::sliceMax(MprSlicePlane plane) const
{
    return m_sliceViewers[planeIndex(plane)]->GetSliceMax();
}

int VtkMprSceneAdapter::currentSlice(MprSlicePlane plane) const
{
    return m_sliceViewers[planeIndex(plane)]->GetSlice();
}

int VtkMprSceneAdapter::zoomPercent(MprSlicePlane plane) const
{
    auto* renderer = m_sliceViewers[planeIndex(plane)]->GetRenderer();
    auto* camera = renderer ? renderer->GetActiveCamera() : nullptr;
    if (!camera)
    {
        return 100;
    }

    const int index = planeIndex(plane);
    const double initialScale = std::max(m_initialParallelScales[index], 1e-6);
    const double currentScale = std::max(camera->GetParallelScale(), 1e-6);
    return std::max(1, static_cast<int>(std::lround((initialScale / currentScale) * 100.0)));
}

int VtkMprSceneAdapter::referenceZoomPercent() const
{
    if (!m_referenceRenderer)
    {
        return 100;
    }

    auto* camera = m_referenceRenderer->GetActiveCamera();
    if (!camera)
    {
        return 100;
    }

    const double initialScale = std::max(m_referenceInitialParallelScale, 1e-6);
    const double currentScale = std::max(camera->GetParallelScale(), 1e-6);
    return std::max(1, static_cast<int>(std::lround((initialScale / currentScale) * 100.0)));
}

void VtkMprSceneAdapter::resetReferenceCamera()
{
    applyDefaultReferenceCamera();
    if (m_referenceRenderWindow)
    {
        m_referenceRenderWindow->Render();
    }
}

void VtkMprSceneAdapter::renderAll() const
{
    for (int index = 0; index < 3; ++index)
    {
        m_sliceViewers[index]->Render();
    }

    if (m_referenceRenderWindow)
    {
        m_referenceRenderWindow->Render();
    }
}

vtkImageData* VtkMprSceneAdapter::imageData() const
{
    return m_imageData;
}

int VtkMprSceneAdapter::planeIndex(MprSlicePlane plane)
{
    switch (plane)
    {
    case MprSlicePlane::Sagittal:
        return 0;
    case MprSlicePlane::Coronal:
        return 1;
    case MprSlicePlane::Axial:
        return 2;
    }

    return 0;
}

void VtkMprSceneAdapter::initializeReferenceScene()
{
    if (!m_imageData || !m_referenceRenderer)
    {
        return;
    }

    if (m_referenceOrientationMarker)
    {
        m_referenceOrientationMarker->SetEnabled(0);
        m_referenceOrientationMarker = nullptr;
        m_referenceOrientationAxes = nullptr;
    }

    m_referenceRenderer->RemoveAllViewProps();
    m_referenceRenderer->SetBackground(0.05, 0.05, 0.05);

    auto outlineFilter = vtkSmartPointer<vtkOutlineFilter>::New();
    outlineFilter->SetInputData(m_imageData);
    auto outlineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    outlineMapper->SetInputConnection(outlineFilter->GetOutputPort());
    m_referenceOutlineActor = vtkSmartPointer<vtkActor>::New();
    m_referenceOutlineActor->SetMapper(outlineMapper);
    m_referenceOutlineActor->GetProperty()->SetColor(0.7, 0.7, 0.7);
    m_referenceOutlineActor->GetProperty()->SetLineWidth(1.5);
    m_referenceRenderer->AddActor(m_referenceOutlineActor);

    addReferenceSliceLineActors();
    addReferenceCursorActor();
    configureOrientationMarker();

    const auto center = centeredCursorPositionWorld();
    updateReferencePlanes(center);
    applyDefaultReferenceCamera();
}

void VtkMprSceneAdapter::applyDefaultReferenceCamera()
{
    if (!m_imageData || !m_referenceRenderer)
    {
        return;
    }

    auto* camera = m_referenceRenderer->GetActiveCamera();
    if (!camera)
    {
        return;
    }

    double bounds[6];
    m_imageData->GetBounds(bounds);
    const double width = bounds[1] - bounds[0];
    const double height = bounds[3] - bounds[2];
    const double depth = bounds[5] - bounds[4];
    const double diagonal = std::max(1.0, std::sqrt(width * width + height * height + depth * depth));

    const double direction[3] = {0.62, -0.62, 0.48};
    const double distance = diagonal * 1.8;
    const auto& focus = m_referenceCursorPosition;

    camera->ParallelProjectionOn();
    camera->SetFocalPoint(focus.x, focus.y, focus.z);
    camera->SetPosition(
        focus.x + direction[0] * distance,
        focus.y + direction[1] * distance,
        focus.z + direction[2] * distance);
    camera->SetViewUp(0.0, 0.0, 1.0);
    camera->OrthogonalizeViewUp();
    camera->SetParallelScale(diagonal * 0.55);
    m_referenceRenderer->ResetCameraClippingRange();
    m_referenceInitialParallelScale = std::max(camera->GetParallelScale(), 1e-6);
}

void VtkMprSceneAdapter::addReferenceSliceLineActors()
{
    if (!m_referenceRenderer)
    {
        return;
    }

    const double colors[3][3] = {
        {0.95, 0.35, 0.35},
        {0.35, 0.85, 0.45},
        {0.35, 0.55, 0.95}};

    for (int planeIndex = 0; planeIndex < 3; ++planeIndex)
    {
        for (int segmentIndex = 0; segmentIndex < 4; ++segmentIndex)
        {
            m_referenceSliceLineSources[planeIndex][segmentIndex] = vtkSmartPointer<vtkLineSource>::New();

            auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            mapper->SetInputConnection(m_referenceSliceLineSources[planeIndex][segmentIndex]->GetOutputPort());

            m_referenceSliceLineActors[planeIndex][segmentIndex] = vtkSmartPointer<vtkActor>::New();
            m_referenceSliceLineActors[planeIndex][segmentIndex]->SetMapper(mapper);
            m_referenceSliceLineActors[planeIndex][segmentIndex]->GetProperty()->SetColor(
                colors[planeIndex][0],
                colors[planeIndex][1],
                colors[planeIndex][2]);
            m_referenceSliceLineActors[planeIndex][segmentIndex]->GetProperty()->SetAmbient(1.0);
            m_referenceSliceLineActors[planeIndex][segmentIndex]->GetProperty()->SetDiffuse(0.0);
            m_referenceSliceLineActors[planeIndex][segmentIndex]->GetProperty()->SetLineWidth(2.0);
            m_referenceRenderer->AddActor(m_referenceSliceLineActors[planeIndex][segmentIndex]);
        }
    }
}

void VtkMprSceneAdapter::addReferenceCursorActor()
{
    if (!m_imageData || !m_referenceRenderer)
    {
        return;
    }

    double bounds[6];
    m_imageData->GetBounds(bounds);
    const double radius = std::max(
        1.0,
        std::min({bounds[1] - bounds[0], bounds[3] - bounds[2], bounds[5] - bounds[4]}) * 0.018);

    m_referenceCursorSphereSource = vtkSmartPointer<vtkSphereSource>::New();
    m_referenceCursorSphereSource->SetRadius(radius);
    m_referenceCursorSphereSource->SetThetaResolution(18);
    m_referenceCursorSphereSource->SetPhiResolution(12);

    auto cursorMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    cursorMapper->SetInputConnection(m_referenceCursorSphereSource->GetOutputPort());

    m_referenceCursorActor = vtkSmartPointer<vtkActor>::New();
    m_referenceCursorActor->SetMapper(cursorMapper);
    m_referenceCursorActor->GetProperty()->SetColor(1.0, 0.93, 0.20);
    m_referenceCursorActor->GetProperty()->SetAmbient(1.0);
    m_referenceCursorActor->GetProperty()->SetDiffuse(0.0);
    m_referenceRenderer->AddActor(m_referenceCursorActor);
}

void VtkMprSceneAdapter::configureOrientationMarker()
{
    if (!m_referenceRenderWindow)
    {
        return;
    }

    auto* interactor = m_referenceRenderWindow->GetInteractor();
    if (!interactor)
    {
        return;
    }

    m_referenceOrientationAxes = vtkSmartPointer<vtkAxesActor>::New();
    m_referenceOrientationAxes->SetXAxisLabelText("R/L");
    m_referenceOrientationAxes->SetYAxisLabelText("A/P");
    m_referenceOrientationAxes->SetZAxisLabelText("S/I");
    m_referenceOrientationAxes->SetTotalLength(1.1, 1.1, 1.1);
    m_referenceOrientationAxes->SetShaftTypeToLine();
    m_referenceOrientationAxes->SetCylinderRadius(0.025);
    m_referenceOrientationAxes->SetConeRadius(0.08);
    m_referenceOrientationAxes->SetSphereRadius(0.08);

    m_referenceOrientationMarker = vtkSmartPointer<vtkOrientationMarkerWidget>::New();
    m_referenceOrientationMarker->SetOrientationMarker(m_referenceOrientationAxes);
    m_referenceOrientationMarker->SetInteractor(interactor);
    m_referenceOrientationMarker->SetViewport(0.0, 0.0, 0.22, 0.22);
    m_referenceOrientationMarker->SetEnabled(1);
    m_referenceOrientationMarker->InteractiveOff();
}

void VtkMprSceneAdapter::updateReferencePlanes(const MprCursorPositionWorld& cursorPosition)
{
    if (!m_imageData || !m_referenceRenderer)
    {
        return;
    }

    m_referenceCursorPosition = cursorPosition;

    double bounds[6];
    m_imageData->GetBounds(bounds);

    if (m_referenceSliceLineSources[0][0])
    {
        m_referenceSliceLineSources[0][0]->SetPoint1(cursorPosition.x, bounds[2], bounds[4]);
        m_referenceSliceLineSources[0][0]->SetPoint2(cursorPosition.x, bounds[3], bounds[4]);
        m_referenceSliceLineSources[0][1]->SetPoint1(cursorPosition.x, bounds[3], bounds[4]);
        m_referenceSliceLineSources[0][1]->SetPoint2(cursorPosition.x, bounds[3], bounds[5]);
        m_referenceSliceLineSources[0][2]->SetPoint1(cursorPosition.x, bounds[3], bounds[5]);
        m_referenceSliceLineSources[0][2]->SetPoint2(cursorPosition.x, bounds[2], bounds[5]);
        m_referenceSliceLineSources[0][3]->SetPoint1(cursorPosition.x, bounds[2], bounds[5]);
        m_referenceSliceLineSources[0][3]->SetPoint2(cursorPosition.x, bounds[2], bounds[4]);
    }

    if (m_referenceSliceLineSources[1][0])
    {
        m_referenceSliceLineSources[1][0]->SetPoint1(bounds[0], cursorPosition.y, bounds[4]);
        m_referenceSliceLineSources[1][0]->SetPoint2(bounds[1], cursorPosition.y, bounds[4]);
        m_referenceSliceLineSources[1][1]->SetPoint1(bounds[1], cursorPosition.y, bounds[4]);
        m_referenceSliceLineSources[1][1]->SetPoint2(bounds[1], cursorPosition.y, bounds[5]);
        m_referenceSliceLineSources[1][2]->SetPoint1(bounds[1], cursorPosition.y, bounds[5]);
        m_referenceSliceLineSources[1][2]->SetPoint2(bounds[0], cursorPosition.y, bounds[5]);
        m_referenceSliceLineSources[1][3]->SetPoint1(bounds[0], cursorPosition.y, bounds[5]);
        m_referenceSliceLineSources[1][3]->SetPoint2(bounds[0], cursorPosition.y, bounds[4]);
    }

    if (m_referenceSliceLineSources[2][0])
    {
        m_referenceSliceLineSources[2][0]->SetPoint1(bounds[0], bounds[2], cursorPosition.z);
        m_referenceSliceLineSources[2][0]->SetPoint2(bounds[1], bounds[2], cursorPosition.z);
        m_referenceSliceLineSources[2][1]->SetPoint1(bounds[1], bounds[2], cursorPosition.z);
        m_referenceSliceLineSources[2][1]->SetPoint2(bounds[1], bounds[3], cursorPosition.z);
        m_referenceSliceLineSources[2][2]->SetPoint1(bounds[1], bounds[3], cursorPosition.z);
        m_referenceSliceLineSources[2][2]->SetPoint2(bounds[0], bounds[3], cursorPosition.z);
        m_referenceSliceLineSources[2][3]->SetPoint1(bounds[0], bounds[3], cursorPosition.z);
        m_referenceSliceLineSources[2][3]->SetPoint2(bounds[0], bounds[2], cursorPosition.z);
    }

    for (auto& planeLineSources : m_referenceSliceLineSources)
    {
        for (auto& lineSource : planeLineSources)
        {
            if (lineSource)
            {
                lineSource->Update();
            }
        }
    }

    if (m_referenceCursorSphereSource)
    {
        m_referenceCursorSphereSource->SetCenter(cursorPosition.x, cursorPosition.y, cursorPosition.z);
        m_referenceCursorSphereSource->Update();
    }

    centerReferenceCameraOnCursor(cursorPosition);
    m_referenceRenderer->ResetCameraClippingRange();
}

void VtkMprSceneAdapter::centerReferenceCameraOnCursor(const MprCursorPositionWorld& cursorPosition)
{
    if (!m_referenceRenderer)
    {
        return;
    }

    auto* camera = m_referenceRenderer->GetActiveCamera();
    if (!camera)
    {
        return;
    }

    const double* focalPoint = camera->GetFocalPoint();
    const double* position = camera->GetPosition();
    const double delta[3] = {
        cursorPosition.x - focalPoint[0],
        cursorPosition.y - focalPoint[1],
        cursorPosition.z - focalPoint[2]};

    camera->SetFocalPoint(cursorPosition.x, cursorPosition.y, cursorPosition.z);
    camera->SetPosition(
        position[0] + delta[0],
        position[1] + delta[1],
        position[2] + delta[2]);
}
