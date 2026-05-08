#pragma once

#include "ViewerTools/IViewerTool.h"

class IViewerToolTarget;

/**
 * @brief Reusable image pan interaction tool.
 *
 * Responsibilities:
 * - Forward pan gestures to the active viewer target.
 * - Keep pan behavior available to main and MPR viewers through one interface.
 */
class PanTool final : public IViewerTool
{
public:
    /** @brief Creates a pan tool bound to a target. */
    explicit PanTool(IViewerToolTarget& target);

    /** @brief Begins pan interaction. */
    void beginInteraction(const ViewerInputEvent& event) override;
    /** @brief Updates pan interaction. */
    void updateInteraction(const ViewerInputEvent& event) override;
    /** @brief Ends pan interaction. */
    void endInteraction(const ViewerInputEvent& event) override;

private:
    IViewerToolTarget& m_target;
};
