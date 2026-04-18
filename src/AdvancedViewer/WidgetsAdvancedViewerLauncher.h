#pragma once

#include "IAdvancedViewerLauncher.h"

#include <memory>

class WidgetsAdvancedViewerLauncher final : public IAdvancedViewerLauncher
{
public:
    QWidget* showMprVolume(
        std::shared_ptr<IVolumeData> volume,
        const QString& title,
        int windowLevel,
        int windowWidth,
        QWidget* parent = nullptr) override;

    QWidget* showThreeDVolume(
        std::shared_ptr<IVolumeData> diagnosticVolume,
        const QString& title,
        ThreeDProfileSelection profileSelection,
        QWidget* parent = nullptr) override;
};
