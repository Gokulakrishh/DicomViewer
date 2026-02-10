#include <QGuiApplication>
#include "DicomViewerWindow/DicomViewerWindow.h"


int main(int argc, char *argv[])
{

    QApplication app(argc, argv);

    app.setApplicationName("Dicom Image Viewer (2D/3D)");
    app.setStyle(QStyleFactory::create("Fusion"));
    //app.setWindowIcon(QIcon(":/resources/dicom.ico"));
    DicomViewerWindow dicom;
    dicom.show();
    return app.exec();
}
