#pragma once

#include "Model/DicomImage.h"
#include <QString>
#include <QVector>

class Series
{
public:
    QString seriesInstanceUID;
    QString seriesDescription;
    QString modality; //Ct, MR..
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
            auto s = std::make_unique<Series>();
            s->seriesInstanceUID = seriesUID;
            it = seriesMap.emplace(seriesUID, std::move(s)).first;
        }
        return *it->second;
    }

private:
    std::unordered_map<QString, std::unique_ptr<Series>> seriesMap;
};

class Patient
{
public:
    QString patientID;
    QString patientName;
    QString patientSex;
    QString dateofBirth;

    Study& getOrCreateStudy(const QString& studyUID)
    {
        auto it = studyMap.find(studyUID);
        if (it == studyMap.end())
        {
            auto s = std::make_unique<Study>();
            s->studyInstanceUID = studyUID;
            it = studyMap.emplace(studyUID, std::move(s)).first;
        }
        return *it->second;
    }

private:
    std::unordered_map<QString, std::unique_ptr<Study>> studyMap;
};
