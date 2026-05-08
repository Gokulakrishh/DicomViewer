#include "Services/AnnotationReportService.h"

#include "Database/DatabaseService.h"

#include <algorithm>

AnnotationReportService::AnnotationReportService(DatabaseService& databaseService)
    : m_databaseService(databaseService)
{
}

AnnotationReportSummaryBySeries AnnotationReportService::loadSeriesSummaries(
    const QList<QString>& seriesInstanceUids) const
{
    return m_databaseService.loadSeriesAnnotationReportSummaries(seriesInstanceUids);
}

AnnotationReportRows AnnotationReportService::loadRows(const AnnotationReportFilter& filter) const
{
    return m_databaseService.loadAnnotationReportRows(filter);
}

AnnotationReportRows AnnotationReportService::loadCurrentSliceRows(
    const QString& seriesInstanceUid,
    const QString& sopInstanceUid,
    int frameIndex) const
{
    AnnotationReportFilter filter;
    filter.seriesInstanceUid = seriesInstanceUid;
    filter.sopInstanceUid = sopInstanceUid;
    filter.frameIndex = std::max(0, frameIndex);
    filter.limit = 100;
    return loadRows(filter);
}

AnnotationSliceGroups AnnotationReportService::loadSliceGroups(const AnnotationReportFilter& filter) const
{
    AnnotationSliceGroups groups;
    const AnnotationReportRows rows = loadRows(filter);
    QHash<QString, int> groupIndexBySlice;

    for (const AnnotationReportRow& row : rows)
    {
        const QString key = row.seriesInstanceUid + "|" + row.sopInstanceUid + "|" + QString::number(row.frameIndex);
        const auto existingIndex = groupIndexBySlice.constFind(key);
        if (existingIndex == groupIndexBySlice.constEnd())
        {
            AnnotationSliceGroup group;
            group.seriesInstanceUid = row.seriesInstanceUid;
            group.sopInstanceUid = row.sopInstanceUid;
            group.frameIndex = row.frameIndex;
            group.instanceNumber = row.instanceNumber;
            group.patientId = row.patientId;
            group.patientName = row.patientName;
            group.studyDate = row.studyDate;
            group.seriesDescription = row.seriesDescription;
            group.modality = row.modality;
            group.rows.append(row);
            groups.append(group);
            groupIndexBySlice.insert(key, groups.size() - 1);
            continue;
        }

        groups[*existingIndex].rows.append(row);
    }

    return groups;
}

bool AnnotationReportService::updateMetadata(
    const QString& annotationId,
    const QString& label,
    const QString& bodyRegion,
    const QString& note)
{
    return m_databaseService.updateAnnotationReportMetadata(annotationId, label, bodyRegion, note);
}

bool AnnotationReportService::deleteAnnotation(const QString& annotationId)
{
    return m_databaseService.markSliceMeasurementAnnotationDeleted(annotationId);
}
