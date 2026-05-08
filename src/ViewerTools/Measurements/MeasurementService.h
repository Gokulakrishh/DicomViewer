#pragma once

#include "ViewerTools/Measurements/MeasurementTypes.h"

#include <QString>
#include <optional>

/**
 * @brief In-memory state machine for slice measurement creation.
 *
 * Responsibilities:
 * - Enforce measurement creation flow for distance, polyline, angle, and ROI.
 * - Limit active measurements per series/slice workflow.
 * - Provide derived lengths, angles, and preview annotations.
 *
 * Assumptions:
 * - Persistence is handled outside this service.
 */
class MeasurementService
{
public:
    static constexpr int kMaxMeasurementsPerSeries = 50;

    /** @brief Returns committed measurements. */
    [[nodiscard]] const QVector<MeasurementAnnotation>& measurements() const;
    /** @brief Returns the active in-progress measurement. */
    [[nodiscard]] std::optional<MeasurementAnnotation> activeMeasurement() const;
    /** @brief Reports whether a new measurement can be created. */
    [[nodiscard]] bool canCreateMeasurement() const;
    /** @brief Returns current committed measurement count. */
    [[nodiscard]] int measurementCount() const;
    /** @brief Computes angle in degrees from measurement points. */
    [[nodiscard]] static double angleDegrees(const QVector<MeasurementPoint>& points);
    /** @brief Formats a length value for display. */
    [[nodiscard]] static QString formattedLength(double lengthMm);
    /** @brief Formats an angle value for display. */
    [[nodiscard]] static QString formattedAngle(double angleDegrees);
    /** @brief Formats an area value for display. */
    [[nodiscard]] static QString formattedArea(double areaMm2);
    /** @brief Builds a displayable preview annotation. */
    [[nodiscard]] static MeasurementAnnotation previewAnnotation(
        const MeasurementAnnotation& annotation,
        const std::optional<MeasurementPoint>& previewPoint);

    /** @brief Clears committed and active measurements. */
    void clear();
    /** @brief Replaces committed measurements. */
    void setMeasurements(const QVector<MeasurementAnnotation>& measurements);
    /** @brief Cancels the active in-progress measurement. */
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
