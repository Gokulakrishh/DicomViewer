#include "WidgetsAdvancedViewerLauncher.h"

#include "MprViewerWindow.h"
#include "ThreeDViewerWindow.h"

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

QWidget* WidgetsAdvancedViewerLauncher::showThreeDVolume(
    std::shared_ptr<IVolumeData> diagnosticVolume,
    const QString& title,
    ThreeDProfileSelection profileSelection,
    QWidget* parent)
{
    auto* viewer = new ThreeDViewerWindow(std::move(diagnosticVolume), std::move(profileSelection), parent);
    viewer->setAttribute(Qt::WA_DeleteOnClose);
    viewer->setWindowTitle(title);
    viewer->show();
    viewer->raise();
    viewer->activateWindow();
    return viewer;
}
