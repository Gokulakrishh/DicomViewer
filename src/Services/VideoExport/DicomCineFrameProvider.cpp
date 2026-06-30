#include "Services/VideoExport/DicomCineFrameProvider.h"

#include "FileHandling/FileHandling.h"
#include "Model/DicomImage.h"
#include "Utilities/DiagnosticImageRenderer.h"

#include <algorithm>

DicomCineFrameProvider::DicomCineFrameProvider(
    const FileHandling& fileHandling,
    QString filePath,
    int frameCount,
    QSize frameSize,
    int windowLevel,
    int windowWidth)
    : m_fileHandling(fileHandling),
      m_filePath(std::move(filePath)),
      m_frameCount(std::max(0, frameCount)),
      m_frameSize(frameSize),
      m_windowLevel(windowLevel),
      m_windowWidth(std::max(1, windowWidth))
{
}

int DicomCineFrameProvider::frameCount() const
{
    return m_frameCount;
}

QSize DicomCineFrameProvider::frameSize() const
{
    return m_frameSize;
}

bool DicomCineFrameProvider::visitFrames(
    int firstFrameIndex,
    int lastFrameIndex,
    const FrameConsumer& consumer,
    const CancellationCheck& isCancelled,
    QString* errorMessage)
{
    if (m_filePath.isEmpty() || m_frameCount <= 0 || !m_frameSize.isValid() || !consumer)
    {
        if (errorMessage)
        {
            *errorMessage = QStringLiteral("The selected cine source is incomplete.");
        }
        return false;
    }

    const int first = std::clamp(firstFrameIndex, 0, m_frameCount - 1);
    const int last = std::clamp(lastFrameIndex, 0, m_frameCount - 1);
    if (first > last)
    {
        if (errorMessage)
        {
            *errorMessage = QStringLiteral("The selected cine frame range is invalid.");
        }
        return false;
    }

    return m_fileHandling.visitImageDataFrames(
        m_filePath,
        first,
        last,
        [this, &consumer](int frameIndex, const DicomImage& image, QString* visitorError) {
            if (!image.isMonochrome() || !image.hasRawPixels())
            {
                if (visitorError)
                {
                    *visitorError = QStringLiteral(
                        "Phase 1 cine export supports monochrome XA frames only.");
                }
                return false;
            }

            QImage frame = createWindowedImage(image, m_windowLevel, m_windowWidth);
            if (frame.isNull())
            {
                if (visitorError)
                {
                    *visitorError = QStringLiteral("Failed to render a selected cine frame.");
                }
                return false;
            }

            frame = frame.convertToFormat(QImage::Format_RGB888);
            return consumer(frameIndex, frame, visitorError);
        },
        isCancelled,
        errorMessage);
}
