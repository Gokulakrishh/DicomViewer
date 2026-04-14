#pragma once

#include <QString>

#include <memory>

class IVolumeData;
class QWidget;

class IAdvancedViewerLauncher
{
public:
    virtual ~IAdvancedViewerLauncher() = default;

    virtual QWidget* showMprVolume(
        std::shared_ptr<IVolumeData> volume,
        const QString& title,
        int windowLevel,
        int windowWidth,
        QWidget* parent = nullptr) = 0;
};
