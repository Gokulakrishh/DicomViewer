#pragma once

#include "Model/MeasurementAnnotationRecord.h"

#include <QList>
#include <QString>

class DatabaseService;

class MeasurementAnnotationStore
{
public:
    explicit MeasurementAnnotationStore(DatabaseService& databaseService);

    [[nodiscard]] bool upsertSliceAnnotation(const SliceMeasurementAnnotationRecord& record);
    [[nodiscard]] QList<SliceMeasurementAnnotationRecord> loadSliceAnnotations(const QString& sopInstanceUid) const;
    [[nodiscard]] bool deleteSliceAnnotation(const QString& annotationId);

private:
    DatabaseService& m_databaseService;
};
