#pragma once

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QWheelEvent>
#include <QImage>
#include <QPixmap>
#include <memory>


class DicomImage;

class DicomGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit DicomGraphicsView(QWidget* parent = nullptr);
    ~DicomGraphicsView() override = default;

    void setImage(const std::shared_ptr<DicomImage>& image);
    void clearImage();

protected:
    void wheelEvent(QWheelEvent* event) override;
private:
    void updatePixmap();
private:
    QGraphicsScene* m_scene{nullptr};
    QGraphicsPixmapItem* m_pixmapItem{nullptr};
    std::shared_ptr<DicomImage> m_image;
    double m_zoomFactor{1.0};
};
