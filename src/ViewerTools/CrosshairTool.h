#pragma once

#include "ViewerTools/IViewerTool.h"

class IViewerToolTarget;

/**
 * @brief Reusable crosshair interaction tool.
 *
 * Responsibilities:
 * - Forward generic input events to a viewer target.
 * - Keep crosshair behavior independent of VTK interaction styles.
 */
class CrosshairTool final : public IViewerTool
{
public:
    /** @brief Creates a crosshair tool bound to a target. */
    explicit CrosshairTool(IViewerToolTarget& target);

    /** @brief Begins crosshair interaction. */
    void beginInteraction(const ViewerInputEvent& event) override;
    /** @brief Updates crosshair interaction. */
    void updateInteraction(const ViewerInputEvent& event) override;
    /** @brief Ends crosshair interaction. */
    void endInteraction(const ViewerInputEvent& event) override;

private:
    IViewerToolTarget& m_target;
};
