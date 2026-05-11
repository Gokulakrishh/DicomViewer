#pragma once

#include "Errors/AppResult.h"
#include "Model/VolumeBuildResult.h"

#include <memory>

class FileHandling;
class IAuditService;
class Series;
class VolumeBuilder;

/**
 * @brief Builds validated diagnostic volumes for advanced viewers.
 *
 * Responsibilities:
 * - Reload lightweight series slices into diagnostic image data.
 * - Build volume data and return non-blocking geometry warnings.
 * - Emit audit information for metadata suitability.
 *
 * Assumptions:
 * - Input series may contain lightweight metadata only.
 * - Some spacing/orientation metadata issues allow approximate viewing and are
 *   returned to the UI for explicit user confirmation.
 */
class AdvancedSeriesVolumeService
{
public:
    /**
     * @brief Creates the service.
     * @param fileHandling DICOM loader used to load missing image data.
     * @param auditService Optional audit service for metadata findings.
     */
    AdvancedSeriesVolumeService(
        const FileHandling& fileHandling,
        IAuditService* auditService = nullptr);
    ~AdvancedSeriesVolumeService();

    /**
     * @brief Builds a diagnostic volume from a series.
     * @param lightweightSeries Series metadata and image references.
     * @return Volume data with warnings, or structured error.
     */
    [[nodiscard]] AppResult<VolumeBuildResult> buildDiagnosticVolume(const Series& lightweightSeries) const;

private:
    const FileHandling& m_fileHandling;
    IAuditService* m_auditService{nullptr};
    std::unique_ptr<VolumeBuilder> m_volumeBuilder;
};
