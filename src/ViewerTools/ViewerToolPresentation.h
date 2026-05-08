#pragma once

#include <QActionGroup>
#include <QObject>

#include <array>
#include <functional>
#include <optional>

class QAction;
class QToolBar;

/**
 * @brief Viewer toolbar tool identifiers shared by main and MPR windows.
 */
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

/**
 * @brief Shared toolbar presentation for viewer tools.
 *
 * Responsibilities:
 * - Create mutually exclusive QAction entries for viewer tools.
 * - Keep labels/tool ids centralized so main and MPR windows use the same
 *   presentation vocabulary while retaining independent behavior.
 */
class ViewerToolPresentation final : public QObject
{
    Q_OBJECT

public:
    /** @brief Creates actions in a toolbar. */
    explicit ViewerToolPresentation(QToolBar& toolBar, QObject* parent = nullptr);

    /** @brief Sets callback invoked when active tool changes. */
    void setSelectionChangedCallback(std::function<void(std::optional<ViewerToolId>)> callback);
    /** @brief Returns the QAction for a tool id. */
    [[nodiscard]] QAction* action(ViewerToolId toolId) const;
    /** @brief Returns the active tool id, if any. */
    [[nodiscard]] std::optional<ViewerToolId> activeTool() const;
    /** @brief Enables or disables one tool action. */
    void setToolEnabled(ViewerToolId toolId, bool enabled);
    /** @brief Sets the active tool programmatically. */
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
