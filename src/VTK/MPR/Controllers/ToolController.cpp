#include "VTK/MPR/Controllers/ToolController.h"

#include "VTK/MPR/Controllers/MprController.h"
#include "VTK/MPR/State/MprScene.h"
#include "ViewerTools/CrosshairTool.h"
#include "ViewerTools/IViewerTool.h"
#include "ViewerTools/IViewerToolTarget.h"
#include "ViewerTools/Measurements/MeasurementTool.h"
#include "ViewerTools/PanTool.h"
#include "ViewerTools/ViewerInputEvent.h"
#include "ViewerTools/WindowLevelTool.h"
#include "ViewerTools/ZoomTool.h"

ToolController::ToolController(
    MprScene& scene,
    MprController& controller,
    MeasurementService& measurementService,
    IMeasurementToolHost& measurementToolHost,
    IViewerToolTarget& toolTarget)
    : m_scene(scene),
      m_controller(controller),
      m_crosshairTool(std::make_unique<CrosshairTool>(toolTarget)),
      m_windowLevelTool(std::make_unique<WindowLevelTool>(toolTarget)),
      m_zoomTool(std::make_unique<ZoomTool>(toolTarget)),
      m_panTool(std::make_unique<PanTool>(toolTarget)),
      m_distanceMeasurementTool(std::make_unique<MeasurementTool>(
          MeasurementToolMode::Distance,
          measurementService,
          measurementToolHost)),
      m_polylineMeasurementTool(std::make_unique<MeasurementTool>(
          MeasurementToolMode::Polyline,
          measurementService,
          measurementToolHost)),
      m_angleMeasurementTool(std::make_unique<MeasurementTool>(
          MeasurementToolMode::Angle,
          measurementService,
          measurementToolHost)),
      m_rectangleRoiMeasurementTool(std::make_unique<MeasurementTool>(
          MeasurementToolMode::RectangleRoi,
          measurementService,
          measurementToolHost))
{
}

MprToolType ToolController::activeTool() const
{
    return m_scene.activeTool();
}

void ToolController::setActiveTool(MprToolType toolType)
{
    m_controller.setActiveTool(toolType);
}

void ToolController::beginInteraction(const ViewerInputEvent& event)
{
    if (auto* tool = activeToolInstance())
    {
        tool->beginInteraction(event);
    }
}

void ToolController::updateInteraction(const ViewerInputEvent& event)
{
    if (auto* tool = activeToolInstance())
    {
        tool->updateInteraction(event);
    }
}

void ToolController::endInteraction(const ViewerInputEvent& event)
{
    if (auto* tool = activeToolInstance())
    {
        tool->endInteraction(event);
    }
}

IViewerTool* ToolController::activeToolInstance() const
{
    switch (m_scene.activeTool())
    {
    case MprToolType::None:
    case MprToolType::Crosshair:
        return m_crosshairTool.get();
    case MprToolType::WindowLevel:
        return m_windowLevelTool.get();
    case MprToolType::Zoom:
        return m_zoomTool.get();
    case MprToolType::Pan:
        return m_panTool.get();
    case MprToolType::DistanceMeasurement:
        return m_distanceMeasurementTool.get();
    case MprToolType::PolylineMeasurement:
        return m_polylineMeasurementTool.get();
    case MprToolType::AngleMeasurement:
        return m_angleMeasurementTool.get();
    case MprToolType::RectangleRoiMeasurement:
        return m_rectangleRoiMeasurementTool.get();
    case MprToolType::Slice:
        return nullptr;
    }

    return nullptr;
}
