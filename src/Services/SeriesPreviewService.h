#pragma once

#include "Services/ISeriesPreviewService.h"

#include <QPixmap>
#include <QString>

class DatabaseService;
class FileHandling;

/**
 * @brief Generates and persists bounded study-browser thumbnails on demand.
 *
 * Responsibilities:
 * - Decode one representative image/frame only when a missing preview is
 *   requested by the browser.
 * - Persist the derived thumbnail through DatabaseService for later reuse.
 * - Avoid coupling the browser controller to GDCM, rendering helpers, or SQLite.
 *
 * Assumptions:
 * - The service is called from UI/navigation workflows, not during bulk folder
 *   import.
 * - A missing or unsupported preview is recoverable and should leave the UI
 *   placeholder in place.
 */
class SeriesPreviewService final : public ISeriesPreviewService
{
public:
    /**
     * @brief Creates the preview service.
     * @param databaseService Persistence service used for metadata and preview storage.
     * @param fileHandling DICOM file-loading service used for one-frame preview decode.
     */
    SeriesPreviewService(DatabaseService& databaseService, FileHandling& fileHandling);

    /**
     * @brief Ensures preview pixmaps are available for study/series items.
     * @param items Items returned by the browser query.
     * @return Items with generated pixmaps where preview decode succeeded.
     */
    DicomPreviewItems ensurePreviewPixmaps(const DicomPreviewItems& items) override;

private:
    [[nodiscard]] QPixmap ensureStudyPreview(const QString& studyInstanceUid);
    [[nodiscard]] QPixmap ensureSeriesPreview(const QString& seriesInstanceUid);

    DatabaseService& m_databaseService;
    FileHandling& m_fileHandling;
};
