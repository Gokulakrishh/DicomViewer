#include "Services/SeriesPreviewService.h"

#include "Database/DatabaseService.h"
#include "FileHandling/FileHandling.h"
#include "Model/DicomParameters.h"
#include "Utilities/DiagnosticImageRenderer.h"

#include <QDebug>

#include <exception>
#include <memory>

namespace
{
constexpr int kPreviewMaxDimension = 120;

QString representativeFilePath(const Series& series)
{
    if (!series.representativeFilePath().trimmed().isEmpty())
    {
        return series.representativeFilePath();
    }

    const auto& images = series.images();
    if (!images.empty() && images.front())
    {
        return images.front()->filePath();
    }

    return {};
}
}

SeriesPreviewService::SeriesPreviewService(DatabaseService& databaseService, FileHandling& fileHandling)
    : m_databaseService(databaseService),
      m_fileHandling(fileHandling)
{
}

DicomPreviewItems SeriesPreviewService::ensurePreviewPixmaps(const DicomPreviewItems& items)
{
    DicomPreviewItems updatedItems = items;
    for (DicomPreviewItem& item : updatedItems)
    {
        if (!item.pixmap.isNull())
        {
            continue;
        }

        switch (item.targetType)
        {
        case DicomPreviewTargetType::Study:
            item.pixmap = ensureStudyPreview(item.targetId);
            break;
        case DicomPreviewTargetType::Series:
            item.pixmap = ensureSeriesPreview(item.targetId);
            break;
        case DicomPreviewTargetType::None:
            break;
        }
    }

    return updatedItems;
}

QPixmap SeriesPreviewService::ensureStudyPreview(const QString& studyInstanceUid)
{
    if (studyInstanceUid.trimmed().isEmpty())
    {
        return {};
    }

    const DicomPreviewItems seriesItems = m_databaseService.getSeriesPreviewItemsForStudy(studyInstanceUid);
    for (const DicomPreviewItem& seriesItem : seriesItems)
    {
        if (!seriesItem.pixmap.isNull())
        {
            return seriesItem.pixmap;
        }

        const QPixmap generatedPreview = ensureSeriesPreview(seriesItem.targetId);
        if (!generatedPreview.isNull())
        {
            return generatedPreview;
        }
    }

    return {};
}

QPixmap SeriesPreviewService::ensureSeriesPreview(const QString& seriesInstanceUid)
{
    const QString normalizedSeriesUid = seriesInstanceUid.trimmed();
    if (normalizedSeriesUid.isEmpty())
    {
        return {};
    }

    const DatabaseService::SeriesPtr series = m_databaseService.getSeries(normalizedSeriesUid);
    if (!series)
    {
        qWarning() << "[SeriesPreview] series not found:" << normalizedSeriesUid;
        return {};
    }

    if (!series->previewPixmap().isNull())
    {
        return series->previewPixmap();
    }

    const QString filePath = representativeFilePath(*series);
    if (filePath.trimmed().isEmpty())
    {
        qWarning() << "[SeriesPreview] representative file path missing for series:" << normalizedSeriesUid;
        return {};
    }

    std::unique_ptr<DicomImage> image;
    try
    {
        image = m_fileHandling.loadImageData(filePath, 0);
    }
    catch (const std::exception& error)
    {
        qWarning() << "[SeriesPreview] preview decode failed for" << filePath << ":" << error.what();
        return {};
    }
    catch (...)
    {
        qWarning() << "[SeriesPreview] preview decode failed for" << filePath;
        return {};
    }

    if (!image)
    {
        qWarning() << "[SeriesPreview] no preview image decoded for" << filePath;
        return {};
    }

    const QPixmap previewPixmap = createDicomPreviewPixmap(*image, kPreviewMaxDimension);
    image->clearRawPixels();

    if (previewPixmap.isNull())
    {
        qWarning() << "[SeriesPreview] decoded image did not produce a preview for" << filePath;
        return {};
    }

    if (!m_databaseService.upsertSeriesPreview(normalizedSeriesUid, previewPixmap))
    {
        qWarning() << "[SeriesPreview] failed to persist preview for series:" << normalizedSeriesUid;
    }

    return previewPixmap;
}
