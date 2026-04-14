#pragma once

#include "Services/MprRenderService.h"
#include "Services/VolumeResampleService.h"

#include <QComboBox>
#include <QPoint>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QResizeEvent>
#include <QSize>
#include <memory>

class IVolumeData;
class QLabel;
class QSlider;
class QWidget;

class MprViewerWindow : public QMainWindow
{
    Q_OBJECT

public:
    struct CrosshairState
    {
        int x{0};
        int y{0};
        int z{0};
    };

    struct PaneWidgets
    {
        MprRenderService::Plane plane{MprRenderService::Plane::Axial};
        QWidget* container{nullptr};
        QLabel* titleLabel{nullptr};
        QLabel* imageLabel{nullptr};
        QSlider* slider{nullptr};
        QSize sourceImageSize;
    };

    explicit MprViewerWindow(
        std::shared_ptr<IVolumeData> volume,
        int initialWindowLevel,
        int initialWindowWidth,
        QWidget* parent = nullptr);

private:
    void setupUi();
    void renderAllPanes();
    void renderPane(MprRenderService::Plane plane, PaneWidgets& pane);
    void updatePaneStretchFactors();
    int paneStretchFactor(MprRenderService::Plane plane) const;
    void syncSlidersToCrosshair();
    void updateCoordinateReadout();
    double currentScalarValue() const;
    void updateWindowControlLabels();
    void applyWindowPreset(int presetIndex);
    int crosshairSliceIndex(MprRenderService::Plane plane) const;
    QSize planeVoxelSize(MprRenderService::Plane plane) const;
    QPoint crosshairPixel(MprRenderService::Plane plane, const QSize& imageSize) const;
    VolumeVector3D currentWorldCoordinate() const;
    QRect displayedImageRect(const PaneWidgets& pane) const;
    void updateCrosshairFromPanePosition(PaneWidgets& pane, const QPoint& labelPosition);
    PaneWidgets* paneForObject(QObject* object);
    bool eventFilter(QObject* watched, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    std::shared_ptr<IVolumeData> m_sourceVolume;
    std::shared_ptr<IVolumeData> m_displayVolume;
    VolumeResampleService m_resampleService;
    MprRenderService m_renderService;
    QHBoxLayout* m_paneLayout{nullptr};
    PaneWidgets m_axialPane;
    PaneWidgets m_coronalPane;
    PaneWidgets m_sagittalPane;
    QComboBox* m_presetComboBox{nullptr};
    QSlider* m_windowLevelSlider{nullptr};
    QSlider* m_windowWidthSlider{nullptr};
    QLabel* m_windowLevelLabel{nullptr};
    QLabel* m_windowWidthLabel{nullptr};
    QLabel* m_coordinateLabel{nullptr};
    CrosshairState m_crosshair;
    int m_windowLevel{40};
    int m_windowWidth{400};
    PaneWidgets* m_activeDragPane{nullptr};
};
