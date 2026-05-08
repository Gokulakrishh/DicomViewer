#pragma once

#include <QString>
#include <QPixmap>

/**
 * @brief Minimal renderable medical image interface.
 *
 * Responsibilities:
 * - Expose a Qt pixmap for display.
 * - Provide a validity check independent of concrete image format.
 *
 * Assumptions:
 * - Format-specific metadata belongs in derived classes such as DicomImage.
 */
class MedicalImage 
{

public:

    virtual ~MedicalImage() = default;

    /**
     * @brief Returns the display pixmap.
     * @return Pixmap used by Qt viewer surfaces.
     */
    virtual const QPixmap& pixmap() const = 0;

    /**
     * @brief Reports whether the image contains displayable data.
     * @return True when image data is valid for display.
     */
    virtual bool isValid() const = 0;

};
