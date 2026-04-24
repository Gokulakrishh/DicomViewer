#pragma once

#include "Errors/AppResult.h"
#include "Model/DicomParameters.h"

class FileHandling;
class DicomImage;

class SeriesDataLoadService
{
public:
    explicit SeriesDataLoadService(const FileHandling& fileHandling);

    [[nodiscard]] AppResult<Series> loadDiagnosticSeries(const Series& lightweightSeries) const;
    [[nodiscard]] bool isDiagnosticSeriesLoaded(const Series& series) const;

private:
    static bool isImageDataLoaded(const DicomImage& image);
    static void copySeriesMetadata(const Series& source, Series& target);

private:
    const FileHandling& m_fileHandling;
};
