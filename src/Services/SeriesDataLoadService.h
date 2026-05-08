#pragma once

#include "Errors/AppResult.h"
#include "Model/DicomParameters.h"

class FileHandling;
class IAuditService;
class DicomImage;

/**
 * @brief Loads full diagnostic image data for a lightweight DICOM series.
 *
 * Responsibilities:
 * - Reload missing raw pixels from source DICOM file paths.
 * - Preserve series metadata while preparing image data for analysis.
 * - Share immutable metadata references across loaded instances.
 *
 * Assumptions:
 * - Lightweight series entries contain valid source file paths.
 */
class SeriesDataLoadService
{
public:
    /**
     * @brief Creates the series data loader.
     * @param fileHandling DICOM file loader.
     * @param auditService Optional audit service.
     */
    explicit SeriesDataLoadService(
        const FileHandling& fileHandling,
        IAuditService* auditService = nullptr);

    /**
     * @brief Loads diagnostic image data for a series.
     * @param lightweightSeries Series with file references.
     * @return Loaded diagnostic series or structured error.
     */
    [[nodiscard]] AppResult<Series> loadDiagnosticSeries(const Series& lightweightSeries) const;

    /**
     * @brief Checks whether every image in a series has diagnostic data loaded.
     * @param series Series to inspect.
     * @return True when image data is already loaded.
     */
    [[nodiscard]] bool isDiagnosticSeriesLoaded(const Series& series) const;

private:
    static bool isImageDataLoaded(const DicomImage& image);
    static void copySeriesMetadata(const Series& source, Series& target);
    static void shareSeriesMetadataAcrossImages(Series& series);

private:
    const FileHandling& m_fileHandling;
    IAuditService* m_auditService{nullptr};
};
