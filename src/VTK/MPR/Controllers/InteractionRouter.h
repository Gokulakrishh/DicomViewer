#pragma once

#include "VTK/MPR/MprTypes.h"

#include <QPointF>
#include <Qt>

class MprController;
class ToolController;

class InteractionRouter
{
public:
    InteractionRouter(ToolController& toolController, MprController& controller);

    void beginInteraction(MprSlicePlane plane, Qt::MouseButton button, const QPointF& position);
    void updateInteraction(MprSlicePlane plane, const QPointF& position, const QPointF& delta);
    void endInteraction(MprSlicePlane plane, Qt::MouseButton button, const QPointF& position);
    void scrollSlices(MprSlicePlane plane, int steps);

private:
    ToolController& m_toolController;
    MprController& m_controller;
};
