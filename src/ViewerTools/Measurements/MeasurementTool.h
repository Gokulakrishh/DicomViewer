#pragma once

#include "ViewerTools/IViewerTool.h"
#include "ViewerTools/Measurements/MeasurementService.h"

#include <QtCore/Qt>

class ViewerInputEvent;

enum class MeasurementToolMode
{
    Distance,
    Polyline,
    Angle,
    RectangleRoi
};

class IMeasurementToolHost
{
public:
    virtual ~IMeasurementToolHost() = default;

    [[nodiscard]] virtual MeasurementPoint measurementPointForInput(const ViewerInputEvent& event) const = 0;
    virtual void onMeasurementToolUpdated() = 0;
};

class MeasurementTool final : public IViewerTool
{
public:
    MeasurementTool(MeasurementToolMode mode, MeasurementService& service, IMeasurementToolHost& host);

    void beginInteraction(const ViewerInputEvent& event) override;
    void updateInteraction(const ViewerInputEvent& event) override;
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
