#include "Services/VideoExport/GStreamerVideoExportService.h"
#include "Services/VideoExport/IVideoFrameProvider.h"

#include <QCoreApplication>
#include <QColor>
#include <QFileInfo>
#include <QImage>
#include <QTemporaryDir>

#include <algorithm>
#include <iostream>

namespace
{
class SyntheticFrameProvider final : public IVideoFrameProvider
{
public:
    int frameCount() const override
    {
        return 12;
    }

    QSize frameSize() const override
    {
        return {160, 120};
    }

    bool visitFrames(
        int firstFrameIndex,
        int lastFrameIndex,
        const FrameConsumer& consumer,
        const CancellationCheck& isCancelled,
        QString* errorMessage) override
    {
        if (firstFrameIndex < 0 || lastFrameIndex >= frameCount() || firstFrameIndex > lastFrameIndex)
        {
            if (errorMessage)
            {
                *errorMessage = QStringLiteral("Invalid synthetic frame range.");
            }
            return false;
        }

        for (int frameIndex = firstFrameIndex; frameIndex <= lastFrameIndex; ++frameIndex)
        {
            if (isCancelled && isCancelled())
            {
                return false;
            }

            QImage frame(frameSize(), QImage::Format_RGB888);
            frame.fill(QColor::fromHsv((frameIndex * 30) % 360, 180, 220));
            if (!consumer(frameIndex, frame, errorMessage))
            {
                return false;
            }
        }
        return true;
    }
};

int fail(const QString& message)
{
    std::cerr << message.toStdString() << '\n';
    return 1;
}
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    QTemporaryDir outputDirectory;
    if (!outputDirectory.isValid())
    {
        return fail(QStringLiteral("Failed to create a temporary export directory."));
    }

    SyntheticFrameProvider frameProvider;
    VideoExportRequest request;
    request.outputPath = outputDirectory.filePath(QStringLiteral("cine-export-smoke.mp4"));
    request.sourceSopInstanceUid = QStringLiteral("1.2.826.0.1.3680043.10.1000.1");
    request.productVersion = QStringLiteral("test");
    request.firstFrameIndex = 2;
    request.lastFrameIndex = 9;
    request.framesPerSecond = 15.0;
    request.timingSource = VideoExportTimingSource::Manual;

    int reportedFrames = 0;
    GStreamerVideoExportService service;
    VideoExportRequest prohibitedRequest = request;
    prohibitedRequest.outputPath =
        outputDirectory.filePath(QStringLiteral("prohibited-cine-export.ogv"));
    const VideoExportResult prohibitedResult = service.exportVideo(
        prohibitedRequest,
        frameProvider);
    if (prohibitedResult.success ||
        prohibitedResult.errorMessage !=
            QStringLiteral("Video export supports MP4 (.mp4) output only.") ||
        QFileInfo::exists(prohibitedRequest.outputPath))
    {
        return fail(QStringLiteral("The prohibited alternate video format was not rejected."));
    }

    const VideoExportResult result = service.exportVideo(
        request,
        frameProvider,
        [&reportedFrames](int completed, int) {
            reportedFrames = completed;
        });

    if (!result.success &&
        result.errorMessage.startsWith(QStringLiteral("Required GStreamer element is unavailable:")))
    {
        if (QFileInfo::exists(request.outputPath))
        {
            return fail(QStringLiteral("Unavailable H.264 support left an output file."));
        }
        if (result.container != QStringLiteral("MP4") ||
            result.codec != QStringLiteral("H.264"))
        {
            return fail(QStringLiteral("Unavailable export did not retain the controlled MP4/H.264 policy."));
        }
        return 0;
    }
    if (!result.success)
    {
        return fail(QStringLiteral("Video export failed: %1").arg(result.errorMessage));
    }
    if (result.exportedFrameCount != 8 || reportedFrames != 8)
    {
        return fail(QStringLiteral("Video export reported the wrong frame count."));
    }
    if (!QFileInfo::exists(request.outputPath) || QFileInfo(request.outputPath).size() <= 0)
    {
        return fail(QStringLiteral("Video export did not create a non-empty output file."));
    }
    if (result.gstreamerVersion.section('.', 0, 1) != QStringLiteral("1.24"))
    {
        return fail(QStringLiteral("Video export did not use GStreamer 1.24."));
    }
    if (result.container != QStringLiteral("MP4") ||
        result.codec != QStringLiteral("H.264") ||
        result.h264Profile != QStringLiteral("baseline") ||
        result.pixelFormat != QStringLiteral("NV12") ||
        result.targetBitrateKbps != 8000 ||
        result.keyframeIntervalFrames != 30)
    {
        return fail(QStringLiteral("Video export did not apply the controlled H.264 encoding profile."));
    }

    return 0;
}
