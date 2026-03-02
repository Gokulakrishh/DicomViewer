#pragma once

#include "Model/MedicalImage.h"
#include <memory>
#include <QStringList>

class FileHandling
{
    
public:

    virtual ~FileHandling() = default;
    virtual loafDicomFolder(const QString& folderPath) = 0;
    virtual std::unique_ptr<MedicalImage> loadImage(const QString& filePath) = 0;
    virtual QStringList getSupportedFormats() const = 0;
    virtual bool canLoad(const QString& filePath) const = 0;

};

