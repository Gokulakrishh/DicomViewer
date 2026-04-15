#pragma once

#include <QGraphicsLineItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsView>
#include <QMouseEvent>
#include <QPoint>
#include <QPointF>
#include <QResizeEvent>
#include <QVector>
#include <QWheelEvent>
#include <memory>

class MedicalImage;
class QLabel;
class QSlider;
class QToolButton;
class QWidget;

class DicomGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    enum class ToolMode
    {
        Pan,
        Distance,
        PixelProbe,
        Angle
    };

    explicit DicomGraphicsView(QWidget* parent = nullptr);
    ~DicomGraphicsView() override = default;

    void setImage(std::shared_ptr<MedicalImage> image);
    void clearImage();
    void setToolMode(ToolMode toolMode);
    void setSliceNavigationState(int currentIndex, int totalCount);
    void setCineAvailable(bool available);
    void setCinePlaying(bool playing);
    void showDistanceMeasurement(const QPointF& startScenePos, const QPointF& endScenePos, const QString& label);
    void showPixelProbe(const QPointF& scenePos, const QString& label);
    void showAngleMeasurement(
        const QPointF& startScenePos,
        const QPointF& vertexScenePos,
        const QPointF& endScenePos,
        const QString& label);
    void clearMeasurementOverlays();

signals:
    void wheelSliceNavigationRequested(int stepCount);
    void toolModeSelected(DicomGraphicsView::ToolMode toolMode);
    void sliceIndexSelected(int index);
    void cinePlaybackToggled(bool checked);
    void distanceMeasurementRequested(const QPoint& startPixel, const QPoint& endPixel);
    void pixelProbeRequested(const QPoint& pixelPos);
    void angleMeasurementRequested(const QPoint& startPixel, const QPoint& vertexPixel, const QPoint& endPixel);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void buildOverlayControls();
    void updateOverlayGeometry();
    void updateToolOverlaySelection();
    void updateSliceNavigationLabel();
    bool mapMouseToImage(const QPoint& viewPos, QPoint& pixelPos, QPointF& scenePos) const;
    void updatePixmap();
    void applyFitToView();

private:
    QGraphicsScene* m_scene{nullptr};
    QGraphicsPixmapItem* m_pixmapItem{nullptr};
    QGraphicsLineItem* m_measurementLineItem{nullptr};
    QGraphicsSimpleTextItem* m_measurementTextItem{nullptr};
    QGraphicsLineItem* m_angleFirstLineItem{nullptr};
    QGraphicsLineItem* m_angleSecondLineItem{nullptr};
    QGraphicsSimpleTextItem* m_angleTextItem{nullptr};
    QGraphicsLineItem* m_probeHorizontalItem{nullptr};
    QGraphicsLineItem* m_probeVerticalItem{nullptr};
    QGraphicsSimpleTextItem* m_probeTextItem{nullptr};
    std::shared_ptr<MedicalImage> m_image;
    ToolMode m_toolMode{ToolMode::Pan};
    bool m_hasDistanceAnchor{false};
    QPoint m_distanceAnchorPixel;
    QPointF m_distanceAnchorScene;
    int m_angleClickCount{0};
    QPoint m_angleStartPixel;
    QPoint m_angleVertexPixel;
    QPointF m_angleStartScene;
    QPointF m_angleVertexScene;
    double m_zoomFactor{1.0};
    bool m_fitToViewPending{true};
    QWidget* m_toolOverlayWidget{nullptr};
    QWidget* m_cineOverlayWidget{nullptr};
    QVector<QToolButton*> m_toolButtons;
    QToolButton* m_cinePlayButton{nullptr};
    QSlider* m_sliceSlider{nullptr};
    QLabel* m_sliceLabel{nullptr};
    int m_currentSliceIndex{0};
    int m_totalSliceCount{0};
};
