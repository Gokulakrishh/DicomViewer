#pragma once

#include "VTK/MPR/Controllers/InteractionRouter.h"
#include "VTK/MPR/Controllers/MprController.h"
#include "VTK/MPR/Controllers/ToolController.h"
#include "VTK/MPR/State/MprScene.h"
#include "Model/MeasurementAnnotationRecord.h"
#include "ViewerTools/Measurements/MeasurementTool.h"
#include "ViewerTools/Measurements/MeasurementService.h"

#include <QHash>
#include <QSet>
#include <QWidget>
#include <vtkSmartPointer.h>

#include <memory>

class IVolumeData;
class MprToolAdapter;
class VtkMprSceneAdapter;
class VtkSliceMprPaneView;
class VtkThreeDReferencePaneView;
class vtkImageData;
class MprMeasurementAnnotationStore;

/**
 * @brief QWidget MPR view containing three orthogonal slice panes and reference pane.
 *
 * Responsibilities:
 * - Coordinate MPR scene, controllers, tools, overlays, and VTK panes.
 * - Keep crosshair and measurement coordinates correct under zoom and pan.
 */
class VtkMprView : public QWidget, public IMeasurementToolHost
{
    Q_OBJECT

public:
    /** @brief Creates an MPR view from diagnostic volume data. */
    explicit VtkMprView(
        std::shared_ptr<IVolumeData> volume,
        int initialWindowLevel,
        int initialWindowWidth,
        QString seriesInstanceUid = {},
        MprMeasurementAnnotationStore* annotationStore = nullptr,
        QWidget* parent = nullptr);
    ~VtkMprView() override;

    /** @brief Sets the active MPR interaction tool. */
    void setActiveTool(MprToolType toolType);
    /** @brief Sets context text displayed in panes. */
    void setContextText(const QString& text);
    /** @brief Sets WL/WW values. */
    void setWindowLevelWidth(int level, int width);
    /** @brief Sets shared orthogonal slab projection settings. */
    void setSlabSettings(const MprSlabSettings& settings);
    /** @brief Sets controlled Phase C oblique reformat settings. */
    void setObliqueSettings(const MprObliqueSettings& settings);
    /** @brief Returns current slab projection settings. */
    [[nodiscard]] MprSlabSettings slabSettings() const;
    /** @brief Returns current controlled oblique settings. */
    [[nodiscard]] MprObliqueSettings obliqueSettings() const;
    /** @brief Returns current window level. */
    [[nodiscard]] int currentWindowLevel() const;
    /** @brief Returns current window width. */
    [[nodiscard]] int currentWindowWidth() const;
    /** @brief Returns active MPR tool. */
    [[nodiscard]] MprToolType activeTool() const;
    /** @brief Returns current active-series MPR annotation records. */
    [[nodiscard]] QList<MprMeasurementAnnotationRecord> mprAnnotationRecords() const;

signals:
    void windowLevelWidthChanged(int level, int width);
    void mprAnnotationsChanged(const QList<MprMeasurementAnnotationRecord>& records);

public slots:
    void goToMprAnnotation(const QString& annotationId);
    void deleteMprAnnotation(const QString& annotationId);
    void updateMprAnnotationMetadata(
        const QString& annotationId,
        const QString& label,
        const QString& bodyRegion,
        const QString& note);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    [[nodiscard]] MeasurementPoint measurementPointForInput(const ViewerInputEvent& event) const override;
    void onMeasurementToolUpdated() override;

private:
    void refreshOverlayState();
    void refreshMeasurementOverlays();
    void updateCursorState();
    void updatePaneStatusText();
    [[nodiscard]] QString projectionStatusText(MprSlicePlane plane) const;
    [[nodiscard]] QString displayContextText() const;
    [[nodiscard]] MprSlicePlane planeForRenderWidget(QObject* watched) const;
    [[nodiscard]] QPointF normalizedPositionForEvent(QObject* watched, const QPointF& position) const;
    [[nodiscard]] QPointF normalizedCrosshairPositionForPane(
        VtkSliceMprPaneView& pane,
        MprSlicePlane plane,
        const MprCursorPositionWorld& position) const;
    void handleWheelEvent(MprSlicePlane plane, QWheelEvent* event);
    bool handleMeasurementEvent(QObject* watched, QEvent* event, MprSlicePlane plane);
    [[nodiscard]] MeasurementPoint measurementPointForEvent(
        MprSlicePlane plane,
        QWidget& widget,
        const QPointF& position) const;
    [[nodiscard]] QString measurementLabel(
        const MeasurementAnnotation& measurement,
        MprSlicePlane plane) const;
    [[nodiscard]] bool measurementBelongsToCurrentSlice(
        const MeasurementAnnotation& measurement,
        MprSlicePlane plane) const;
    [[nodiscard]] double sliceToleranceMm(MprSlicePlane plane) const;
    void loadPersistedMprAnnotations();
    void captureNewMeasurementPlaneContexts();
    void persistNewMprMeasurements();
    void publishMprAnnotationRecords();
    [[nodiscard]] QList<MprMeasurementAnnotationRecord> currentMprAnnotationRecords() const;
    [[nodiscard]] MprMeasurementAnnotationRecord toMprAnnotationRecord(
        const MeasurementAnnotation& measurement,
        MprSlicePlane plane) const;
    [[nodiscard]] QVector<DisplayMeasurement> displayMeasurementsForPane(
        VtkSliceMprPaneView& pane,
        MprSlicePlane plane) const;
    bool handlePanEvent(QObject* watched, QEvent* event, MprSlicePlane plane);
    void setupUi();
    void configureScene();
    void configureBindings();
    void configureSliders();

private:
    std::shared_ptr<IVolumeData> m_volume;
    vtkSmartPointer<vtkImageData> m_imageData;
    MprScene m_scene;
    MprController m_controller;
    std::unique_ptr<VtkMprSceneAdapter> m_sceneAdapter;
    std::unique_ptr<MprToolAdapter> m_toolAdapter;
    ToolController m_toolController;
    InteractionRouter m_interactionRouter;
    MeasurementService m_measurementService;
    std::unique_ptr<VtkSliceMprPaneView> m_axialPane;
    std::unique_ptr<VtkSliceMprPaneView> m_coronalPane;
    std::unique_ptr<VtkSliceMprPaneView> m_sagittalPane;
    std::unique_ptr<VtkThreeDReferencePaneView> m_referencePane;
    QPointF m_lastInteractionPosition{0.5, 0.5};
    bool m_panDragActive{false};
    QString m_contextText;
    MprSlabSettings m_slabSettings;
    MprObliqueSettings m_obliqueSettings;
    QString m_seriesInstanceUid;
    MprMeasurementAnnotationStore* m_annotationStore{nullptr};
    QHash<QString, MprSlicePlane> m_measurementPlaneById;
    QHash<QString, MprMeasurementAnnotationRecord> m_annotationRecordById;
    QSet<QString> m_persistedMprAnnotationIds;
    MprSlicePlane m_lastMeasurementInputPlane{MprSlicePlane::Axial};
};
