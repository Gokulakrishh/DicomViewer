#include "ViewerTools/Measurements/MeasurementTool.h"

#include "ViewerTools/ViewerInputEvent.h"

MeasurementTool::MeasurementTool(MeasurementToolMode mode, MeasurementService& service, IMeasurementToolHost& host)
    : m_mode(mode),
      m_service(service),
      m_host(host)
{
}

void MeasurementTool::beginInteraction(const ViewerInputEvent& event)
{
    if (event.eventType == ViewerInputEvent::EventType::MouseDoubleClick)
    {
        if (m_mode == MeasurementToolMode::Polyline)
        {
            m_service.completePolyline();
            m_host.onMeasurementToolUpdated();
        }
        return;
    }

    if (event.eventType != ViewerInputEvent::EventType::MousePress)
    {
        return;
    }

    const MeasurementPoint point = m_host.measurementPointForInput(event);
    switch (m_mode)
    {
    case MeasurementToolMode::Distance:
        handleDistancePress(point);
        break;
    case MeasurementToolMode::Polyline:
        handlePolylinePress(event.button, point);
        break;
    case MeasurementToolMode::Angle:
        handleAnglePress(event.button, point);
        break;
    case MeasurementToolMode::RectangleRoi:
        handleRectangleRoiPress(event.button, point);
        break;
    }
    m_host.onMeasurementToolUpdated();
}

void MeasurementTool::updateInteraction(const ViewerInputEvent& event)
{
    if (event.eventType != ViewerInputEvent::EventType::MouseMove)
    {
        return;
    }

    const MeasurementPoint point = m_host.measurementPointForInput(event);
    switch (m_mode)
    {
    case MeasurementToolMode::Distance:
        m_service.updateDistancePreview(point);
        break;
    case MeasurementToolMode::Polyline:
        m_service.updatePolylinePreview(point);
        break;
    case MeasurementToolMode::Angle:
        m_service.updateAnglePreview(point);
        break;
    case MeasurementToolMode::RectangleRoi:
        if (m_dragActive)
        {
            m_service.updateRectangleRoiPreview(point);
        }
        break;
    }
    m_host.onMeasurementToolUpdated();
}

void MeasurementTool::endInteraction(const ViewerInputEvent& event)
{
    if (event.eventType != ViewerInputEvent::EventType::MouseRelease)
    {
        return;
    }

    if (m_mode == MeasurementToolMode::RectangleRoi && m_dragActive)
    {
        m_dragActive = false;
        if (event.button == Qt::LeftButton)
        {
            m_service.completeRectangleRoi(m_host.measurementPointForInput(event));
        }
        else if (event.button == Qt::RightButton)
        {
            m_service.cancelActiveMeasurement();
        }
        m_host.onMeasurementToolUpdated();
    }
}

void MeasurementTool::handleDistancePress(const MeasurementPoint& point)
{
    if (const auto activeMeasurement = m_service.activeMeasurement();
        activeMeasurement && activeMeasurement->type == MeasurementType::Distance)
    {
        m_service.completeDistance(point);
        return;
    }

    m_service.beginDistance(point);
}

void MeasurementTool::handlePolylinePress(Qt::MouseButton button, const MeasurementPoint& point)
{
    if (button == Qt::RightButton)
    {
        m_service.completePolyline();
        return;
    }
    if (button == Qt::LeftButton)
    {
        m_service.appendPolylinePoint(point);
    }
}

void MeasurementTool::handleAnglePress(Qt::MouseButton button, const MeasurementPoint& point)
{
    if (button == Qt::RightButton)
    {
        m_service.cancelActiveMeasurement();
        return;
    }
    if (button == Qt::LeftButton)
    {
        m_service.appendAnglePoint(point);
    }
}

void MeasurementTool::handleRectangleRoiPress(Qt::MouseButton button, const MeasurementPoint& point)
{
    if (button == Qt::RightButton)
    {
        m_dragActive = false;
        m_service.cancelActiveMeasurement();
        return;
    }
    if (button == Qt::LeftButton)
    {
        m_dragActive = m_service.beginRectangleRoi(point);
    }
}
