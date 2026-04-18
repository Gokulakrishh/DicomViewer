#include "DicomRenderService.h"

#include "Model/DicomImage.h"

#include <QImage>
#include <algorithm>

namespace
{
QImage createWindowedImage(const DicomImage& image, int windowLevel, int windowWidth)
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
}

void DicomRenderService::ensureDiagnosticPixmap(DicomImage& image) const
{
    if (image.pixmap().isNull() && image.hasRawPixels())
    {
        image.setPixmap(renderDiagnosticImage(
                            image,
                            RenderSettings{image.defaultWindowLevel(), image.defaultWindowWidth()})
                            ->pixmap());
    }
}

std::shared_ptr<DicomImage> DicomRenderService::renderDiagnosticImage(
    const DicomImage& image,
    const RenderSettings& settings) const
{
    auto renderedImage = std::make_shared<DicomImage>(image);
    if (image.hasRawPixels() && image.isMonochrome())
    {
        renderedImage->setPixmap(QPixmap::fromImage(createWindowedImage(image, settings.windowLevel, settings.windowWidth)));
    }

    return renderedImage;
}
