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

    virtual ~DatabaseService() = default;

    virtual void savePatient(const PatientPtr& patient) = 0;
    virtual PatientPtr getPatient(const QString& patientID) = 0;
    virtual QList<PatientPtr> getAllPatients() = 0;

    virtual void saveStudy(const StudyPtr& study) = 0;
    virtual StudyPtr getStudy(const QString& studyUID) = 0;

    virtual void saveSeries(const SeriesPtr& series) = 0;
    virtual SeriesPtr getSeries(const QString& seriesUID) = 0;
};
