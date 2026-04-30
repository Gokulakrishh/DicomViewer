#pragma once

#include "ViewerTools/Measurements/MeasurementTypes.h"

#include <QString>
#include <optional>

class MeasurementService
{
public:
    static constexpr int kMaxMeasurementsPerSeries = 50;

    [[nodiscard]] const QVector<MeasurementAnnotation>& measurements() const;
    [[nodiscard]] std::optional<MeasurementAnnotation> activeMeasurement() const;
    [[nodiscard]] bool canCreateMeasurement() const;
    [[nodiscard]] int measurementCount() const;
    [[nodiscard]] static double angleDegrees(const QVector<MeasurementPoint>& points);
    [[nodiscard]] static QString formattedLength(double lengthMm);
    [[nodiscard]] static QString formattedAngle(double angleDegrees);
    [[nodiscard]] static QString formattedArea(double areaMm2);
    [[nodiscard]] static MeasurementAnnotation previewAnnotation(
        const MeasurementAnnotation& annotation,
        const std::optional<MeasurementPoint>& previewPoint);

    void clear();
    void setMeasurements(const QVector<MeasurementAnnotation>& measurements);
    void cancelActiveMeasurement();

    bool beginDistance(const MeasurementPoint& point);
    void updateDistancePreview(const MeasurementPoint& point);
    bool completeDistance(const MeasurementPoint& point);

    bool beginPolyline(const MeasurementPoint& point);
    bool appendPolylinePoint(const MeasurementPoint& point);
    void updatePolylinePreview(const MeasurementPoint& point);
    bool completePolyline();

    bool beginAngle(const MeasurementPoint& point);
    bool appendAnglePoint(const MeasurementPoint& point);
    void updateAnglePreview(const MeasurementPoint& point);

    bool beginRectangleRoi(const MeasurementPoint& point);
    void updateRectangleRoiPreview(const MeasurementPoint& point);
    bool completeRectangleRoi(const MeasurementPoint& point);

private:
    [[nodiscard]] QColor nextColor() const;
    [[nodiscard]] QString nextId() const;
    static double totalLengthMm(const QVector<MeasurementPoint>& points);
    static double segmentLengthMm(const MeasurementPoint& first, const MeasurementPoint& second);
    bool commitActiveMeasurement();

private:
    QVector<MeasurementAnnotation> m_measurements;
    std::optional<MeasurementAnnotation> m_activeMeasurement;
    std::optional<MeasurementPoint> m_previewPoint;
};
