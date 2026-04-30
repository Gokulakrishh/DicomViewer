#include <QApplication>

#include "AdvancedViewer/WidgetsAdvancedViewerLauncher.h"
#include "DicomViewerWindow/DicomMainWindow.h"
#include "AppVersion.h"
#include "Utilities/AppStyle.h"
#include "Utilities/QSettingsAppConfigService.h"
#include "Utilities/WarningDialogService.h"

#if defined(DICOMVIEWER_ENABLE_VTK)
#include <QSurfaceFormat>
#include <QVTKOpenGLNativeWidget.h>
#endif

int main(int argc, char* argv[])
{
#if defined(DICOMVIEWER_ENABLE_VTK)
    QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());
#endif

    QApplication app(argc, argv);

    app.setApplicationName(QString::fromUtf8(AppVersion::kProductName));
    app.setApplicationDisplayName(QString::fromUtf8(AppVersion::kDisplayName));
    app.setApplicationVersion(QString::fromUtf8(AppVersion::kVersionString));
    AppStyle::apply(app);

    DicomMainWindow dicom(
        std::make_unique<QSettingsAppConfigService>(),
        std::make_unique<WidgetsAdvancedViewerLauncher>(),
        std::make_unique<WarningDialogService>());
    dicom.show();
    return app.exec();
}
