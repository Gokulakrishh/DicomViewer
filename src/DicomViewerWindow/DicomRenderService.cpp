#include "DicomRenderService.h"

#include "Model/DicomImage.h"

#include <QImage>
#include <algorithm>

void DicomRenderService::ensureDefaultPixmap(DicomImage& image) const
{
    if (image.pixmap().isNull() && image.hasRawPixels())
    {
        image.setPixmap(renderImage(image, image.defaultWindowLevel(), image.defaultWindowWidth())->pixmap());
    }
}

std::shared_ptr<DicomImage> DicomRenderService::renderImage(
    const DicomImage& image,
    int windowLevel,
    int windowWidth) const
{
    auto renderedImage = std::make_shared<DicomImage>(image);
    if (image.hasRawPixels() && image.isMonochrome())
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

        renderedImage->setPixmap(QPixmap::fromImage(renderedPixmap));
    }

    return renderedImage;
}
