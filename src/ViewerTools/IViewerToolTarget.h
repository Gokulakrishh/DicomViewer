#pragma once

struct ViewerInputEvent;

/**
 * @brief Viewer-side target interface for reusable interaction tools.
 *
 * Responsibilities:
 * - Receive generic tool input events.
 * - Map tool actions to concrete viewer behavior such as crosshair, WL/WW, zoom,
 *   and pan.
 */
class IViewerToolTarget
{
public:
    virtual ~IViewerToolTarget() = default;

    /** @brief Handles crosshair placement/movement input. */
    virtual void handleCrosshairInput(const ViewerInputEvent& event) = 0;
    /** @brief Handles window-level input. */
    virtual void handleWindowLevelInput(const ViewerInputEvent& event) = 0;
    /** @brief Handles zoom input. */
    virtual void handleZoomInput(const ViewerInputEvent& event) = 0;
    /** @brief Handles pan input. */
    virtual void handlePanInput(const ViewerInputEvent& event) = 0;
};
