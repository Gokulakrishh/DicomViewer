#pragma once

#include "VTK/MPR/MprTypes.h"

#include <array>
#include <QPointF>
#include <QSize>
#include <vtkSmartPointer.h>

class QVTKOpenGLNativeWidget;
class vtkImageData;
class vtkInteractorStyleUser;
class vtkActor;
class vtkPlaneSource;
class vtkRenderer;
class vtkResliceImageViewer;
class vtkGenericOpenGLRenderWindow;

class VtkMprSceneAdapter
{
public:
    VtkMprSceneAdapter();
    ~VtkMprSceneAdapter();

    void attachPane(MprSlicePlane plane, QVTKOpenGLNativeWidget& widget);
    void attachReferencePane(QVTKOpenGLNativeWidget& widget);
    void initialize(vtkImageData& imageData, int windowLevel, int windowWidth);
    void applyCursorPositionWorld(const MprCursorPositionWorld& cursorPosition);
    void applyWindowLevelWidth(int level, int width);
    void zoom(MprSlicePlane plane, const QPointF& normalizedDelta);
    void pan(MprSlicePlane plane, const QPointF& displayDelta, const QSize& widgetSize);
    [[nodiscard]] MprCursorPositionWorld worldPositionFromDisplayPosition(
        MprSlicePlane plane,
        const QPointF& widgetPosition,
        const QSize& widgetSize,
        const MprCursorPositionWorld& currentCursorPosition) const;
    [[nodiscard]] QPointF normalizedImagePositionFromDisplayPosition(
        MprSlicePlane plane,
        const QPointF& widgetPosition,
        const QSize& widgetSize,
        const MprCursorPositionWorld& currentCursorPosition) const;
    [[nodiscard]] QPointF normalizedDisplayPositionForCursorWorld(
        MprSlicePlane plane,
        const MprCursorPositionWorld& cursorPosition,
        const QSize& widgetSize) const;
    [[nodiscard]] std::array<double, 3> continuousIndexFromWorldPosition(
        const MprCursorPositionWorld& worldPosition) const;
    [[nodiscard]] MprCursorPositionWorld centeredCursorPositionWorld() const;
    [[nodiscard]] int sliceMin(MprSlicePlane plane) const;
    [[nodiscard]] int sliceMax(MprSlicePlane plane) const;
    [[nodiscard]] int currentSlice(MprSlicePlane plane) const;
    [[nodiscard]] int zoomPercent(MprSlicePlane plane) const;
    [[nodiscard]] int referenceZoomPercent() const;
    void renderAll() const;
    vtkImageData* imageData() const;

private:
    static int planeIndex(MprSlicePlane plane);
    void initializeReferenceScene();
    void updateReferencePlanes(const MprCursorPositionWorld& cursorPosition);

private:
    vtkImageData* m_imageData{nullptr};
    vtkSmartPointer<vtkInteractorStyleUser> m_neutralInteractorStyles[3];
    vtkSmartPointer<vtkResliceImageViewer> m_sliceViewers[3];
    double m_initialParallelScales[3]{1.0, 1.0, 1.0};
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_referenceRenderWindow;
    vtkSmartPointer<vtkRenderer> m_referenceRenderer;
    vtkSmartPointer<vtkInteractorStyleUser> m_referenceInteractorStyle;
    double m_referenceInitialParallelScale{1.0};
    vtkSmartPointer<vtkActor> m_referenceOutlineActor;
    vtkSmartPointer<vtkPlaneSource> m_referencePlaneSources[3];
    vtkSmartPointer<vtkActor> m_referencePlaneActors[3];
};
