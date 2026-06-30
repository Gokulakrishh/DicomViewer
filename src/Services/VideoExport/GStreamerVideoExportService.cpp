#include "Services/VideoExport/GStreamerVideoExportService.h"

#include "Services/VideoExport/IVideoFrameProvider.h"

#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QtSystemDetection>

#include <gst/app/gstappsrc.h>
#include <gst/gst.h>

#include <algorithm>
#include <cmath>
#include <numeric>

namespace
{
constexpr int kMinimumFrameRate = 1;
constexpr int kMaximumFrameRate = 120;
constexpr int kMinimumH264Dimension = 64;
constexpr guint64 kPipelineMessagePollNs = 100 * GST_MSECOND;
constexpr qint64 kPipelineFinalizeTimeoutMs = 120000;

struct PlatformEncoder
{
    const char* factoryName{nullptr};
    const char* elementName{nullptr};
    const char* displayName{nullptr};
};

struct PipelineGuard
{
    GstElement* pipeline{nullptr};

    void stop() const
    {
        if (pipeline)
        {
            gst_element_set_state(pipeline, GST_STATE_NULL);
        }
    }

    ~PipelineGuard()
    {
        if (pipeline)
        {
            stop();
            gst_object_unref(pipeline);
        }
    }
};

bool cancelled(const IVideoExportService::CancellationCheck& isCancelled)
{
    return isCancelled && isCancelled();
}

bool validateRequest(
    const VideoExportRequest& request,
    const IVideoFrameProvider& frameProvider,
    QString* errorMessage)
{
    if (request.outputPath.trimmed().isEmpty())
    {
        *errorMessage = QStringLiteral("Choose an output video file.");
        return false;
    }
    if (!request.derivedNonDiagnosticOutput || request.includePatientOverlays)
    {
        *errorMessage = QStringLiteral(
            "Phase 1 export requires derived non-diagnostic output without patient overlays.");
        return false;
    }
    if (frameProvider.frameCount() <= 1 || !frameProvider.frameSize().isValid())
    {
        *errorMessage = QStringLiteral("The selected source is not a valid multi-frame cine object.");
        return false;
    }
    if (request.firstFrameIndex < 0 ||
        request.lastFrameIndex < request.firstFrameIndex ||
        request.lastFrameIndex >= frameProvider.frameCount())
    {
        *errorMessage = QStringLiteral("The selected cine frame range is invalid.");
        return false;
    }
    if (!std::isfinite(request.framesPerSecond) ||
        request.framesPerSecond < kMinimumFrameRate ||
        request.framesPerSecond > kMaximumFrameRate)
    {
        *errorMessage = QStringLiteral("The export frame rate must be between 1 and 120 FPS.");
        return false;
    }

    const QFileInfo outputInfo(request.outputPath);
    if (outputInfo.suffix().compare(
            QString::fromLatin1(VideoExportPolicy::FileExtension),
            Qt::CaseInsensitive) != 0)
    {
        *errorMessage = QStringLiteral("Video export supports MP4 (.mp4) output only.");
        return false;
    }
    const QDir outputDirectory = outputInfo.dir();
    if (!outputDirectory.exists())
    {
        *errorMessage = QStringLiteral("The selected output directory does not exist.");
        return false;
    }

    const QSize frameSize = frameProvider.frameSize();
    if (frameSize.width() < kMinimumH264Dimension ||
        frameSize.height() < kMinimumH264Dimension ||
        (frameSize.width() % 2) != 0 ||
        (frameSize.height() % 2) != 0)
    {
        *errorMessage = QStringLiteral(
            "MP4/H.264 export requires even frame dimensions of at least 64 x 64 pixels.");
        return false;
    }

    return true;
}

QString gstreamerErrorMessage(GstMessage* message)
{
    GError* error = nullptr;
    gchar* debugDetails = nullptr;
    gst_message_parse_error(message, &error, &debugDetails);
    const QString text = error
        ? QString::fromUtf8(error->message)
        : QStringLiteral("Unknown GStreamer pipeline error.");
    if (error)
    {
        g_error_free(error);
    }
    if (debugDetails)
    {
        g_free(debugDetails);
    }
    return text;
}

bool initializeGStreamer(QString* version, QString* errorMessage)
{
    GError* error = nullptr;
    if (!gst_init_check(nullptr, nullptr, &error))
    {
        *errorMessage = error
            ? QString::fromUtf8(error->message)
            : QStringLiteral("GStreamer initialization failed.");
        if (error)
        {
            g_error_free(error);
        }
        return false;
    }

    guint major = 0;
    guint minor = 0;
    guint micro = 0;
    guint nano = 0;
    gst_version(&major, &minor, &micro, &nano);
    *version = QStringLiteral("%1.%2.%3").arg(major).arg(minor).arg(micro);
    if (major != 1 || minor != 24)
    {
        *errorMessage = QStringLiteral(
            "GStreamer 1.24 is required, but runtime version %1 was loaded.").arg(*version);
        return false;
    }
    return true;
}

GstElement* makeElement(const char* factoryName, const char* elementName, QString* errorMessage)
{
    GstElement* element = gst_element_factory_make(factoryName, elementName);
    if (!element && errorMessage && errorMessage->isEmpty())
    {
        *errorMessage = QStringLiteral("Required GStreamer element is unavailable: %1")
                            .arg(QString::fromLatin1(factoryName));
    }
    return element;
}

PlatformEncoder platformEncoder()
{
#if defined(Q_OS_MACOS)
    return {"vtenc_h264", "h264-videotoolbox-encoder", "Apple VideoToolbox H.264"};
#elif defined(Q_OS_WIN)
    return {"mfh264enc", "h264-media-foundation-encoder", "Microsoft Media Foundation H.264"};
#else
    return {};
#endif
}

bool hasProperty(GstElement* element, const char* propertyName)
{
    return element &&
           g_object_class_find_property(G_OBJECT_GET_CLASS(element), propertyName) != nullptr;
}

void configureEncoder(
    GstElement* encoder,
    const PlatformEncoder& platform,
    int keyframeIntervalFrames)
{
#if defined(Q_OS_MACOS)
    Q_UNUSED(platform);
    g_object_set(
        encoder,
        "bitrate",
        static_cast<guint>(VideoExportPolicy::TargetBitrateKbps),
        "max-keyframe-interval",
        keyframeIntervalFrames,
        "allow-frame-reordering",
        FALSE,
        "realtime",
        FALSE,
        nullptr);
#elif defined(Q_OS_WIN)
    Q_UNUSED(platform);
    g_object_set(
        encoder,
        "bitrate",
        static_cast<guint>(VideoExportPolicy::TargetBitrateKbps),
        "max-bitrate",
        static_cast<guint>(VideoExportPolicy::TargetBitrateKbps),
        "gop-size",
        keyframeIntervalFrames,
        "bframes",
        static_cast<guint>(0),
        "cabac",
        FALSE,
        "low-latency",
        FALSE,
        nullptr);
    if (hasProperty(encoder, "rc-mode"))
    {
        gst_util_set_object_arg(G_OBJECT(encoder), "rc-mode", "cbr");
    }
#else
    Q_UNUSED(encoder);
    Q_UNUSED(platform);
    Q_UNUSED(keyframeIntervalFrames);
#endif
}
}

