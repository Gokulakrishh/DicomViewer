#include "Services/VideoExport/DicomSeriesFrameProvider.h"

#include "FileHandling/FileHandling.h"
#include "Model/DicomImage.h"
#include "Utilities/DiagnosticImageRenderer.h"

#include <algorithm>
#include <memory>

DicomSeriesFrameProvider::DicomSeriesFrameProvider(
    const FileHandling& fileHandling,
    std::vector<VideoExportFrameSource> frameSources,
    QSize frameSize,
    int windowLevel,
    int windowWidth)
    : m_fileHandling(fileHandling),
      m_frameSources(std::move(frameSources)),
      m_frameSize(frameSize),
      m_windowLevel(windowLevel),
      m_windowWidth(std::max(1, windowWidth))
{
}

int DicomSeriesFrameProvider::frameCount() const
{
    return static_cast<int>(m_frameSources.size());
}

QSize DicomSeriesFrameProvider::frameSize() const
{
    return m_frameSize;
}

bool DicomSeriesFrameProvider::visitFrames(
    int firstFrameIndex,
    int lastFrameIndex,
    const FrameConsumer& consumer,
    const CancellationCheck& isCancelled,
    QString* errorMessage)
{
    if (m_frameSources.empty() || !m_frameSize.isValid() || !consumer)
    {
        if (errorMessage)
        {
            *errorMessage = QStringLiteral("The selected slice-series source is incomplete.");
        }
        return false;
    }

    const int first = std::clamp(firstFrameIndex, 0, frameCount() - 1);
    const int last = std::clamp(lastFrameIndex, 0, frameCount() - 1);
    if (first > last)
    {
        if (errorMessage)
        {
            *errorMessage = QStringLiteral("The selected slice-series frame range is invalid.");
        }
        return false;
    }

    for (int frameIndex = first; frameIndex <= last; ++frameIndex)
    {
        if (isCancelled && isCancelled())
        {
            return false;
        }

        const VideoExportFrameSource& source =
            m_frameSources[static_cast<std::size_t>(frameIndex)];
        if (source.filePath.trimmed().isEmpty())
        {
            if (errorMessage)
            {
                *errorMessage = QStringLiteral("A selected slice has no source file path.");
            }
            return false;
        }

        const std::unique_ptr<DicomImage> image =
            m_fileHandling.loadImageData(source.filePath, source.sourceFrameIndex);
        if (!image)
        {
            if (errorMessage)
            {
                *errorMessage = QStringLiteral("Failed to decode a selected slice for export.");
            }
            return false;
        }
        if (QSize(image->width(), image->height()) != m_frameSize)
        {
            if (errorMessage)
            {
                *errorMessage = QStringLiteral("A selected slice has inconsistent dimensions.");
            }
            return false;
        }
        if (!image->isMonochrome() || !image->hasRawPixels())
        {
            if (errorMessage)
            {
                *errorMessage = QStringLiteral(
                    "Series video export currently supports monochrome DICOM slices only.");
            }
            return false;
        }

        QImage frame = createWindowedImage(*image, m_windowLevel, m_windowWidth);
        if (frame.isNull())
        {
            if (errorMessage)
            {
                *errorMessage = QStringLiteral("Failed to render a selected slice for export.");
            }
            return false;
        }

        frame = frame.convertToFormat(QImage::Format_RGB888);
        if (!consumer(frameIndex, frame, errorMessage))
        {
            return false;
        }
    }

    return true;
}
