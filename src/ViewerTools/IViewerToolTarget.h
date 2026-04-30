#pragma once

struct ViewerInputEvent;

class IViewerToolTarget
{
public:
    virtual ~IViewerToolTarget() = default;

    virtual void handleCrosshairInput(const ViewerInputEvent& event) = 0;
    virtual void handleWindowLevelInput(const ViewerInputEvent& event) = 0;
    virtual void handleZoomInput(const ViewerInputEvent& event) = 0;
    virtual void handlePanInput(const ViewerInputEvent& event) = 0;
};
