#include "GDCMFileHandling.h"
#include "Model/DicomImage.h"
#include <QImageReader>
#include <QFileInfo>
#include <QDateTime>
#include <QPixmap>
#include <gdcmImageReader.h>
#include <gdcmImage.h>


GDCMFileHandling::GDCMFileHandling()
{
    for (const auto& format : QImageReader::supportedImageFormats()) {
        supportedFormats_ << ("*." + QString(format));
    }
}

std::unique_ptr<MedicalImage> GDCMFileHandling::loadImage(const QString& filePath)
{


    gdcm::ImageReader reader;
    reader.SetFileName(filePath.toStdString().c_str());

    if (!reader.Read()) {
        qDebug() << "Failed to read DICOM file:" << filePath;
        return QPixmap();
    }

    const gdcm::Image& gdcmImage = reader.GetImage();
    unsigned int width = gdcmImage.GetDimensions()[0];
    unsigned int height = gdcmImage.GetDimensions()[1];
    unsigned long bufferLength = gdcmImage.GetBufferLength();

    std::vector<char> buffer(bufferLength);
    if (!gdcmImage.GetBuffer(&buffer[0])) {
        qDebug() << "Failed to get pixel buffer!";
        return QPixmap();
    }

    // Determine the pixel type
    QImage::Format format = QImage::Format_Grayscale8;
    if (gdcmImage.GetPixelFormat().GetSamplesPerPixel() == 3) {
        format = QImage::Format_RGB888;
    }

    // Create QImage from buffer
    QImage image(reinterpret_cast<const uchar*>(&buffer[0]), width, height, format);

    // For RGB images, swap RGB bytes
    if (format == QImage::Format_RGB888) {
        image = image.rgbSwapped();
    }

    return QPixmap::fromImage(image);




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

QStringList GDCMFileHandling::getSupportedFormats() const
{
    return supportedFormats_;
}

bool GDCMFileHandling::canLoad(const QString& filePath) const
{
    QFileInfo info(filePath);
    QString suffix = "*." + info.suffix();
    return supportedFormats_.contains(suffix, Qt::CaseInsensitive);
}
