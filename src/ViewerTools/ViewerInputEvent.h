#pragma once

#include "VTK/MPR/MprTypes.h"

#include <QPointF>
#include <QSize>
#include <QtCore/Qt>

/**
 * @brief Generic viewer interaction event.
 *
 * Responsibilities:
 * - Carry display, normalized, and plane-aware coordinates.
 * - Decouple reusable tools from Qt/VTK event classes.
 */
struct ViewerInputEvent
{
    /** @brief Mouse event category. */
    enum class EventType
    {
        MousePress,
        MouseMove,
        MouseRelease,
        MouseDoubleClick
    };

    /** @brief Interaction phase. */
    enum class Phase
    {
        Begin,
        Update,
        End
    };

    EventType eventType{EventType::MousePress};
    Phase phase{Phase::Begin};
    MprSlicePlane plane{MprSlicePlane::Axial};
    Qt::MouseButton button{Qt::NoButton};
    QPointF normalizedPosition;
    QPointF normalizedDelta;
    QPointF displayPosition;
    QPointF displayDelta;
    QSize widgetSize;
};

/**
 * @brief Creates a generic pan input event.
 * @param phase Interaction phase.
 * @param plane MPR slice plane.
 * @param button Mouse button.
 * @param displayPosition Position in display pixels.
 * @param displayDelta Delta in display pixels.
 * @param widgetSize Source widget size.
 * @return Viewer input event for pan tools.
 */
inline ViewerInputEvent makePanViewerInputEvent(
    ViewerInputEvent::Phase phase,
    MprSlicePlane plane,
    Qt::MouseButton button,
    const QPointF& displayPosition,
    const QPointF& displayDelta,
    const QSize& widgetSize)
{
    return {
        phase == ViewerInputEvent::Phase::Begin
            ? ViewerInputEvent::EventType::MousePress
            : (phase == ViewerInputEvent::Phase::Update
                   ? ViewerInputEvent::EventType::MouseMove
                   : ViewerInputEvent::EventType::MouseRelease),
        phase,
        plane,
        button,
        {},
        {},
        displayPosition,
        displayDelta,
        widgetSize};
}
