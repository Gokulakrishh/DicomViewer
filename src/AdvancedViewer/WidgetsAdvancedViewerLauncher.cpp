#include "WidgetsAdvancedViewerLauncher.h"

#include "MprViewerWindow.h"
#if defined(DICOMVIEWER_ENABLE_VTK)
#include "VTK/Viewer/VtkVolumeViewerWindow.h"
#elif defined(DICOMVIEWER_ENABLE_QML3D)
#include "ThreeDViewerWindow.h"
#endif

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
#if defined(DICOMVIEWER_ENABLE_VTK)
    auto* viewer = new VtkVolumeViewerWindow(std::move(diagnosticVolume), std::move(profileSelection), parent);
#elif defined(DICOMVIEWER_ENABLE_QML3D)
    auto* viewer = new ThreeDViewerWindow(std::move(diagnosticVolume), std::move(profileSelection), parent);
#else
    Q_UNUSED(diagnosticVolume);
    Q_UNUSED(title);
    Q_UNUSED(profileSelection);
    Q_UNUSED(parent);
    return nullptr;
#endif
    viewer->setAttribute(Qt::WA_DeleteOnClose);
    viewer->setWindowTitle(title);
    viewer->show();
    viewer->raise();
    viewer->activateWindow();
    return viewer;
}
