#pragma once

#include <QFutureWatcher>
#include <QHash>
#include <QList>
#include <QObject>
#include <QSet>
#include <QString>
#include <QThreadPool>
#include <memory>

#include "ViewportSession.h"

class DicomImage;
class FileHandling;
class Series;
class CineFrameCache;

/**
 * @brief Session controller for the main diagnostic slice viewport.
 *
 * Responsibilities:
 * - Track active series/slice and viewport windowing state.
 * - Load DICOM image pixels on demand through FileHandling.
 * - Preload nearby slices and evict raw pixels outside a bounded cache radius.
 *
 * Assumptions:
 * - The controller owns no UI widgets; it prepares data for the view.
 * - Large pixel buffers must not remain loaded for all slices in large series.
 */
class DicomViewportController : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Window/level UI constraints and current values.
     *
     * The state combines DICOM defaults, built-in presets, and current user
     * adjustments for the active image.
     */
    struct WindowControlState
    {
        bool hasImage{false};
        int levelMin{-100};
        int levelMax{100};
        int widthMin{10};
        int widthMax{300};
        int level{0};
        int width{100};
        ViewportWindowPreset preset{ViewportWindowPreset::Custom};
        int dicomPresetIndex{-1};
    };

    /**
     * @brief Creates a viewport controller.
     * @param fileHandling DICOM file loader used on demand; not owned.
     * @param parent Optional Qt parent.
     */
    explicit DicomViewportController(
        FileHandling* fileHandling,
        QObject* parent = nullptr);
    ~DicomViewportController() override;

    /**
     * @brief Clears the active series/image and pending preload state.
     */
    void clear();

    /**
     * @brief Ensures raw image pixels are loaded for an image.
     * @param image Image metadata/pixel container to load.
     * @return True when image data is available.
     */
    bool ensureImageLoaded(DicomImage& image);

    /**
     * @brief Sets the active DICOM series.
     * @param series Series metadata and slice list.
     * @param initialIndex Initial slice index.
     */
    void setSeries(const std::shared_ptr<Series>& series, int initialIndex = 0);

    /**
     * @brief Sets a single active image outside a loaded series.
     * @param image Image to display.
     */
    void setSingleImage(const std::shared_ptr<DicomImage>& image);

    /**
     * @brief Returns the active series.
     * @return Current series, or null for single-image mode/no image.
     */
    std::shared_ptr<Series> currentSeries() const;

    /**
     * @brief Returns the mutable current image.
     * @return Current image, or null when none is active.
     */
    DicomImage* currentImage();

    /**
     * @brief Returns the current image.
     * @return Current image, or null when none is active.
     */
    const DicomImage* currentImage() const;

    /**
     * @brief Reports whether the current series supports cine playback.
     * @return True when multiple slices are available.
     */
    bool hasPlayableSeries() const;

    /**
     * @brief Returns the current image count.
     * @return Number of images in active series or one for single-image mode.
     */
    int imageCount() const;

    /**
     * @brief Returns the active image index.
     * @return Zero-based index, or -1 when no image is active.
     */
    int currentImageIndex() const;

    /**
     * @brief Sets the active image index.
     * @param index Desired zero-based slice index.
     */
    void setCurrentImageIndex(int index);

    /**
     * @brief Calculates a clamped index after wheel/step navigation.
     * @param stepCount Relative step count.
     * @return Valid image index.
     */
    int clampedIndexWithStep(int stepCount) const;

    /**
     * @brief Calculates the next cine playback index.
     * @return Next valid image index.
     */
    int nextCineIndex() const;

    /**
     * @brief Loads/prepares the active image for rendering.
     * @param cinePlaying True when called during cine playback.
     * @param errorMessage Optional destination for load failure text.
     * @return True when the current image can be rendered.
     */
    bool prepareCurrentSeriesImage(bool cinePlaying, QString* errorMessage = nullptr);

    /**
     * @brief Returns current WL/WW limits and values.
     * @param resetWindowState True to reinitialize from active image metadata.
     * @return Window control state for UI synchronization.
     */
    WindowControlState windowControlState(bool resetWindowState);

    /**
     * @brief Renders the current image using diagnostic WL/WW settings.
     * @return Rendered image object, or null when no image is active.
     */
    std::shared_ptr<DicomImage> renderCurrentDiagnosticImage() const;

    /**
     * @brief Sets the current window level.
     * @param value Window level value.
     */
    void setWindowLevel(int value);

    /**
     * @brief Sets the current window width.
     * @param value Window width value.
     */
    void setWindowWidth(int value);

    /**
     * @brief Returns current window level.
     * @return Window level value.
     */
    int currentWindowLevel() const;

    /**
     * @brief Returns current window width.
     * @return Window width value.
     */
    int currentWindowWidth() const;

    /**
     * @brief Returns the active built-in/custom preset.
     * @return Current viewport window preset.
     */
    ViewportWindowPreset currentPreset() const;

    /**
     * @brief Returns the active DICOM window preset index.
     * @return DICOM preset index, or -1 when not active.
     */
    int currentDicomWindowPresetIndex() const;

    /**
     * @brief Resets the window preset to custom/current image defaults.
     */
    void resetPreset();

    /**
     * @brief Applies a built-in viewport window preset.
     * @param preset Preset to apply.
     * @return True when the preset was applied.
     */
    bool applyPreset(ViewportWindowPreset preset);

    /**
     * @brief Returns number of DICOM-provided window presets.
     * @return Preset count for the current image.
     */
    int dicomWindowPresetCount() const;

    /**
     * @brief Returns the display label for a DICOM window preset.
     * @param index Preset index.
     * @return User-visible preset label.
     */
    QString dicomWindowPresetLabel(int index) const;

    /**
     * @brief Applies a DICOM-provided window preset.
     * @param index Preset index.
     * @return True when the preset was applied.
     */
    bool applyDicomWindowPreset(int index);

    /**
     * @brief Enables or disables cine playback state.
     * @param playing True when cine is active.
     */
    void setCinePlaying(bool playing);

    /**
     * @brief Reports current cine playback state.
     * @return True when cine is active.
     */
    bool isCinePlaying() const;

    /**
     * @brief Returns the preferred cine timer interval for the current image.
     * @return Clamped timer interval in milliseconds.
     */
    int cineIntervalMs() const;

    /**
     * @brief Returns the immutable viewport session.
     * @return Current viewport session state.
     */
    const ViewportSession& session() const;

    /**
     * @brief Temporarily disables raw pixel cache eviction.
     * @param suspended True to suspend eviction.
     */
    void suspendRawPixelEviction(bool suspended);

