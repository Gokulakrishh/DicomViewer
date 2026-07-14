#pragma once

#include <QImage>
#include <QSize>
#include <QString>

#include <functional>

/**
 * @brief Supplies deterministic, display-ready frames to a video encoder.
 *
 * Responsibilities:
 * - Hide DICOM decoding and WL/WW conversion from the encoder.
 * - Visit only the requested source frames in ascending order.
 * - Respect cancellation without modifying viewer or source DICOM state.
 *
 * Assumptions:
 * - A provider instance represents one ordered export source such as a
 *   multi-frame SOP or a slice series.
 * - Visited images are temporary and must not be retained by the consumer.
 */
class IVideoFrameProvider
{
public:
    using FrameConsumer = std::function<bool(int frameIndex, const QImage& frame, QString* errorMessage)>;
    using CancellationCheck = std::function<bool()>;

    virtual ~IVideoFrameProvider() = default;

    /** @brief Returns the number of frames in the represented source. */
    virtual int frameCount() const = 0;

    /** @brief Returns the expected output frame dimensions. */
    virtual QSize frameSize() const = 0;

    /**
     * @brief Visits a selected inclusive frame range in ascending order.
     * @param firstFrameIndex First zero-based source frame.
     * @param lastFrameIndex Last zero-based source frame.
     * @param consumer Consumer invoked once for every decoded frame.
     * @param isCancelled Cancellation callback.
     * @param errorMessage Receives a recoverable failure description.
     * @return True when all requested frames were visited.
     */
    virtual bool visitFrames(
        int firstFrameIndex,
        int lastFrameIndex,
        const FrameConsumer& consumer,
        const CancellationCheck& isCancelled,
        QString* errorMessage) = 0;
};
