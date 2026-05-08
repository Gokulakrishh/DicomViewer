#pragma once

#include "Model/MeasurementAnnotationRecord.h"

#include <QList>
#include <QString>

class DatabaseService;

/**
 * @brief Persistence facade for slice measurement annotations.
 *
 * Responsibilities:
 * - Save, load, and soft-delete measurement records for one DICOM slice.
 * - Keep viewer code independent of database-specific APIs.
 *
 * Assumptions:
 * - Records are scoped by SOP Instance UID and frame index, and stored outside
 *   source DICOM files.
 */
class MeasurementAnnotationStore
{
public:
    /**
     * @brief Creates an annotation store.
     * @param databaseService Database service used for persistence.
     */
    explicit MeasurementAnnotationStore(DatabaseService& databaseService);

    /**
     * @brief Inserts or updates a slice annotation.
     * @param record Annotation record to save.
     * @return True when saved.
     */
    [[nodiscard]] bool upsertSliceAnnotation(const SliceMeasurementAnnotationRecord& record);

    /**
     * @brief Loads active annotations for one DICOM slice/frame.
     * @param sopInstanceUid DICOM SOP Instance UID.
     * @param frameIndex Zero-based frame index for multi-frame instances.
     * @return Non-deleted annotations for the slice/frame.
     */
    [[nodiscard]] QList<SliceMeasurementAnnotationRecord> loadSliceAnnotations(
        const QString& sopInstanceUid,
        int frameIndex = 0) const;

    /**
     * @brief Soft-deletes an annotation.
     * @param annotationId Stable annotation identifier.
     * @return True when deleted.
     */
    [[nodiscard]] bool deleteSliceAnnotation(const QString& annotationId);

private:
    DatabaseService& m_databaseService;
};
