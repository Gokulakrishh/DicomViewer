#pragma once

#include "Services/VideoExport/IVideoFrameProvider.h"
#include "Services/VideoExport/VideoExportTypes.h"

#include <vector>

class FileHandling;

/**
 * @brief Converts a DICOM slice series into display-ready export frames.
 *
 * Responsibilities:
 * - Decode one referenced slice at a time through the FileHandling boundary.
 * - Apply the selected WL/WW consistently with the main viewer.
 * - Keep CT/MR style series export independent from the GStreamer adapter.
 *
 * Assumptions:
 * - The source list is ordered in the same order as the active viewer series.
 * - All exported slices are expected to have the same dimensions.
 * - The provider does not retain decoded pixel buffers after a frame is consumed.
 */
class DicomSeriesFrameProvider final : public IVideoFrameProvider
{
public:
    /**
     * @brief Creates a provider for an ordered slice series.
     * @param fileHandling DICOM decoding service; must outlive the provider.
     * @param frameSources Ordered lightweight references to source slices.
     * @param frameSize Expected output frame dimensions.
     * @param windowLevel Export window level.
     * @param windowWidth Export window width.
     */
    DicomSeriesFrameProvider(
        const FileHandling& fileHandling,
        std::vector<VideoExportFrameSource> frameSources,
        QSize frameSize,
        int windowLevel,
        int windowWidth);

    int frameCount() const override;
    QSize frameSize() const override;
    bool visitFrames(
        int firstFrameIndex,
        int lastFrameIndex,
        const FrameConsumer& consumer,
        const CancellationCheck& isCancelled,
        QString* errorMessage) override;

private:
    const FileHandling& m_fileHandling;
    std::vector<VideoExportFrameSource> m_frameSources;
    QSize m_frameSize;
    int m_windowLevel{0};
    int m_windowWidth{1};
};
