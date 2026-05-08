#pragma once

#include "VTK/MPR/MprTypes.h"

#include <QPointF>
#include <QtCore/Qt>

class MprController;
class ToolController;

/**
 * @brief Routes raw MPR pane interactions to tools or slice navigation.
 */
class InteractionRouter
{
public:
    /** @brief Creates the router. */
    InteractionRouter(ToolController& toolController, MprController& controller);

    /** @brief Begins an interaction on a slice plane. */
    void beginInteraction(MprSlicePlane plane, Qt::MouseButton button, const QPointF& position);
    /** @brief Updates an interaction on a slice plane. */
    void updateInteraction(MprSlicePlane plane, const QPointF& position, const QPointF& delta);
    /** @brief Ends an interaction on a slice plane. */
    void endInteraction(MprSlicePlane plane, Qt::MouseButton button, const QPointF& position);
    /** @brief Scrolls slices for a plane. */
    void scrollSlices(MprSlicePlane plane, int steps);

private:
    ToolController& m_toolController;
    MprController& m_controller;
};
