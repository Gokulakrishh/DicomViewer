#pragma once

#include "Model/MeasurementAnnotationRecord.h"

#include <QList>
#include <QString>

class DatabaseService;

/**
 * @brief Persistence facade for MPR measurement annotations.
 *
 * Responsibilities:
 * - Save, load, and soft-delete derived MPR annotation records.
 * - Keep MPR viewer code independent of database-specific APIs.
 *
 * Assumptions:
 * - Records are scoped to a DICOM series and MPR plane context, not to an
 *   original source SOP/frame unless explicitly modeled later.
 */
class MprMeasurementAnnotationStore
{
public:
    /**
     * @brief Creates an MPR annotation store.
     * @param databaseService Database service used for persistence.
     */
    explicit MprMeasurementAnnotationStore(DatabaseService& databaseService);

    /**
     * @brief Inserts or updates an MPR annotation.
     * @param record Annotation record to save.
     * @return True when saved.
     */
    [[nodiscard]] bool upsertMprAnnotation(const MprMeasurementAnnotationRecord& record);

    /**
     * @brief Loads active MPR annotations for one series.
     * @param seriesInstanceUid DICOM Series Instance UID.
     * @return Non-deleted annotations for the series.
     */
    [[nodiscard]] QList<MprMeasurementAnnotationRecord> loadMprAnnotations(
        const QString& seriesInstanceUid) const;

    /**
     * @brief Soft-deletes an MPR annotation.
     * @param annotationId Stable annotation identifier.
     * @return True when deleted.
     */
    [[nodiscard]] bool deleteMprAnnotation(const QString& annotationId);

    /**
     * @brief Updates editable metadata for an MPR annotation.
     * @param annotationId Stable annotation identifier.
     * @param label User-visible annotation name.
     * @param bodyRegion Body region/group label.
     * @param note Optional note text.
     * @return True when updated.
     */
    [[nodiscard]] bool updateMprAnnotationMetadata(
        const QString& annotationId,
        const QString& label,
        const QString& bodyRegion,
        const QString& note = {});

private:
    DatabaseService& m_databaseService;
};
