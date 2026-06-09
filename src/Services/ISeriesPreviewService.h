#pragma once

#include "Model/DicomPreviewItem.h"

/**
 * @brief Application boundary for on-demand DICOM browser preview generation.
 *
 * Responsibilities:
 * - Resolve missing study/series thumbnails without changing tree-model data.
 * - Keep preview generation separate from folder import and viewer loading.
 * - Return bounded derived pixmaps suitable for the study browser UI.
 *
 * Assumptions:
 * - Preview generation may decode one representative DICOM image, but must not
 *   preload all slices in a series.
 * - Source DICOM files remain the canonical image data; persisted previews are
 *   disposable derived UI artifacts.
 */
class ISeriesPreviewService
{
public:
    virtual ~ISeriesPreviewService() = default;

    /**
     * @brief Ensures preview pixmaps are present for the provided items.
     * @param items Study or series preview/navigation items.
     * @return Items with available generated/persisted pixmaps filled in.
     */
    virtual DicomPreviewItems ensurePreviewPixmaps(const DicomPreviewItems& items) = 0;
};
