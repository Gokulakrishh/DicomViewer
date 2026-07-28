#pragma once

#include <QIcon>
#include <QPixmap>
#include <QSize>
#include <QString>

/**
 * @brief Central access point for bundled application icons and logos.
 *
 * Purpose:
 * - Keep official branding resource paths in one place.
 * - Provide consistent icons for windows, dialogs, and future packaging work.
 *
 * Assumptions:
 * - Branding images are compiled into the Qt resource system under `:/icons/`.
 * - `icon_*` is used for window/application icons and `logo_*` for in-dialog branding.
 */
class AppIcons final
{
public:
    /**
     * @brief Builds the standard application icon from all bundled icon sizes.
     * @return Multi-size Qt icon suitable for QApplication and top-level windows.
     */
    static QIcon applicationIcon();

    /**
     * @brief Loads the standard in-application logo pixmap.
     * @param targetSize Preferred display size in device-independent pixels.
     * @return Scaled logo pixmap, or an empty pixmap if the resource cannot be loaded.
     */
    static QPixmap logoPixmap(const QSize& targetSize);

    /**
     * @brief Loads a bundled medical toolbar icon for the dark application theme.
     * @param iconId Stable icon identifier from `src/resources/medical-icons/manifest.json`.
     * @return Qt icon backed by the compiled SVG resource, or an empty icon if not found.
     */
    static QIcon toolbarIcon(const QString& iconId);

private:
    AppIcons() = delete;
};
