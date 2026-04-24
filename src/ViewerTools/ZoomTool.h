#pragma once

#include "ViewerTools/IViewerTool.h"

class IViewerToolTarget;

class ZoomTool final : public IViewerTool
{
public:
    explicit ZoomTool(IViewerToolTarget& target);

    void beginInteraction(const ViewerInputEvent& event) override;
    void updateInteraction(const ViewerInputEvent& event) override;
    void endInteraction(const ViewerInputEvent& event) override;

private:
    IViewerToolTarget& m_target;
};
