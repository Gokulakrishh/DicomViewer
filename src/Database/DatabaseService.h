#pragma once

#include "Model/DicomParameters.h"

#include <QList>
#include <QString>

class DatabaseService
{
public:
    virtual ~DatabaseService() = default;

    virtual void savePatient(const Patient& patient) = 0;
    virtual Patient* getPatient(const QString& patientID) = 0;
    virtual QList<Patient*> getAllPatients() = 0;

    virtual void saveStudy(const Study& study) = 0;
    virtual Study* getStudy(const QString& studyUID) = 0;

    virtual void saveSeries(const Series& series) = 0;
    virtual Series* getSeries(const QString& seriesUID) = 0;
};
