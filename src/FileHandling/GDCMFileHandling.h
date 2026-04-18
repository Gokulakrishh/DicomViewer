#pragma once

#include "FileHandling.h"

#include <QStringList>
#include <gdcmImageReader.h>
#include <gdcmStringFilter.h>

class GDCMFileHandling : public FileHandling
{
public:
    GDCMFileHandling();

    PatientList loadDicomFolder(const QString& folderPath, ProgressCallback progressCallback = {}) override;
    std::unique_ptr<MedicalImage> loadImage(const QString& filePath) override;
    std::unique_ptr<DicomImage> loadImageData(const QString& filePath) const override;
    PatientPtr loadDicomHierarchy(const QString& filePath) override;
    QStringList getSupportedFormats() const override;
    bool canLoad(const QString& filePath) const override;

private:
    void mergePatientHierarchy(const PatientPtr& sourcePatient, Patient& targetPatient) const;
    QString normalizeDicomDate(const QString& dicomDate) const;
    QString readStringTag(const gdcm::StringFilter& stringFilter, uint16_t group, uint16_t element) const;
    std::unique_ptr<DicomImage> loadDicomImage(
        const QString& filePath,
        const gdcm::ImageReader& reader,
        bool renderPixmap) const;
    PatientPtr buildHierarchy(const QString& filePath, const gdcm::ImageReader& reader) const;

private:
    QStringList m_supportedFormats;
};
