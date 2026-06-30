#pragma once

#include "Services/VideoExport/VideoExportTypes.h"

#include <functional>

class IVideoFrameProvider;

/**
 * @brief Encoding boundary for derived cine video export.
 *
 * Concrete implementations own third-party encoder integration. Callers remain
 * independent of GStreamer types and execute this synchronous operation on a
 * worker thread.
 */
class IVideoExportService
{
public:
    using ProgressCallback = std::function<void(int completedFrames, int totalFrames)>;
    using CancellationCheck = std::function<bool()>;

    virtual ~IVideoExportService() = default;

    /**
     * @brief Exports frames supplied by a provider.
     * @param request Validated export intent and output policy.
     * @param frameProvider Source-frame provider.
     * @param progressCallback Optional frame progress callback.
     * @param isCancelled Optional cancellation callback.
     * @return Export result with audit and verification fields.
     */
    virtual VideoExportResult exportVideo(
        const VideoExportRequest& request,
        IVideoFrameProvider& frameProvider,
        const ProgressCallback& progressCallback = {},
        const CancellationCheck& isCancelled = {}) const = 0;
};
