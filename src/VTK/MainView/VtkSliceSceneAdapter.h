#pragma once

#include <cstdint>
#include <memory>

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
    int zoomPercent() const;
    std::int64_t currentImageByteCount() const;

private:
    vtkSmartPointer<vtkImageData> createEmptyImageData() const;
    vtkSmartPointer<vtkImageData> createDicomImageData(const DicomImage& image) const;
    vtkSmartPointer<vtkImageData> createQImageData(const QImage& image) const;
    void applyImageData(vtkImageData& imageData, bool resetCamera, int windowLevel, int windowWidth);

private:
    vtkSmartPointer<vtkImageViewer2> m_imageViewer;
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
    vtkSmartPointer<vtkInteractorStyleUser> m_interactorStyle;
    vtkSmartPointer<vtkImageData> m_currentImageData;
    bool m_isAttached{false};
    bool m_hasImage{false};
};
