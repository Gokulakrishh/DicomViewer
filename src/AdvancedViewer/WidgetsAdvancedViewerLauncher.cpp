#include "WidgetsAdvancedViewerLauncher.h"
#include <QWidget> 

#if defined(DICOMVIEWER_ENABLE_VTK)
#include "VTK/MPR/Window/VtkMprViewerWindow.h"
#include "VTK/Viewer/VtkVolumeViewerWindow.h"
#elif defined(DICOMVIEWER_ENABLE_QML3D)
#include "ThreeDViewerWindow.h"
#endif

QWidget* WidgetsAdvancedViewerLauncher::showMprVolume(
    std::shared_ptr<IVolumeData> volume,
    const QString& title,
    int windowLevel,
    int windowWidth,
    std::vector<DicomWindowPreset> dicomWindowPresets,
    int activeDicomWindowPresetIndex,
    const QString& seriesInstanceUid,
    DatabaseService* databaseService,
    QWidget* parent)
{
#if defined(DICOMVIEWER_ENABLE_VTK)
    auto* viewer = new VtkMprViewerWindow(
        std::move(volume),
        windowLevel,
        windowWidth,
        std::move(dicomWindowPresets),
        activeDicomWindowPresetIndex,
        seriesInstanceUid,
        databaseService,
        parent);
    viewer->setAttribute(Qt::WA_DeleteOnClose);
    viewer->setWindowTitle(title);
    viewer->show();
    viewer->raise();
    viewer->activateWindow();
    return viewer;
#else
    Q_UNUSED(volume);
    Q_UNUSED(title);
    Q_UNUSED(windowLevel);
    Q_UNUSED(windowWidth);
    Q_UNUSED(dicomWindowPresets);
    Q_UNUSED(activeDicomWindowPresetIndex);
    Q_UNUSED(seriesInstanceUid);
    Q_UNUSED(databaseService);
    Q_UNUSED(parent);
    return nullptr;
#endif
}

QWidget* WidgetsAdvancedViewerLauncher::showThreeDVolume(
    std::shared_ptr<IVolumeData> diagnosticVolume,
    const QString& title,
    ThreeDProfileSelection profileSelection,
    QWidget* parent)
{
    QWidget* viewer = nullptr;

#if defined(DICOMVIEWER_ENABLE_VTK)
    viewer = new VtkVolumeViewerWindow(std::move(diagnosticVolume), std::move(profileSelection), parent);
#elif defined(DICOMVIEWER_ENABLE_QML3D)
    viewer = new ThreeDViewerWindow(std::move(diagnosticVolume), std::move(profileSelection), parent);
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
