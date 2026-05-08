#include "DicomViewportController.h"

#include <QFileInfo>
#include <QDebug>
#include <QDateTime>
#include <QtConcurrent/QtConcurrent>
#include <algorithm>
#include <cmath>

#include "FileHandling/FileHandling.h"
#include "FileHandling/GDCMFileHandling.h"
#include "Model/DicomImage.h"
#include "Model/MedicalImage.h"
#include "Model/DicomParameters.h"
#include "Services/CineFrameCache.h"
#include "Utilities/DiagnosticImageRenderer.h"
#include "ViewerTools/WindowLevelPreset.h"

namespace
{
bool builtInPresetIdForViewportPreset(ViewportWindowPreset preset, BuiltInWindowLevelPresetId& presetId)
{
    switch (preset)
    {
    case ViewportWindowPreset::Brain:
        presetId = BuiltInWindowLevelPresetId::Brain;
        return true;
    case ViewportWindowPreset::SoftTissue:
        presetId = BuiltInWindowLevelPresetId::SoftTissue;
        return true;
    case ViewportWindowPreset::Bone:
        presetId = BuiltInWindowLevelPresetId::Bone;
        return true;
    case ViewportWindowPreset::Lung:
        presetId = BuiltInWindowLevelPresetId::Lung;
        return true;
    case ViewportWindowPreset::Custom:
        return false;
    }

    return false;
}
}

DicomViewportController::DicomViewportController(
    FileHandling* fileHandling,
    QObject* parent)
    : QObject(parent),
      m_fileHandling(fileHandling),
      m_cineFrameCache(std::make_unique<CineFrameCache>())
{
    m_cineDecodeThreadPool.setMaxThreadCount(1);
    m_cineDecodeThreadPool.setExpiryTimeout(-1);
}

DicomViewportController::~DicomViewportController()
{
    cancelPendingPreloads();
}

void DicomViewportController::clear()
{
    ++m_session.seriesGeneration;
    cancelPendingPreloads();
    m_session.clear();
    m_rawPixelEvictionSuspended = false;
    m_cineFrameCache->reset();
    m_pendingFullCineCacheFilePath.clear();
    m_activeFullCineCacheFilePath.clear();
}

bool DicomViewportController::ensureImageLoaded(DicomImage& image)
{
    if (image.hasRawPixels())
    {
        return true;
    }

    const bool canReloadDicom = m_fileHandling && m_fileHandling->canLoad(image.filePath());
    if (image.isValid() && !canReloadDicom)
    {
        return true;
    }

    if (!canReloadDicom)
    {
        return false;
    }

    std::unique_ptr<DicomImage> loadedImage = m_fileHandling->loadImageData(image.filePath(), image.frameIndex());
    if (!loadedImage || !loadedImage->isValid())
    {
        return false;
    }

    image = *loadedImage;
    return image.isValid();
}

void DicomViewportController::setSeries(const std::shared_ptr<Series>& series, int initialIndex)
{
    m_session.singleImage.reset();
    m_session.currentSeries = series;
    ++m_session.seriesGeneration;
    cancelPendingPreloads();
    m_cineFrameCache->reset();
    m_pendingFullCineCacheFilePath.clear();
    m_activeFullCineCacheFilePath.clear();

    const int count = imageCount();
    m_session.currentImageIndex = count > 0 ? std::clamp(initialIndex, 0, count - 1) : -1;
    m_session.windowStateInitialized = false;
    m_session.currentPreset = ViewportWindowPreset::Custom;
    m_session.cinePlaying = false;
}

void DicomViewportController::setSingleImage(const std::shared_ptr<DicomImage>& image)
{
    ++m_session.seriesGeneration;
    cancelPendingPreloads();
    m_cineFrameCache->reset();
    m_pendingFullCineCacheFilePath.clear();
    m_activeFullCineCacheFilePath.clear();
    m_session.currentSeries.reset();
    m_session.singleImage = image;
    m_session.currentImageIndex = -1;
    m_session.windowStateInitialized = false;
    m_session.currentPreset = ViewportWindowPreset::Custom;
    m_session.cinePlaying = false;
}

std::shared_ptr<Series> DicomViewportController::currentSeries() const
{
    return m_session.currentSeries;
}

