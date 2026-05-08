#pragma once

#include <QColor>
#include <QPointF>
#include <QString>
#include <QVector>

/**
 * @brief Supported annotation/measurement geometry types.
 */
enum class MeasurementType
{
    Distance,
    Polyline,
    Angle,
    RectangleRoi
};

/**
 * @brief Measurement point in patient/world coordinates.
 */
struct MeasurementPoint
{
    double x{0.0};
    double y{0.0};
    double z{0.0};
};

/**
 * @brief In-memory measurement annotation geometry and derived length.
 *
 * Assumptions:
 * - Persistence metadata such as body region is stored in
 *   SliceMeasurementAnnotationRecord.
 */
struct MeasurementAnnotation
{
    QString id;
    MeasurementType type{MeasurementType::Distance};
    QVector<MeasurementPoint> points;
    QColor color{Qt::yellow};
    double lengthMm{0.0};
};

/**
 * @brief Derived statistics for a rectangular ROI.
 */
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

/**
 * @brief Display-space measurement prepared for overlay rendering.
 */
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
