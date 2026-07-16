#include "Services/MprMeasurementAnnotationStore.h"

#include "Database/DatabaseService.h"

MprMeasurementAnnotationStore::MprMeasurementAnnotationStore(DatabaseService& databaseService)
    : m_databaseService(databaseService)
{
}

bool MprMeasurementAnnotationStore::upsertMprAnnotation(const MprMeasurementAnnotationRecord& record)
{
    return m_databaseService.upsertMprMeasurementAnnotation(record);
}

QList<MprMeasurementAnnotationRecord> MprMeasurementAnnotationStore::loadMprAnnotations(
    const QString& seriesInstanceUid) const
{
    return m_databaseService.loadMprMeasurementAnnotations(seriesInstanceUid);
}

bool MprMeasurementAnnotationStore::deleteMprAnnotation(const QString& annotationId)
{
    return m_databaseService.markMprMeasurementAnnotationDeleted(annotationId);
}

bool MprMeasurementAnnotationStore::updateMprAnnotationMetadata(
    const QString& annotationId,
    const QString& label,
    const QString& bodyRegion,
    const QString& note)
{
    return m_databaseService.updateMprMeasurementAnnotationMetadata(annotationId, label, bodyRegion, note);
}
