#pragma once

#include "Services/ThreeDimensionalPipelineResult.h"
#include "Utilities/VolumeValidationSettings.h"

class I3dPipelineProfile;
class Series;

class ThreeDSeriesBuildService
{
public:
    explicit ThreeDSeriesBuildService(VolumeValidationSettings validationSettings = {});

    [[nodiscard]] ThreeDimensionalPipelineResult buildFromDiagnosticSeries(
        const Series& diagnosticSeries,
        const I3dPipelineProfile& profile) const;

private:
    VolumeValidationSettings m_validationSettings;
};
