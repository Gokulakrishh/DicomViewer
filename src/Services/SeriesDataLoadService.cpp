#include "Services/SeriesDataLoadService.h"

#include "FileHandling/FileHandling.h"
#include "Model/DicomImage.h"

#include <stdexcept>

SeriesDataLoadService::SeriesDataLoadService(const FileHandling& fileHandling)
    : m_fileHandling(fileHandling)
{
}

Series SeriesDataLoadService::loadDiagnosticSeries(const Series& lightweightSeries) const
{
    if (lightweightSeries.images().empty())
    {
        throw std::runtime_error("Series does not contain any image references to reload");
    }

    Series diagnosticSeries;
    copySeriesMetadata(lightweightSeries, diagnosticSeries);
    diagnosticSeries.setImageCount(static_cast<int>(lightweightSeries.images().size()));

    for (const auto& imagePtr : lightweightSeries.images())
    {
        if (!imagePtr)
        {
            throw std::runtime_error("Series contains a null image reference");
        }

        const QString filePath = imagePtr->filePath().trimmed();
        if (filePath.isEmpty())
        {
            throw std::runtime_error("Series contains an image without a source file path");
        }

        std::unique_ptr<DicomImage> loadedImage = m_fileHandling.loadImageData(filePath);
        if (!loadedImage)
        {
            throw std::runtime_error("Failed to reload full image data for series reconstruction");
        }

        diagnosticSeries.addImage(std::move(loadedImage));
    }

    return diagnosticSeries;
}

bool SeriesDataLoadService::isDiagnosticSeriesLoaded(const Series& series) const
{
    if (series.images().empty())
    {
        return false;
    }

    for (const auto& imagePtr : series.images())
    {
        if (!imagePtr || !isImageDataLoaded(*imagePtr))
        {
            return false;
        }
    }

    return true;
}

bool SeriesDataLoadService::isImageDataLoaded(const DicomImage& image)
{
    return image.hasRawPixels() &&
           image.width() > 0 &&
           image.height() > 0 &&
           image.hasImageOrientationPatient();
}

void SeriesDataLoadService::copySeriesMetadata(const Series& source, Series& target)
{
    target.setSeriesInstanceUid(source.seriesInstanceUid());
    target.setSeriesDescription(source.seriesDescription());
    target.setModality(source.modality());
    target.setSeriesNumber(source.seriesNumber());
    target.setPreviewPixmap(source.previewPixmap());
    target.setRepresentativeFilePath(source.representativeFilePath());
    target.setImageCount(source.imageCount());
}
