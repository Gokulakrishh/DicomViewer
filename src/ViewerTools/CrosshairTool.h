#pragma once

#include "ViewerTools/IViewerTool.h"

class IViewerToolTarget;

class CrosshairTool final : public IViewerTool
{
public:
    explicit CrosshairTool(IViewerToolTarget& target);

    void beginInteraction(const ViewerInputEvent& event) override;
    void updateInteraction(const ViewerInputEvent& event) override;
    void endInteraction(const ViewerInputEvent& event) override;

private:
    IViewerToolTarget& m_target;
};
