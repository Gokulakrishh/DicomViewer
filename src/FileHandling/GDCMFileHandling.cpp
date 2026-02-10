#include "GDCMFileHandling.h"
#include "Model/DicomImage.h"
#include <QImageReader>
#include <QFileInfo>
#include <QDateTime>


GDCMFileHandler::GDCMFileHandler()
{
    for (const auto& format : QImageReader::supportedImageFormats()) {
        supportedFormats_ << ("*." + QString(format));
    }
}

std::unique_ptr<MedicalImage> GDCMFileHandler::loadImage(const QString& filePath)
{
    auto image = std::make_unique<DicomImage>();

    QPixmap pixmap(filePath);
    if (pixmap.isNull()) {
        return nullptr;
    }

    QFileInfo fileInfo(filePath);
    image->setPixmap(pixmap);
    image->setFilePath(filePath);
    image->setDimensions(pixmap.width(), pixmap.height());
    image->setPatientId("IMG_" + fileInfo.baseName());
    image->setStudyDate(fileInfo.lastModified().toString("yyyy-MM-dd"));
    image->setModality("Standard Image");

    return image;
}

QStringList GDCMFileHandler::getSupportedFormats() const
{
    return supportedFormats_;
}

bool GDCMFileHandler::canLoad(const QString& filePath) const
{
    QFileInfo info(filePath);
    QString suffix = "*." + info.suffix();
    return supportedFormats_.contains(suffix, Qt::CaseInsensitive);
}
