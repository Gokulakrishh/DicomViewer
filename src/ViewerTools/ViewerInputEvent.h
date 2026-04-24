#pragma once

#include "VTK/MPR/MprTypes.h"

#include <QPointF>
#include <Qt>

struct ViewerInputEvent
{
    enum class Phase
    {
        Begin,
        Update,
        End
    };

    Phase phase{Phase::Begin};
    MprSlicePlane plane{MprSlicePlane::Axial};
    Qt::MouseButton button{Qt::NoButton};
    QPointF normalizedPosition;
    QPointF normalizedDelta;
};
