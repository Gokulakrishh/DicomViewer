#pragma once

#include "FileHandling.h"
#include <memory>
#include <QStringList>

class GDCMFileHandler: public FileHandling
{
    
public:
    GDCMFileHandler();
    std::unique_ptr<MedicalImage> loadImage(const QString& filePath) override;
    QStringList getSupportedFormats() const override;
    bool canLoad(const QString& filePath) const override;
    
private:
    QStringList supportedFormats_;
};
