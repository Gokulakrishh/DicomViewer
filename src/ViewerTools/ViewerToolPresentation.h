#pragma once

#include <QActionGroup>
#include <QObject>

#include <array>
#include <functional>
#include <optional>

class QAction;
class QToolBar;

enum class ViewerToolId
{
    WindowLevel,
    Zoom,
    Pan,
    Distance,
    Polyline,
    Angle,
    RectangleRoi
};

class ViewerToolPresentation final : public QObject
{
    Q_OBJECT

public:
    explicit ViewerToolPresentation(QToolBar& toolBar, QObject* parent = nullptr);

    void setSelectionChangedCallback(std::function<void(std::optional<ViewerToolId>)> callback);
    [[nodiscard]] QAction* action(ViewerToolId toolId) const;
    [[nodiscard]] std::optional<ViewerToolId> activeTool() const;
    void setToolEnabled(ViewerToolId toolId, bool enabled);
    void setActiveTool(std::optional<ViewerToolId> toolId);

private:
    struct ToolEntry
    {
        ViewerToolId id;
        const char* label;
        QAction* action{nullptr};
    };

    void scheduleSelectionNotification();
    static constexpr std::size_t toolIndex(ViewerToolId toolId);

private:
    QActionGroup* m_actionGroup{nullptr};
    std::array<ToolEntry, 7> m_tools{{
        {ViewerToolId::WindowLevel, "WL/WW", nullptr},
        {ViewerToolId::Zoom, "Zoom", nullptr},
        {ViewerToolId::Pan, "Pan", nullptr},
        {ViewerToolId::Distance, "Distance", nullptr},
        {ViewerToolId::Polyline, "Polyline", nullptr},
        {ViewerToolId::Angle, "Angle", nullptr},
        {ViewerToolId::RectangleRoi, "ROI", nullptr},
    }};
    std::function<void(std::optional<ViewerToolId>)> m_selectionChangedCallback;
    bool m_notificationPending{false};
};
