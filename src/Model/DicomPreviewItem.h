#pragma once

#include <QList>
#include <QPixmap>
#include <QString>

/**
 * @brief Navigation target represented by a DICOM preview tile.
 */
enum class DicomPreviewTargetType
{
    None,
    Study,
    Series
};

/**
 * @brief Lightweight thumbnail/navigation item for the study browser.
 *
 * Responsibilities:
 * - Present a bounded preview image and count badge.
 * - Carry stable target identifiers for synchronized tree/view navigation.
 */
struct DicomPreviewItem
{
    QString title;
    QString subtitle;
    QString badgeText;
    DicomPreviewTargetType targetType{DicomPreviewTargetType::None};
    QString targetId;
    QString parentId;
    QPixmap pixmap;
};

using DicomPreviewItems = QList<DicomPreviewItem>;
