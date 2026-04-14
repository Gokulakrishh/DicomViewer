#include "WidgetsAdvancedViewerLauncher.h"

#include "MprViewerWindow.h"

QWidget* WidgetsAdvancedViewerLauncher::showMprVolume(
    std::shared_ptr<IVolumeData> volume,
    const QString& title,
    int windowLevel,
    int windowWidth,
    QWidget* parent)
{
    auto* viewer = new MprViewerWindow(std::move(volume), windowLevel, windowWidth, parent);
    viewer->setAttribute(Qt::WA_DeleteOnClose);
    viewer->setWindowTitle(title);
    viewer->show();
    viewer->raise();
    viewer->activateWindow();
    return viewer;
}
