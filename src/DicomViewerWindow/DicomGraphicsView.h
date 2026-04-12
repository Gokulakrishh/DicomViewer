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
#include <QWheelEvent>
#include <memory>

class MedicalImage;

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
    void distanceMeasurementRequested(const QPoint& startPixel, const QPoint& endPixel);
    void pixelProbeRequested(const QPoint& pixelPos);
    void angleMeasurementRequested(const QPoint& startPixel, const QPoint& vertexPixel, const QPoint& endPixel);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    bool mapMouseToImage(const QPoint& viewPos, QPoint& pixelPos, QPointF& scenePos) const;
    void updatePixmap();

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
};
