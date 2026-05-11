#include "Services/AdvancedSeriesVolumeService.h"

#include "Errors/AppError.h"
#include "FileHandling/FileHandling.h"
#include "Model/DicomParameters.h"
#include "Services/SeriesDataLoadService.h"
#include "Services/VolumeBuilder.h"

namespace
{
AppError makeAdvancedViewerBuildError(const QString& technicalMessage, const QString& userMessage)
{
    return AppError{
        ErrorCode::VolumeBuildInvalidInput,
        ErrorSeverity::Recoverable,
        "Advanced Viewer Volume Build",
        technicalMessage,
        userMessage};
}
}

AdvancedSeriesVolumeService::AdvancedSeriesVolumeService(
    const FileHandling& fileHandling,
    IAuditService* auditService)
    : m_fileHandling(fileHandling),
      m_auditService(auditService),
      m_volumeBuilder(std::make_unique<VolumeBuilder>())
{
}

AdvancedSeriesVolumeService::~AdvancedSeriesVolumeService() = default;

AppResult<VolumeBuildResult> AdvancedSeriesVolumeService::buildDiagnosticVolume(const Series& lightweightSeries) const
{
    if (lightweightSeries.images().size() < 2)
    {
        return makeAdvancedViewerBuildError(
            "Series does not contain enough slices to build a diagnostic volume",
            "The selected series does not contain enough slices to build a diagnostic volume.");
    }

    SeriesDataLoadService seriesDataLoadService(m_fileHandling, m_auditService);
    const auto diagnosticSeriesResult = seriesDataLoadService.loadDiagnosticSeries(lightweightSeries);
    if (!diagnosticSeriesResult)
    {
        return diagnosticSeriesResult.error();
    }

    return m_volumeBuilder->buildFromDiagnosticSeries(diagnosticSeriesResult.value());
}
