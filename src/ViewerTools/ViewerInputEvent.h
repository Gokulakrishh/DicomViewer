#pragma once

#include "VTK/MPR/MprTypes.h"

#include <QPointF>
#include <QSize>
#include <QtCore/Qt>

struct ViewerInputEvent
{
    enum class EventType
    {
        MousePress,
        MouseMove,
        MouseRelease,
        MouseDoubleClick
    };

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