DicomImage* DicomViewportController::currentImage()
{
    if (m_session.singleImage)
    {
        return m_session.singleImage.get();
    }

    if (!m_session.currentSeries)
    {
        return nullptr;
    }

    auto& images = m_session.currentSeries->images();
    if (m_session.currentImageIndex < 0 || m_session.currentImageIndex >= static_cast<int>(images.size()))
    {
        return nullptr;
    }

    auto& currentImage = images[static_cast<std::size_t>(m_session.currentImageIndex)];
    return currentImage ? currentImage.get() : nullptr;
}

const DicomImage* DicomViewportController::currentImage() const
{
    return const_cast<DicomViewportController*>(this)->currentImage();
}

bool DicomViewportController::hasPlayableSeries() const
{
    return m_session.currentSeries && imageCount() > 1;
}

int DicomViewportController::imageCount() const
{
    return m_session.currentSeries ? static_cast<int>(m_session.currentSeries->images().size()) : 0;
}

int DicomViewportController::currentImageIndex() const
{
    return m_session.currentImageIndex;
}

void DicomViewportController::setCurrentImageIndex(int index)
{
    m_session.currentImageIndex = index;
    enforceRawPixelCache();
}

int DicomViewportController::clampedIndexWithStep(int stepCount) const
{
    if (!m_session.currentSeries || stepCount == 0)
    {
        return m_session.currentImageIndex;
    }

    return std::clamp(m_session.currentImageIndex + stepCount, 0, imageCount() - 1);
}

int DicomViewportController::nextCineIndex() const
{
    const int loadedIndex = nextLoadedCineIndex();
    if (loadedIndex >= 0)
    {
        return loadedIndex;
    }
    if (m_session.cinePlaying)
    {
        return m_session.currentImageIndex;
    }

    const int count = imageCount();
    if (count <= 1)
    {
        return m_session.currentImageIndex;
    }

    return (m_session.currentImageIndex + 1) % count;
}

bool DicomViewportController::prepareCurrentSeriesImage(bool cinePlaying, QString* errorMessage)
{
    m_session.cinePlaying = cinePlaying;

    if (!m_session.currentSeries)
    {
        return false;
    }

    auto& images = m_session.currentSeries->images();
    if (images.empty() || m_session.currentImageIndex < 0 ||
        m_session.currentImageIndex >= static_cast<int>(images.size()) ||
        !images[static_cast<std::size_t>(m_session.currentImageIndex)])
    {
        return false;
    }

    auto& currentImage = images[static_cast<std::size_t>(m_session.currentImageIndex)];
    if (cinePlaying && !currentImage->hasRawPixels())
    {
        scheduleSlicePreload(true);
        return false;
    }

    if (!ensureImageLoaded(*currentImage))
    {
        if (errorMessage)
        {
            *errorMessage = currentImage->filePath();
        }
        return false;
    }

    if (currentImage->hasRawPixels())
    {
        m_cineFrameCache->recordAllocatedFrame(m_session.currentImageIndex, *currentImage);
    }
    scheduleSlicePreload(cinePlaying);
    return true;
}

DicomViewportController::WindowControlState DicomViewportController::windowControlState(bool resetWindowState)
{
    WindowControlState state;
    const DicomImage* displayedImage = currentImage();
    state.hasImage = displayedImage && displayedImage->isValid();
    if (!state.hasImage)
    {
        return state;
    }

    if (displayedImage->hasRawPixels())
    {
        state.levelMin = displayedImage->minimumStoredValue();
        state.levelMax = displayedImage->maximumStoredValue();
        state.widthMin = 1;
        state.widthMax = std::max(1, state.levelMax - state.levelMin);

        if (resetWindowState || !m_session.windowStateInitialized)
        {
            m_session.currentWindowLevel = displayedImage->defaultWindowLevel();
            m_session.currentWindowWidth = std::max(1, displayedImage->defaultWindowWidth());
            m_session.currentPreset = ViewportWindowPreset::Custom;
            m_session.currentDicomWindowPresetIndex =
                displayedImage->metadata() && !displayedImage->metadata()->windowPresets.empty() ? 0 : -1;
            m_session.windowStateInitialized = true;
        }
        else
        {
            if (m_session.currentDicomWindowPresetIndex >= 0 && displayedImage->metadata() &&
                m_session.currentDicomWindowPresetIndex <
                    static_cast<int>(displayedImage->metadata()->windowPresets.size()))
            {
                const auto& preset =
                    displayedImage->metadata()->windowPresets[static_cast<std::size_t>(m_session.currentDicomWindowPresetIndex)];
                if (preset.width > 0.0)
                {
                    m_session.currentWindowLevel = static_cast<int>(std::lround(preset.center));
                    m_session.currentWindowWidth = static_cast<int>(std::lround(preset.width));
                }
            }
            m_session.currentWindowLevel = std::clamp(m_session.currentWindowLevel, state.levelMin, state.levelMax);
            m_session.currentWindowWidth = std::clamp(m_session.currentWindowWidth, state.widthMin, state.widthMax);
        }
    }
    else
    {
        state.levelMin = -100;
        state.levelMax = 100;
        state.widthMin = 10;
        state.widthMax = 300;
        if (resetWindowState || !m_session.windowStateInitialized)
        {
            m_session.currentWindowLevel = 0;
            m_session.currentWindowWidth = 100;
            m_session.currentPreset = ViewportWindowPreset::Custom;
            m_session.currentDicomWindowPresetIndex = -1;
            m_session.windowStateInitialized = true;
        }
    }

    state.level = m_session.currentWindowLevel;
    state.width = m_session.currentWindowWidth;
    state.preset = m_session.currentPreset;
    state.dicomPresetIndex = m_session.currentDicomWindowPresetIndex;
    return state;
}

