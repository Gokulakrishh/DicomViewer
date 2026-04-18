#include "Services/ThreeDProfiles/ThreeDProfileSelection.h"

#include "Model/DicomParameters.h"
#include "Services/ThreeDProfiles/Bone3dPipelineProfile.h"
#include "Services/ThreeDProfiles/Lung3dPipelineProfile.h"

#include <QString>

namespace
{
QString buildSeriesSearchText(const Series& series)
{
    return QStringLiteral("%1 %2")
        .arg(series.seriesDescription(), series.modality())
        .trimmed()
        .toLower();
}

bool containsAnyKeyword(const QString& text, std::initializer_list<const char*> keywords)
{
    for (const char* keyword : keywords)
    {
        if (text.contains(QString::fromLatin1(keyword)))
        {
            return true;
        }
    }

    return false;
}
}

ThreeDProfileSelection ThreeDProfileSelector::selectForSeries(const Series& series)
{
    const QString searchText = buildSeriesSearchText(series);
    ThreeDAnatomyKind anatomyKind = ThreeDAnatomyKind::Bone;

    if (containsAnyKeyword(searchText, {"lung", "thorax", "chest", "pulmo", "pulmonary"}))
    {
        anatomyKind = ThreeDAnatomyKind::Lung;
    }
    else if (containsAnyKeyword(searchText, {"brain", "head", "cranial", "neuro"}))
    {
        anatomyKind = ThreeDAnatomyKind::Brain;
    }
    else if (containsAnyKeyword(searchText, {"liver", "hepatic"}))
    {
        anatomyKind = ThreeDAnatomyKind::Liver;
    }
    else if (containsAnyKeyword(searchText, {"kidney", "renal"}))
    {
        anatomyKind = ThreeDAnatomyKind::Kidney;
    }
    else if (containsAnyKeyword(searchText, {"spleen", "splenic"}))
    {
        anatomyKind = ThreeDAnatomyKind::Spleen;
    }
    else if (containsAnyKeyword(searchText, {"vessel", "vascular", "artery", "vein", "angi"}))
    {
        anatomyKind = ThreeDAnatomyKind::Vessels;
    }
    else if (containsAnyKeyword(searchText, {"skin", "surface", "body"}))
    {
        anatomyKind = ThreeDAnatomyKind::SkinSurface;
    }
    else if (containsAnyKeyword(searchText, {"bone", "osseous", "skull", "spine"}))
    {
        anatomyKind = ThreeDAnatomyKind::Bone;
    }

    ThreeDProfileSelection selection;
    selection.visualStyle = visualStyleForKind(anatomyKind);

    switch (anatomyKind)
    {
    case ThreeDAnatomyKind::Lung:
        selection.pipelineProfile = std::make_shared<Lung3dPipelineProfile>();
        break;
    default:
        selection.pipelineProfile = std::make_shared<Bone3dPipelineProfile>();
        if (selection.visualStyle.anatomyKind != ThreeDAnatomyKind::Lung &&
            selection.visualStyle.anatomyKind != ThreeDAnatomyKind::Bone)
        {
            selection.visualStyle.anatomyLabel += QStringLiteral(" (bone profile)");
        }
        break;
    }

    return selection;
}

ThreeDProfileVisualStyle ThreeDProfileSelector::visualStyleForKind(ThreeDAnatomyKind anatomyKind)
{
    switch (anatomyKind)
    {
    case ThreeDAnatomyKind::Bone:
        return {anatomyKind, QStringLiteral("Bone"), QColor(QStringLiteral("#F0E6C8"))};
    case ThreeDAnatomyKind::Lung:
        return {anatomyKind, QStringLiteral("Lung"), QColor(QStringLiteral("#7EC8E3"))};
    case ThreeDAnatomyKind::Vessels:
        return {anatomyKind, QStringLiteral("Vessels"), QColor(QStringLiteral("#C44747"))};
    case ThreeDAnatomyKind::Liver:
        return {anatomyKind, QStringLiteral("Liver"), QColor(QStringLiteral("#8C5A3C"))};
    case ThreeDAnatomyKind::Kidney:
        return {anatomyKind, QStringLiteral("Kidney"), QColor(QStringLiteral("#6E2C3A"))};
    case ThreeDAnatomyKind::Spleen:
        return {anatomyKind, QStringLiteral("Spleen"), QColor(QStringLiteral("#7A5AA6"))};
    case ThreeDAnatomyKind::SkinSurface:
        return {anatomyKind, QStringLiteral("Skin / Surface"), QColor(QStringLiteral("#DDB89A"))};
    case ThreeDAnatomyKind::Brain:
        return {anatomyKind, QStringLiteral("Brain"), QColor(QStringLiteral("#D8BCC3"))};
    case ThreeDAnatomyKind::Generic:
    default:
        return {ThreeDAnatomyKind::Generic, QStringLiteral("Generic"), QColor(QStringLiteral("#C8CED6"))};
    }
}
