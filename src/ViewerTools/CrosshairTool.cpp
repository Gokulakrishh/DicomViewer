#include "ViewerTools/CrosshairTool.h"

#include "ViewerTools/IViewerToolTarget.h"
#include "ViewerTools/ViewerInputEvent.h"

CrosshairTool::CrosshairTool(IViewerToolTarget& target)
    : m_target(target)
{
}

void CrosshairTool::beginInteraction(const ViewerInputEvent& event)
{
    m_target.handleCrosshairInput(event);
}

void CrosshairTool::updateInteraction(const ViewerInputEvent& event)
{
    m_target.handleCrosshairInput(event);
}

void CrosshairTool::endInteraction(const ViewerInputEvent& event)
{
    m_target.handleCrosshairInput(event);
}
