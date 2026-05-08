#pragma once

#include "Model/DicomImage.h"

#include <QString>
#include <map>
#include <memory>
#include <vector>

/**
 * @brief Lightweight DICOM series model used by browser, loading, and viewers.
 *
 * Responsibilities:
 * - Store series identifiers and display metadata.
 * - Own lightweight DicomImage entries for slices in the series.
 *
 * Assumptions:
 * - DicomImage entries may contain only metadata until pixels are loaded on
 *   demand.
 */
class Series
{
public:
    /** @brief Returns the DICOM Series Instance UID. */
    const QString& seriesInstanceUid() const { return m_seriesInstanceUid; }
    /** @brief Returns the series description. */
    const QString& seriesDescription() const { return m_seriesDescription; }
    /** @brief Returns the DICOM modality. */
    const QString& modality() const { return m_modality; }
    /** @brief Returns the DICOM series number. */
    const QString& seriesNumber() const { return m_seriesNumber; }
    /** @brief Returns the representative preview pixmap. */
    const QPixmap& previewPixmap() const { return m_previewPixmap; }
    /** @brief Returns a representative source DICOM file path. */
    const QString& representativeFilePath() const { return m_representativeFilePath; }
    /** @brief Returns persisted image count when slices are not fully loaded. */
    int imageCount() const { return m_imageCount; }

    /** @brief Sets the DICOM Series Instance UID. */
    void setSeriesInstanceUid(const QString& seriesInstanceUid) { m_seriesInstanceUid = seriesInstanceUid; }
    /** @brief Sets the series description. */
    void setSeriesDescription(const QString& seriesDescription) { m_seriesDescription = seriesDescription; }
    /** @brief Sets the DICOM modality. */
    void setModality(const QString& modality) { m_modality = modality; }
    /** @brief Sets the DICOM series number. */
    void setSeriesNumber(const QString& seriesNumber) { m_seriesNumber = seriesNumber; }
    /** @brief Sets the representative preview pixmap. */
    void setPreviewPixmap(const QPixmap& previewPixmap) { m_previewPixmap = previewPixmap; }
    /** @brief Sets a representative source DICOM file path. */
    void setRepresentativeFilePath(const QString& representativeFilePath) { m_representativeFilePath = representativeFilePath; }
    /** @brief Sets the persisted image count. */
    void setImageCount(int imageCount) { m_imageCount = imageCount; }

    /**
     * @brief Adds a DICOM image to the series.
     * @param img Image metadata/pixel container; ownership is transferred.
     */
    void addImage(std::unique_ptr<DicomImage> img)
    {
        m_images.push_back(std::move(img));
    }

    /** @brief Returns mutable image entries owned by the series. */
    std::vector<std::unique_ptr<DicomImage>>& images() { return m_images; }
    /** @brief Returns image entries owned by the series. */
    const std::vector<std::unique_ptr<DicomImage>>& images() const { return m_images; }

private:
    QString m_seriesInstanceUid;
    QString m_seriesDescription;
    QString m_modality;
    QString m_seriesNumber;
    QPixmap m_previewPixmap;
    QString m_representativeFilePath;
    int m_imageCount{0};
    std::vector<std::unique_ptr<DicomImage>> m_images;
};

/**
 * @brief Lightweight DICOM study model containing series.
 *
 * Responsibilities:
 * - Store study-level identifiers and display metadata.
 * - Own series grouped under the study.
 */
class Study
{
public:
    /** @brief Returns the DICOM Study Instance UID. */
    const QString& studyInstanceUid() const { return m_studyInstanceUid; }
    /** @brief Returns the study description. */
    const QString& studyDescription() const { return m_studyDescription; }
    /** @brief Returns the DICOM study date. */
    const QString& studyDate() const { return m_studyDate; }
    /** @brief Returns the referring/performing doctor name captured for display. */
    const QString& doctorName() const { return m_doctorName; }

    /** @brief Sets the DICOM Study Instance UID. */
    void setStudyInstanceUid(const QString& studyInstanceUid) { m_studyInstanceUid = studyInstanceUid; }
    /** @brief Sets the study description. */
    void setStudyDescription(const QString& studyDescription) { m_studyDescription = studyDescription; }
    /** @brief Sets the DICOM study date. */
    void setStudyDate(const QString& studyDate) { m_studyDate = studyDate; }
    /** @brief Sets the doctor/referrer display name. */
    void setDoctorName(const QString& doctorName) { m_doctorName = doctorName; }

    /**
     * @brief Returns an existing series or creates one.
     * @param seriesUid DICOM Series Instance UID.
     * @return Mutable series reference owned by the study.
     */
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

    /** @brief Returns all series keyed by Series Instance UID. */
    const std::map<QString, std::unique_ptr<Series>>& seriesMap() const { return m_seriesMap; }

private:
    QString m_studyInstanceUid;
    QString m_studyDescription;
    QString m_studyDate;
    QString m_doctorName;
    std::map<QString, std::unique_ptr<Series>> m_seriesMap;
};

/**
 * @brief Lightweight DICOM patient model containing studies.
 *
 * Responsibilities:
 * - Store patient-level identifiers and demographics used by the browser.
 * - Own study hierarchy without duplicating image pixel data.
 */
class Patient
{
public:
    /** @brief Returns the DICOM Patient ID. */
    const QString& patientId() const { return m_patientId; }
    /** @brief Returns the DICOM Patient Name. */
    const QString& patientName() const { return m_patientName; }
    /** @brief Returns the DICOM Patient Sex. */
    const QString& patientSex() const { return m_patientSex; }
    /** @brief Returns the DICOM Patient Birth Date. */
    const QString& dateOfBirth() const { return m_dateOfBirth; }

    /** @brief Sets the DICOM Patient ID. */
    void setPatientId(const QString& patientId) { m_patientId = patientId; }
    /** @brief Sets the DICOM Patient Name. */
    void setPatientName(const QString& patientName) { m_patientName = patientName; }
    /** @brief Sets the DICOM Patient Sex. */
    void setPatientSex(const QString& patientSex) { m_patientSex = patientSex; }
    /** @brief Sets the DICOM Patient Birth Date. */
    void setDateOfBirth(const QString& dateOfBirth) { m_dateOfBirth = dateOfBirth; }

    /**
     * @brief Returns an existing study or creates one.
     * @param studyUid DICOM Study Instance UID.
     * @return Mutable study reference owned by the patient.
     */
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

    /** @brief Returns all studies keyed by Study Instance UID. */
    const std::map<QString, std::unique_ptr<Study>>& studyMap() const { return m_studyMap; }

private:
    QString m_patientId;
    QString m_patientName;
    QString m_patientSex;
    QString m_dateOfBirth;
    std::map<QString, std::unique_ptr<Study>> m_studyMap;
};
