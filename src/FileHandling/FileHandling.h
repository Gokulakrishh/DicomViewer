#pragma once

#include "Model/DicomParameters.h"
#include "Model/MedicalImage.h"

#include <QList>
#include <memory>
#include <QStringList>
#include <functional>

class FileHandling
{
public:
    using PatientPtr = std::shared_ptr<Patient>;
    using PatientList = QList<PatientPtr>;
    using ProgressCallback = std::function<void(int current, int total)>;

    virtual ~FileHandling() = default;
    virtual PatientList loadDicomFolder(const QString& folderPath, ProgressCallback progressCallback = {}) = 0;
    virtual std::unique_ptr<MedicalImage> loadImage(const QString& filePath) = 0;
    virtual std::unique_ptr<DicomImage> loadImageData(const QString& filePath) const = 0;
    virtual PatientPtr loadDicomHierarchy(const QString& filePath) = 0;
    virtual QStringList getSupportedFormats() const = 0;
    virtual bool canLoad(const QString& filePath) const = 0;
};
