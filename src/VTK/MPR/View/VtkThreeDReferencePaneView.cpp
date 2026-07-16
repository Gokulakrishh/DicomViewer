#include "VTK/MPR/View/VtkThreeDReferencePaneView.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <QVTKOpenGLNativeWidget.h>

VtkThreeDReferencePaneView::VtkThreeDReferencePaneView(const QString& title, QWidget* parent)
{
    m_rootWidget = new QWidget(parent);
    auto* layout = new QVBoxLayout(m_rootWidget);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(4);

    m_titleLabel = new QLabel(title, m_rootWidget);
    m_statusLabel = new QLabel("Spatial navigator - not diagnostic 3D", m_rootWidget);
    m_statusLabel->setObjectName("mprReferenceStatusLabel");
    m_renderWidget = new QVTKOpenGLNativeWidget(m_rootWidget);
    m_renderWidget->setMinimumSize(240, 240);

    layout->addWidget(m_titleLabel);
    layout->addWidget(m_statusLabel);
    layout->addWidget(m_renderWidget, 1);
}

VtkThreeDReferencePaneView::~VtkThreeDReferencePaneView() = default;

QWidget* VtkThreeDReferencePaneView::widget() const
{
    return m_rootWidget;
}

QVTKOpenGLNativeWidget* VtkThreeDReferencePaneView::renderWidget() const
{
    return m_renderWidget;
}
