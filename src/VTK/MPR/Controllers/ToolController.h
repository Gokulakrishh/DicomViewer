#pragma once

#include "VTK/MPR/MprTypes.h"
#include "ViewerTools/IViewerTool.h"

#include <memory>

class MprController;
class MprScene;
class IViewerToolTarget;
class MeasurementService;
class IMeasurementToolHost;
struct ViewerInputEvent;

/**
 * @brief Owns and dispatches reusable tools for the MPR viewer.
 */
class ToolController
{
public:
    /** @brief Creates the MPR tool controller. */
    ToolController(
        MprScene& scene,
        MprController& controller,
        MeasurementService& measurementService,
        IMeasurementToolHost& measurementToolHost,
        IViewerToolTarget& toolTarget);

    /** @brief Returns the active MPR tool type. */
    [[nodiscard]] MprToolType activeTool() const;
    /** @brief Sets the active MPR tool type. */
    void setActiveTool(MprToolType toolType);
    /** @brief Begins active tool interaction. */
    void beginInteraction(const ViewerInputEvent& event);
    /** @brief Updates active tool interaction. */
    void updateInteraction(const ViewerInputEvent& event);
    /** @brief Ends active tool interaction. */
    void endInteraction(const ViewerInputEvent& event);

private:
    [[nodiscard]] IViewerTool* activeToolInstance() const;

private:
    MprScene& m_scene;
    MprController& m_controller;
    std::unique_ptr<IViewerTool> m_crosshairTool;
    std::unique_ptr<IViewerTool> m_windowLevelTool;
    std::unique_ptr<IViewerTool> m_zoomTool;
    std::unique_ptr<IViewerTool> m_panTool;
    std::unique_ptr<IViewerTool> m_distanceMeasurementTool;
    std::unique_ptr<IViewerTool> m_polylineMeasurementTool;
    std::unique_ptr<IViewerTool> m_angleMeasurementTool;
    std::unique_ptr<IViewerTool> m_rectangleRoiMeasurementTool;
};
