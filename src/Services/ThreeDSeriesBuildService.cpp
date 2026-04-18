#include "Services/ThreeDSeriesBuildService.h"

#include "Model/DicomParameters.h"
#include "Services/ThreeDProfiles/I3dPipelineProfile.h"
#include "Services/ThreeDimensionalPipelineService.h"
#include "Services/VolumeBuilder.h"

#include <stdexcept>

ThreeDSeriesBuildService::ThreeDSeriesBuildService(VolumeValidationSettings validationSettings)
    : m_validationSettings(validationSettings)
{
}

ThreeDimensionalPipelineResult ThreeDSeriesBuildService::buildFromDiagnosticSeries(
    const Series& diagnosticSeries,
    const I3dPipelineProfile& profile) const
{
    VolumeBuilder volumeBuilder(m_validationSettings);
    const std::shared_ptr<IVolumeData> volume = volumeBuilder.buildFromDiagnosticSeries(diagnosticSeries);
    if (!volume)
    {
        throw std::runtime_error("Failed to build 3D volume from diagnostic series");
    }

    ThreeDimensionalPipelineService pipelineService;
    return pipelineService.buildMesh(*volume, profile);
}
