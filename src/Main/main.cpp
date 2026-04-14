#include <QApplication>
#include <QPalette>
#include <QStyleFactory>

#include "AdvancedViewer/WidgetsAdvancedViewerLauncher.h"
#include "DicomViewerWindow/DicomMainWindow.h"
#include "Utilities/QSettingsAppConfigService.h"
#include "Utilities/WarningDialogService.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName("Dicom Image Viewer (2D/3D)");
    app.setStyle(QStyleFactory::create("Fusion"));

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    app.setPalette(darkPalette);

    DicomMainWindow dicom(
        std::make_unique<QSettingsAppConfigService>(),
        std::make_unique<WidgetsAdvancedViewerLauncher>(),
        std::make_unique<WarningDialogService>());
    dicom.show();
    return app.exec();
}
