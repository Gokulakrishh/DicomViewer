#include "VTK/MainView/VtkSliceSceneAdapter.h"

#include "Model/DicomImage.h"

#include <QImage>
#include <QVTKOpenGLNativeWidget.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageData.h>
#include <vtkImageViewer2.h>
#include <vtkInteractorStyleUser.h>
#include <vtkCamera.h>
#include <vtkDataArray.h>
#include <vtkPointData.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>

#include <algorithm>
#include <cmath>

VtkSliceSceneAdapter::VtkSliceSceneAdapter()
{
    m_imageViewer = vtkSmartPointer<vtkImageViewer2>::New();
    m_renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    m_imageViewer->SetRenderWindow(m_renderWindow);
    m_imageViewer->GetRenderer()->SetBackground(0.0, 0.0, 0.0);
    m_currentImageData = createEmptyImageData();
    m_imageViewer->SetInputData(m_currentImageData);
    m_imageViewer->SetColorLevel(127);
    m_imageViewer->SetColorWindow(255);
    m_imageViewer->SetSlice(0);
    m_hasImage = false;
}

VtkSliceSceneAdapter::~VtkSliceSceneAdapter() = default;

void VtkSliceSceneAdapter::attach(QVTKOpenGLNativeWidget& widget)
{
    widget.setRenderWindow(m_renderWindow);
    m_imageViewer->SetupInteractor(widget.renderWindow()->GetInteractor());
    m_interactorStyle = vtkSmartPointer<vtkInteractorStyleUser>::New();
    widget.renderWindow()->GetInteractor()->SetInteractorStyle(m_interactorStyle);
    m_isAttached = true;
    auto* renderer = m_imageViewer->GetRenderer();
    if (renderer)
    {
        renderer->ResetCamera();
        renderer->ResetCameraClippingRange();
    }
    m_renderWindow->Render();
}

void VtkSliceSceneAdapter::clear()
{
    m_currentImageData = createEmptyImageData();
    m_hasImage = false;
    auto* renderer = m_imageViewer->GetRenderer();
    if (renderer)
    {
        renderer->SetBackground(0.0, 0.0, 0.0);
    }
    applyImageData(*m_currentImageData, true, 127, 255);
}

void VtkSliceSceneAdapter::setDicomImage(const DicomImage& image, int windowLevel, int windowWidth, bool resetCamera)
{
    m_currentImageData = createDicomImageData(image);
    applyImageData(*m_currentImageData, resetCamera, windowLevel, windowWidth);
    m_hasImage = true;
}

void VtkSliceSceneAdapter::setQImage(const QImage& image, bool resetCamera)
{
    m_currentImageData = createQImageData(image);
    applyImageData(*m_currentImageData, resetCamera, 127, 255);
    m_hasImage = true;
}

void VtkSliceSceneAdapter::fitToView()
{
    if (!m_hasImage || !m_currentImageData)
    {
        return;
    }

    auto* renderer = m_imageViewer->GetRenderer();
    if (!renderer)
    {
        return;
    }

    renderer->ResetCamera();
    renderer->ResetCameraClippingRange();
    m_imageViewer->Render();
}

void VtkSliceSceneAdapter::applyZoomDelta(int delta)
{
    auto* renderer = m_imageViewer->GetRenderer();
    auto* camera = renderer ? renderer->GetActiveCamera() : nullptr;
    if (!camera || !m_hasImage || delta == 0)
    {
        return;
    }

    const double magnitude = 1.0 + (static_cast<double>(std::abs(delta)) * 0.01);
    const double factor = delta > 0 ? magnitude : (1.0 / magnitude);
    camera->Zoom(factor);
    renderer->ResetCameraClippingRange();
    m_imageViewer->Render();
}

int VtkSliceSceneAdapter::zoomPercent() const
{
    auto* renderer = m_imageViewer->GetRenderer();
    auto* camera = renderer ? renderer->GetActiveCamera() : nullptr;
    if (!camera || !m_hasImage || !m_currentImageData)
    {
        return 100;
    }

    int extent[6];
    m_currentImageData->GetExtent(extent);
    const double imageHeight = std::max(1, extent[3] - extent[2] + 1);
    const double parallelScale = std::max(1e-6, camera->GetParallelScale());
    return std::max(1, static_cast<int>(std::lround((imageHeight / (parallelScale * 2.0)) * 100.0)));
}

