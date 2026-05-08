#pragma once

#include "ViewerTools/IViewerToolTarget.h"

class MprController;
class VtkMprSceneAdapter;

/**
 * @brief Bridges reusable viewer tools into the MPR controller/scene adapter.
 */
class MprToolAdapter final : public IViewerToolTarget
{
public:
    /** @brief Creates the adapter. */
    MprToolAdapter(MprController& controller, VtkMprSceneAdapter& sceneAdapter);

    /** @brief Handles crosshair tool input. */
    void handleCrosshairInput(const ViewerInputEvent& event) override;
    /** @brief Handles WL/WW tool input. */
    void handleWindowLevelInput(const ViewerInputEvent& event) override;
    /** @brief Handles zoom tool input. */
    void handleZoomInput(const ViewerInputEvent& event) override;
    /** @brief Handles pan tool input. */
    void handlePanInput(const ViewerInputEvent& event) override;

private:
    MprController& m_controller;
    VtkMprSceneAdapter& m_sceneAdapter;
};
