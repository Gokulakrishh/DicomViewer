#pragma once

#include <QString>

class QLabel;
class QVBoxLayout;
class QWidget;
class QVTKOpenGLNativeWidget;

/**
 * @brief 3D reference pane used by the MPR viewer.
 */
class VtkThreeDReferencePaneView
{
public:
    /** @brief Creates the reference pane. */
    VtkThreeDReferencePaneView(const QString& title, QWidget* parent);
    ~VtkThreeDReferencePaneView();

    /** @brief Returns root widget. */
    [[nodiscard]] QWidget* widget() const;
    /** @brief Returns VTK render widget. */
    [[nodiscard]] QVTKOpenGLNativeWidget* renderWidget() const;

private:
    QWidget* m_rootWidget{nullptr};
    QLabel* m_titleLabel{nullptr};
    QLabel* m_statusLabel{nullptr};
    QVTKOpenGLNativeWidget* m_renderWidget{nullptr};
};
