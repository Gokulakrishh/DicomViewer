#include "Services/ThreeDSeriesBuildService.h"

#include "Errors/AppError.h"
#include "Model/DicomParameters.h"
#include "Services/ThreeDProfiles/I3dPipelineProfile.h"
#include "Services/ThreeDimensionalPipelineService.h"
#include "Services/VolumeBuilder.h"

#include <stdexcept>

namespace
{
AppError makeThreeDBuildError(ErrorCode code, const QString& technicalMessage, const QString& userMessage)
{
    return AppError{
        code,
        ErrorSeverity::Recoverable,
        "3D Pipeline",
        technicalMessage,
        userMessage};
}

AppError mapThreeDPipelineException(const std::exception& exception)
{
    const QString technicalMessage = QString::fromUtf8(exception.what());
    const QString userMessage = "Failed to build the selected 3D reconstruction.";

    if (technicalMessage.contains("incomplete strategy set", Qt::CaseInsensitive))
    {
        return makeThreeDBuildError(ErrorCode::ThreeDPipelineIncompleteStrategySet, technicalMessage, userMessage);
    }

    if (technicalMessage.contains("null mask", Qt::CaseInsensitive))
    {
        if (technicalMessage.contains("filtered", Qt::CaseInsensitive) ||
            technicalMessage.contains("component", Qt::CaseInsensitive))
        {
            return makeThreeDBuildError(ErrorCode::ThreeDPipelineNullFilteredMask, technicalMessage, userMessage);
        }

        return makeThreeDBuildError(ErrorCode::ThreeDPipelineNullSegmentationMask, technicalMessage, userMessage);
    }

    if (technicalMessage.contains("null mesh", Qt::CaseInsensitive))
    {
        if (technicalMessage.contains("post-processing", Qt::CaseInsensitive))
        {
            return makeThreeDBuildError(ErrorCode::ThreeDPipelineNullPostProcessedMesh, technicalMessage, userMessage);
        }

        return makeThreeDBuildError(ErrorCode::ThreeDPipelineNullExtractedMesh, technicalMessage, userMessage);
    }

    return makeThreeDBuildError(ErrorCode::ThreeDOpenFailed, technicalMessage, userMessage);
}
}

ThreeDSeriesBuildService::ThreeDSeriesBuildService(VolumeValidationSettings validationSettings)
    : m_validationSettings(validationSettings)
{
}

AppResult<ThreeDimensionalPipelineResult> ThreeDSeriesBuildService::buildFromDiagnosticSeries(
    const Series& diagnosticSeries,
    const I3dPipelineProfile& profile) const
{
    VolumeBuilder volumeBuilder(m_validationSettings);
    const AppResult<std::shared_ptr<IVolumeData>> volumeResult = volumeBuilder.buildFromDiagnosticSeries(diagnosticSeries);
    if (!volumeResult)
    {
        return volumeResult.error();
    }

    ThreeDimensionalPipelineService pipelineService;
    try
    {
        return pipelineService.buildMesh(*volumeResult.value(), profile);
    }
    catch (const AppError& error)
    {
        return error;
    }
    catch (const std::exception& exception)
    {
        return mapThreeDPipelineException(exception);
    }
}
