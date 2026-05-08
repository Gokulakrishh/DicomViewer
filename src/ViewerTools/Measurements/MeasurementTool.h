#pragma once

#include "ViewerTools/IViewerTool.h"
#include "ViewerTools/Measurements/MeasurementService.h"

#include <QtCore/Qt>

class ViewerInputEvent;

/**
 * @brief Interactive measurement mode selected by the toolbar.
 */
enum class MeasurementToolMode
{
    Distance,
    Polyline,
    Angle,
    RectangleRoi
};

/**
 * @brief Viewer-side host interface for measurement tools.
 */
class IMeasurementToolHost
{
public:
    virtual ~IMeasurementToolHost() = default;

    /** @brief Converts viewer input into a measurement point. */
    [[nodiscard]] virtual MeasurementPoint measurementPointForInput(const ViewerInputEvent& event) const = 0;
    /** @brief Notifies the host that measurement state changed. */
    virtual void onMeasurementToolUpdated() = 0;
};

/**
 * @brief Reusable interaction tool for distance, polyline, angle, and ROI measurements.
 *
 * Responsibilities:
 * - Translate generic input phases into MeasurementService operations.
 * - Notify the host so overlays/persistence can refresh.
 */
class MeasurementTool final : public IViewerTool
{
public:
    /** @brief Creates a measurement tool for one mode. */
    MeasurementTool(MeasurementToolMode mode, MeasurementService& service, IMeasurementToolHost& host);

    /** @brief Begins measurement interaction. */
    void beginInteraction(const ViewerInputEvent& event) override;
    /** @brief Updates measurement interaction. */
    void updateInteraction(const ViewerInputEvent& event) override;
    /** @brief Ends measurement interaction. */
    void endInteraction(const ViewerInputEvent& event) override;

private:
    void handleDistancePress(const MeasurementPoint& point);
    void handlePolylinePress(Qt::MouseButton button, const MeasurementPoint& point);
    void handleAnglePress(Qt::MouseButton button, const MeasurementPoint& point);
    void handleRectangleRoiPress(Qt::MouseButton button, const MeasurementPoint& point);

private:
    MeasurementToolMode m_mode;
    MeasurementService& m_service;
    IMeasurementToolHost& m_host;
    bool m_dragActive{false};
};
