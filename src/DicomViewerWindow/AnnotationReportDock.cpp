#include "DicomViewerWindow/AnnotationReportDock.h"

#include "DicomViewerWindow/AnnotationSliceBrowserPanel.h"
#include "DicomViewerWindow/CurrentSliceAnnotationPanel.h"

#include <QFrame>
#include <QVBoxLayout>

AnnotationReportDock::AnnotationReportDock(QWidget* parent)
    : QWidget(parent)
{
    buildUi();
}

AnnotationReportFilter AnnotationReportDock::currentFilter() const
{
    return m_sliceBrowserPanel ? m_sliceBrowserPanel->currentFilter() : AnnotationReportFilter{};
}

void AnnotationReportDock::setCurrentSliceContext(const AnnotationCurrentSliceContext& context)
{
    if (m_currentSlicePanel)
    {
        m_currentSlicePanel->setSliceContext(context);
    }
}

void AnnotationReportDock::setCurrentSliceRows(const AnnotationReportRows& rows)
{
    if (m_currentSlicePanel)
    {
        m_currentSlicePanel->setRows(rows);
    }
}

void AnnotationReportDock::setSliceGroups(const AnnotationSliceGroups& groups)
{
    if (m_sliceBrowserPanel)
    {
        m_sliceBrowserPanel->setGroups(groups);
    }
}

void AnnotationReportDock::buildUi()
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(8, 8, 8, 8);
    rootLayout->setSpacing(10);

    m_currentSlicePanel = new CurrentSliceAnnotationPanel(this);
    rootLayout->addWidget(m_currentSlicePanel);

    auto* separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    rootLayout->addWidget(separator);

    m_sliceBrowserPanel = new AnnotationSliceBrowserPanel(this);
    rootLayout->addWidget(m_sliceBrowserPanel, 1);

    connect(m_currentSlicePanel, &CurrentSliceAnnotationPanel::metadataChanged, this, &AnnotationReportDock::metadataChanged);
    connect(m_currentSlicePanel, &CurrentSliceAnnotationPanel::deleteRequested, this, &AnnotationReportDock::deleteRequested);
    connect(m_sliceBrowserPanel, &AnnotationSliceBrowserPanel::filterChanged, this, &AnnotationReportDock::filterChanged);
    connect(m_sliceBrowserPanel, &AnnotationSliceBrowserPanel::goToSliceRequested, this, &AnnotationReportDock::goToSliceRequested);
}
