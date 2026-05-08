#pragma once

#include "Errors/AppResult.h"
#include "Utilities/VolumeValidationSettings.h"

#include <memory>

class FileHandling;
class IAuditService;
class IVolumeData;
class Series;
class VolumeBuilder;

/**
 * @brief Builds validated diagnostic volumes for advanced viewers.
 *
 * Responsibilities:
 * - Reload lightweight series slices into diagnostic image data.
 * - Build volume data with geometry validation.
 * - Emit audit information for metadata suitability.
 *
 * Assumptions:
 * - Input series may contain lightweight metadata only.
 * - Volume construction requires coherent spacing/orientation metadata.
 */
class AdvancedSeriesVolumeService
{
public:
    /**
     * @brief Creates the service.
     * @param fileHandling DICOM loader used to load missing image data.
     * @param validationSettings Geometry validation tolerances.
     * @param auditService Optional audit service for metadata findings.
     */
    AdvancedSeriesVolumeService(
        const FileHandling& fileHandling,
        VolumeValidationSettings validationSettings = {},
        IAuditService* auditService = nullptr);
    ~AdvancedSeriesVolumeService();

    /**
     * @brief Builds a diagnostic volume from a series.
     * @param lightweightSeries Series metadata and image references.
     * @return Volume data or structured error.
     */
    [[nodiscard]] AppResult<std::shared_ptr<IVolumeData>> buildDiagnosticVolume(const Series& lightweightSeries) const;

private:
    const FileHandling& m_fileHandling;
    IAuditService* m_auditService{nullptr};
    std::unique_ptr<VolumeBuilder> m_volumeBuilder;
};
