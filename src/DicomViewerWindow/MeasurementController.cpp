#include "MeasurementController.h"

#include "Model/DicomImage.h"
#include "Model/DicomParameters.h"

#include <QChar>
#include <algorithm>
#include <cmath>

DicomGraphicsView::ToolMode MeasurementController::toolModeForIndex(int index)
{
    switch (index)
    {
    case 1:
        return DicomGraphicsView::ToolMode::Distance;
    case 2:
        return DicomGraphicsView::ToolMode::PixelProbe;
    case 3:
        return DicomGraphicsView::ToolMode::Angle;
    default:
        return DicomGraphicsView::ToolMode::Pan;
    }
}

MeasurementController::DistanceResult MeasurementController::createDistanceResult(
    const DicomImage& image,
    const QPoint& startPixel,
    const QPoint& endPixel) const
{
    const double deltaX = static_cast<double>(endPixel.x() - startPixel.x());
    const double deltaY = static_cast<double>(endPixel.y() - startPixel.y());

    QString label;
    if (image.hasPixelSpacing())
    {
        const double distanceMm = std::hypot(deltaX * image.pixelSpacingX(), deltaY * image.pixelSpacingY());
        label = QString::number(distanceMm, 'f', 2) + " mm";
    }
    else
    {
        const double distancePx = std::hypot(deltaX, deltaY);
        label = QString::number(distancePx, 'f', 1) + " px";
    }

    return {QPointF(startPixel), QPointF(endPixel), label};
}

MeasurementController::ProbeResult MeasurementController::createProbeResult(
    const DicomImage& image,
    const Series* series,
    const QPoint& pixelPos) const
{
    const int pixelValue = image.rawPixelValueAt(pixelPos.x(), pixelPos.y());
    const QString modality = series ? series->modality().trimmed().toUpper() : QString();
    const QString valueUnit = modality == "CT" ? "HU" : "Intensity";
    const QString label =
        QString("(%1, %2)  %3 %4").arg(pixelPos.x()).arg(pixelPos.y()).arg(pixelValue).arg(valueUnit);

    return {QPointF(pixelPos), label};
}

MeasurementController::AngleResult MeasurementController::createAngleResult(
    const QPoint& startPixel,
    const QPoint& vertexPixel,
    const QPoint& endPixel) const
{
    const QPointF firstVector = QPointF(startPixel - vertexPixel);
    const QPointF secondVector = QPointF(endPixel - vertexPixel);
    const double firstLength = std::hypot(firstVector.x(), firstVector.y());
    const double secondLength = std::hypot(secondVector.x(), secondVector.y());

    QString label = "0.0" + QString(QChar(0x00B0));
    if (firstLength > 0.0 && secondLength > 0.0)
    {
        const double dotProduct = (firstVector.x() * secondVector.x()) + (firstVector.y() * secondVector.y());
        const double cosineValue = std::clamp(dotProduct / (firstLength * secondLength), -1.0, 1.0);
        const double angleDegrees = std::acos(cosineValue) * (180.0 / M_PI);
        label = QString::number(angleDegrees, 'f', 1) + QChar(0x00B0);
    }

    return {QPointF(startPixel), QPointF(vertexPixel), QPointF(endPixel), label};
}
