#include "Services/MeasurementAnnotationStore.h"

#include "Database/DatabaseService.h"

MeasurementAnnotationStore::MeasurementAnnotationStore(DatabaseService& databaseService)
    : m_databaseService(databaseService)
{
}

bool MeasurementAnnotationStore::upsertSliceAnnotation(const SliceMeasurementAnnotationRecord& record)
{
    return m_databaseService.upsertSliceMeasurementAnnotation(record);
}

QList<SliceMeasurementAnnotationRecord> MeasurementAnnotationStore::loadSliceAnnotations(const QString& sopInstanceUid) const
{
    return m_databaseService.loadSliceMeasurementAnnotations(sopInstanceUid);
}

bool MeasurementAnnotationStore::deleteSliceAnnotation(const QString& annotationId)
{
    return m_databaseService.markSliceMeasurementAnnotationDeleted(annotationId);
}
