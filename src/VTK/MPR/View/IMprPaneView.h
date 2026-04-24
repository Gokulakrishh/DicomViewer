#pragma once

#include "VTK/MPR/MprTypes.h"

class QWidget;
class QSlider;
class QVTKOpenGLNativeWidget;

class IMprPaneView
{
public:
    virtual ~IMprPaneView() = default;

    [[nodiscard]] virtual MprSlicePlane plane() const = 0;
    [[nodiscard]] virtual QWidget* widget() const = 0;
    [[nodiscard]] virtual QVTKOpenGLNativeWidget* renderWidget() const = 0;
    [[nodiscard]] virtual QSlider* sliceSlider() const = 0;
};
