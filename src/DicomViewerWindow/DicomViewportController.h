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
        ViewportWindowPreset preset{ViewportWindowPreset::Custom};
        int dicomPresetIndex{-1};
    };

    explicit DicomViewportController(
        FileHandling* fileHandling,
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
    std::shared_ptr<DicomImage> renderCurrentDiagnosticImage() const;

    void setWindowLevel(int value);
    void setWindowWidth(int value);
    int currentWindowLevel() const;
    int currentWindowWidth() const;
    ViewportWindowPreset currentPreset() const;
    int currentDicomWindowPresetIndex() const;
    void resetPreset();
    bool applyPreset(ViewportWindowPreset preset);
    int dicomWindowPresetCount() const;
    QString dicomWindowPresetLabel(int index) const;
    bool applyDicomWindowPreset(int index);
    void setCinePlaying(bool playing);
    bool isCinePlaying() const;
    const ViewportSession& session() const;
    void suspendRawPixelEviction(bool suspended);

private:
    void cancelPendingPreloads();
    void applyPreloadedSlice(int index, int generation, const std::shared_ptr<DicomImage>& loadedImage);
    void requestSlicePreload(int index);
    void scheduleSlicePreload(bool cinePlaying);
    void enforceRawPixelCache();

private:
    static constexpr int kRawPixelCacheRadius = 10;
    FileHandling* m_fileHandling{nullptr};
    ViewportSession m_session;
    QSet<int> m_pendingPreloadIndices;
    QList<QFutureWatcher<std::shared_ptr<DicomImage>>*> m_preloadWatchers;
    bool m_rawPixelEvictionSuspended{false};
};
