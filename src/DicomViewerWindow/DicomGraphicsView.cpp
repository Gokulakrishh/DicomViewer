#include "DicomGraphicsView.h"

#include "Model/MedicalImage.h"

#include <QPainter>

DicomGraphicsView::DicomGraphicsView(QWidget* parent)
    : QGraphicsView(parent)
{
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);

    m_pixmapItem = new QGraphicsPixmapItem();
    m_scene->addItem(m_pixmapItem);

    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
}

void DicomGraphicsView::setImage(std::shared_ptr<MedicalImage> image)
{
    m_image = std::move(image);
    updatePixmap();
}

void DicomGraphicsView::clearImage()
{
    m_image.reset();
    m_pixmapItem->setPixmap(QPixmap());
    resetTransform();
    m_zoomFactor = 1.0;
}

void DicomGraphicsView::wheelEvent(QWheelEvent* event)
{
    constexpr double scaleFactor = 1.15;

    if (event->angleDelta().y() > 0)
    {
        scale(scaleFactor, scaleFactor);
        m_zoomFactor *= scaleFactor;
    }
    else
    {
        scale(1.0 / scaleFactor, 1.0 / scaleFactor);
        m_zoomFactor /= scaleFactor;
    }
}

void DicomGraphicsView::updatePixmap()
{
    if (!m_image || !m_image->isValid())
    {
        clearImage();
        return;
    }

    const QPixmap pixmap = m_image->pixmap();
    m_pixmapItem->setPixmap(pixmap);
    m_scene->setSceneRect(pixmap.rect());
    fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    m_zoomFactor = 1.0;
}
