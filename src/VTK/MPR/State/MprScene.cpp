#include "VTK/MPR/State/MprScene.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr double kCursorTolerance = 1e-6;
}

MprScene::MprScene(QObject* parent)
    : QObject(parent)
{
}

MprCursorPositionWorld MprScene::cursorPosition() const
{
    return m_cursorPosition;
}

int MprScene::windowLevel() const
{
    return m_windowLevel;
}

int MprScene::windowWidth() const
{
    return m_windowWidth;
}

MprToolType MprScene::activeTool() const
{
    return m_activeTool;
}

void MprScene::setCursorPosition(const MprCursorPositionWorld& position)
{
    const bool unchanged =
        std::abs(m_cursorPosition.x - position.x) < kCursorTolerance &&
        std::abs(m_cursorPosition.y - position.y) < kCursorTolerance &&
        std::abs(m_cursorPosition.z - position.z) < kCursorTolerance;
    if (unchanged)
    {
        return;
    }

    m_cursorPosition = position;
    emit cursorPositionChanged(m_cursorPosition);
}

void MprScene::setWindowLevelWidth(int level, int width)
{
    const int clampedWidth = std::max(1, width);
    if (m_windowLevel == level && m_windowWidth == clampedWidth)
    {
        return;
    }

    m_windowLevel = level;
    m_windowWidth = clampedWidth;
    emit windowLevelWidthChanged(m_windowLevel, m_windowWidth);
}

void MprScene::setActiveTool(MprToolType toolType)
{
    if (m_activeTool == toolType)
    {
        return;
    }

    m_activeTool = toolType;
    emit activeToolChanged(m_activeTool);
}
