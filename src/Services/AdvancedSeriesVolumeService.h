#pragma once

#include "Errors/AppResult.h"
#include "Utilities/VolumeValidationSettings.h"

#include <memory>

class FileHandling;
class IVolumeData;
class Series;
class VolumeBuilder;

class AdvancedSeriesVolumeService
{
public:
    AdvancedSeriesVolumeService(
        const FileHandling& fileHandling,
        VolumeValidationSettings validationSettings = {});
    ~AdvancedSeriesVolumeService();

    [[nodiscard]] AppResult<std::shared_ptr<IVolumeData>> buildDiagnosticVolume(const Series& lightweightSeries) const;

private:
    const FileHandling& m_fileHandling;
    std::unique_ptr<VolumeBuilder> m_volumeBuilder;
};
