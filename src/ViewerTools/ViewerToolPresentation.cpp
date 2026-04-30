#include "ViewerTools/ViewerToolPresentation.h"

#include <QAction>
#include <QSignalBlocker>
#include <QTimer>
#include <QToolBar>

ViewerToolPresentation::ViewerToolPresentation(QToolBar& toolBar, QObject* parent)
    : QObject(parent),
      m_actionGroup(new QActionGroup(this))
{
    m_actionGroup->setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional);

    for (auto& tool : m_tools)
    {
        tool.action = new QAction(QString::fromUtf8(tool.label), this);
        tool.action->setCheckable(true);
        m_actionGroup->addAction(tool.action);
        toolBar.addAction(tool.action);

        connect(tool.action, &QAction::toggled, this, [this](bool) {
            scheduleSelectionNotification();
        });
    }
}

void ViewerToolPresentation::setSelectionChangedCallback(
    std::function<void(std::optional<ViewerToolId>)> callback)
{
    m_selectionChangedCallback = std::move(callback);
}

QAction* ViewerToolPresentation::action(ViewerToolId toolId) const
{
    return m_tools[toolIndex(toolId)].action;
}

std::optional<ViewerToolId> ViewerToolPresentation::activeTool() const
{
    for (const auto& tool : m_tools)
    {
        if (tool.action && tool.action->isChecked())
        {
            return tool.id;
        }
    }

    return std::nullopt;
}

void ViewerToolPresentation::setToolEnabled(ViewerToolId toolId, bool enabled)
{
    if (QAction* toolAction = action(toolId))
    {
        toolAction->setEnabled(enabled);
    }
}

void ViewerToolPresentation::setActiveTool(std::optional<ViewerToolId> toolId)
{
    for (auto& tool : m_tools)
    {
        if (!tool.action)
        {
            continue;
        }

        const QSignalBlocker blocker(tool.action);
        tool.action->setChecked(toolId && *toolId == tool.id);
    }
    scheduleSelectionNotification();
}

void ViewerToolPresentation::scheduleSelectionNotification()
{
    if (m_notificationPending)
    {
        return;
    }

    m_notificationPending = true;
    QTimer::singleShot(0, this, [this]() {
        m_notificationPending = false;
        if (m_selectionChangedCallback)
        {
            m_selectionChangedCallback(activeTool());
        }
    });
}

constexpr std::size_t ViewerToolPresentation::toolIndex(ViewerToolId toolId)
{
    return static_cast<std::size_t>(toolId);
}
