#pragma once

struct ViewerInputEvent;

/**
 * @brief Generic interaction tool interface for image viewers.
 *
 * Responsibilities:
 * - Express mouse interaction phases independent of VTK/Qt view internals.
 * - Allow the same tool concept to be used by main and MPR viewers.
 */
class IViewerTool
{
public:
    virtual ~IViewerTool() = default;

    /** @brief Handles the start of an interaction. */
    virtual void beginInteraction(const ViewerInputEvent& event) = 0;
    /** @brief Handles movement/update during an interaction. */
    virtual void updateInteraction(const ViewerInputEvent& event) = 0;
    /** @brief Handles completion/cancel of an interaction. */
    virtual void endInteraction(const ViewerInputEvent& event) = 0;
};
