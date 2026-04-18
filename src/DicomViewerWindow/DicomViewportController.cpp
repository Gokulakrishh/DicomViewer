#include "DicomViewportController.h"

#include <QFileInfo>
#include <QtConcurrent/QtConcurrent>
#include <algorithm>

#include "DicomRenderService.h"
#include "FileHandling/FileHandling.h"
#include "FileHandling/GDCMFileHandling.h"
#include "Model/DicomImage.h"
#include "Model/MedicalImage.h"
#include "Model/DicomParameters.h"
#include "Services/WindowingAnalysisService.h"

DicomViewportController::DicomViewportController(
    FileHandling* fileHandling,
    const DicomRenderService* renderService,
    const WindowingAnalysisService* windowingAnalysisService,
    QObject* parent)
    : QObject(parent),
      m_fileHandling(fileHandling),
      m_renderService(renderService),
      m_windowingAnalysisService(windowingAnalysisService)
{
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
}

bool DicomViewportController::ensureImageLoaded(DicomImage& image)
{
    if (image.hasRawPixels())
    {
        if (image.pixmap().isNull() && m_renderService)
        {
            m_renderService->ensureDiagnosticPixmap(image);
        }
        return true;
    }

    const QString suffix = QFileInfo(image.filePath()).suffix();
    const bool shouldLoadRawDicom = suffix.compare("dcm", Qt::CaseInsensitive) == 0;
    if (image.isValid() && !shouldLoadRawDicom)
    {
        return true;
    }

    if (!m_fileHandling)
    {
        return false;
    }

    std::unique_ptr<MedicalImage> loadedImage = m_fileHandling->loadImage(image.filePath());
    if (!loadedImage || !loadedImage->isValid())
    {
        return false;
    }

    if (auto* loadedDicomImage = dynamic_cast<DicomImage*>(loadedImage.get()))
    {
        image = *loadedDicomImage;
        if (image.hasRawPixels() && image.pixmap().isNull() && m_renderService)
        {
            m_renderService->ensureDiagnosticPixmap(image);
        }
        return image.isValid();
    }

    return false;
}

void DicomViewportController::setSeries(const std::shared_ptr<Series>& series, int initialIndex)
{
    m_session.singleImage.reset();
    m_session.currentSeries = series;
    ++m_session.seriesGeneration;
    cancelPendingPreloads();

    const int count = imageCount();
    m_session.currentImageIndex = count > 0 ? std::clamp(initialIndex, 0, count - 1) : -1;
    m_session.windowStateInitialized = false;
    m_session.currentPresetIndex = 0;
    m_session.currentAutoWindowPresetIndex = 0;
    m_session.cinePlaying = false;
}

