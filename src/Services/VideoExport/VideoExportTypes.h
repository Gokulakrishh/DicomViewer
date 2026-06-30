#pragma once

#include <QString>
#include <QStringList>

namespace VideoExportPolicy
{
inline constexpr int TargetBitrateKbps = 8000;
inline constexpr int KeyframeIntervalSeconds = 2;
inline constexpr char H264Profile[] = "baseline";
inline constexpr char EncoderPixelFormat[] = "NV12";
inline constexpr char Container[] = "MP4";
inline constexpr char Codec[] = "H.264";
inline constexpr char FileExtension[] = "mp4";
}

/**
 * @brief Identifies the timing source used for exported cine frames.
 */
enum class VideoExportTimingSource
{
    DicomFrameTime,
    DicomCineRate,
    Manual
};

/**
 * @brief Immutable input parameters for one derived cine export.
 *
 * Responsibilities:
 * - Identify the source SOP and selected zero-based frame range.
 * - Carry the requested constant playback rate and output location.
 * - State the privacy and non-diagnostic labeling policy for the output.
 *
 * Assumptions:
 * - The request contains identifiers only; it does not own DICOM pixels.
 * - Patient-identifying overlays are excluded from the Phase 1 workflow.
 */
struct VideoExportRequest
{
    QString outputPath;
    QString sourceSopInstanceUid;
    QString productVersion;
    int firstFrameIndex{0};
    int lastFrameIndex{0};
    double framesPerSecond{10.0};
    VideoExportTimingSource timingSource{VideoExportTimingSource::Manual};
    bool derivedNonDiagnosticOutput{true};
    bool includePatientOverlays{false};
};

/**
 * @brief Outcome and evidence fields produced by one video export attempt.
 */
struct VideoExportResult
{
    bool success{false};
    bool cancelled{false};
    QString outputPath;
    int exportedFrameCount{0};
    qint64 durationMs{0};
    QString timingSource;
    QString container;
    QString codec;
    QString encoder;
    QString h264Profile;
    QString pixelFormat;
    int targetBitrateKbps{0};
    int keyframeIntervalFrames{0};
    QString gstreamerVersion;
    QString errorMessage;
    QStringList warnings;
};

/**
 * @brief Returns a stable display/audit label for a timing source.
 * @param source Timing source value.
 * @return Human-readable timing source label.
 */
inline QString videoExportTimingSourceName(VideoExportTimingSource source)
{
    switch (source)
    {
    case VideoExportTimingSource::DicomFrameTime:
        return QStringLiteral("DICOM Frame Time");
    case VideoExportTimingSource::DicomCineRate:
        return QStringLiteral("DICOM Cine Rate");
    case VideoExportTimingSource::Manual:
        return QStringLiteral("Manual");
    }

    return QStringLiteral("Unknown");
}
