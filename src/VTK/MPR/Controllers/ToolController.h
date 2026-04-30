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

class ToolController
{
public:
    ToolController(
        MprScene& scene,
        MprController& controller,
        MeasurementService& measurementService,
        IMeasurementToolHost& measurementToolHost,
        IViewerToolTarget& toolTarget);

    [[nodiscard]] MprToolType activeTool() const;
    void setActiveTool(MprToolType toolType);
    void beginInteraction(const ViewerInputEvent& event);
    void updateInteraction(const ViewerInputEvent& event);
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
