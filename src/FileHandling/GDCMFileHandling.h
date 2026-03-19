#pragma once

#include "FileHandling.h"
#include <QStringList>

class GDCMFileHandling : public FileHandling
{
public:
    GDCMFileHandling();

    void loadDicomFolder(const QString& folderPath) override;
    std::unique_ptr<MedicalImage> loadImage(const QString& filePath) override;
    QStringList getSupportedFormats() const override;
    bool canLoad(const QString& filePath) const override;

private:
    QStringList supportedFormats_;
};
