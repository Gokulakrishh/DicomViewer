#include "VTK/MPR/View/VtkSliceMprPaneView.h"

#include <QLabel>
#include <QSlider>
#include <QVBoxLayout>
#include <QWidget>
#include <QVTKOpenGLNativeWidget.h>

VtkSliceMprPaneView::VtkSliceMprPaneView(const QString& title, MprSlicePlane plane, QWidget* parent)
    : m_plane(plane)
{
    m_rootWidget = new QWidget(parent);
    auto* layout = new QVBoxLayout(m_rootWidget);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(4);

    m_titleLabel = new QLabel(title, m_rootWidget);
    m_renderWidget = new QVTKOpenGLNativeWidget(m_rootWidget);
    m_renderWidget->setMinimumSize(240, 240);

    const QString overlayStyle =
        "color: rgba(245, 247, 250, 235);"
        "background-color: rgba(12, 16, 24, 128);"
        "padding: 2px 6px;"
        "border-radius: 4px;";
    m_contextLabel = new QLabel("Patient/Study: -", m_renderWidget);
    m_contextLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_contextLabel->setStyleSheet(overlayStyle);
    m_sliceInfoLabel = new QLabel("Slice: -", m_renderWidget);
    m_sliceInfoLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_sliceInfoLabel->setStyleSheet(overlayStyle);
    m_windowLevelLabel = new QLabel("WL/WW: - / -", m_renderWidget);
    m_windowLevelLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_windowLevelLabel->setStyleSheet(overlayStyle);
    m_zoomLabel = new QLabel("Zoom: -", m_renderWidget);
    m_zoomLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_zoomLabel->setStyleSheet(overlayStyle);

    m_crosshairMarker = new QWidget(m_renderWidget);
    m_crosshairMarker->setFixedSize(12, 12);
    m_crosshairMarker->setStyleSheet(
        "background-color: rgba(72, 164, 255, 200);"
        "border: 2px solid rgba(72, 164, 255, 255);"
        "border-radius: 6px;");
    m_crosshairMarker->hide();
    m_crosshairMarker->raise();

    m_sliceSlider = new QSlider(Qt::Horizontal, m_rootWidget);
    m_sliceSlider->setRange(0, 0);

    layout->addWidget(m_titleLabel);
    layout->addWidget(m_renderWidget, 1);
    layout->addWidget(m_sliceSlider);
    layoutStatusLabels();
}

VtkSliceMprPaneView::~VtkSliceMprPaneView() = default;

MprSlicePlane VtkSliceMprPaneView::plane() const
{
    return m_plane;
}

QWidget* VtkSliceMprPaneView::widget() const
{
    return m_rootWidget;
}

QVTKOpenGLNativeWidget* VtkSliceMprPaneView::renderWidget() const
{
    return m_renderWidget;
}

QSlider* VtkSliceMprPaneView::sliceSlider() const
{
    return m_sliceSlider;
}

void VtkSliceMprPaneView::setContextText(const QString& text)
{
    m_contextLabel->setText(text);
    layoutStatusLabels();
}

void VtkSliceMprPaneView::setSliceText(const QString& text)
{
    m_sliceInfoLabel->setText(text);
    layoutStatusLabels();
}

void VtkSliceMprPaneView::setWindowLevelText(const QString& text)
{
    m_windowLevelLabel->setText(text);
    layoutStatusLabels();
}

void VtkSliceMprPaneView::setZoomText(const QString& text)
{
    m_zoomLabel->setText(text);
    layoutStatusLabels();
}

void VtkSliceMprPaneView::setCrosshairVisible(bool visible)
{
    m_crosshairMarker->setVisible(visible);
    if (visible)
    {
        m_crosshairMarker->raise();
    }
}

void VtkSliceMprPaneView::setCrosshairPosition(const QPointF& normalizedPosition)
{
    m_crosshairNormalizedPosition = normalizedPosition;
    const int x = static_cast<int>(m_crosshairNormalizedPosition.x() * m_renderWidget->width()) - (m_crosshairMarker->width() / 2);
    const int y = static_cast<int>(m_crosshairNormalizedPosition.y() * m_renderWidget->height()) - (m_crosshairMarker->height() / 2);
    m_crosshairMarker->move(x, y);
    m_crosshairMarker->raise();
}

void VtkSliceMprPaneView::layoutStatusLabels()
{
    constexpr int margin = 8;

    m_contextLabel->adjustSize();
    m_sliceInfoLabel->adjustSize();
    m_windowLevelLabel->adjustSize();
    m_zoomLabel->adjustSize();

    m_contextLabel->move(margin, margin);
    const int zoomY = std::max(margin, m_renderWidget->height() - m_zoomLabel->height() - margin);
    const int wlY = std::max(margin, zoomY - m_windowLevelLabel->height() - 4);
    const int sliceY = std::max(margin, wlY - m_sliceInfoLabel->height() - 4);
    m_sliceInfoLabel->move(margin, sliceY);
    m_windowLevelLabel->move(margin, wlY);
    m_zoomLabel->move(margin, zoomY);

    m_contextLabel->raise();
    m_sliceInfoLabel->raise();
    m_windowLevelLabel->raise();
    m_zoomLabel->raise();
    setCrosshairPosition(m_crosshairNormalizedPosition);
}
