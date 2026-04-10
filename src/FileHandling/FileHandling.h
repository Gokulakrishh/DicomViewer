#pragma once

#include "Model/DicomParameters.h"
#include "Model/MedicalImage.h"

#include <QList>
#include <memory>
#include <QStringList>

class FileHandling
{
public:
    using PatientPtr = std::shared_ptr<Patient>;
    using PatientList = QList<PatientPtr>;

    virtual ~FileHandling() = default;
    virtual PatientList loadDicomFolder(const QString& folderPath) = 0;
    virtual std::unique_ptr<MedicalImage> loadImage(const QString& filePath) = 0;
    virtual PatientPtr loadDicomHierarchy(const QString& filePath) = 0;
    virtual QStringList getSupportedFormats() const = 0;
    virtual bool canLoad(const QString& filePath) const = 0;
};
