#pragma once

#include <QColor>
#include <QString>
#include <memory>

class I3dPipelineProfile;
class Series;

/**
 * @brief Anatomy category used to select 3D profile defaults and visual style.
 */
enum class ThreeDAnatomyKind
{
    Bone,
    Lung,
    Vessels,
    Liver,
    Kidney,
    Spleen,
    SkinSurface,
    Brain,
    Generic
};

/**
 * @brief Visual style associated with a 3D anatomy/profile selection.
 */
struct ThreeDProfileVisualStyle
{
    ThreeDAnatomyKind anatomyKind{ThreeDAnatomyKind::Generic};
    QString anatomyLabel;
    QColor surfaceColor;
};

/**
 * @brief Selected 3D pipeline profile and its visual presentation style.
 */
struct ThreeDProfileSelection
{
    std::shared_ptr<I3dPipelineProfile> pipelineProfile;
    ThreeDProfileVisualStyle visualStyle;
};

/**
 * @brief Selects default 3D reconstruction profiles from series metadata.
 *
 * Responsibilities:
 * - Infer anatomy/profile defaults from modality and series descriptors.
 * - Provide stable visual styles for supported anatomy kinds.
 */
class ThreeDProfileSelector
{
public:
    /**
     * @brief Selects a profile for a DICOM series.
     * @param series Series metadata.
     * @return Pipeline profile and visual style selection.
     */
    [[nodiscard]] static ThreeDProfileSelection selectForSeries(const Series& series);

    /**
     * @brief Returns visual style for an anatomy kind.
     * @param anatomyKind Anatomy category.
     * @return Visual style settings.
     */
    [[nodiscard]] static ThreeDProfileVisualStyle visualStyleForKind(ThreeDAnatomyKind anatomyKind);
};
