#pragma once

#include "ViewerTools/IViewerToolTarget.h"

class MprController;
class VtkMprSceneAdapter;

class MprToolAdapter final : public IViewerToolTarget
{
public:
    MprToolAdapter(MprController& controller, VtkMprSceneAdapter& sceneAdapter);

    void handleCrosshairInput(const ViewerInputEvent& event) override;
    void handleWindowLevelInput(const ViewerInputEvent& event) override;
    void handleZoomInput(const ViewerInputEvent& event) override;

private:
    MprController& m_controller;
    VtkMprSceneAdapter& m_sceneAdapter;
};
