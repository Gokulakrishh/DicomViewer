#pragma once

#include "Errors/AppResult.h"
#include "Model/DicomParameters.h"

class FileHandling;
class IAuditService;
class DicomImage;

class SeriesDataLoadService
{
public:
    explicit SeriesDataLoadService(
        const FileHandling& fileHandling,
        IAuditService* auditService = nullptr);

    [[nodiscard]] AppResult<Series> loadDiagnosticSeries(const Series& lightweightSeries) const;
    [[nodiscard]] bool isDiagnosticSeriesLoaded(const Series& series) const;

private:
    static bool isImageDataLoaded(const DicomImage& image);
    static void copySeriesMetadata(const Series& source, Series& target);
    static void shareSeriesMetadataAcrossImages(Series& series);

private:
    const FileHandling& m_fileHandling;
    IAuditService* m_auditService{nullptr};
};
