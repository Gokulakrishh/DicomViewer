#pragma once

#include "Model/DicomPreviewItem.h"

#include <QWidget>

class QGridLayout;
class QLabel;
class QVBoxLayout;
class QWidget;

/**
 * @brief Thumbnail preview panel for the study browser dock.
 *
 * Responsibilities:
 * - Present lightweight study/series preview tiles.
 * - Keep preview rendering bounded to small thumbnails.
 * - Emit navigation intents for selected preview items.
 *
 * Assumptions:
 * - Preview pixmaps are derived images and not full DICOM pixel payloads.
 * - Series previews summarize the series rather than showing every slice.
 */
class DicomPreviewPanel final : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Creates the preview panel.
     * @param parent Optional Qt parent.
     */
    explicit DicomPreviewPanel(QWidget* parent = nullptr);

    /**
     * @brief Displays a grid of preview items.
     * @param title Panel title.
     * @param items Preview/navigation items.
     * @param emptyText Text shown when there are no items.
     */
    void showItems(
        const QString& title,
        const DicomPreviewItems& items,
        const QString& emptyText = "No preview");

    /**
     * @brief Displays one representative preview image.
     * @param title Panel title.
     * @param pixmap Preview image.
     * @param badgeText Optional count/status badge.
     * @param emptyText Text shown when the pixmap is empty.
     */
    void showSinglePreview(
        const QString& title,
        const QPixmap& pixmap,
        const QString& badgeText = {},
        const QString& emptyText = "No preview");

signals:
    /**
     * @brief Emitted when a preview tile is activated.
     * @param item Preview item carrying navigation target metadata.
     */
    void itemDoubleClicked(const DicomPreviewItem& item);

private:
    void buildUi();
    void clearItems();
    void addTile(const DicomPreviewItem& item, int row, int column);
    [[nodiscard]] QPixmap composeThumbnail(const QPixmap& source, const QString& badgeText) const;

private:
    static constexpr int kThumbnailSize = 120;
    static constexpr int kColumns = 2;

    QLabel* m_titleLabel{nullptr};
    QWidget* m_gridWidget{nullptr};
    QGridLayout* m_gridLayout{nullptr};
};