std::shared_ptr<DicomImage> DicomViewportController::renderCurrentDiagnosticImage() const
{
    const DicomImage* displayedImage = currentImage();
    if (!displayedImage || !displayedImage->isValid())
    {
        return {};
    }

    auto diagnosticImageModel = std::make_shared<DicomImage>(*displayedImage);
    if (displayedImage->hasRawPixels() && displayedImage->isMonochrome())
    {
        return renderDiagnosticImage(
            *displayedImage,
            m_session.currentWindowLevel,
            m_session.currentWindowWidth);
    }

    return diagnosticImageModel;
}

void DicomViewportController::setWindowLevel(int value)
{
    m_session.currentWindowLevel = value;
}

void DicomViewportController::setWindowWidth(int value)
{
    m_session.currentWindowWidth = value;
}

int DicomViewportController::currentWindowLevel() const
{
    return m_session.currentWindowLevel;
}

int DicomViewportController::currentWindowWidth() const
{
    return m_session.currentWindowWidth;
}

ViewportWindowPreset DicomViewportController::currentPreset() const
{
    return m_session.currentPreset;
}

int DicomViewportController::currentDicomWindowPresetIndex() const
{
    return m_session.currentDicomWindowPresetIndex;
}

void DicomViewportController::resetPreset()
{
    m_session.currentPreset = ViewportWindowPreset::Custom;
    m_session.currentDicomWindowPresetIndex = -1;
}

bool DicomViewportController::applyPreset(ViewportWindowPreset preset)
{
    const DicomImage* displayedImage = currentImage();
    if (!displayedImage || !displayedImage->hasRawPixels())
    {
        return false;
    }

    const int minimumValue = displayedImage->minimumStoredValue();
    const int maximumValue = displayedImage->maximumStoredValue();

    BuiltInWindowLevelPresetId presetId;
    if (!builtInPresetIdForViewportPreset(preset, presetId))
    {
        return false;
    }

    const auto presetValues = windowLevelPreset(presetId);
    m_session.currentWindowLevel = std::clamp(presetValues.level, minimumValue, maximumValue);
    m_session.currentWindowWidth = std::clamp(presetValues.width, 1, std::max(1, maximumValue - minimumValue));
    m_session.currentPreset = preset;
    m_session.currentDicomWindowPresetIndex = -1;
    return true;
}

int DicomViewportController::dicomWindowPresetCount() const
{
    const DicomImage* displayedImage = currentImage();
    if (!displayedImage || !displayedImage->metadata())
    {
        return 0;
    }

    return static_cast<int>(displayedImage->metadata()->windowPresets.size());
}

