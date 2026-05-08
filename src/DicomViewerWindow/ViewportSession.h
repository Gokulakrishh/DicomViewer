#pragma once

#include <memory>

class DicomImage;
class Series;

/**
 * @brief Built-in window/level presets available in the viewport.
 *
 * DICOM-provided presets are tracked separately by index so the built-in set can
 * remain stable and centrally reusable.
 */
enum class ViewportWindowPreset
{
    Custom = 0,
    Brain,
    SoftTissue,
    Bone,
    Lung
};

/**
 * @brief Mutable session state for the main DICOM viewport.
 *
 * Responsibilities:
 * - Store active series/image selection.
 * - Store current windowing and cine state.
 * - Track a generation counter for asynchronous preload validity.
 *
 * Assumptions:
 * - The session is owned by DicomViewportController and not shared directly for
 *   mutation outside that controller.
 */
class ViewportSession
{
public:
    /**
     * @brief Resets the viewport session to an empty state.
     */
    void clear();

    std::shared_ptr<Series> currentSeries;
    std::shared_ptr<DicomImage> singleImage;
    int currentImageIndex{-1};
    int currentWindowLevel{0};
    int currentWindowWidth{100};
    ViewportWindowPreset currentPreset{ViewportWindowPreset::Custom};
    int currentDicomWindowPresetIndex{-1};
    bool windowStateInitialized{false};
    int seriesGeneration{0};
    bool cinePlaying{false};
};
