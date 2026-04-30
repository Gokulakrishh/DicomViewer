#include "VTK/MPR/Adapters/MprToolAdapter.h"

#include "VTK/MPR/Adapters/VtkMprSceneAdapter.h"
#include "VTK/MPR/Controllers/MprController.h"
#include "ViewerTools/ViewerInputEvent.h"

MprToolAdapter::MprToolAdapter(MprController& controller, VtkMprSceneAdapter& sceneAdapter)
    : m_controller(controller),
      m_sceneAdapter(sceneAdapter)
{
}

void MprToolAdapter::handleCrosshairInput(const ViewerInputEvent& event)
{
    m_controller.setCursorFromNormalizedPosition(event.plane, event.normalizedPosition);
}

void MprToolAdapter::handleWindowLevelInput(const ViewerInputEvent& event)
{
    if (event.phase == ViewerInputEvent::Phase::Update)
    {
        m_controller.adjustWindowLevelWidth(event.normalizedDelta);
    }
}

void MprToolAdapter::handleZoomInput(const ViewerInputEvent& event)
{
    if (event.phase == ViewerInputEvent::Phase::Update)
    {
        m_sceneAdapter.zoom(event.plane, event.normalizedDelta);
    }
}

void MprToolAdapter::handlePanInput(const ViewerInputEvent& event)
{
    if (event.phase == ViewerInputEvent::Phase::Update)
    {
        m_sceneAdapter.pan(event.plane, event.displayDelta, event.widgetSize);
    }
}
