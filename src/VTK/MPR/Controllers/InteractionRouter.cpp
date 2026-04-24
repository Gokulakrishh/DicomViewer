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
    m_toolController.beginInteraction({ViewerInputEvent::Phase::Begin, plane, button, position, {}});
}

void InteractionRouter::updateInteraction(MprSlicePlane plane, const QPointF& position, const QPointF& delta)
{
    m_toolController.updateInteraction({ViewerInputEvent::Phase::Update, plane, Qt::NoButton, position, delta});
}

void InteractionRouter::endInteraction(MprSlicePlane plane, Qt::MouseButton button, const QPointF& position)
{
    m_toolController.endInteraction({ViewerInputEvent::Phase::End, plane, button, position, {}});
}

void InteractionRouter::scrollSlices(MprSlicePlane plane, int steps)
{
    m_controller.incrementSlice(plane, steps);
}
