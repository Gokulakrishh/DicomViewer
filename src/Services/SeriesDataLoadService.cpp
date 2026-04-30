#include "Services/SeriesDataLoadService.h"

#include "Audit/DicomMetadataAudit.h"
#include "Audit/IAuditService.h"
#include "Errors/AppError.h"
#include "FileHandling/FileHandling.h"
#include "Model/DicomImage.h"

namespace
{
AppError makeSeriesLoadError(ErrorCode code, const QString& technicalMessage, const QString& userMessage)
{
    return AppError{
        code,
        ErrorSeverity::Recoverable,
        "Series Loading",
        technicalMessage,
        userMessage};
}
}

SeriesDataLoadService::SeriesDataLoadService(
    const FileHandling& fileHandling,
    IAuditService* auditService)
    : m_fileHandling(fileHandling),
      m_auditService(auditService)
{
}

AppResult<Series> SeriesDataLoadService::loadDiagnosticSeries(const Series& lightweightSeries) const
{
    if (lightweightSeries.images().empty())
    {
        return makeSeriesLoadError(
            ErrorCode::SeriesLoadMissingImageReference,
            "Series does not contain any image references to reload",
            "The selected series does not contain any image references to reload.");
    }

    Series diagnosticSeries;
    copySeriesMetadata(lightweightSeries, diagnosticSeries);
    diagnosticSeries.setImageCount(static_cast<int>(lightweightSeries.images().size()));

    for (const auto& imagePtr : lightweightSeries.images())
    {
        if (!imagePtr)
        {
            return makeSeriesLoadError(
                ErrorCode::SeriesLoadNullImageReference,
                "Series contains a null image reference",
                "The selected series contains an invalid image reference.");
        }

        const QString filePath = imagePtr->filePath().trimmed();
        if (filePath.isEmpty())
        {
            return makeSeriesLoadError(
                ErrorCode::SeriesLoadMissingFilePath,
                "Series contains an image without a source file path",
                "The selected series contains an image without a valid source file path.");
        }

        std::unique_ptr<DicomImage> loadedImage = m_fileHandling.loadImageData(filePath);
        if (!loadedImage)
        {
            return makeSeriesLoadError(
                ErrorCode::SeriesLoadReloadFailed,
                "Failed to reload full image data for series reconstruction",
                "Failed to reload full image data for the selected series.");
        }

        diagnosticSeries.addImage(std::move(loadedImage));
    }

    shareSeriesMetadataAcrossImages(diagnosticSeries);
    if (m_auditService)
    {
        DicomMetadataAudit audit;
        const DicomMetadataAuditResult auditResult = audit.evaluateSeries(diagnosticSeries);
        m_auditService->record(audit.toAuditEvent(auditResult, diagnosticSeries.seriesInstanceUid()));
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

void SeriesDataLoadService::shareSeriesMetadataAcrossImages(Series& series)
{
    std::shared_ptr<const DicomSeriesMetadata> sharedSeriesMetadata;
    for (const auto& imagePtr : series.images())
    {
        if (imagePtr && imagePtr->metadata() && imagePtr->metadata()->series)
        {
            sharedSeriesMetadata = imagePtr->metadata()->series;
            break;
        }
    }

    if (!sharedSeriesMetadata)
    {
        return;
    }

    for (const auto& imagePtr : series.images())
    {
        if (!imagePtr || !imagePtr->metadata())
        {
            continue;
        }

        DicomInstanceMetadata metadata = *imagePtr->metadata();
        metadata.series = sharedSeriesMetadata;
        imagePtr->setMetadata(metadata);
    }
}
