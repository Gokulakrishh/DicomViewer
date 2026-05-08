#pragma once

#include "Errors/AppResult.h"
#include "Services/ThreeDimensionalPipelineResult.h"
#include "Utilities/VolumeValidationSettings.h"

class I3dPipelineProfile;
class Series;

/**
 * @brief Builds 3D pipeline outputs from a loaded diagnostic DICOM series.
 *
 * Responsibilities:
 * - Convert a validated series to volume data.
 * - Run a selected 3D pipeline profile.
 */
class ThreeDSeriesBuildService
{
public:
    /**
     * @brief Creates the service.
     * @param validationSettings Volume geometry validation settings.
     */
    explicit ThreeDSeriesBuildService(VolumeValidationSettings validationSettings = {});

    /**
     * @brief Builds 3D outputs from a diagnostic series.
     * @param diagnosticSeries Series with loaded raw pixels.
     * @param profile 3D reconstruction profile.
     * @return Pipeline result or structured error.
     */
    [[nodiscard]] AppResult<ThreeDimensionalPipelineResult> buildFromDiagnosticSeries(
        const Series& diagnosticSeries,
        const I3dPipelineProfile& profile) const;

private:
    VolumeValidationSettings m_validationSettings;
};
