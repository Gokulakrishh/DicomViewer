#include "ViewerTools/WindowLevelTool.h"

#include "ViewerTools/IViewerToolTarget.h"
#include "ViewerTools/ViewerInputEvent.h"

WindowLevelTool::WindowLevelTool(IViewerToolTarget& target)
    : m_target(target)
{
}

void WindowLevelTool::beginInteraction(const ViewerInputEvent& event)
{
    m_target.handleWindowLevelInput(event);
}

void WindowLevelTool::updateInteraction(const ViewerInputEvent& event)
{
    m_target.handleWindowLevelInput(event);
}

void WindowLevelTool::endInteraction(const ViewerInputEvent& event)
{
    m_target.handleWindowLevelInput(event);
}
