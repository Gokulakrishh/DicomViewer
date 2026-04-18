#pragma once

#include <QColor>
#include <QString>
#include <memory>

class I3dPipelineProfile;
class Series;

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

struct ThreeDProfileVisualStyle
{
    ThreeDAnatomyKind anatomyKind{ThreeDAnatomyKind::Generic};
    QString anatomyLabel;
    QColor surfaceColor;
};

struct ThreeDProfileSelection
{
    std::shared_ptr<I3dPipelineProfile> pipelineProfile;
    ThreeDProfileVisualStyle visualStyle;
};

class ThreeDProfileSelector
{
public:
    [[nodiscard]] static ThreeDProfileSelection selectForSeries(const Series& series);
    [[nodiscard]] static ThreeDProfileVisualStyle visualStyleForKind(ThreeDAnatomyKind anatomyKind);
};