QString DicomViewportController::dicomWindowPresetLabel(int index) const
{
    const DicomImage* displayedImage = currentImage();
    if (!displayedImage || !displayedImage->metadata() || index < 0 ||
        index >= static_cast<int>(displayedImage->metadata()->windowPresets.size()))
    {
        return {};
    }

    const auto& preset = displayedImage->metadata()->windowPresets[static_cast<std::size_t>(index)];
    const QString explanation = preset.explanation.trimmed();
    const QString prefix = explanation.isEmpty() ? QString("DICOM %1").arg(index + 1) : QString("DICOM: %1").arg(explanation);
    return QString("%1 (%2/%3)")
        .arg(prefix)
        .arg(static_cast<int>(std::lround(preset.center)))
        .arg(static_cast<int>(std::lround(preset.width)));
}

bool DicomViewportController::applyDicomWindowPreset(int index)
{
    const DicomImage* displayedImage = currentImage();
    if (!displayedImage || !displayedImage->hasRawPixels() || !displayedImage->metadata() || index < 0 ||
        index >= static_cast<int>(displayedImage->metadata()->windowPresets.size()))
    {
        return false;
    }

    const auto& preset = displayedImage->metadata()->windowPresets[static_cast<std::size_t>(index)];
    if (preset.width <= 0.0)
    {
        return false;
    }

    const int minimumValue = displayedImage->minimumStoredValue();
    const int maximumValue = displayedImage->maximumStoredValue();
    m_session.currentWindowLevel = std::clamp(
        static_cast<int>(std::lround(preset.center)),
        minimumValue,
        maximumValue);
    m_session.currentWindowWidth = std::clamp(
        static_cast<int>(std::lround(preset.width)),
        1,
        std::max(1, maximumValue - minimumValue));
    m_session.currentPreset = ViewportWindowPreset::Custom;
    m_session.currentDicomWindowPresetIndex = index;
    return true;
}

void DicomViewportController::setCinePlaying(bool playing)
{
    m_session.cinePlaying = playing;
}

bool DicomViewportController::isCinePlaying() const
{
    return m_session.cinePlaying;
}

int DicomViewportController::cineIntervalMs() const
{
    const DicomImage* image = currentImage();
    if (!image)
    {
        return 100;
    }

    const int intervalMs = std::clamp(
        static_cast<int>(std::lround(image->cineFrameIntervalMs())),
        kMinimumCineIntervalMs,
        kMaximumCineIntervalMs);
    /*qDebug() << "[XACineTiming] frame" << image->frameIndex() + 1
             << "of" << image->frameCount()
             << "| interval ms:" << intervalMs
             << "| DICOM Frame Time:" << image->frameTimeMs()
             << "| DICOM Cine Rate:" << image->cineRateFps();*/
    return intervalMs;
}

const ViewportSession& DicomViewportController::session() const
{
    return m_session;
}

void DicomViewportController::suspendRawPixelEviction(bool suspended)
{
    m_rawPixelEvictionSuspended = suspended;
    if (!m_rawPixelEvictionSuspended)
    {
        enforceRawPixelCache();
    }
}

void DicomViewportController::cancelPendingPreloads()
{
    for (auto* watcher : m_preloadWatchers)
    {
        if (!watcher)
        {
            continue;
        }

        watcher->disconnect(this);
        watcher->cancel();
        watcher->deleteLater();
    }

    m_preloadWatchers.clear();

    for (auto* watcher : m_batchPreloadWatchers)
    {
        if (!watcher)
        {
            continue;
        }

        watcher->disconnect(this);
        watcher->cancel();
        watcher->deleteLater();
    }

    m_batchPreloadWatchers.clear();
    m_pendingPreloadIndices.clear();
    m_pendingFullCineCacheFilePath.clear();
    m_cineDecodeThreadPool.clear();
}

void DicomViewportController::applyPreloadedSlice(int index, int generation, const std::shared_ptr<DicomImage>& loadedImage)
{
    m_pendingPreloadIndices.remove(index);

    if (generation != m_session.seriesGeneration || !m_session.currentSeries || !loadedImage)
    {
        return;
    }

    auto& images = m_session.currentSeries->images();
    if (index < 0 || index >= static_cast<int>(images.size()) || !images[static_cast<std::size_t>(index)])
    {
        return;
    }

    DicomImage& targetImage = *images[static_cast<std::size_t>(index)];
    if (!targetImage.hasRawPixels())
    {
        targetImage = *loadedImage;
    }
    if (targetImage.hasRawPixels())
    {
        m_cineFrameCache->recordAllocatedFrame(index, targetImage);
    }

    enforceRawPixelCache();
}

