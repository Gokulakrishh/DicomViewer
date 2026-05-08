#pragma once

#include "VTK/MPR/MprTypes.h"

#include <QObject>

/**
 * @brief Observable state model for MPR cursor, WL/WW, and active tool.
 */
class MprScene : public QObject
{
    Q_OBJECT

public:
    /** @brief Creates the MPR scene state object. */
    explicit MprScene(QObject* parent = nullptr);

    /** @brief Returns current cursor world position. */
    [[nodiscard]] MprCursorPositionWorld cursorPosition() const;
    /** @brief Returns current window level. */
    [[nodiscard]] int windowLevel() const;
    /** @brief Returns current window width. */
    [[nodiscard]] int windowWidth() const;
    /** @brief Returns active MPR tool. */
    [[nodiscard]] MprToolType activeTool() const;

    /** @brief Sets cursor world position. */
    void setCursorPosition(const MprCursorPositionWorld& position);
    /** @brief Sets WL/WW values. */
    void setWindowLevelWidth(int level, int width);
    /** @brief Sets active MPR tool. */
    void setActiveTool(MprToolType toolType);

signals:
    void cursorPositionChanged(const MprCursorPositionWorld& position);
    void windowLevelWidthChanged(int level, int width);
    void activeToolChanged(MprToolType toolType);

private:
    MprCursorPositionWorld m_cursorPosition;
    int m_windowLevel{40};
    int m_windowWidth{400};
    MprToolType m_activeTool{MprToolType::Crosshair};
};
