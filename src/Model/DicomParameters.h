#pragma once

#include "Model/DicomImage.h"

#include <QHash>
#include <QString>
#include <QVector>
#include <memory>

class Series
{
public:
    QString seriesInstanceUID;
    QString seriesDescription;
    QString modality;
    QString seriesNumber;

    void addImage(std::unique_ptr<DicomImage> img)
    {
        images.push_back(std::move(img));
    }

private:
    QVector<std::unique_ptr<DicomImage>> images;
};

class Study
{
public:
    QString studyInstanceUID;
    QString studyDescription;
    QString studyDate;
    QString doctorName;

    Series& getOrCreateSeries(const QString& seriesUID)
    {
        auto it = seriesMap.find(seriesUID);
        if (it == seriesMap.end())
        {
            auto series = std::make_unique<Series>();
            series->seriesInstanceUID = seriesUID;
            it = seriesMap.insert(seriesUID, std::move(series));
        }
        return *it.value();
    }

private:
    QHash<QString, std::unique_ptr<Series>> seriesMap;
};

class Patient
{
public:
    QString patientID;
    QString patientName;
    QString patientSex;
    QString dateOfBirth;

    Study& getOrCreateStudy(const QString& studyUID)
    {
        auto it = studyMap.find(studyUID);
        if (it == studyMap.end())
        {
            auto study = std::make_unique<Study>();
            study->studyInstanceUID = studyUID;
            it = studyMap.insert(studyUID, std::move(study));
        }
        return *it.value();
    }

private:
    QHash<QString, std::unique_ptr<Study>> studyMap;
};
