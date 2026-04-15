#pragma once

#include "DicomGraphicsView.h"

#include <QPoint>
#include <QPointF>
#include <QString>

class DicomImage;
class Series;

class MeasurementController
{
public:
    struct DistanceResult
    {
        QPointF startScenePos;
        QPointF endScenePos;
        QString label;
    };

    struct ProbeResult
    {
        QPointF scenePos;
        QString label;
    };

    struct AngleResult
    {
        QPointF startScenePos;
        QPointF vertexScenePos;
        QPointF endScenePos;
        QString label;
    };

    static DicomGraphicsView::ToolMode toolModeForIndex(int index);
    static int toolIndexForMode(DicomGraphicsView::ToolMode toolMode);
    DistanceResult createDistanceResult(const DicomImage& image, const QPoint& startPixel, const QPoint& endPixel) const;
    ProbeResult createProbeResult(const DicomImage& image, const Series* series, const QPoint& pixelPos) const;
    AngleResult createAngleResult(const QPoint& startPixel, const QPoint& vertexPixel, const QPoint& endPixel) const;
};
