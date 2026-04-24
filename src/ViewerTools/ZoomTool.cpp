#include "ViewerTools/ZoomTool.h"

#include "ViewerTools/IViewerToolTarget.h"
#include "ViewerTools/ViewerInputEvent.h"

ZoomTool::ZoomTool(IViewerToolTarget& target)
    : m_target(target)
{
}

void ZoomTool::beginInteraction(const ViewerInputEvent& event)
{
    m_target.handleZoomInput(event);
}

void ZoomTool::updateInteraction(const ViewerInputEvent& event)
{
    m_target.handleZoomInput(event);
}

void ZoomTool::endInteraction(const ViewerInputEvent& event)
{
    m_target.handleZoomInput(event);
}