void DicomViewportController::applyPreloadedSliceBatch(int generation, const PreloadedSliceBatch& loadedBatch)
{
    for (int index : loadedBatch.requestedSeriesIndices)
    {
        m_pendingPreloadIndices.remove(index);
    }
    if (!loadedBatch.fullCacheFilePath.isEmpty() && loadedBatch.fullCacheFilePath == m_pendingFullCineCacheFilePath)
    {
        m_pendingFullCineCacheFilePath.clear();
    }

    if (generation != m_session.seriesGeneration || !m_session.currentSeries)
    {
        return;
    }

    auto& images = m_session.currentSeries->images();
    bool appliedAnyFrame = false;
    for (auto it = loadedBatch.imagesBySeriesIndex.cbegin(); it != loadedBatch.imagesBySeriesIndex.cend(); ++it)
    {
        const int index = it.key();
        const std::shared_ptr<DicomImage>& loadedImage = it.value();
        if (index < 0 || index >= static_cast<int>(images.size()) || !images[static_cast<std::size_t>(index)] ||
            !loadedImage)
        {
            continue;
        }

        DicomImage& targetImage = *images[static_cast<std::size_t>(index)];
        if (!targetImage.hasRawPixels())
        {
            targetImage = *loadedImage;
        }
        if (targetImage.hasRawPixels())
        {
            m_cineFrameCache->recordAllocatedFrame(index, targetImage);
            appliedAnyFrame = true;
        }
    }

    if (appliedAnyFrame && !loadedBatch.fullCacheFilePath.isEmpty())
    {
        m_activeFullCineCacheFilePath = loadedBatch.fullCacheFilePath;
        /*qDebug() << "[CineFrameCache] full active XA cache ready"
                 << "| file:" << QFileInfo(m_activeFullCineCacheFilePath).fileName()
                 << "| frames:" << loadedBatch.imagesBySeriesIndex.size();*/
    }

    enforceRawPixelCache();
}

void DicomViewportController::requestSlicePreload(int index)
{
    if (!m_session.currentSeries)
    {
        return;
    }

    auto& images = m_session.currentSeries->images();
    if (index < 0 || index >= static_cast<int>(images.size()) || !images[static_cast<std::size_t>(index)])
    {
        return;
    }

    DicomImage& image = *images[static_cast<std::size_t>(index)];
    if (image.hasRawPixels() || m_pendingPreloadIndices.contains(index))
    {
        return;
    }

    auto* watcher = new QFutureWatcher<std::shared_ptr<DicomImage>>(this);
    const QString filePath = image.filePath();
    const int frameIndex = image.frameIndex();
    const int generation = m_session.seriesGeneration;
    m_pendingPreloadIndices.insert(index);
    m_preloadWatchers.append(watcher);

    connect(watcher, &QFutureWatcher<std::shared_ptr<DicomImage>>::finished, this, [this, watcher, index, generation]() {
        const std::shared_ptr<DicomImage> loadedImage = watcher->result();
        applyPreloadedSlice(index, generation, loadedImage);
        m_preloadWatchers.removeAll(watcher);
        watcher->deleteLater();
    });

    watcher->setFuture(QtConcurrent::run([filePath, frameIndex]() -> std::shared_ptr<DicomImage> {
        GDCMFileHandling loader;
        std::unique_ptr<DicomImage> loadedImage = loader.loadImageData(filePath, frameIndex);
        if (!loadedImage)
        {
            return {};
        }

        return std::shared_ptr<DicomImage>(loadedImage.release());
    }));
}

