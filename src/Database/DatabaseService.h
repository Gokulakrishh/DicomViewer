#pragma once

#include "Model/MeasurementAnnotationRecord.h"
#include "Model/DicomParameters.h"

#include <QList>
#include <QPixmap>
#include <QString>
#include <memory>

class DatabaseService
{
public:
    using PatientPtr = std::shared_ptr<Patient>;
    using StudyPtr = std::shared_ptr<Study>;
    using SeriesPtr = std::shared_ptr<Series>;
    using DicomImagePtr = std::shared_ptr<DicomImage>;

    virtual ~DatabaseService() = default;

    virtual bool initialize() = 0;
    virtual QString lastErrorText() const = 0;

    virtual bool savePatient(const PatientPtr& patient) = 0;
    virtual PatientPtr getPatient(const QString& patientId) = 0;
    virtual QList<PatientPtr> getAllPatients(const QString& filterText = {}) = 0;
    virtual QList<StudyPtr> getStudiesForPatient(const QString& patientId, const QString& filterText = {}) = 0;
    virtual QList<SeriesPtr> getSeriesForStudy(const QString& studyInstanceUid, const QString& filterText = {}) = 0;

    virtual StudyPtr getStudy(const QString& studyInstanceUid) = 0;
    virtual SeriesPtr getSeries(const QString& seriesInstanceUid) = 0;
    virtual DicomImagePtr getImage(const QString& sopInstanceUid) = 0;
    virtual bool upsertSliceMeasurementAnnotation(const SliceMeasurementAnnotationRecord& record) = 0;
    virtual QList<SliceMeasurementAnnotationRecord> loadSliceMeasurementAnnotations(const QString& sopInstanceUid) = 0;
    virtual bool markSliceMeasurementAnnotationDeleted(const QString& annotationId) = 0;
    virtual QPixmap getPreviewForPatient(const QString& patientId) = 0;
    virtual QPixmap getPreviewForStudy(const QString& studyInstanceUid) = 0;
    virtual QPixmap getPreviewForSeries(const QString& seriesInstanceUid) = 0;
};
