#pragma once

#include "ViewerTools/IViewerTool.h"

class IViewerToolTarget;

/**
 * @brief Reusable window-level interaction tool.
 */
class WindowLevelTool final : public IViewerTool
{
public:
    /** @brief Creates a WL/WW tool bound to a target. */
    explicit WindowLevelTool(IViewerToolTarget& target);

    /** @brief Begins WL/WW interaction. */
    void beginInteraction(const ViewerInputEvent& event) override;
    /** @brief Updates WL/WW interaction. */
    void updateInteraction(const ViewerInputEvent& event) override;
    /** @brief Ends WL/WW interaction. */
    void endInteraction(const ViewerInputEvent& event) override;

private:
    IViewerToolTarget& m_target;
};
