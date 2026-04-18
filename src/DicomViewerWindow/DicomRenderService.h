#pragma once

#include <memory>

class DicomImage;

class DicomRenderService
{
public:
    struct RenderSettings
    {
        int windowLevel{0};
        int windowWidth{100};
    };

    void ensureDiagnosticPixmap(DicomImage& image) const;
    std::shared_ptr<DicomImage> renderDiagnosticImage(const DicomImage& image, const RenderSettings& settings) const;
};
