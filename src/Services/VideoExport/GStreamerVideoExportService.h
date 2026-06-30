#pragma once

#include "Services/VideoExport/IVideoExportService.h"

/**
 * @brief GStreamer 1.24 adapter for controlled MP4/H.264 cine export.
 *
 * GStreamer types are intentionally confined to the implementation file so the
 * rest of the application remains independent of the SOUP API. The adapter
 * accepts only the approved platform encoder and never substitutes another
 * codec or container.
 */
class GStreamerVideoExportService final : public IVideoExportService
{
public:
    VideoExportResult exportVideo(
        const VideoExportRequest& request,
        IVideoFrameProvider& frameProvider,
        const ProgressCallback& progressCallback = {},
        const CancellationCheck& isCancelled = {}) const override;
};
