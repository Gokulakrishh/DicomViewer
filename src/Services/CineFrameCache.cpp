#include "Services/CineFrameCache.h"

#include "Model/DicomImage.h"
#include "Model/DicomParameters.h"

#include <QDebug>
#include <algorithm>
#include <cstdlib>

void CineFrameCache::reset()
{
    m_bytesBySeriesIndex.clear();
    m_totalBytes = 0;
}

void CineFrameCache::recordAllocatedFrame(int seriesIndex, const DicomImage& image)
{
    if (seriesIndex < 0 || !image.hasRawPixels())
    {
        return;
    }

    const qint64 bytes = static_cast<qint64>(image.rawPixelByteCount());
    const auto existingFrame = m_bytesBySeriesIndex.constFind(seriesIndex);
    const qint64 previousBytes = existingFrame == m_bytesBySeriesIndex.constEnd() ? 0 : existingFrame.value();
    if (existingFrame != m_bytesBySeriesIndex.constEnd() && previousBytes == bytes)
    {
        return;
    }

    m_bytesBySeriesIndex.insert(seriesIndex, bytes);
    m_totalBytes += bytes - previousBytes;
    //logAllocatedFrame(seriesIndex);
}

void CineFrameCache::evictOutsideWindow(
    Series& series,
    int currentIndex,
    int radius,
    bool wrapAround,
    const QSet<int>& protectedIndices)
{
    auto& images = series.images();
    if (images.empty() || currentIndex < 0)
    {
        reset();
        return;
    }

    for (int index = 0; index < static_cast<int>(images.size()); ++index)
    {
        if (protectedIndices.contains(index))
        {
            continue;
        }

        auto& image = images[static_cast<std::size_t>(index)];
        if (!image || !image->hasRawPixels())
        {
            forgetFrame(index);
            continue;
        }

        if (distanceFromCurrent(index, currentIndex, static_cast<int>(images.size()), wrapAround) <= radius)
        {
            recordAllocatedFrame(index, *image);
            continue;
        }

        image->clearRawPixels();
        forgetFrame(index);
        //logEvictedFrame(index);
    }
}

qint64 CineFrameCache::totalBytes() const
{
    return m_totalBytes;
}

int CineFrameCache::cachedFrameCount() const
{
    return m_bytesBySeriesIndex.size();
}

int CineFrameCache::distanceFromCurrent(int index, int currentIndex, int count, bool wrapAround) const
{
    const int directDistance = std::abs(index - currentIndex);
    if (!wrapAround || count <= 1)
    {
        return directDistance;
    }

    return std::min(directDistance, count - directDistance);
}

void CineFrameCache::forgetFrame(int seriesIndex)
{
    const auto it = m_bytesBySeriesIndex.constFind(seriesIndex);
    if (it == m_bytesBySeriesIndex.constEnd())
    {
        return;
    }

    m_totalBytes -= it.value();
    m_bytesBySeriesIndex.erase(it);
}

void CineFrameCache::logAllocatedFrame(int seriesIndex) const
{
    qDebug() << "[CineFrameCache] allocated frame" << seriesIndex
             << "| total bytes:" << m_totalBytes
             << "| cached frames:" << m_bytesBySeriesIndex.size();
}

void CineFrameCache::logEvictedFrame(int seriesIndex) const
{
    qDebug() << "[CineFrameCache] evicted frame" << seriesIndex
             << "| total bytes:" << m_totalBytes
             << "| cached frames:" << m_bytesBySeriesIndex.size();
}
