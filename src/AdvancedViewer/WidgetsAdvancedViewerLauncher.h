#pragma once

#include "IAdvancedViewerLauncher.h"

#include <memory>
#include <QWidget> 

/**
 * @brief QWidget implementation of advanced viewer launching.
 *
 * Responsibilities:
 * - Create and show concrete MPR and 3D windows.
 * - Keep construction details outside DicomMainWindow.
 *
 * Assumptions:
 * - Volumes passed into this launcher outlive viewer construction and are shared
 *   through std::shared_ptr.
 */
class WidgetsAdvancedViewerLauncher final : public IAdvancedViewerLauncher
{
public:
    /**
     * @brief Opens a QWidget-based MPR viewer.
     * @param volume Volume data to inspect.
     * @param title Window title.
     * @param windowLevel Initial DICOM window level.
     * @param windowWidth Initial DICOM window width.
     * @param dicomWindowPresets Optional DICOM-provided WL/WW presets.
     * @param activeDicomWindowPresetIndex Active DICOM preset index, or -1.
     * @param parent Optional parent widget.
     * @return Created MPR viewer widget.
     */
    QWidget* showMprVolume(
        std::shared_ptr<IVolumeData> volume,
        const QString& title,
        int windowLevel,
        int windowWidth,
        std::vector<DicomWindowPreset> dicomWindowPresets = {},
        int activeDicomWindowPresetIndex = -1,
        QWidget* parent = nullptr) override;

    /**
     * @brief Opens a QWidget-based 3D viewer.
     * @param diagnosticVolume Volume data to render.
     * @param title Window title.
     * @param profileSelection Initial 3D pipeline profile selection.
     * @param parent Optional parent widget.
     * @return Created 3D viewer widget.
     */
    QWidget* showThreeDVolume(
        std::shared_ptr<IVolumeData> diagnosticVolume,
        const QString& title,
        ThreeDProfileSelection profileSelection,
        QWidget* parent = nullptr) override;
};
