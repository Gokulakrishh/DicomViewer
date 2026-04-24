#pragma once

#include "VTK/MPR/MprTypes.h"

#include <QObject>

class MprScene : public QObject
{
    Q_OBJECT

public:
    explicit MprScene(QObject* parent = nullptr);

    [[nodiscard]] MprCursorPositionWorld cursorPosition() const;
    [[nodiscard]] int windowLevel() const;
    [[nodiscard]] int windowWidth() const;
    [[nodiscard]] MprToolType activeTool() const;

    void setCursorPosition(const MprCursorPositionWorld& position);
    void setWindowLevelWidth(int level, int width);
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
