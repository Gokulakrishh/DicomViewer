#include "Model/DicomImage.h"

const QPixmap& DicomImage::pixmap() const
{
    return m_pixmap;
}

bool DicomImage::isValid() const
{
    return m_width > 0 && m_height > 0 && (!m_pixmap.isNull() || !m_rawPixels.empty());
}

const QString& DicomImage::filePath() const
{
    return m_filePath;
}

const QString& DicomImage::sopInstanceUid() const
{
    return m_sopInstanceUid;
}

const QString& DicomImage::instanceNumber() const
{
    return m_instanceNumber;
}

int DicomImage::width() const
{
    return m_width;
}

int DicomImage::height() const
{
    return m_height;
}

bool DicomImage::hasRawPixels() const
{
    return !m_rawPixels.empty() && m_width > 0 && m_height > 0;
}

bool DicomImage::isMonochrome() const
{
    return m_isMonochrome;
}

bool DicomImage::isMonochrome1() const
{
    return m_isMonochrome1;
}

int DicomImage::minimumStoredValue() const
{
    return m_minimumStoredValue;
}

int DicomImage::maximumStoredValue() const
{
    return m_maximumStoredValue;
}

int DicomImage::defaultWindowLevel() const
{
    return m_defaultWindowLevel;
}

int DicomImage::defaultWindowWidth() const
{
    return m_defaultWindowWidth;
}

bool DicomImage::hasPixelSpacing() const
{
    return m_pixelSpacingX > 0.0 && m_pixelSpacingY > 0.0;
}

double DicomImage::pixelSpacingX() const
{
    return m_pixelSpacingX;
}

double DicomImage::pixelSpacingY() const
{
    return m_pixelSpacingY;
}

bool DicomImage::hasImagePositionPatient() const
{
    return m_hasImagePositionPatient;
}

bool DicomImage::hasImageOrientationPatient() const
{
    return m_hasImageOrientationPatient;
}

const std::array<double, 3>& DicomImage::imagePositionPatient() const
{
    return m_imagePositionPatient;
}

const std::array<double, 6>& DicomImage::imageOrientationPatient() const
{
    return m_imageOrientationPatient;
}

double DicomImage::sliceThickness() const
{
    return m_sliceThickness;
}

double DicomImage::spacingBetweenSlices() const
{
    return m_spacingBetweenSlices;
}

int DicomImage::rawPixelValueAt(int x, int y) const
{
    if (!hasRawPixels() || x < 0 || y < 0 || x >= m_width || y >= m_height)
    {
        return 0;
    }

    return m_rawPixels[(y * m_width) + x];
}

std::size_t DicomImage::rawPixelByteCount() const
{
    return m_rawPixels.size() * sizeof(std::int16_t);
}

void DicomImage::setPixmap(const QPixmap& pixmap)
{
    m_pixmap = pixmap;
}

void DicomImage::setFilePath(const QString& filePath)
{
    m_filePath = filePath;
}

void DicomImage::setSopInstanceUid(const QString& sopInstanceUid)
{
    m_sopInstanceUid = sopInstanceUid;
}

void DicomImage::setInstanceNumber(const QString& instanceNumber)
{
    m_instanceNumber = instanceNumber;
}

void DicomImage::setDimensions(int width, int height)
{
    m_width = width;
    m_height = height;
}

void DicomImage::setRawPixels(const std::vector<int16_t>& rawPixels)
{
    m_rawPixels = rawPixels;
}

void DicomImage::clearRawPixels()
{
    m_rawPixels.clear();
    m_rawPixels.shrink_to_fit();
}

void DicomImage::setMonochrome(bool isMonochrome)
{
    m_isMonochrome = isMonochrome;
}

void DicomImage::setMonochrome1(bool isMonochrome1)
{
    m_isMonochrome1 = isMonochrome1;
}

void DicomImage::setValueRange(int minimumStoredValue, int maximumStoredValue)
{
    m_minimumStoredValue = minimumStoredValue;
    m_maximumStoredValue = maximumStoredValue;
}

void DicomImage::setDefaultWindow(int windowLevel, int windowWidth)
{
    m_defaultWindowLevel = windowLevel;
    m_defaultWindowWidth = windowWidth;
}

void DicomImage::setPixelSpacing(double pixelSpacingX, double pixelSpacingY)
{
    m_pixelSpacingX = pixelSpacingX;
    m_pixelSpacingY = pixelSpacingY;
}

void DicomImage::setImagePositionPatient(const std::array<double, 3>& imagePositionPatient)
{
    m_imagePositionPatient = imagePositionPatient;
    m_hasImagePositionPatient = true;
}

void DicomImage::setImageOrientationPatient(const std::array<double, 6>& imageOrientationPatient)
{
    m_imageOrientationPatient = imageOrientationPatient;
    m_hasImageOrientationPatient = true;
}

void DicomImage::setSliceThickness(double sliceThickness)
{
    m_sliceThickness = sliceThickness;
}

void DicomImage::setSpacingBetweenSlices(double spacingBetweenSlices)
{
    m_spacingBetweenSlices = spacingBetweenSlices;
}
