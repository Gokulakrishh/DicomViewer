#pragma once

#include "VTK/MPR/View/IMprPaneView.h"
#include "ViewerTools/Measurements/MeasurementTypes.h"

#include <QPointF>
#include <QString>
#include <memory>

class CrosshairOverlayWidget;
class MeasurementOverlayWidget;
class QLabel;
class QSlider;
class QVBoxLayout;
class QWidget;
class QVTKOpenGLNativeWidget;

class VtkSliceMprPaneView final : public IMprPaneView
{
public:
    VtkSliceMprPaneView(const QString& title, MprSlicePlane plane, QWidget* parent);
    ~VtkSliceMprPaneView() override;

    [[nodiscard]] MprSlicePlane plane() const override;
    [[nodiscard]] QWidget* widget() const override;
    [[nodiscard]] QVTKOpenGLNativeWidget* renderWidget() const override;
    [[nodiscard]] QSlider* sliceSlider() const override;
    void setContextText(const QString& text);
    void setSliceText(const QString& text);
    void setWindowLevelText(const QString& text);
    void setZoomText(const QString& text);
    void setCrosshairVisible(bool visible);
    void setCrosshairPosition(const QPointF& normalizedPosition);
    void setMeasurements(const QVector<DisplayMeasurement>& measurements);

private:
    void layoutStatusLabels();

private:
    MprSlicePlane m_plane;
    QWidget* m_rootWidget{nullptr};
    QLabel* m_titleLabel{nullptr};
    QVTKOpenGLNativeWidget* m_renderWidget{nullptr};
    QLabel* m_contextLabel{nullptr};
    QLabel* m_sliceInfoLabel{nullptr};
    QLabel* m_windowLevelLabel{nullptr};
    QLabel* m_zoomLabel{nullptr};
    CrosshairOverlayWidget* m_crosshairOverlay{nullptr};
    MeasurementOverlayWidget* m_measurementOverlay{nullptr};
    QSlider* m_sliceSlider{nullptr};
    QPointF m_crosshairNormalizedPosition{0.5, 0.5};
};
