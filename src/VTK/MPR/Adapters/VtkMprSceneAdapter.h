#pragma once

#include "VTK/MPR/MprTypes.h"

#include <array>
#include <QPointF>
#include <QSize>
#include <vtkSmartPointer.h>

class QVTKOpenGLNativeWidget;
class vtkImageData;
class vtkInteractorStyleTrackballCamera;
class vtkInteractorStyleUser;
class vtkActor;
class vtkAxesActor;
class vtkLineSource;
class vtkOrientationMarkerWidget;
class vtkPolyDataMapper;
class vtkRenderer;
class vtkResliceImageViewer;
class vtkGenericOpenGLRenderWindow;
class vtkSphereSource;
class vtkCallbackCommand;

/**
 * @brief VTK scene adapter for MPR slice and reference panes.
 *
 * Responsibilities:
 * - Own VTK reslice viewers/renderers for MPR panes.
 * - Convert between display positions, normalized positions, and world indices.
 * - Apply WL/WW, crosshair, zoom, and pan state to VTK.
 */
class VtkMprSceneAdapter
{
public:
    /** @brief Creates an empty MPR scene adapter. */
    VtkMprSceneAdapter();
    ~VtkMprSceneAdapter();

    /** @brief Attaches a VTK widget to one slice plane. */
    void attachPane(MprSlicePlane plane, QVTKOpenGLNativeWidget& widget);
    /** @brief Attaches the 3D reference pane widget. */
    void attachReferencePane(QVTKOpenGLNativeWidget& widget);
    /** @brief Initializes VTK viewers from volume image data. */
    void initialize(vtkImageData& imageData, int windowLevel, int windowWidth);
    /** @brief Applies crosshair position in world coordinates. */
    void applyCursorPositionWorld(const MprCursorPositionWorld& cursorPosition);
    /** @brief Applies WL/WW to all MPR panes. */
    void applyWindowLevelWidth(int level, int width);
    /** @brief Applies shared orthogonal slab projection settings to all MPR panes. */
    void applySlabSettings(const MprSlabSettings& settings);
    /** @brief Applies controlled Phase C oblique settings to the selected pane. */
    void applyObliqueSettings(const MprObliqueSettings& settings);
    /** @brief Applies zoom to a plane. */
    void zoom(MprSlicePlane plane, const QPointF& normalizedDelta);
    /** @brief Applies pan to a plane. */
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
    /** @brief Restores the fourth-pane 3D reference camera to its default oblique view. */
    void resetReferenceCamera();
    void renderAll() const;
    vtkImageData* imageData() const;

private:
    static int planeIndex(MprSlicePlane plane);
    void initializeReferenceScene();
    void applyDefaultReferenceCamera();
    void addReferenceSliceLineActors();
    void addReferenceCursorActor();
    void configureOrientationMarker();
    void updateReferencePlanes(const MprCursorPositionWorld& cursorPosition);
    void centerReferenceCameraOnCursor(const MprCursorPositionWorld& cursorPosition);

private:
    vtkImageData* m_imageData{nullptr};
    MprSlabSettings m_slabSettings;
    MprObliqueSettings m_obliqueSettings;
    vtkSmartPointer<vtkInteractorStyleUser> m_neutralInteractorStyles[3];
    std::array<vtkSmartPointer<vtkCallbackCommand>, 3> m_sliceInputSwallowCallbacks;
    vtkSmartPointer<vtkResliceImageViewer> m_sliceViewers[3];
    double m_initialParallelScales[3]{1.0, 1.0, 1.0};
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_referenceRenderWindow;
    vtkSmartPointer<vtkRenderer> m_referenceRenderer;
    vtkSmartPointer<vtkInteractorStyleTrackballCamera> m_referenceInteractorStyle;
    double m_referenceInitialParallelScale{1.0};
    vtkSmartPointer<vtkActor> m_referenceOutlineActor;
    vtkSmartPointer<vtkLineSource> m_referenceSliceLineSources[3][4];
    vtkSmartPointer<vtkActor> m_referenceSliceLineActors[3][4];
    vtkSmartPointer<vtkSphereSource> m_referenceCursorSphereSource;
    vtkSmartPointer<vtkActor> m_referenceCursorActor;
    vtkSmartPointer<vtkAxesActor> m_referenceOrientationAxes;
    vtkSmartPointer<vtkOrientationMarkerWidget> m_referenceOrientationMarker;
    MprCursorPositionWorld m_referenceCursorPosition;
};