VideoExportResult GStreamerVideoExportService::exportVideo(
    const VideoExportRequest& request,
    IVideoFrameProvider& frameProvider,
    const ProgressCallback& progressCallback,
    const CancellationCheck& isCancelled) const
{
    VideoExportResult result;
    result.outputPath = request.outputPath;
    result.timingSource = videoExportTimingSourceName(request.timingSource);
    result.container = QString::fromLatin1(VideoExportPolicy::Container);
    result.codec = QString::fromLatin1(VideoExportPolicy::Codec);
    result.h264Profile = QString::fromLatin1(VideoExportPolicy::H264Profile);
    result.pixelFormat = QString::fromLatin1(VideoExportPolicy::EncoderPixelFormat);
    result.targetBitrateKbps = VideoExportPolicy::TargetBitrateKbps;

    if (!validateRequest(request, frameProvider, &result.errorMessage))
    {
        return result;
    }
    result.keyframeIntervalFrames = std::max(
        1,
        static_cast<int>(std::lround(
            request.framesPerSecond * VideoExportPolicy::KeyframeIntervalSeconds)));
    if (!initializeGStreamer(&result.gstreamerVersion, &result.errorMessage))
    {
        return result;
    }

    const PlatformEncoder selectedEncoder = platformEncoder();
    if (!selectedEncoder.factoryName)
    {
        result.errorMessage = QStringLiteral(
            "MP4/H.264 export is not supported by this platform build.");
        return result;
    }
    result.encoder = QString::fromLatin1(selectedEncoder.displayName);

    GstElement* pipeline = gst_pipeline_new("cine-export-pipeline");
    PipelineGuard pipelineGuard{pipeline};
    if (!pipeline)
    {
        result.errorMessage = QStringLiteral("Failed to create the GStreamer export pipeline.");
        return result;
    }

    GstElement* source = makeElement("appsrc", "cine-source", &result.errorMessage);
    GstElement* inputConvert = makeElement("videoconvert", "input-convert", &result.errorMessage);
    GstElement* textOverlay = makeElement("textoverlay", "derived-label", &result.errorMessage);
    GstElement* outputConvert = makeElement("videoconvert", "encoder-convert", &result.errorMessage);
    GstElement* rawCapsFilter = makeElement("capsfilter", "encoder-input-policy", &result.errorMessage);
    GstElement* encoder = makeElement(
        selectedEncoder.factoryName,
        selectedEncoder.elementName,
        &result.errorMessage);
    GstElement* parser = makeElement("h264parse", "h264-parser", &result.errorMessage);
    GstElement* h264CapsFilter = makeElement(
        "capsfilter",
        "h264-output-policy",
        &result.errorMessage);
    GstElement* muxer = makeElement("mp4mux", "mp4-muxer", &result.errorMessage);
    GstElement* sink = makeElement("filesink", "video-file-sink", &result.errorMessage);
    if (!source || !inputConvert || !textOverlay || !outputConvert || !rawCapsFilter ||
        !encoder || !parser || !h264CapsFilter || !muxer || !sink)
    {
        for (GstElement* element : {
                 source,
                 inputConvert,
                 textOverlay,
                 outputConvert,
                 rawCapsFilter,
                 encoder,
                 parser,
                 h264CapsFilter,
                 muxer,
                 sink})
        {
            if (element)
            {
                gst_object_unref(element);
            }
        }
        return result;
    }

    gst_bin_add_many(
        GST_BIN(pipeline),
        source,
        inputConvert,
        textOverlay,
        outputConvert,
        rawCapsFilter,
        encoder,
        parser,
        h264CapsFilter,
        muxer,
        sink,
        nullptr);
    if (!gst_element_link_many(
            source,
            inputConvert,
            textOverlay,
            outputConvert,
            rawCapsFilter,
            encoder,
            parser,
            h264CapsFilter,
            muxer,
            sink,
            nullptr))
    {
        result.errorMessage = QStringLiteral("Failed to link the GStreamer export pipeline.");
        return result;
    }

    const int fpsNumeratorUnreduced =
        std::max(1, static_cast<int>(std::lround(request.framesPerSecond * 1000.0)));
    const int fpsDenominatorUnreduced = 1000;
    const int divisor = std::gcd(fpsNumeratorUnreduced, fpsDenominatorUnreduced);
    const int fpsNumerator = fpsNumeratorUnreduced / divisor;
    const int fpsDenominator = fpsDenominatorUnreduced / divisor;
    const GstClockTime frameDuration = gst_util_uint64_scale_int(
        GST_SECOND,
        fpsDenominator,
        fpsNumerator);

    const QSize frameSize = frameProvider.frameSize();
    GstCaps* caps = gst_caps_new_simple(
        "video/x-raw",
        "format",
        G_TYPE_STRING,
        "RGB",
        "width",
        G_TYPE_INT,
        frameSize.width(),
        "height",
        G_TYPE_INT,
        frameSize.height(),
        "framerate",
        GST_TYPE_FRACTION,
        fpsNumerator,
        fpsDenominator,
        nullptr);

    GstCaps* encoderInputCaps = gst_caps_new_simple(
        "video/x-raw",
        "format",
        G_TYPE_STRING,
        VideoExportPolicy::EncoderPixelFormat,
        "width",
        G_TYPE_INT,
        frameSize.width(),
        "height",
        G_TYPE_INT,
        frameSize.height(),
        "framerate",
        GST_TYPE_FRACTION,
        fpsNumerator,
        fpsDenominator,
        nullptr);
    GstCaps* encoderOutputCaps = gst_caps_new_simple(
        "video/x-h264",
        "profile",
        G_TYPE_STRING,
        VideoExportPolicy::H264Profile,
        "stream-format",
        G_TYPE_STRING,
        "avc",
        "alignment",
        G_TYPE_STRING,
        "au",
        nullptr);
    g_object_set(rawCapsFilter, "caps", encoderInputCaps, nullptr);
    g_object_set(h264CapsFilter, "caps", encoderOutputCaps, nullptr);
    gst_caps_unref(encoderInputCaps);
    gst_caps_unref(encoderOutputCaps);

    const guint64 maxQueuedBytes =
        static_cast<guint64>(frameSize.width()) *
        static_cast<guint64>(frameSize.height()) *
        3U *
        3U;
    g_object_set(
        source,
        "caps",
        caps,
        "format",
        GST_FORMAT_TIME,
        "stream-type",
        GST_APP_STREAM_TYPE_STREAM,
        "is-live",
        FALSE,
        "block",
        TRUE,
        "max-bytes",
        maxQueuedBytes,
        "emit-signals",
        FALSE,
        nullptr);
    gst_caps_unref(caps);

    g_object_set(
        textOverlay,
        "text",
        "DERIVED - NON-DIAGNOSTIC",
        "halignment",
        2,
        "valignment",
        2,
        "shaded-background",
        TRUE,
        nullptr);
    configureEncoder(encoder, selectedEncoder, result.keyframeIntervalFrames);
    g_object_set(parser, "config-interval", -1, nullptr);
    if (hasProperty(muxer, "faststart"))
    {
        g_object_set(muxer, "faststart", TRUE, nullptr);
    }
    const QByteArray outputPathUtf8 = request.outputPath.toUtf8();
    g_object_set(sink, "location", outputPathUtf8.constData(), "sync", FALSE, nullptr);

    if (gst_element_set_state(pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE)
    {
        result.errorMessage = QStringLiteral("Failed to start the GStreamer export pipeline.");
        pipelineGuard.stop();
        QFile::remove(request.outputPath);
        return result;
    }

    const int totalFrames = request.lastFrameIndex - request.firstFrameIndex + 1;
    int exportedFrames = 0;
    QString frameError;
    const bool framesVisited = frameProvider.visitFrames(
        request.firstFrameIndex,
        request.lastFrameIndex,
        [&](int, const QImage& sourceFrame, QString* consumerError) {
            if (cancelled(isCancelled))
            {
                return false;
            }
            if (sourceFrame.size() != frameSize)
            {
                if (consumerError)
                {
                    *consumerError = QStringLiteral("A decoded cine frame has inconsistent dimensions.");
                }
                return false;
            }

            const QImage frame = sourceFrame.format() == QImage::Format_RGB888
                ? sourceFrame
                : sourceFrame.convertToFormat(QImage::Format_RGB888);
            const gsize frameBytes =
                static_cast<gsize>(frameSize.width()) *
                static_cast<gsize>(frameSize.height()) *
                3U;
            GstBuffer* buffer = gst_buffer_new_allocate(nullptr, frameBytes, nullptr);
            if (!buffer)
            {
                if (consumerError)
                {
                    *consumerError = QStringLiteral("Failed to allocate a GStreamer frame buffer.");
                }
                return false;
            }

            GstMapInfo mapInfo{};
            if (!gst_buffer_map(buffer, &mapInfo, GST_MAP_WRITE))
            {
                gst_buffer_unref(buffer);
                if (consumerError)
                {
                    *consumerError = QStringLiteral("Failed to map a GStreamer frame buffer.");
                }
                return false;
            }

            const qsizetype rowBytes = static_cast<qsizetype>(frameSize.width()) * 3;
            for (int row = 0; row < frameSize.height(); ++row)
            {
                std::copy_n(
                    frame.constScanLine(row),
                    rowBytes,
                    mapInfo.data + (static_cast<qsizetype>(row) * rowBytes));
            }
            gst_buffer_unmap(buffer, &mapInfo);

            GST_BUFFER_PTS(buffer) = static_cast<GstClockTime>(exportedFrames) * frameDuration;
            GST_BUFFER_DTS(buffer) = GST_CLOCK_TIME_NONE;
            GST_BUFFER_DURATION(buffer) = frameDuration;

            const GstFlowReturn flowResult = gst_app_src_push_buffer(GST_APP_SRC(source), buffer);
            if (flowResult != GST_FLOW_OK)
            {
                if (consumerError)
                {
                    *consumerError = QStringLiteral("GStreamer rejected an exported cine frame.");
                }
                return false;
            }

            ++exportedFrames;
            if (progressCallback)
            {
                progressCallback(exportedFrames, totalFrames);
            }
            return true;
        },
        isCancelled,
        &frameError);

    if (!framesVisited || cancelled(isCancelled))
    {
        result.cancelled = cancelled(isCancelled);
        result.errorMessage = result.cancelled
            ? QStringLiteral("Video export was cancelled.")
            : (frameError.isEmpty() ? QStringLiteral("Failed to supply cine frames.") : frameError);
        pipelineGuard.stop();
        QFile::remove(request.outputPath);
        return result;
    }

    if (gst_app_src_end_of_stream(GST_APP_SRC(source)) != GST_FLOW_OK)
    {
        result.errorMessage = QStringLiteral("Failed to finalize the GStreamer source stream.");
        pipelineGuard.stop();
        QFile::remove(request.outputPath);
        return result;
    }

    GstBus* bus = gst_element_get_bus(pipeline);
    const qint64 deadlineMs = QDateTime::currentMSecsSinceEpoch() + kPipelineFinalizeTimeoutMs;
    bool reachedEndOfStream = false;
    while (!reachedEndOfStream && QDateTime::currentMSecsSinceEpoch() < deadlineMs)
    {
        if (cancelled(isCancelled))
        {
            result.cancelled = true;
            result.errorMessage = QStringLiteral("Video export was cancelled.");
            break;
        }

        GstMessage* message = gst_bus_timed_pop_filtered(
            bus,
            kPipelineMessagePollNs,
            static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
        if (!message)
        {
            continue;
        }

        if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_ERROR)
        {
            result.errorMessage = gstreamerErrorMessage(message);
        }
        else if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_EOS)
        {
            reachedEndOfStream = true;
        }
        gst_message_unref(message);
    }
    gst_object_unref(bus);

    if (!reachedEndOfStream)
    {
        if (result.errorMessage.isEmpty())
        {
            result.errorMessage = QStringLiteral("Timed out while finalizing the exported video.");
        }
        pipelineGuard.stop();
        QFile::remove(request.outputPath);
        return result;
    }

    result.exportedFrameCount = exportedFrames;
    result.durationMs = static_cast<qint64>(
        std::llround((static_cast<double>(exportedFrames) / request.framesPerSecond) * 1000.0));
    result.success = exportedFrames == totalFrames && QFileInfo(request.outputPath).size() > 0;
    if (!result.success)
    {
        result.errorMessage = QStringLiteral("The exported video file is incomplete.");
        pipelineGuard.stop();
        QFile::remove(request.outputPath);
    }
    return result;
}