void DicomViewportController::requestSlicePreloadBatch(const QList<int>& indices)
{
    if (!m_session.currentSeries || indices.isEmpty())
    {
        return;
    }
    if (!m_batchPreloadWatchers.isEmpty())
    {
        return;
    }

    auto& images = m_session.currentSeries->images();
    QList<int> selectedIndices;
    selectedIndices.reserve(indices.size());
    QString filePath;
    QHash<int, int> frameIndexBySeriesIndex;

    for (int index : indices)
    {
        if (index < 0 || index >= static_cast<int>(images.size()) || !images[static_cast<std::size_t>(index)])
        {
            continue;
        }

        DicomImage& image = *images[static_cast<std::size_t>(index)];
        if (image.hasRawPixels() || m_pendingPreloadIndices.contains(index))
        {
            continue;
        }
        if (filePath.isEmpty())
        {
            filePath = image.filePath();
        }
        if (image.filePath() != filePath)
        {
            continue;
        }

        selectedIndices.append(index);
        frameIndexBySeriesIndex.insert(index, image.frameIndex());
        m_pendingPreloadIndices.insert(index);
    }

    if (selectedIndices.isEmpty() || filePath.isEmpty())
    {
        return;
    }

    auto* watcher = new QFutureWatcher<PreloadedSliceBatch>(this);
    const int generation = m_session.seriesGeneration;
    const QString fullCacheFilePath = filePath == m_pendingFullCineCacheFilePath ? filePath : QString();
    m_batchPreloadWatchers.append(watcher);

    connect(watcher, &QFutureWatcher<PreloadedSliceBatch>::finished, this, [this, watcher, generation]() {
        const PreloadedSliceBatch loadedBatch = watcher->result();
        applyPreloadedSliceBatch(generation, loadedBatch);
        m_batchPreloadWatchers.removeAll(watcher);
        watcher->deleteLater();
    });

    watcher->setFuture(QtConcurrent::run(&m_cineDecodeThreadPool, [filePath, frameIndexBySeriesIndex, fullCacheFilePath]() {
        GDCMFileHandling loader;
        const QList<int> frameIndices = frameIndexBySeriesIndex.values();
        const qint64 decodeStartedAtMs = QDateTime::currentMSecsSinceEpoch();
        const QHash<int, std::shared_ptr<DicomImage>> loadedFrames = loader.loadImageDataFrames(filePath, frameIndices);
        const qint64 decodeFinishedAtMs = QDateTime::currentMSecsSinceEpoch();
        /*qDebug() << "[GDCM] batch decode ms:" << (decodeFinishedAtMs - decodeStartedAtMs)
                 << "| frames:" << frameIndices.size();*/

        PreloadedSliceBatch batch;
        batch.fullCacheFilePath = fullCacheFilePath;
        for (auto it = frameIndexBySeriesIndex.cbegin(); it != frameIndexBySeriesIndex.cend(); ++it)
        {
            batch.requestedSeriesIndices.insert(it.key());
        }
        for (auto it = frameIndexBySeriesIndex.cbegin(); it != frameIndexBySeriesIndex.cend(); ++it)
        {
            const std::shared_ptr<DicomImage> loadedImage = loadedFrames.value(it.value());
            if (loadedImage)
            {
                batch.imagesBySeriesIndex.insert(it.key(), loadedImage);
            }
        }
        return batch;
    }));
}

void DicomViewportController::requestFullMultiframeCache(const DicomImage& image)
{
    if (!m_session.currentSeries || image.filePath().isEmpty() || !isFullMultiframeCacheEligible(image) ||
        isFullMultiframeCacheReady(image) || m_pendingFullCineCacheFilePath == image.filePath() ||
        !m_batchPreloadWatchers.isEmpty())
    {
        return;
    }

    const QList<int> indices = missingFrameIndicesForFile(image.filePath());
    if (indices.isEmpty())
    {
        m_activeFullCineCacheFilePath = image.filePath();
        return;
    }

    m_pendingFullCineCacheFilePath = image.filePath();
    /*qDebug() << "[CineFrameCache] full active XA cache requested"
             << "| file:" << QFileInfo(image.filePath()).fileName()
             << "| frames:" << indices.size()
             << "| estimated bytes:" << static_cast<qint64>(estimatedFullMultiframeCacheBytes(image));*/
    requestSlicePreloadBatch(indices);
}

bool DicomViewportController::isFullMultiframeCacheEligible(const DicomImage& image) const
{
    return image.frameCount() > 1 && estimatedFullMultiframeCacheBytes(image) <= kMaxFullMultiframeCacheBytes;
}

bool DicomViewportController::isFullMultiframeCacheReady(const DicomImage& image) const
{
    if (!m_session.currentSeries || image.filePath().isEmpty())
    {
        return false;
    }

    const auto& images = m_session.currentSeries->images();
    bool foundFrameForFile = false;
    for (const auto& frameImage : images)
    {
        if (!frameImage || frameImage->filePath() != image.filePath())
        {
            continue;
        }

        foundFrameForFile = true;
        if (!frameImage->hasRawPixels())
        {
            return false;
        }
    }

    return foundFrameForFile;
}

