#pragma once

#include "Model/DicomImage.h"

#include <QImage>
#include <QPixmap>
#include <algorithm>
#include <memory>

/**
 * @brief Creates a bounded thumbnail pixmap.
 * @param image Source image.
 * @param maxDimension Maximum width/height in pixels.
 * @return Scaled pixmap, or empty pixmap for null input.
 */
inline QPixmap createThumbnailPixmap(const QImage& image, int maxDimension = 192)
{
    if (image.isNull())
    {
        return {};
    }

    const QImage scaledImage = image.scaled(
        maxDimension,
        maxDimension,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation);
    return QPixmap::fromImage(scaledImage);
}

/**
 * @brief Renders a monochrome DICOM image using WL/WW.
 * @param image Source DICOM image with raw pixels.
 * @param windowLevel Window center/level.
 * @param windowWidth Window width.
 * @return 8-bit grayscale image.
 */
inline QImage createWindowedImage(const DicomImage& image, int windowLevel, int windowWidth)
{
    const double widthValue = std::max(1, windowWidth);
    const double lowerBound = static_cast<double>(windowLevel) - (widthValue / 2.0);
    QImage renderedPixmap(image.width(), image.height(), QImage::Format_Grayscale8);

    for (int y = 0; y < image.height(); ++y)
    {
        uchar* scanLine = renderedPixmap.scanLine(y);
        for (int x = 0; x < image.width(); ++x)
        {
            const double value = static_cast<double>(image.rawPixelValueAt(x, y));
            const int scaledValue = std::clamp(
                static_cast<int>(((value - lowerBound) / widthValue) * 255.0),
                0,
                255);
            scanLine[x] = static_cast<uchar>(image.isMonochrome1() ? 255 - scaledValue : scaledValue);
        }
    }

    return renderedPixmap;
}

/**
 * @brief Creates a thumbnail preview for a DICOM image.
 * @param image Source DICOM image.
 * @param maxDimension Maximum thumbnail size.
 * @return Preview pixmap.
 */
inline QPixmap createDicomPreviewPixmap(const DicomImage& image, int maxDimension = 192)
{
    if (image.hasRawPixels() && image.isMonochrome())
    {
        return createThumbnailPixmap(
            createWindowedImage(image, image.defaultWindowLevel(), image.defaultWindowWidth()),
            maxDimension);
    }

    return createThumbnailPixmap(image.pixmap().toImage(), maxDimension);
}

/**
 * @brief Creates a rendered copy of a DICOM image for display.
 * @param image Source image.
 * @param windowLevel Window center/level.
 * @param windowWidth Window width.
 * @return Shared rendered DICOM image copy.
 */
inline std::shared_ptr<DicomImage> renderDiagnosticImage(const DicomImage& image, int windowLevel, int windowWidth)
{
    auto renderedImage = std::make_shared<DicomImage>(image);
    if (image.hasRawPixels() && image.isMonochrome())
    {
        renderedImage->setPixmap(QPixmap::fromImage(createWindowedImage(image, windowLevel, windowWidth)));
    }

    return renderedImage;
}
