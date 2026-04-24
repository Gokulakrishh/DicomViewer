#pragma once

struct ViewerInputEvent;

class IViewerTool
{
public:
    virtual ~IViewerTool() = default;

    virtual void beginInteraction(const ViewerInputEvent& event) = 0;
    virtual void updateInteraction(const ViewerInputEvent& event) = 0;
    virtual void endInteraction(const ViewerInputEvent& event) = 0;
};
