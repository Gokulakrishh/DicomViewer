#pragma once

#include <QByteArray>
#include <cstdint>
#include <QWidget>
#include <memory>

class DicomImage;
class MedicalImage;
class QLabel;
class QSlider;
class QToolButton;
class QVTKOpenGLNativeWidget;
class VtkSliceSceneAdapter;

class VtkDiagnosticSliceView : public QWidget
{
    Q_OBJECT

public:
    explicit VtkDiagnosticSliceView(QWidget* parent = nullptr);
    ~VtkDiagnosticSliceView() override;

    void setImage(std::shared_ptr<MedicalImage> image, bool resetCamera = true);
    void setDicomImage(const DicomImage& image, int windowLevel, int windowWidth, bool resetCamera = true);
    //void clearImage();
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
    void applyZoomDelta(int delta);
    QByteArray captureSnapshotPng() const;
    std::int64_t currentImageByteCount() const;

signals:
    void wheelSliceNavigationRequested(int stepCount);
    void sliceIndexSelected(int index);
    void cinePlaybackToggled(bool checked);
    void windowLevelDragDelta(int deltaLevel, int deltaWidth);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void updateDragState(bool enabled, bool& dragActive);
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
    std::unique_ptr<VtkSliceSceneAdapter> m_sceneAdapter;
};
