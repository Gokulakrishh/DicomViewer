#pragma once

#include <QFutureWatcher>
#include <QList>
#include <QObject>
#include <QSet>
#include <memory>

#include "ViewportSession.h"

class DicomImage;
class FileHandling;
class Series;
class DicomRenderService;
class WindowingAnalysisService;

class DicomViewportController : public QObject
{
    Q_OBJECT

public:
    struct WindowControlState
    {
        bool hasImage{false};
        int levelMin{-100};
        int levelMax{100};
        int widthMin{10};
        int widthMax{300};
        int level{0};
        int width{100};
        int presetIndex{0};
        int autoWindowPresetIndex{0};
    };

    explicit DicomViewportController(
        FileHandling* fileHandling,
        const DicomRenderService* renderService,
        const WindowingAnalysisService* windowingAnalysisService,
        QObject* parent = nullptr);
    ~DicomViewportController() override;

    void clear();
    bool ensureImageLoaded(DicomImage& image);

    void setSeries(const std::shared_ptr<Series>& series, int initialIndex = 0);
    void setSingleImage(const std::shared_ptr<DicomImage>& image);

    std::shared_ptr<Series> currentSeries() const;
    DicomImage* currentImage();
    const DicomImage* currentImage() const;
    bool hasPlayableSeries() const;
    int imageCount() const;
    int currentImageIndex() const;
    void setCurrentImageIndex(int index);
    int clampedIndexWithStep(int stepCount) const;
    int nextCineIndex() const;

    bool prepareCurrentSeriesImage(bool cinePlaying, QString* errorMessage = nullptr);
    WindowControlState windowControlState(bool resetWindowState);
    std::shared_ptr<DicomImage> renderCurrentImage() const;

    void setWindowLevel(int value);
    void setWindowWidth(int value);
    int currentWindowLevel() const;
    int currentWindowWidth() const;
    int currentPresetIndex() const;
    int currentAutoWindowPresetIndex() const;
    void resetPreset();
    bool applyPreset(int index);
    void resetAutoWindowPreset();
    bool applyAutoWindowPreset(int index);
    void setToolIndex(int index);
    int toolIndex() const;
    void setCinePlaying(bool playing);
    bool isCinePlaying() const;
    const ViewportSession& session() const;

private:
    void cancelPendingPreloads();
    void applyPreloadedSlice(int index, int generation, const std::shared_ptr<DicomImage>& loadedImage);
    void requestSlicePreload(int index);
    void scheduleSlicePreload(bool cinePlaying);

private:
    FileHandling* m_fileHandling{nullptr};
    const DicomRenderService* m_renderService{nullptr};
    const WindowingAnalysisService* m_windowingAnalysisService{nullptr};
    ViewportSession m_session;
    QSet<int> m_pendingPreloadIndices;
    QList<QFutureWatcher<std::shared_ptr<DicomImage>>*> m_preloadWatchers;
};
