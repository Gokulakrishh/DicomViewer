#pragma once

#include <QByteArray>
#include <cstdint>
#include <QList>
#include <QWidget>
#include <memory>

#include "Model/MeasurementAnnotationRecord.h"
#include "ViewerTools/IViewerToolTarget.h"
#include "ViewerTools/Measurements/MeasurementService.h"
#include "ViewerTools/Measurements/MeasurementTool.h"

class DicomImage;
class IViewerTool;
class MedicalImage;
class QLabel;
class MeasurementOverlayWidget;
class QSlider;
class QToolButton;
class QVTKOpenGLNativeWidget;
class VtkSliceSceneAdapter;

class VtkDiagnosticSliceView : public QWidget, public IViewerToolTarget, public IMeasurementToolHost
{
    Q_OBJECT

public:
    explicit VtkDiagnosticSliceView(QWidget* parent = nullptr);
    ~VtkDiagnosticSliceView() override;

    void setImage(std::shared_ptr<MedicalImage> image, bool resetCamera = true);
    void setDicomImage(const DicomImage& image, int windowLevel, int windowWidth, bool resetCamera = true);
    void clearImage();
    void setSliceNavigationState(int currentIndex, int totalCount);
    void setCineAvailable(bool available);
    void setCinePlaying(bool playing);
    void setPatientInfoText(
        const QString& patientName,
        const QString& age,
        const QString& dateOfBirth,
        const QString& doctor,
        const QString& modality,
        const QString& scanDate);
    void setWindowLevelWidth(int windowLevel, int windowWidth);
    void setWindowLevelInteractionEnabled(bool enabled);
    void setZoomInteractionEnabled(bool enabled);
    void setPanInteractionEnabled(bool enabled);
    void setDistanceMeasurementEnabled(bool enabled);
    void setPolylineMeasurementEnabled(bool enabled);
    void setAngleMeasurementEnabled(bool enabled);
    void setRectangleRoiMeasurementEnabled(bool enabled);
    void clearMeasurements();
    void setSliceAnnotationContext(const QString& seriesInstanceUid, const QString& sopInstanceUid);
    void loadSliceAnnotations(const QList<SliceMeasurementAnnotationRecord>& records);
    void applyZoomDelta(int delta);
    QByteArray captureSnapshotPng() const;
    std::int64_t currentImageByteCount() const;

signals:
    void wheelSliceNavigationRequested(int stepCount);
    void sliceIndexSelected(int index);
    void cinePlaybackToggled(bool checked);
    void windowLevelDragDelta(int deltaLevel, int deltaWidth);
    void sliceAnnotationsChanged(const QList<SliceMeasurementAnnotationRecord>& records);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    [[nodiscard]] MeasurementPoint measurementPointForInput(const ViewerInputEvent& event) const override;
    void onMeasurementToolUpdated() override;
    void handleCrosshairInput(const ViewerInputEvent& event) override;
    void handleWindowLevelInput(const ViewerInputEvent& event) override;
    void handleZoomInput(const ViewerInputEvent& event) override;
    void handlePanInput(const ViewerInputEvent& event) override;

private:
    enum class ActiveMeasurementTool
    {
        None,
        Distance,
        Polyline,
        Angle,
        RectangleRoi
    };

    void updateDragState(bool enabled, bool& dragActive);
    bool handleMeasurementEvent(QEvent* event);
    void updateCursorState();
    void refreshMeasurementOverlay();
    void emitSliceAnnotationsChangedIfNeeded();
    [[nodiscard]] IViewerTool* activeMeasurementTool() const;
    [[nodiscard]] MeasurementPoint measurementPointForMousePosition(const QPointF& position) const;
    [[nodiscard]] QString measurementLabel(const MeasurementAnnotation& measurement) const;
    [[nodiscard]] QVector<DisplayMeasurement> displayMeasurements() const;
    [[nodiscard]] QList<SliceMeasurementAnnotationRecord> currentSliceAnnotationRecords() const;
    void buildControls();
    void updateSliceNavigationLabel();
    void updateStatusText();
    void layoutOverlayWidgets();

private:
    QVTKOpenGLNativeWidget* m_renderWidget{nullptr};
    QWidget* m_cineBar{nullptr};
    QToolButton* m_cinePlayButton{nullptr};
    QSlider* m_sliceSlider{nullptr};
    QLabel* m_sliceLabel{nullptr};
    QLabel* m_patientInfoLabel{nullptr};
    QLabel* m_sliceInfoLabel{nullptr};
    QLabel* m_windowLevelLabel{nullptr};
    QLabel* m_zoomLabel{nullptr};
    MeasurementOverlayWidget* m_measurementOverlay{nullptr};
    int m_currentSliceIndex{0};
    int m_totalSliceCount{0};
    int m_currentWindowLevel{0};
    int m_currentWindowWidth{100};
    bool m_windowLevelInteractionEnabled{false};
    bool m_windowLevelDragActive{false};
    QPoint m_lastWindowLevelDragPosition;
    bool m_zoomInteractionEnabled{false};
    bool m_zoomDragActive{false};
    QPoint m_lastZoomDragPosition;
    bool m_panInteractionEnabled{false};
    bool m_panDragActive{false};
    QPoint m_lastPanDragPosition;
    ActiveMeasurementTool m_activeMeasurementTool{ActiveMeasurementTool::None};
    const DicomImage* m_currentDicomImage{nullptr};
    QString m_currentSeriesInstanceUid;
    QString m_currentSopInstanceUid;
    MeasurementService m_measurementService;
    QVector<MeasurementAnnotation> m_lastPersistedMeasurements;
    bool m_suppressSliceAnnotationSignal{false};
    std::unique_ptr<IViewerTool> m_distanceMeasurementTool;
    std::unique_ptr<IViewerTool> m_polylineMeasurementTool;
    std::unique_ptr<IViewerTool> m_angleMeasurementTool;
    std::unique_ptr<IViewerTool> m_rectangleRoiMeasurementTool;
    std::unique_ptr<IViewerTool> m_panTool;
    std::unique_ptr<VtkSliceSceneAdapter> m_sceneAdapter;
};
