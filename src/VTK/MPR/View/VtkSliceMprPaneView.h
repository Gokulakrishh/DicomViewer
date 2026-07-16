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

/**
 * @brief One 2D slice pane in the MPR viewer.
 */
class VtkSliceMprPaneView final : public IMprPaneView
{
public:
    /** @brief Creates a slice pane for one plane. */
    VtkSliceMprPaneView(const QString& title, MprSlicePlane plane, QWidget* parent);
    ~VtkSliceMprPaneView() override;

    /** @brief Returns pane slice plane. */
    [[nodiscard]] MprSlicePlane plane() const override;
    /** @brief Returns root widget. */
    [[nodiscard]] QWidget* widget() const override;
    /** @brief Returns VTK render widget. */
    [[nodiscard]] QVTKOpenGLNativeWidget* renderWidget() const override;
    /** @brief Returns slice slider. */
    [[nodiscard]] QSlider* sliceSlider() const override;
    /** @brief Sets context label text. */
    void setContextText(const QString& text);
    /** @brief Sets slice label text. */
    void setSliceText(const QString& text);
    /** @brief Sets WL/WW label text. */
    void setWindowLevelText(const QString& text);
    /** @brief Sets slab projection label text. */
    void setSlabText(const QString& text);
    /** @brief Sets zoom label text. */
    void setZoomText(const QString& text);
    /** @brief Shows or hides crosshair overlay. */
    void setCrosshairVisible(bool visible);
    /** @brief Sets crosshair normalized pane position. */
    void setCrosshairPosition(const QPointF& normalizedPosition);
    /** @brief Sets display measurements for this pane. */
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
    QLabel* m_slabLabel{nullptr};
    QLabel* m_zoomLabel{nullptr};
    CrosshairOverlayWidget* m_crosshairOverlay{nullptr};
    MeasurementOverlayWidget* m_measurementOverlay{nullptr};
    QSlider* m_sliceSlider{nullptr};
    QPointF m_crosshairNormalizedPosition{0.5, 0.5};
};
