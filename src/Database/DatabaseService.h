#pragma once

#include "Model/DicomParameters.h"

#include <QList>
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
    virtual QList<PatientPtr> getAllPatients() = 0;

    virtual StudyPtr getStudy(const QString& studyInstanceUid) = 0;
    virtual SeriesPtr getSeries(const QString& seriesInstanceUid) = 0;
    virtual DicomImagePtr getImage(const QString& sopInstanceUid) = 0;
};