private:
    struct PreloadedSliceBatch
    {
        QSet<int> requestedSeriesIndices;
        QHash<int, std::shared_ptr<DicomImage>> imagesBySeriesIndex;
        QString fullCacheFilePath;
    };

    void cancelPendingPreloads();
    void applyPreloadedSlice(int index, int generation, const std::shared_ptr<DicomImage>& loadedImage);
    void applyPreloadedSliceBatch(int generation, const PreloadedSliceBatch& loadedBatch);
    void requestSlicePreload(int index);
    void requestSlicePreloadBatch(const QList<int>& indices);
    void requestFullMultiframeCache(const DicomImage& image);
    [[nodiscard]] bool isFullMultiframeCacheEligible(const DicomImage& image) const;
    [[nodiscard]] bool isFullMultiframeCacheReady(const DicomImage& image) const;
    [[nodiscard]] QList<int> missingFrameIndicesForFile(const QString& filePath) const;
    [[nodiscard]] std::size_t estimatedFullMultiframeCacheBytes(const DicomImage& image) const;
    void scheduleSlicePreload(bool cinePlaying);
    void enforceRawPixelCache();
    [[nodiscard]] int nextLoadedCineIndex() const;

private:
    static constexpr int kRawPixelCacheRadius = 10;
    static constexpr int kCineRawPixelCacheRadius = 18;
    static constexpr int kMinimumCineIntervalMs = 20;
    static constexpr int kMaximumCineIntervalMs = 1000;
    static constexpr std::size_t kMaxFullMultiframeCacheBytes = 512ULL * 1024ULL * 1024ULL;
    FileHandling* m_fileHandling{nullptr};
    std::unique_ptr<CineFrameCache> m_cineFrameCache;
    QThreadPool m_cineDecodeThreadPool;
    ViewportSession m_session;
    QSet<int> m_pendingPreloadIndices;
    QString m_pendingFullCineCacheFilePath;
    QString m_activeFullCineCacheFilePath;
    QList<QFutureWatcher<std::shared_ptr<DicomImage>>*> m_preloadWatchers;
    QList<QFutureWatcher<PreloadedSliceBatch>*> m_batchPreloadWatchers;
    bool m_rawPixelEvictionSuspended{false};
};
