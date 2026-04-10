#pragma once

#include "Model/DicomImage.h"

#include <QString>
#include <map>
#include <memory>
#include <vector>

class Series
{
public:
    const QString& seriesInstanceUid() const { return m_seriesInstanceUid; }
    const QString& seriesDescription() const { return m_seriesDescription; }
    const QString& modality() const { return m_modality; }
    const QString& seriesNumber() const { return m_seriesNumber; }

    void setSeriesInstanceUid(const QString& seriesInstanceUid) { m_seriesInstanceUid = seriesInstanceUid; }
    void setSeriesDescription(const QString& seriesDescription) { m_seriesDescription = seriesDescription; }
    void setModality(const QString& modality) { m_modality = modality; }
    void setSeriesNumber(const QString& seriesNumber) { m_seriesNumber = seriesNumber; }

    void addImage(std::unique_ptr<DicomImage> img)
    {
        m_images.push_back(std::move(img));
    }

    const std::vector<std::unique_ptr<DicomImage>>& images() const { return m_images; }

private:
    QString m_seriesInstanceUid;
    QString m_seriesDescription;
    QString m_modality;
    QString m_seriesNumber;
    std::vector<std::unique_ptr<DicomImage>> m_images;
};

class Study
{
public:
    const QString& studyInstanceUid() const { return m_studyInstanceUid; }
    const QString& studyDescription() const { return m_studyDescription; }
    const QString& studyDate() const { return m_studyDate; }
    const QString& doctorName() const { return m_doctorName; }

    void setStudyInstanceUid(const QString& studyInstanceUid) { m_studyInstanceUid = studyInstanceUid; }
    void setStudyDescription(const QString& studyDescription) { m_studyDescription = studyDescription; }
    void setStudyDate(const QString& studyDate) { m_studyDate = studyDate; }
    void setDoctorName(const QString& doctorName) { m_doctorName = doctorName; }

    Series& getOrCreateSeries(const QString& seriesUid)
    {
        auto it = m_seriesMap.find(seriesUid);
        if (it == m_seriesMap.end())
        {
            auto series = std::make_unique<Series>();
            series->setSeriesInstanceUid(seriesUid);
            it = m_seriesMap.emplace(seriesUid, std::move(series)).first;
        }
        return *it->second;
    }

    const std::map<QString, std::unique_ptr<Series>>& seriesMap() const { return m_seriesMap; }

private:
    QString m_studyInstanceUid;
    QString m_studyDescription;
    QString m_studyDate;
    QString m_doctorName;
    std::map<QString, std::unique_ptr<Series>> m_seriesMap;
};

class Patient
{
public:
    const QString& patientId() const { return m_patientId; }
    const QString& patientName() const { return m_patientName; }
    const QString& patientSex() const { return m_patientSex; }
    const QString& dateOfBirth() const { return m_dateOfBirth; }

    void setPatientId(const QString& patientId) { m_patientId = patientId; }
    void setPatientName(const QString& patientName) { m_patientName = patientName; }
    void setPatientSex(const QString& patientSex) { m_patientSex = patientSex; }
    void setDateOfBirth(const QString& dateOfBirth) { m_dateOfBirth = dateOfBirth; }

    Study& getOrCreateStudy(const QString& studyUid)
    {
        auto it = m_studyMap.find(studyUid);
        if (it == m_studyMap.end())
        {
            auto study = std::make_unique<Study>();
            study->setStudyInstanceUid(studyUid);
            it = m_studyMap.emplace(studyUid, std::move(study)).first;
        }
        return *it->second;
    }

    const std::map<QString, std::unique_ptr<Study>>& studyMap() const { return m_studyMap; }

private:
    QString m_patientId;
    QString m_patientName;
    QString m_patientSex;
    QString m_dateOfBirth;
    std::map<QString, std::unique_ptr<Study>> m_studyMap;
};
