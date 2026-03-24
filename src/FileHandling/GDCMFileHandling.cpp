#include "GDCMFileHandling.h"

#include "Model/DicomImage.h"

#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QPixmap>
#include <QtGlobal>
#include <gdcmImage.h>
#include <gdcmImageReader.h>
#include <vector>

GDCMFileHandling::GDCMFileHandling()
{
    for (const auto& format : QImageReader::supportedImageFormats())
    {
        supportedFormats_ << ("*." + QString::fromLatin1(format));
    }

    if (!supportedFormats_.contains("*.dcm", Qt::CaseInsensitive))
    {
        supportedFormats_ << "*.dcm";
    }
}

void GDCMFileHandling::loadDicomFolder(const QString& folderPath)
{
    Q_UNUSED(folderPath);
}

std::unique_ptr<MedicalImage> GDCMFileHandling::loadImage(const QString& filePath)
{
    gdcm::ImageReader reader;
    reader.SetFileName(filePath.toStdString().c_str());

    if (!reader.Read())
    {
        qDebug() << "Failed to read DICOM file:" << filePath;
        return nullptr;
    }

    const gdcm::Image& gdcmImage = reader.GetImage();
    const unsigned int width = gdcmImage.GetDimensions()[0];
    const unsigned int height = gdcmImage.GetDimensions()[1];
    const unsigned long bufferLength = gdcmImage.GetBufferLength();

    std::vector<char> buffer(bufferLength);
    if (!gdcmImage.GetBuffer(buffer.data()))
    {
        qDebug() << "Failed to get pixel buffer!";
        return nullptr;
    }

    QImage::Format format = QImage::Format_Grayscale8;
    if (gdcmImage.GetPixelFormat().GetSamplesPerPixel() == 3)
    {
        format = QImage::Format_RGB888;
    }

    QImage image(reinterpret_cast<const uchar*>(buffer.data()), static_cast<int>(width), static_cast<int>(height), format);
    if (format == QImage::Format_RGB888)
    {
        image = image.rgbSwapped();
    }
    else
    {
        image = image.copy();
    }

    auto dicomImage = std::make_unique<DicomImage>();
    const QFileInfo fileInfo(filePath);
    const QPixmap pixmap = QPixmap::fromImage(image);

    dicomImage->setPixmap(pixmap);
    dicomImage->setFilePath(filePath);
    dicomImage->setDimensions(pixmap.width(), pixmap.height());
    dicomImage->setPatientId("IMG_" + fileInfo.baseName());
    dicomImage->setStudyDate(fileInfo.lastModified().toString("yyyy-MM-dd"));
    dicomImage->setModality("DICOM");

    return dicomImage;
}

QStringList GDCMFileHandling::getSupportedFormats() const
{
    return supportedFormats_;
}

bool GDCMFileHandling::canLoad(const QString& filePath) const
{
    const QFileInfo info(filePath);
    const QString suffix = "*." + info.suffix();
    return supportedFormats_.contains(suffix, Qt::CaseInsensitive);
}
