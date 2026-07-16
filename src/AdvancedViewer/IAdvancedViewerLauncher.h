#pragma once

#include "Services/ThreeDProfiles/ThreeDProfileSelection.h"
#include "Model/DicomMetadata.h"

#include <QString>
#include <QWidget> 
#include <memory>
#include <vector>

class IVolumeData;
class QWidget;
class DatabaseService;

/**
 * @brief Interface for opening advanced viewers from the main DICOM window.
 *
 * Responsibilities:
 * - Launch MPR and 3D volume windows without coupling the main window to their
 *   concrete widget classes.
 * - Pass lightweight volume data and display parameters across viewer modules.
 *
 * Assumptions:
 * - The diagnostic volume is already built and validated by the caller.
 * - Viewer windows remain independently interactive after launch.
 */
class IAdvancedViewerLauncher
{
public:
    virtual ~IAdvancedViewerLauncher() = default;

    /**
     * @brief Opens an MPR viewer for a diagnostic volume.
     * @param volume Volume data to inspect.
     * @param title Window title shown to the user.
     * @param windowLevel Initial DICOM window level.
     * @param windowWidth Initial DICOM window width.
     * @param dicomWindowPresets Optional DICOM-provided WL/WW presets.
     * @param activeDicomWindowPresetIndex Active DICOM preset index, or -1.
     * @param seriesInstanceUid Active DICOM Series Instance UID for derived MPR annotations.
     * @param databaseService Optional persistence service for MPR annotations.
     * @param parent Optional Qt parent widget.
     * @return Created viewer widget, owned according to Qt parent/window rules.
     */
    virtual QWidget* showMprVolume(
        std::shared_ptr<IVolumeData> volume,
        const QString& title,
        int windowLevel,
        int windowWidth,
        std::vector<DicomWindowPreset> dicomWindowPresets = {},
        int activeDicomWindowPresetIndex = -1,
        const QString& seriesInstanceUid = {},
        DatabaseService* databaseService = nullptr,
        QWidget* parent = nullptr) = 0;

    /**
     * @brief Opens a 3D volume viewer.
     * @param diagnosticVolume Volume data to render.
     * @param title Window title shown to the user.
     * @param profileSelection Initial 3D pipeline profile selection.
     * @param parent Optional Qt parent widget.
     * @return Created viewer widget, owned according to Qt parent/window rules.
     */
    virtual QWidget* showThreeDVolume(
        std::shared_ptr<IVolumeData> diagnosticVolume,
        const QString& title,
        ThreeDProfileSelection profileSelection,
        QWidget* parent = nullptr) = 0;
};
