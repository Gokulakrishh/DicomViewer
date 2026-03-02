#pragma once
#include "Model/DicomParameters.h".h"

#include <QString>
#include <QList>

class DatabaseService {
public:
    virtual ~DatabaseService() = default;

    // Patient-level
    virtual void savePatient(const Patient& patient) = 0;
    virtual Patient* getPatient(const QString& patientID) = 0;
    virtual QList<Patient*> getAllPatients() = 0;

    // Study-level
    virtual void saveStudy(const Study& study) = 0;
    virtual Study* getStudy(const QString& studyUID) = 0;

    // Series-level
    virtual void saveSeries(const Series& series) = 0;
    virtual Series* getSeries(const QString& seriesUID) = 0;
};
