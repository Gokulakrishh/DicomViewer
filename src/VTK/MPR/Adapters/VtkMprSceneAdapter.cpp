#include "VTK/MPR/Adapters/VtkMprSceneAdapter.h"

#include <QVTKOpenGLNativeWidget.h>
#include <algorithm>
#include <cmath>
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageData.h>
#include <vtkInteractorStyleUser.h>
#include <vtkOutlineFilter.h>
#include <vtkPlaneSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkResliceImageViewer.h>

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
    m_neutralInteractorStyles[index] = vtkSmartPointer<vtkInteractorStyleUser>::New();
    widget.renderWindow()->GetInteractor()->SetInteractorStyle(m_neutralInteractorStyles[index]);
}

void VtkMprSceneAdapter::attachReferencePane(QVTKOpenGLNativeWidget& widget)
{
    m_referenceRenderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    m_referenceRenderer = vtkSmartPointer<vtkRenderer>::New();
    m_referenceRenderWindow->AddRenderer(m_referenceRenderer);
    widget.setRenderWindow(m_referenceRenderWindow);

    m_referenceInteractorStyle = vtkSmartPointer<vtkInteractorStyleUser>::New();
    widget.renderWindow()->GetInteractor()->SetInteractorStyle(m_referenceInteractorStyle);
}

void VtkMprSceneAdapter::initialize(vtkImageData& imageData, int windowLevel, int windowWidth)
{
    m_imageData = &imageData;

    for (int index = 0; index < 3; ++index)
    {
        m_sliceViewers[index]->SetInputData(&imageData);
        m_sliceViewers[index]->SetSliceOrientation(index);
        m_sliceViewers[index]->SetResliceModeToAxisAligned();
        m_sliceViewers[index]->SetColorLevel(windowLevel);
        m_sliceViewers[index]->SetColorWindow(windowWidth);
        m_sliceViewers[index]->SetSliceScrollOnMouseWheel(false);
        m_sliceViewers[index]->GetRenderer()->ResetCamera();
        if (auto* camera = m_sliceViewers[index]->GetRenderer()->GetActiveCamera())
        {
            m_initialParallelScales[index] = std::max(camera->GetParallelScale(), 1e-6);
        }
    }

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

    const double colors[3][3] = {
        {0.95, 0.35, 0.35},
        {0.35, 0.85, 0.45},
        {0.35, 0.55, 0.95}};

    for (int index = 0; index < 3; ++index)
    {
        m_referencePlaneSources[index] = vtkSmartPointer<vtkPlaneSource>::New();
        auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(m_referencePlaneSources[index]->GetOutputPort());

        m_referencePlaneActors[index] = vtkSmartPointer<vtkActor>::New();
        m_referencePlaneActors[index]->SetMapper(mapper);
        m_referencePlaneActors[index]->GetProperty()->SetColor(
            colors[index][0],
            colors[index][1],
            colors[index][2]);
        m_referencePlaneActors[index]->GetProperty()->SetOpacity(0.22);
        m_referencePlaneActors[index]->GetProperty()->SetAmbient(1.0);
        m_referencePlaneActors[index]->GetProperty()->SetDiffuse(0.0);
        m_referenceRenderer->AddActor(m_referencePlaneActors[index]);
    }

    const auto center = centeredCursorPositionWorld();
    updateReferencePlanes(center);
    m_referenceRenderer->ResetCamera();
    m_referenceRenderer->GetActiveCamera()->Azimuth(35.0);
    m_referenceRenderer->GetActiveCamera()->Elevation(25.0);
    m_referenceRenderer->ResetCameraClippingRange();
    if (auto* camera = m_referenceRenderer->GetActiveCamera())
    {
        m_referenceInitialParallelScale = std::max(camera->GetParallelScale(), 1e-6);
    }
}

void VtkMprSceneAdapter::updateReferencePlanes(const MprCursorPositionWorld& cursorPosition)
{
    if (!m_imageData || !m_referenceRenderer)
    {
        return;
    }

    double bounds[6];
    m_imageData->GetBounds(bounds);

    if (m_referencePlaneSources[0])
    {
        m_referencePlaneSources[0]->SetOrigin(cursorPosition.x, bounds[2], bounds[4]);
        m_referencePlaneSources[0]->SetPoint1(cursorPosition.x, bounds[3], bounds[4]);
        m_referencePlaneSources[0]->SetPoint2(cursorPosition.x, bounds[2], bounds[5]);
        m_referencePlaneSources[0]->Update();
    }

    if (m_referencePlaneSources[1])
    {
        m_referencePlaneSources[1]->SetOrigin(bounds[0], cursorPosition.y, bounds[4]);
        m_referencePlaneSources[1]->SetPoint1(bounds[1], cursorPosition.y, bounds[4]);
        m_referencePlaneSources[1]->SetPoint2(bounds[0], cursorPosition.y, bounds[5]);
        m_referencePlaneSources[1]->Update();
    }

    if (m_referencePlaneSources[2])
    {
        m_referencePlaneSources[2]->SetOrigin(bounds[0], bounds[2], cursorPosition.z);
        m_referencePlaneSources[2]->SetPoint1(bounds[1], bounds[2], cursorPosition.z);
        m_referencePlaneSources[2]->SetPoint2(bounds[0], bounds[3], cursorPosition.z);
        m_referencePlaneSources[2]->Update();
    }

    m_referenceRenderer->ResetCameraClippingRange();
}
