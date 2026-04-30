#pragma once

#include <QColor>
#include <QPointF>
#include <QString>
#include <QVector>

enum class MeasurementType
{
    Distance,
    Polyline,
    Angle,
    RectangleRoi
};

struct MeasurementPoint
{
    double x{0.0};
    double y{0.0};
    double z{0.0};
};

struct MeasurementAnnotation
{
    QString id;
    MeasurementType type{MeasurementType::Distance};
    QVector<MeasurementPoint> points;
    QColor color{Qt::yellow};
    double lengthMm{0.0};
};

struct RoiStatistics
{
    bool valid{false};
    int sampleCount{0};
    double mean{0.0};
    double standardDeviation{0.0};
    double minimum{0.0};
    double maximum{0.0};
    double areaMm2{0.0};
};

struct DisplayMeasurement
{
    MeasurementType type{MeasurementType::Distance};
    QVector<QPointF> points;
    QColor color{Qt::yellow};
    QString label;
    bool preview{false};
    bool closedShape{false};
    bool filled{false};
    QPointF labelAnchor;
};
