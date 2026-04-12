#include "DicomGraphicsView.h"

#include "Model/MedicalImage.h"

#include <QPainter>
#include <QPen>

DicomGraphicsView::DicomGraphicsView(QWidget* parent)
    : QGraphicsView(parent)
{
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);

    m_pixmapItem = new QGraphicsPixmapItem();
    m_scene->addItem(m_pixmapItem);
    m_measurementLineItem = m_scene->addLine(QLineF(), QPen(QColor(255, 196, 0), 2));
    m_measurementLineItem->setVisible(false);
    m_measurementTextItem = m_scene->addSimpleText(QString());
    m_measurementTextItem->setBrush(QBrush(Qt::yellow));
    m_measurementTextItem->setVisible(false);
    m_angleFirstLineItem = m_scene->addLine(QLineF(), QPen(QColor(255, 128, 64), 2));
    m_angleFirstLineItem->setVisible(false);
    m_angleSecondLineItem = m_scene->addLine(QLineF(), QPen(QColor(255, 128, 64), 2));
    m_angleSecondLineItem->setVisible(false);
    m_angleTextItem = m_scene->addSimpleText(QString());
    m_angleTextItem->setBrush(QBrush(QColor(255, 128, 64)));
    m_angleTextItem->setVisible(false);
    m_probeHorizontalItem = m_scene->addLine(QLineF(), QPen(QColor(64, 220, 255), 1));
    m_probeHorizontalItem->setVisible(false);
    m_probeVerticalItem = m_scene->addLine(QLineF(), QPen(QColor(64, 220, 255), 1));
    m_probeVerticalItem->setVisible(false);
    m_probeTextItem = m_scene->addSimpleText(QString());
    m_probeTextItem->setBrush(QBrush(QColor(64, 220, 255)));
    m_probeTextItem->setVisible(false);

    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setCacheMode(QGraphicsView::CacheNone);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
}

void DicomGraphicsView::setImage(std::shared_ptr<MedicalImage> image)
{
    m_image = std::move(image);
    updatePixmap();
}

void DicomGraphicsView::clearImage()
{
    clearMeasurementOverlays();
    m_image.reset();
    m_pixmapItem->setPixmap(QPixmap());
    resetTransform();
    m_zoomFactor = 1.0;
}

void DicomGraphicsView::setToolMode(ToolMode toolMode)
{
    m_toolMode = toolMode;
    m_hasDistanceAnchor = false;
    m_angleClickCount = 0;
    if (m_toolMode == ToolMode::Pan)
    {
        setDragMode(QGraphicsView::ScrollHandDrag);
    }
    else
    {
        setDragMode(QGraphicsView::NoDrag);
    }
}

void DicomGraphicsView::showDistanceMeasurement(const QPointF& startScenePos, const QPointF& endScenePos, const QString& label)
{
    m_measurementLineItem->setLine(QLineF(startScenePos, endScenePos));
    m_measurementLineItem->setVisible(true);
    m_measurementTextItem->setText(label);
    m_measurementTextItem->setPos((startScenePos + endScenePos) / 2.0);
    m_measurementTextItem->setVisible(true);
}

void DicomGraphicsView::showPixelProbe(const QPointF& scenePos, const QString& label)
{
    constexpr qreal markerRadius = 8.0;
    m_probeHorizontalItem->setLine(
        scenePos.x() - markerRadius,
        scenePos.y(),
        scenePos.x() + markerRadius,
        scenePos.y());
    m_probeVerticalItem->setLine(
        scenePos.x(),
        scenePos.y() - markerRadius,
        scenePos.x(),
        scenePos.y() + markerRadius);
    m_probeHorizontalItem->setVisible(true);
    m_probeVerticalItem->setVisible(true);
    m_probeTextItem->setText(label);
    m_probeTextItem->setPos(scenePos + QPointF(10.0, 10.0));
    m_probeTextItem->setVisible(true);
}

void DicomGraphicsView::showAngleMeasurement(
    const QPointF& startScenePos,
    const QPointF& vertexScenePos,
    const QPointF& endScenePos,
    const QString& label)
{
    m_angleFirstLineItem->setLine(QLineF(vertexScenePos, startScenePos));
    m_angleSecondLineItem->setLine(QLineF(vertexScenePos, endScenePos));
    m_angleFirstLineItem->setVisible(true);
    m_angleSecondLineItem->setVisible(true);
    m_angleTextItem->setText(label);
    m_angleTextItem->setPos(vertexScenePos + QPointF(10.0, -10.0));
    m_angleTextItem->setVisible(true);
}

