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

namespace
{
QSize rendererDisplaySize(vtkRenderer& renderer, const QSize& fallbackSize)
{
    const int* size = renderer.GetSize();
    if (!size || size[0] <= 0 || size[1] <= 0)
    {
        return fallbackSize;
    }

    return {size[0], size[1]};
}
}

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
    updateMeasurementGeometry(nullptr);
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
    updateMeasurementGeometry(&image);
    applyImageData(*m_currentImageData, resetCamera, windowLevel, windowWidth);
    m_hasImage = true;
}

void VtkSliceSceneAdapter::setQImage(const QImage& image, bool resetCamera)
{
    m_currentImageData = createQImageData(image);
    updateMeasurementGeometry(nullptr);
    m_measurementGeometry.width = std::max(1, image.width());
    m_measurementGeometry.height = std::max(1, image.height());
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

void VtkSliceSceneAdapter::pan(const QPointF& displayDelta, const QSize& widgetSize)
{
    auto* renderer = m_imageViewer->GetRenderer();
    auto* camera = renderer ? renderer->GetActiveCamera() : nullptr;
    if (!renderer || !camera || !m_hasImage || widgetSize.width() <= 0 || widgetSize.height() <= 0)
    {
        return;
    }

    const QSize displaySize = rendererDisplaySize(*renderer, widgetSize);

    renderer->SetWorldPoint(
        camera->GetFocalPoint()[0],
        camera->GetFocalPoint()[1],
        camera->GetFocalPoint()[2],
        1.0);
    renderer->WorldToDisplay();

    double focalDisplay[3];
    renderer->GetDisplayPoint(focalDisplay);

    const double dx = displayDelta.x() * static_cast<double>(displaySize.width()) /
        static_cast<double>(widgetSize.width());
    const double dy = displayDelta.y() * static_cast<double>(displaySize.height()) /
        static_cast<double>(widgetSize.height());

    renderer->SetDisplayPoint(focalDisplay[0], focalDisplay[1], focalDisplay[2]);
    renderer->DisplayToWorld();
    double beforeWorld[4];
    renderer->GetWorldPoint(beforeWorld);

    renderer->SetDisplayPoint(focalDisplay[0] - dx, focalDisplay[1] + dy, focalDisplay[2]);
    renderer->DisplayToWorld();
    double afterWorld[4];
    renderer->GetWorldPoint(afterWorld);

    if (std::abs(beforeWorld[3]) <= 1e-12 || std::abs(afterWorld[3]) <= 1e-12)
    {
        return;
    }

    const double worldDelta[3] = {
        afterWorld[0] / afterWorld[3] - beforeWorld[0] / beforeWorld[3],
        afterWorld[1] / afterWorld[3] - beforeWorld[1] / beforeWorld[3],
        afterWorld[2] / afterWorld[3] - beforeWorld[2] / beforeWorld[3]};

    const double* focalPoint = camera->GetFocalPoint();
    const double* position = camera->GetPosition();
    camera->SetFocalPoint(
        focalPoint[0] + worldDelta[0],
        focalPoint[1] + worldDelta[1],
        focalPoint[2] + worldDelta[2]);
    camera->SetPosition(
        position[0] + worldDelta[0],
        position[1] + worldDelta[1],
        position[2] + worldDelta[2]);
    renderer->ResetCameraClippingRange();
    m_imageViewer->Render();
}

MeasurementPoint VtkSliceSceneAdapter::measurementPointFromDisplayPosition(
    const QPointF& widgetPosition,
    const QSize& widgetSize) const
{
    return pointFromImageIndex(imageIndexFromDisplayPosition(widgetPosition, widgetSize));
}

QPointF VtkSliceSceneAdapter::displayPositionForMeasurementPoint(
    const MeasurementPoint& point,
    const QSize& widgetSize) const
{
    return displayPositionFromImageIndex(imageIndexFromPoint(point), widgetSize);
}

QPointF VtkSliceSceneAdapter::displayPositionForImageIndex(const QPointF& imageIndex, const QSize& widgetSize) const
{
    return displayPositionFromImageIndex(imageIndex, widgetSize);
}

QPointF VtkSliceSceneAdapter::imageIndexForMeasurementPoint(const MeasurementPoint& point) const
{
    return imageIndexFromPoint(point);
}

QSize VtkSliceSceneAdapter::imageSize() const
{
    return {m_measurementGeometry.width, m_measurementGeometry.height};
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

void VtkSliceSceneAdapter::updateMeasurementGeometry(const DicomImage* image)
{
    m_measurementGeometry = MeasurementGeometry{};
    if (!image)
    {
        return;
    }

    m_measurementGeometry.width = std::max(1, image->width());
    m_measurementGeometry.height = std::max(1, image->height());
    m_measurementGeometry.pixelSpacingX = image->hasPixelSpacing() ? image->pixelSpacingX() : 1.0;
    m_measurementGeometry.pixelSpacingY = image->hasPixelSpacing() ? image->pixelSpacingY() : 1.0;

    if (image->hasImagePositionPatient() && image->hasImageOrientationPatient())
    {
        m_measurementGeometry.hasPatientGeometry = true;
        const auto& origin = image->imagePositionPatient();
        const auto& orientation = image->imageOrientationPatient();
        for (int index = 0; index < 3; ++index)
        {
            m_measurementGeometry.origin[index] = origin[index];
            m_measurementGeometry.rowDirection[index] = orientation[index];
            m_measurementGeometry.columnDirection[index] = orientation[index + 3];
        }
    }
}

QPointF VtkSliceSceneAdapter::imageIndexFromDisplayPosition(const QPointF& widgetPosition, const QSize& widgetSize) const
{
    if (!m_hasImage || !m_currentImageData || widgetSize.width() <= 0 || widgetSize.height() <= 0)
    {
        return {};
    }

    auto* renderer = m_imageViewer->GetRenderer();
    if (!renderer)
    {
        return {};
    }

    const int* displaySize = renderer->GetSize();
    const double width = displaySize && displaySize[0] > 0 ? displaySize[0] : widgetSize.width();
    const double height = displaySize && displaySize[1] > 0 ? displaySize[1] : widgetSize.height();

    renderer->SetWorldPoint(0.0, 0.0, 0.0, 1.0);
    renderer->WorldToDisplay();
    double baseDisplay[3];
    renderer->GetDisplayPoint(baseDisplay);

    renderer->SetDisplayPoint(
        widgetPosition.x() * width / static_cast<double>(widgetSize.width()),
        height - widgetPosition.y() * height / static_cast<double>(widgetSize.height()),
        baseDisplay[2]);
    renderer->DisplayToWorld();

    double worldPoint[4];
    renderer->GetWorldPoint(worldPoint);
    if (std::abs(worldPoint[3]) <= 1e-12)
    {
        return {};
    }

    double continuousIndex[3];
    m_currentImageData->TransformPhysicalPointToContinuousIndex(
        worldPoint[0] / worldPoint[3],
        worldPoint[1] / worldPoint[3],
        worldPoint[2] / worldPoint[3],
        continuousIndex);

    return {
        std::clamp(continuousIndex[0], 0.0, static_cast<double>(m_measurementGeometry.width - 1)),
        std::clamp(static_cast<double>(m_measurementGeometry.height - 1) - continuousIndex[1],
                   0.0,
                   static_cast<double>(m_measurementGeometry.height - 1))};
}

QPointF VtkSliceSceneAdapter::displayPositionFromImageIndex(const QPointF& imageIndex, const QSize& widgetSize) const
{
    if (!m_hasImage || !m_currentImageData || widgetSize.width() <= 0 || widgetSize.height() <= 0)
    {
        return {};
    }

    auto* renderer = m_imageViewer->GetRenderer();
    if (!renderer)
    {
        return {};
    }

    const int* displaySize = renderer->GetSize();
    const double width = displaySize && displaySize[0] > 0 ? displaySize[0] : widgetSize.width();
    const double height = displaySize && displaySize[1] > 0 ? displaySize[1] : widgetSize.height();

    renderer->SetWorldPoint(
        imageIndex.x(),
        static_cast<double>(m_measurementGeometry.height - 1) - imageIndex.y(),
        0.0,
        1.0);
    renderer->WorldToDisplay();

    double displayPoint[3];
    renderer->GetDisplayPoint(displayPoint);
    return {
        displayPoint[0] * static_cast<double>(widgetSize.width()) / width,
        (height - displayPoint[1]) * static_cast<double>(widgetSize.height()) / height};
}

MeasurementPoint VtkSliceSceneAdapter::pointFromImageIndex(const QPointF& imageIndex) const
{
    if (!m_measurementGeometry.hasPatientGeometry)
    {
        return {
            imageIndex.x() * m_measurementGeometry.pixelSpacingX,
            imageIndex.y() * m_measurementGeometry.pixelSpacingY,
            0.0};
    }

    MeasurementPoint point;
    for (int index = 0; index < 3; ++index)
    {
        point.x += index == 0 ? m_measurementGeometry.origin[index] : 0.0;
        point.y += index == 1 ? m_measurementGeometry.origin[index] : 0.0;
        point.z += index == 2 ? m_measurementGeometry.origin[index] : 0.0;
    }

    point.x = m_measurementGeometry.origin[0] +
        imageIndex.x() * m_measurementGeometry.pixelSpacingX * m_measurementGeometry.rowDirection[0] +
        imageIndex.y() * m_measurementGeometry.pixelSpacingY * m_measurementGeometry.columnDirection[0];
    point.y = m_measurementGeometry.origin[1] +
        imageIndex.x() * m_measurementGeometry.pixelSpacingX * m_measurementGeometry.rowDirection[1] +
        imageIndex.y() * m_measurementGeometry.pixelSpacingY * m_measurementGeometry.columnDirection[1];
    point.z = m_measurementGeometry.origin[2] +
        imageIndex.x() * m_measurementGeometry.pixelSpacingX * m_measurementGeometry.rowDirection[2] +
        imageIndex.y() * m_measurementGeometry.pixelSpacingY * m_measurementGeometry.columnDirection[2];
    return point;
}

QPointF VtkSliceSceneAdapter::imageIndexFromPoint(const MeasurementPoint& point) const
{
    if (!m_measurementGeometry.hasPatientGeometry)
    {
        return {
            point.x / std::max(m_measurementGeometry.pixelSpacingX, 1e-6),
            point.y / std::max(m_measurementGeometry.pixelSpacingY, 1e-6)};
    }

    const double delta[3] = {
        point.x - m_measurementGeometry.origin[0],
        point.y - m_measurementGeometry.origin[1],
        point.z - m_measurementGeometry.origin[2]};

    const double rowDistance =
        delta[0] * m_measurementGeometry.rowDirection[0] +
        delta[1] * m_measurementGeometry.rowDirection[1] +
        delta[2] * m_measurementGeometry.rowDirection[2];
    const double columnDistance =
        delta[0] * m_measurementGeometry.columnDirection[0] +
        delta[1] * m_measurementGeometry.columnDirection[1] +
        delta[2] * m_measurementGeometry.columnDirection[2];

    return {
        rowDistance / std::max(m_measurementGeometry.pixelSpacingX, 1e-6),
        columnDistance / std::max(m_measurementGeometry.pixelSpacingY, 1e-6)};
}
