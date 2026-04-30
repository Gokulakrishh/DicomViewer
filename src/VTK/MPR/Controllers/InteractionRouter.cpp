#include "VTK/MPR/Controllers/InteractionRouter.h"

#include "VTK/MPR/Controllers/MprController.h"
#include "VTK/MPR/Controllers/ToolController.h"
#include "ViewerTools/ViewerInputEvent.h"

InteractionRouter::InteractionRouter(ToolController& toolController, MprController& controller)
    : m_toolController(toolController),
      m_controller(controller)
{
}

void InteractionRouter::beginInteraction(MprSlicePlane plane, Qt::MouseButton button, const QPointF& position)
{
    ViewerInputEvent event;
    event.eventType = ViewerInputEvent::EventType::MousePress;
    event.phase = ViewerInputEvent::Phase::Begin;
    event.plane = plane;
    event.button = button;
    event.normalizedPosition = position;
    m_toolController.beginInteraction(event);
}

void InteractionRouter::updateInteraction(MprSlicePlane plane, const QPointF& position, const QPointF& delta)
{
    ViewerInputEvent event;
    event.eventType = ViewerInputEvent::EventType::MouseMove;
    event.phase = ViewerInputEvent::Phase::Update;
    event.plane = plane;
    event.button = Qt::NoButton;
    event.normalizedPosition = position;
    event.normalizedDelta = delta;
    m_toolController.updateInteraction(event);
}

void InteractionRouter::endInteraction(MprSlicePlane plane, Qt::MouseButton button, const QPointF& position)
{
    ViewerInputEvent event;
    event.eventType = ViewerInputEvent::EventType::MouseRelease;
    event.phase = ViewerInputEvent::Phase::End;
    event.plane = plane;
    event.button = button;
    event.normalizedPosition = position;
    m_toolController.endInteraction(event);
}

void InteractionRouter::scrollSlices(MprSlicePlane plane, int steps)
{
    m_controller.incrementSlice(plane, steps);
}
