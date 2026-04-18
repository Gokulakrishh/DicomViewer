#pragma once

#include "Services/ThreeDProfiles/ThreeDProfileSelection.h"

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

    virtual QWidget* showThreeDVolume(
        std::shared_ptr<IVolumeData> diagnosticVolume,
        const QString& title,
        ThreeDProfileSelection profileSelection,
        QWidget* parent = nullptr) = 0;
};
