#pragma once

#include "Errors/AppResult.h"
#include "Services/ThreeDimensionalPipelineResult.h"
#include "Utilities/VolumeValidationSettings.h"

class I3dPipelineProfile;
class Series;

class ThreeDSeriesBuildService
{
public:
    explicit ThreeDSeriesBuildService(VolumeValidationSettings validationSettings = {});

    [[nodiscard]] AppResult<ThreeDimensionalPipelineResult> buildFromDiagnosticSeries(
        const Series& diagnosticSeries,
        const I3dPipelineProfile& profile) const;

private:
    VolumeValidationSettings m_validationSettings;
};
