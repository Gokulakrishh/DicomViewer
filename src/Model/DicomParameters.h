#pragma once

#include "Model/DicomImage.h"

#include <QString>
#include <QVector>
#include <cstddef>
#include <memory>
#include <unordered_map>

struct QStringHash
{
    std::size_t operator()(const QString& value) const noexcept
    {
        return static_cast<std::size_t>(qHash(value));
    }
};

class Series
{
public:
    using ImagePtr = std::shared_ptr<DicomImage>;

    QString seriesInstanceUID;
    QString seriesDescription;
    QString modality;
    QString seriesNumber;

    void addImage(ImagePtr img)
    {
        images.push_back(std::move(img));
    }

private:
    QVector<ImagePtr> images;
};

class Study
{
public:
    using SeriesPtr = std::shared_ptr<Series>;

    QString studyInstanceUID;
    QString studyDescription;
    QString studyDate;
    QString doctorName;

    SeriesPtr getOrCreateSeries(const QString& seriesUID)
    {
        auto [it, inserted] = seriesMap.try_emplace(seriesUID, nullptr);
        if (inserted)
        {
            auto series = std::make_shared<Series>();
            series->seriesInstanceUID = seriesUID;
            it->second = std::move(series);
        }
        return it->second;
    }

private:
    std::unordered_map<QString, SeriesPtr, QStringHash> seriesMap;
};

class Patient
{
public:
    using StudyPtr = std::shared_ptr<Study>;

    QString patientID;
    QString patientName;
    QString patientSex;
    QString dateOfBirth;

    StudyPtr getOrCreateStudy(const QString& studyUID)
    {
        auto [it, inserted] = studyMap.try_emplace(studyUID, nullptr);
        if (inserted)
        {
            auto study = std::make_shared<Study>();
            study->studyInstanceUID = studyUID;
            it->second = std::move(study);
        }
        return it->second;
    }

private:
    std::unordered_map<QString, StudyPtr, QStringHash> studyMap;
};
