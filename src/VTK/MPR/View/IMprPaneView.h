#pragma once

#include "VTK/MPR/MprTypes.h"

class QWidget;
class QSlider;
class QVTKOpenGLNativeWidget;

/**
 * @brief Minimal view interface for an MPR pane.
 */
class IMprPaneView
{
public:
    virtual ~IMprPaneView() = default;

    /** @brief Returns pane slice plane. */
    [[nodiscard]] virtual MprSlicePlane plane() const = 0;
    /** @brief Returns root Qt widget. */
    [[nodiscard]] virtual QWidget* widget() const = 0;
    /** @brief Returns VTK render widget. */
    [[nodiscard]] virtual QVTKOpenGLNativeWidget* renderWidget() const = 0;
    /** @brief Returns pane slice slider. */
    [[nodiscard]] virtual QSlider* sliceSlider() const = 0;
};
