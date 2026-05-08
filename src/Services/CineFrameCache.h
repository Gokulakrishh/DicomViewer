#pragma once

#include <QHash>
#include <QSet>
#include <QtGlobal>

class DicomImage;
class Series;

/**
 * @brief Tracks and evicts decoded cine frame pixels for the active viewer series.
 *
 * Responsibilities:
 * - Account for raw pixel memory held by decoded cine frames.
 * - Evict frames outside a bounded playback/navigation window.
 * - Provide simple debug profiling for XA/cine memory behavior.
 *
 * Assumptions:
 * - The service does not own pixel buffers; the active Series owns DicomImage
 *   instances and this service clears raw pixels through those objects.
 * - One service instance is scoped to one viewport session.
 */
class CineFrameCache
{
public:
    /**
     * @brief Clears memory accounting for a new series/session.
     */
    void reset();

    /**
     * @brief Records one decoded frame already stored in the series image.
     * @param seriesIndex Index of the DicomImage in the active Series.
     * @param image Image containing decoded raw pixels.
     */
    void recordAllocatedFrame(int seriesIndex, const DicomImage& image);

    /**
     * @brief Evicts decoded frames outside a bounded window.
     * @param series Active series whose DicomImage objects own raw pixels.
     * @param currentIndex Current viewer index.
     * @param radius Number of neighboring frames to keep.
     * @param wrapAround True for cine playback over a looping sequence.
     * @param protectedIndices Indices currently being loaded asynchronously.
     */
    void evictOutsideWindow(
        Series& series,
        int currentIndex,
        int radius,
        bool wrapAround,
        const QSet<int>& protectedIndices = {});

    /**
     * @brief Returns tracked raw pixel bytes.
     * @return Total bytes currently tracked.
     */
    [[nodiscard]] qint64 totalBytes() const;

    /**
     * @brief Returns tracked frame count.
     * @return Number of decoded frames tracked by this cache.
     */
    [[nodiscard]] int cachedFrameCount() const;

private:
    [[nodiscard]] int distanceFromCurrent(int index, int currentIndex, int count, bool wrapAround) const;
    void forgetFrame(int seriesIndex);
    void logAllocatedFrame(int seriesIndex) const;
    void logEvictedFrame(int seriesIndex) const;

private:
    QHash<int, qint64> m_bytesBySeriesIndex;
    qint64 m_totalBytes{0};
};