std::int64_t VtkSliceSceneAdapter::currentImageByteCount() const
{
    if (!m_currentImageData)
    {
        return 0;
    }

    auto* scalars = m_currentImageData->GetPointData() ? m_currentImageData->GetPointData()->GetScalars() : nullptr;
    if (!scalars)
    {
        return 0;
    }

    return static_cast<std::int64_t>(scalars->GetNumberOfValues()) * static_cast<std::int64_t>(scalars->GetDataTypeSize());
}

vtkSmartPointer<vtkImageData> VtkSliceSceneAdapter::createEmptyImageData() const
{
    auto imageData = vtkSmartPointer<vtkImageData>::New();
    imageData->SetDimensions(1, 1, 1);
    imageData->SetSpacing(1.0, 1.0, 1.0);
    imageData->SetOrigin(0.0, 0.0, 0.0);
    imageData->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
    auto* scalar = static_cast<unsigned char*>(imageData->GetScalarPointer(0, 0, 0));
    scalar[0] = 0;
    return imageData;
}

vtkSmartPointer<vtkImageData> VtkSliceSceneAdapter::createDicomImageData(const DicomImage& image) const
{
    auto imageData = vtkSmartPointer<vtkImageData>::New();
    imageData->SetDimensions(image.width(), image.height(), 1);
    imageData->SetSpacing(1.0, 1.0, 1.0);
    imageData->SetOrigin(0.0, 0.0, 0.0);
    imageData->AllocateScalars(VTK_SHORT, 1);

    const int minimumValue = image.minimumStoredValue();
    const int maximumValue = image.maximumStoredValue();

    for (int y = 0; y < image.height(); ++y)
    {
        for (int x = 0; x < image.width(); ++x)
        {
            int pixelValue = image.rawPixelValueAt(x, y);
            if (image.isMonochrome1())
            {
                pixelValue = minimumValue + maximumValue - pixelValue;
            }

            auto* scalar = static_cast<short*>(imageData->GetScalarPointer(x, image.height() - 1 - y, 0));
            scalar[0] = static_cast<short>(pixelValue);
        }
    }

    return imageData;
}

vtkSmartPointer<vtkImageData> VtkSliceSceneAdapter::createQImageData(const QImage& image) const
{
    const QImage normalizedImage = image.convertToFormat(QImage::Format_RGB888);

    auto imageData = vtkSmartPointer<vtkImageData>::New();
    imageData->SetDimensions(normalizedImage.width(), normalizedImage.height(), 1);
    imageData->SetSpacing(1.0, 1.0, 1.0);
    imageData->SetOrigin(0.0, 0.0, 0.0);
    imageData->AllocateScalars(VTK_UNSIGNED_CHAR, 3);

    for (int y = 0; y < normalizedImage.height(); ++y)
    {
        const uchar* scanLine = normalizedImage.constScanLine(y);
        for (int x = 0; x < normalizedImage.width(); ++x)
        {
            auto* scalar = static_cast<unsigned char*>(imageData->GetScalarPointer(x, normalizedImage.height() - 1 - y, 0));
            const int offset = x * 3;
            scalar[0] = scanLine[offset];
            scalar[1] = scanLine[offset + 1];
            scalar[2] = scanLine[offset + 2];
        }
    }

    return imageData;
}

void VtkSliceSceneAdapter::applyImageData(vtkImageData& imageData, bool resetCamera, int windowLevel, int windowWidth)
{
    m_imageViewer->SetInputData(&imageData);
    m_imageViewer->SetColorLevel(windowLevel);
    m_imageViewer->SetColorWindow(windowWidth);
    m_imageViewer->SetSlice(0);
    if (resetCamera)
    {
        m_imageViewer->GetRenderer()->ResetCamera();
        m_imageViewer->GetRenderer()->ResetCameraClippingRange();
    }
    if (m_isAttached)
    {
        m_imageViewer->Render();
    }
}