QList<int> DicomViewportController::missingFrameIndicesForFile(const QString& filePath) const
{
    QList<int> indices;
    if (!m_session.currentSeries || filePath.isEmpty())
    {
        return indices;
    }

    const auto& images = m_session.currentSeries->images();
    for (int index = 0; index < static_cast<int>(images.size()); ++index)
    {
        const auto& image = images[static_cast<std::size_t>(index)];
        if (!image || image->filePath() != filePath || image->hasRawPixels() || m_pendingPreloadIndices.contains(index))
        {
            continue;
        }

        indices.append(index);
    }

    return indices;
}

std::size_t DicomViewportController::estimatedFullMultiframeCacheBytes(const DicomImage& image) const
{
    if (image.width() <= 0 || image.height() <= 0 || image.frameCount() <= 0)
    {
        return 0;
    }

    return static_cast<std::size_t>(image.width()) *
        static_cast<std::size_t>(image.height()) *
        sizeof(std::int16_t) *
        static_cast<std::size_t>(image.frameCount());
}

void DicomViewportController::scheduleSlicePreload(bool cinePlaying)
{
    if (!m_session.currentSeries)
    {
        return;
    }

    const int count = imageCount();
    if (count <= 1)
    {
        return;
    }

    const int forwardCount = cinePlaying ? kCineRawPixelCacheRadius : kRawPixelCacheRadius;
    const int backwardCount = cinePlaying ? 0 : 3;

    const DicomImage* current = currentImage();
    if (current && current->frameCount() > 1)
    {
        if (isFullMultiframeCacheEligible(*current))
        {
            requestFullMultiframeCache(*current);
            return;
        }

        constexpr int kStreamingFallbackBatchSize = 60;
        QList<int> indices;
        indices.reserve(kStreamingFallbackBatchSize);
        for (int offset = 1; offset <= std::min(kStreamingFallbackBatchSize, count - 1); ++offset)
        {
            const int index = (m_session.currentImageIndex + offset) % count;
            const auto& image = m_session.currentSeries->images()[static_cast<std::size_t>(index)];
            if (!image || image->hasRawPixels() || m_pendingPreloadIndices.contains(index))
            {
                continue;
            }
            indices.append(index);
        }

        QHash<QString, QList<int>> indicesByFilePath;
        for (int index : indices)
        {
            const auto& image = m_session.currentSeries->images()[static_cast<std::size_t>(index)];
            if (image)
            {
                indicesByFilePath[image->filePath()].append(index);
            }
        }

        for (auto it = indicesByFilePath.cbegin(); it != indicesByFilePath.cend(); ++it)
        {
            requestSlicePreloadBatch(it.value());
        }
        return;
    }

    for (int offset = 1; offset <= forwardCount; ++offset)
    {
        requestSlicePreload((m_session.currentImageIndex + offset) % count);
    }

    for (int offset = 1; offset <= backwardCount; ++offset)
    {
        requestSlicePreload(m_session.currentImageIndex - offset);
    }
}

void DicomViewportController::enforceRawPixelCache()
{
    if (m_rawPixelEvictionSuspended || !m_session.currentSeries)
    {
        return;
    }

    if (m_session.currentSeries->images().empty() || m_session.currentImageIndex < 0)
    {
        return;
    }

    const DicomImage* current = currentImage();
    const bool fullMultiframeCacheActive = current &&
        current->filePath() == m_activeFullCineCacheFilePath &&
        isFullMultiframeCacheEligible(*current);
    const int cacheRadius = fullMultiframeCacheActive
        ? imageCount()
        : (m_session.cinePlaying ? kCineRawPixelCacheRadius : kRawPixelCacheRadius);
    m_cineFrameCache->evictOutsideWindow(
        *m_session.currentSeries,
        m_session.currentImageIndex,
        cacheRadius,
        m_session.cinePlaying || fullMultiframeCacheActive,
        m_pendingPreloadIndices);
}

int DicomViewportController::nextLoadedCineIndex() const
{
    const int count = imageCount();
    if (count <= 1 || !m_session.currentSeries)
    {
        return -1;
    }

    const auto& images = m_session.currentSeries->images();
    if (m_session.cinePlaying)
    {
        const int nextIndex = (m_session.currentImageIndex + 1) % count;
        const auto& image = images[static_cast<std::size_t>(nextIndex)];
        return image && image->hasRawPixels() ? nextIndex : -1;
    }

    return -1;
}