void DicomViewportController::setSingleImage(const std::shared_ptr<DicomImage>& image)
{
    ++m_session.seriesGeneration;
    cancelPendingPreloads();
    m_session.currentSeries.reset();
    m_session.singleImage = image;
    m_session.currentImageIndex = -1;
    m_session.windowStateInitialized = false;
    m_session.currentPresetIndex = 0;
    m_session.currentAutoWindowPresetIndex = 0;
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
    if (!ensureImageLoaded(*currentImage))
    {
        if (errorMessage)
        {
            *errorMessage = currentImage->filePath();
        }
        return false;
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
            m_session.currentPresetIndex = 0;
            m_session.currentAutoWindowPresetIndex = 0;
            m_session.windowStateInitialized = true;
        }
        else
        {
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
            m_session.currentPresetIndex = 0;
            m_session.currentAutoWindowPresetIndex = 0;
            m_session.windowStateInitialized = true;
        }
    }

    state.level = m_session.currentWindowLevel;
    state.width = m_session.currentWindowWidth;
    state.presetIndex = m_session.currentPresetIndex;
    state.autoWindowPresetIndex = m_session.currentAutoWindowPresetIndex;
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
    if (m_renderService)
    {
        return m_renderService->renderDiagnosticImage(
            *displayedImage,
            DicomRenderService::RenderSettings{
                m_session.currentWindowLevel,
                m_session.currentWindowWidth});
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

int DicomViewportController::currentPresetIndex() const
{
    return m_session.currentPresetIndex;
}

int DicomViewportController::currentAutoWindowPresetIndex() const
{
    return m_session.currentAutoWindowPresetIndex;
}

void DicomViewportController::resetPreset()
{
    m_session.currentPresetIndex = 0;
}

bool DicomViewportController::applyPreset(int index)
{
    const DicomImage* displayedImage = currentImage();
    if (!displayedImage || !displayedImage->hasRawPixels())
    {
        return false;
    }

    const int minimumValue = displayedImage->minimumStoredValue();
    const int maximumValue = displayedImage->maximumStoredValue();

    struct PresetValues
    {
        int level;
        int width;
    };

    PresetValues presetValues{
        displayedImage->defaultWindowLevel(),
        displayedImage->defaultWindowWidth()};
    switch (index)
    {
    case 1:
        presetValues = {300, 1500};
        break;
    case 2:
        presetValues = {-500, 1500};
        break;
    case 3:
        presetValues = {40, 80};
        break;
    case 4:
        presetValues = {50, 400};
        break;
    default:
        return false;
    }

    m_session.currentWindowLevel = std::clamp(presetValues.level, minimumValue, maximumValue);
    m_session.currentWindowWidth = std::clamp(presetValues.width, 1, std::max(1, maximumValue - minimumValue));
    m_session.currentPresetIndex = index;
    m_session.currentAutoWindowPresetIndex = 0;
    return true;
}

void DicomViewportController::resetAutoWindowPreset()
{
    m_session.currentAutoWindowPresetIndex = 0;
}

bool DicomViewportController::applyAutoWindowPreset(int index)
{
    const DicomImage* displayedImage = currentImage();
    if (!displayedImage || !displayedImage->hasRawPixels() || !m_windowingAnalysisService)
    {
        return false;
    }

    const auto result = m_windowingAnalysisService->analyzePreset(
        *displayedImage,
        static_cast<WindowingAnalysisService::Preset>(index));
    if (!result.valid)
    {
        return false;
    }

    const int minimumValue = displayedImage->minimumStoredValue();
    const int maximumValue = displayedImage->maximumStoredValue();
    m_session.currentWindowLevel = std::clamp(result.windowLevel, minimumValue, maximumValue);
    m_session.currentWindowWidth = std::clamp(result.windowWidth, 1, std::max(1, maximumValue - minimumValue));
    m_session.currentAutoWindowPresetIndex = index;
    m_session.currentPresetIndex = 0;
    return true;
}

void DicomViewportController::setToolIndex(int index)
{
    m_session.toolIndex = index;
}

int DicomViewportController::toolIndex() const
{
    return m_session.toolIndex;
}

void DicomViewportController::setCinePlaying(bool playing)
{
    m_session.cinePlaying = playing;
}

bool DicomViewportController::isCinePlaying() const
{
    return m_session.cinePlaying;
}

const ViewportSession& DicomViewportController::session() const
{
    return m_session;
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
    m_pendingPreloadIndices.clear();
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
    const int generation = m_session.seriesGeneration;
    m_pendingPreloadIndices.insert(index);
    m_preloadWatchers.append(watcher);

    connect(watcher, &QFutureWatcher<std::shared_ptr<DicomImage>>::finished, this, [this, watcher, index, generation]() {
        const std::shared_ptr<DicomImage> loadedImage = watcher->result();
        applyPreloadedSlice(index, generation, loadedImage);
        m_preloadWatchers.removeAll(watcher);
        watcher->deleteLater();
    });

    watcher->setFuture(QtConcurrent::run([filePath]() -> std::shared_ptr<DicomImage> {
        GDCMFileHandling loader;
        std::unique_ptr<DicomImage> loadedImage = loader.loadImageData(filePath);
        if (!loadedImage)
        {
            return {};
        }

        return std::shared_ptr<DicomImage>(loadedImage.release());
    }));
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

    const int forwardCount = cinePlaying ? 12 : 10;
    const int backwardCount = cinePlaying ? 0 : 3;

    for (int offset = 1; offset <= forwardCount; ++offset)
    {
        requestSlicePreload((m_session.currentImageIndex + offset) % count);
    }

    for (int offset = 1; offset <= backwardCount; ++offset)
    {
        requestSlicePreload(m_session.currentImageIndex - offset);
    }
}