void DicomGraphicsView::clearMeasurementOverlays()
{
    m_hasDistanceAnchor = false;
    m_angleClickCount = 0;
    m_measurementLineItem->setVisible(false);
    m_measurementTextItem->setVisible(false);
    m_angleFirstLineItem->setVisible(false);
    m_angleSecondLineItem->setVisible(false);
    m_angleTextItem->setVisible(false);
    m_probeHorizontalItem->setVisible(false);
    m_probeVerticalItem->setVisible(false);
    m_probeTextItem->setVisible(false);
}

void DicomGraphicsView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton || m_toolMode == ToolMode::Pan)
    {
        QGraphicsView::mousePressEvent(event);
        return;
    }

    QPoint pixelPos;
    QPointF scenePos;
    if (!mapMouseToImage(event->pos(), pixelPos, scenePos))
    {
        QGraphicsView::mousePressEvent(event);
        return;
    }

    if (m_toolMode == ToolMode::PixelProbe)
    {
        emit pixelProbeRequested(pixelPos);
        event->accept();
        return;
    }

    if (m_toolMode == ToolMode::Distance)
    {
        if (!m_hasDistanceAnchor)
        {
            m_hasDistanceAnchor = true;
            m_distanceAnchorPixel = pixelPos;
            m_distanceAnchorScene = scenePos;
            m_measurementLineItem->setLine(QLineF(scenePos, scenePos));
            m_measurementLineItem->setVisible(true);
            m_measurementTextItem->setVisible(false);
        }
        else
        {
            emit distanceMeasurementRequested(m_distanceAnchorPixel, pixelPos);
            m_hasDistanceAnchor = false;
        }
        event->accept();
        return;
    }

    if (m_toolMode == ToolMode::Angle)
    {
        if (m_angleClickCount == 0)
        {
            m_angleStartPixel = pixelPos;
            m_angleStartScene = scenePos;
            m_angleClickCount = 1;
            m_angleFirstLineItem->setLine(QLineF(scenePos, scenePos));
            m_angleFirstLineItem->setVisible(true);
            m_angleSecondLineItem->setVisible(false);
            m_angleTextItem->setVisible(false);
        }
        else if (m_angleClickCount == 1)
        {
            m_angleVertexPixel = pixelPos;
            m_angleVertexScene = scenePos;
            m_angleClickCount = 2;
            m_angleFirstLineItem->setLine(QLineF(m_angleVertexScene, m_angleStartScene));
            m_angleSecondLineItem->setLine(QLineF(m_angleVertexScene, m_angleVertexScene));
            m_angleSecondLineItem->setVisible(true);
        }
        else
        {
            emit angleMeasurementRequested(m_angleStartPixel, m_angleVertexPixel, pixelPos);
            m_angleClickCount = 0;
        }
        event->accept();
        return;
    }

    QGraphicsView::mousePressEvent(event);
}

void DicomGraphicsView::wheelEvent(QWheelEvent* event)
{
    if (event->angleDelta().y() == 0)
    {
        QGraphicsView::wheelEvent(event);
        return;
    }

    const int stepCount = event->angleDelta().y() > 0 ? -1 : 1;
    emit wheelSliceNavigationRequested(stepCount);
    event->accept();
}

void DicomGraphicsView::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);

    if (!m_image || !m_image->isValid() || m_scene->sceneRect().isEmpty())
    {
        return;
    }

    fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    m_zoomFactor = 1.0;
}

bool DicomGraphicsView::mapMouseToImage(const QPoint& viewPos, QPoint& pixelPos, QPointF& scenePos) const
{
    if (!m_pixmapItem || m_pixmapItem->pixmap().isNull())
    {
        return false;
    }

    scenePos = mapToScene(viewPos);
    const QPointF itemPos = m_pixmapItem->mapFromScene(scenePos);
    const QRectF imageRect = m_pixmapItem->boundingRect();
    if (!imageRect.contains(itemPos))
    {
        return false;
    }

    pixelPos.setX(std::clamp(static_cast<int>(itemPos.x()), 0, m_pixmapItem->pixmap().width() - 1));
    pixelPos.setY(std::clamp(static_cast<int>(itemPos.y()), 0, m_pixmapItem->pixmap().height() - 1));
    scenePos = m_pixmapItem->mapToScene(QPointF(pixelPos));
    return true;
}

void DicomGraphicsView::updatePixmap()
{
    if (!m_image || !m_image->isValid())
    {
        clearImage();
        return;
    }

    const QPixmap pixmap = m_image->pixmap();
    m_pixmapItem->setPixmap(QPixmap());
    m_pixmapItem->setPixmap(pixmap);
    m_scene->setSceneRect(pixmap.rect());
    clearMeasurementOverlays();
    fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    m_zoomFactor = 1.0;
    m_scene->update();
    viewport()->update();
}
