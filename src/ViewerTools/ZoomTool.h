#pragma once

#include "ViewerTools/IViewerTool.h"

class IViewerToolTarget;

/**
 * @brief Reusable image zoom interaction tool.
 */
class ZoomTool final : public IViewerTool
{
public:
    /** @brief Creates a zoom tool bound to a target. */
    explicit ZoomTool(IViewerToolTarget& target);

    /** @brief Begins zoom interaction. */
    void beginInteraction(const ViewerInputEvent& event) override;
    /** @brief Updates zoom interaction. */
    void updateInteraction(const ViewerInputEvent& event) override;
    /** @brief Ends zoom interaction. */
    void endInteraction(const ViewerInputEvent& event) override;

private:
    IViewerToolTarget& m_target;
};
