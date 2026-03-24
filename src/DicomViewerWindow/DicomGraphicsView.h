#pragma once

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QWheelEvent>
#include <memory>

class MedicalImage;

class DicomGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit DicomGraphicsView(QWidget* parent = nullptr);
    ~DicomGraphicsView() override = default;

    void setImage(std::shared_ptr<MedicalImage> image);
    void clearImage();

protected:
    void wheelEvent(QWheelEvent* event) override;

private:
    void updatePixmap();

private:
    QGraphicsScene* m_scene{nullptr};
    QGraphicsPixmapItem* m_pixmapItem{nullptr};
    std::shared_ptr<MedicalImage> m_image;
    double m_zoomFactor{1.0};
};
