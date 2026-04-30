#pragma once

#include <cstdint>
#include <memory>

#include "ViewerTools/Measurements/MeasurementTypes.h"

#include <QPointF>
#include <QSize>
#include <vtkSmartPointer.h>

class DicomImage;
class QImage;
class QVTKOpenGLNativeWidget;
class vtkGenericOpenGLRenderWindow;
class vtkImageData;
class vtkImageViewer2;
class vtkInteractorStyleUser;

class VtkSliceSceneAdapter
{
public:
    VtkSliceSceneAdapter();
    ~VtkSliceSceneAdapter();

    void attach(QVTKOpenGLNativeWidget& widget);
    void clear();
    void setDicomImage(const DicomImage& image, int windowLevel, int windowWidth, bool resetCamera);
    void setQImage(const QImage& image, bool resetCamera);
    void fitToView();
    void applyZoomDelta(int delta);
    void pan(const QPointF& displayDelta, const QSize& widgetSize);
    [[nodiscard]] MeasurementPoint measurementPointFromDisplayPosition(
        const QPointF& widgetPosition,
        const QSize& widgetSize) const;
    [[nodiscard]] QPointF displayPositionForMeasurementPoint(
        const MeasurementPoint& point,
        const QSize& widgetSize) const;
    [[nodiscard]] QPointF displayPositionForImageIndex(const QPointF& imageIndex, const QSize& widgetSize) const;
    [[nodiscard]] QPointF imageIndexForMeasurementPoint(const MeasurementPoint& point) const;
    [[nodiscard]] QSize imageSize() const;
    int zoomPercent() const;
    std::int64_t currentImageByteCount() const;

private:
    vtkSmartPointer<vtkImageData> createEmptyImageData() const;
    vtkSmartPointer<vtkImageData> createDicomImageData(const DicomImage& image) const;
    vtkSmartPointer<vtkImageData> createQImageData(const QImage& image) const;
    void applyImageData(vtkImageData& imageData, bool resetCamera, int windowLevel, int windowWidth);
    void updateMeasurementGeometry(const DicomImage* image);

    struct MeasurementGeometry
    {
        bool hasPatientGeometry{false};
        int width{1};
        int height{1};
        double origin[3]{0.0, 0.0, 0.0};
        double rowDirection[3]{1.0, 0.0, 0.0};
        double columnDirection[3]{0.0, 1.0, 0.0};
        double pixelSpacingX{1.0};
        double pixelSpacingY{1.0};
    };

    [[nodiscard]] QPointF imageIndexFromDisplayPosition(const QPointF& widgetPosition, const QSize& widgetSize) const;
    [[nodiscard]] QPointF displayPositionFromImageIndex(const QPointF& imageIndex, const QSize& widgetSize) const;
    [[nodiscard]] MeasurementPoint pointFromImageIndex(const QPointF& imageIndex) const;
    [[nodiscard]] QPointF imageIndexFromPoint(const MeasurementPoint& point) const;

private:
    vtkSmartPointer<vtkImageViewer2> m_imageViewer;
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
    vtkSmartPointer<vtkInteractorStyleUser> m_interactorStyle;
    vtkSmartPointer<vtkImageData> m_currentImageData;
    MeasurementGeometry m_measurementGeometry;
    bool m_isAttached{false};
    bool m_hasImage{false};
};
