#include "ViewerTools/PanTool.h"

#include "ViewerTools/IViewerToolTarget.h"
#include "ViewerTools/ViewerInputEvent.h"

PanTool::PanTool(IViewerToolTarget& target)
    : m_target(target)
{
}

void PanTool::beginInteraction(const ViewerInputEvent& event)
{
    m_target.handlePanInput(event);
}

void PanTool::updateInteraction(const ViewerInputEvent& event)
{
    m_target.handlePanInput(event);
}

void PanTool::endInteraction(const ViewerInputEvent& event)
{
    m_target.handlePanInput(event);
}
