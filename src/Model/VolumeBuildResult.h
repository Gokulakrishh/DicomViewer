#pragma once

#include "Model/IVolumeData.h"

#include <QStringList>

#include <memory>

/**
 * @brief Result of building a diagnostic volume.
 *
 * Responsibilities:
 * - Carry the constructed volume for MPR/3D workflows.
 * - Carry non-blocking DICOM geometry warnings that require user awareness
 *   before opening an advanced viewer.
 *
 * Assumptions:
 * - Warnings describe reduced confidence in derived geometry, not a failure to
 *   allocate or construct the volume.
 */
struct VolumeBuildResult
{
    std::shared_ptr<IVolumeData> volume;
    QStringList warnings;

    /**
     * @brief Reports whether any non-blocking geometry warnings were produced.
     * @return True when warnings is not empty.
     */
    [[nodiscard]] bool hasWarnings() const
    {
        return !warnings.isEmpty();
    }
};
