#pragma once

#include <QString>

class QLabel;
class QVBoxLayout;
class QWidget;
class QVTKOpenGLNativeWidget;

class VtkThreeDReferencePaneView
{
public:
    VtkThreeDReferencePaneView(const QString& title, QWidget* parent);
    ~VtkThreeDReferencePaneView();

    [[nodiscard]] QWidget* widget() const;
    [[nodiscard]] QVTKOpenGLNativeWidget* renderWidget() const;

private:
    QWidget* m_rootWidget{nullptr};
    QLabel* m_titleLabel{nullptr};
    QVTKOpenGLNativeWidget* m_renderWidget{nullptr};
};
