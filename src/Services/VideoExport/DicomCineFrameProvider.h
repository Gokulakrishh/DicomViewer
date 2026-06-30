#pragma once

#include "Services/VideoExport/IVideoFrameProvider.h"

class FileHandling;

/**
 * @brief Converts one multi-frame DICOM SOP into display-ready export frames.
 *
 * The provider delegates source decoding to FileHandling, applies the selected
 * WL/WW to monochrome pixels, and exposes no patient overlays or UI state.
 */
class DicomCineFrameProvider final : public IVideoFrameProvider
{
public:
    /**
     * @brief Creates a provider for one source SOP.
     * @param fileHandling DICOM decoding service; must outlive the provider.
     * @param filePath Canonical source DICOM path.
     * @param frameCount Number of frames in the SOP.
     * @param frameSize Expected source dimensions.
     * @param windowLevel Export window level.
     * @param windowWidth Export window width.
     */
    DicomCineFrameProvider(
        const FileHandling& fileHandling,
        QString filePath,
        int frameCount,
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
    QString m_filePath;
    int m_frameCount{0};
    QSize m_frameSize;
    int m_windowLevel{0};
    int m_windowWidth{1};
};
