#pragma once

#include <memory>

class DicomImage;

class DicomRenderService
{
public:
    void ensureDefaultPixmap(DicomImage& image) const;
    std::shared_ptr<DicomImage> renderImage(const DicomImage& image, int windowLevel, int windowWidth) const;
};
