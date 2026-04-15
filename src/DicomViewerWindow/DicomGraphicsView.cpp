#include "DicomGraphicsView.h"

#include "Model/MedicalImage.h"

#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPainter>
#include <QPen>
#include <QSize>
#include <QSlider>
#include <QToolButton>
#include <QVBoxLayout>
#include <algorithm>

namespace
{
struct ToolDescriptor
{
    DicomGraphicsView::ToolMode mode;
    const char* iconPath;
    const char* toolTip;
};

const ToolDescriptor kToolDescriptors[] = {
    {DicomGraphicsView::ToolMode::Pan, ":/icons/tool_pan.svg", "Pan"},
    {DicomGraphicsView::ToolMode::Distance, ":/icons/tool_distance.svg", "Distance measurement"},
    {DicomGraphicsView::ToolMode::PixelProbe, ":/icons/tool_probe.svg", "Pixel probe"},
    {DicomGraphicsView::ToolMode::Angle, ":/icons/tool_angle.svg", "Angle measurement"}};
}

DicomGraphicsView::DicomGraphicsView(QWidget* parent)
    : QGraphicsView(parent)
{
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);

    m_pixmapItem = new QGraphicsPixmapItem();
    m_scene->addItem(m_pixmapItem);
    m_measurementLineItem = m_scene->addLine(QLineF(), QPen(QColor(255, 196, 0), 2));
    m_measurementLineItem->setVisible(false);
    m_measurementTextItem = m_scene->addSimpleText(QString());
    m_measurementTextItem->setBrush(QBrush(Qt::yellow));
    m_measurementTextItem->setVisible(false);
    m_angleFirstLineItem = m_scene->addLine(QLineF(), QPen(QColor(255, 128, 64), 2));
    m_angleFirstLineItem->setVisible(false);
    m_angleSecondLineItem = m_scene->addLine(QLineF(), QPen(QColor(255, 128, 64), 2));
    m_angleSecondLineItem->setVisible(false);
    m_angleTextItem = m_scene->addSimpleText(QString());
    m_angleTextItem->setBrush(QBrush(QColor(255, 128, 64)));
    m_angleTextItem->setVisible(false);
    m_probeHorizontalItem = m_scene->addLine(QLineF(), QPen(QColor(64, 220, 255), 1));
    m_probeHorizontalItem->setVisible(false);
    m_probeVerticalItem = m_scene->addLine(QLineF(), QPen(QColor(64, 220, 255), 1));
    m_probeVerticalItem->setVisible(false);
    m_probeTextItem = m_scene->addSimpleText(QString());
    m_probeTextItem->setBrush(QBrush(QColor(64, 220, 255)));
    m_probeTextItem->setVisible(false);

    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setCacheMode(QGraphicsView::CacheNone);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    buildOverlayControls();
}

void DicomGraphicsView::setImage(std::shared_ptr<MedicalImage> image)
{
    m_image = std::move(image);
    updatePixmap();
}

void DicomGraphicsView::clearImage()
{
    clearMeasurementOverlays();
    m_image.reset();
    m_pixmapItem->setPixmap(QPixmap());
    resetTransform();
    m_zoomFactor = 1.0;
    m_fitToViewPending = true;
    setSliceNavigationState(0, 0);
}

void DicomGraphicsView::setToolMode(ToolMode toolMode)
{
    m_toolMode = toolMode;
    m_hasDistanceAnchor = false;
    m_angleClickCount = 0;
    if (m_toolMode == ToolMode::Pan)
    {
        setDragMode(QGraphicsView::ScrollHandDrag);
    }
    else
    {
        setDragMode(QGraphicsView::NoDrag);
    }
    updateToolOverlaySelection();
}

void DicomGraphicsView::setSliceNavigationState(int currentIndex, int totalCount)
{
    m_totalSliceCount = std::max(0, totalCount);
    m_currentSliceIndex = m_totalSliceCount > 0 ? std::clamp(currentIndex, 0, m_totalSliceCount - 1) : 0;

    if (m_sliceSlider)
    {
        m_sliceSlider->blockSignals(true);
        m_sliceSlider->setEnabled(m_totalSliceCount > 1);
        m_sliceSlider->setMinimum(0);
        m_sliceSlider->setMaximum(std::max(0, m_totalSliceCount - 1));
        m_sliceSlider->setValue(m_currentSliceIndex);
        m_sliceSlider->blockSignals(false);
    }

    updateSliceNavigationLabel();
    if (m_cineOverlayWidget)
    {
        m_cineOverlayWidget->setVisible(m_totalSliceCount > 0);
    }
}

void DicomGraphicsView::setCineAvailable(bool available)
{
    if (m_cinePlayButton)
    {
        m_cinePlayButton->setEnabled(available);
        if (!available)
        {
            m_cinePlayButton->blockSignals(true);
            m_cinePlayButton->setChecked(false);
            m_cinePlayButton->setIcon(QIcon(":/icons/cine_play.svg"));
            m_cinePlayButton->blockSignals(false);
        }
    }
}

void DicomGraphicsView::setCinePlaying(bool playing)
{
    if (!m_cinePlayButton)
    {
        return;
    }

    m_cinePlayButton->blockSignals(true);
    m_cinePlayButton->setChecked(playing);
    m_cinePlayButton->setIcon(QIcon(playing ? ":/icons/cine_pause.svg" : ":/icons/cine_play.svg"));
    m_cinePlayButton->blockSignals(false);
}

void DicomGraphicsView::showDistanceMeasurement(const QPointF& startScenePos, const QPointF& endScenePos, const QString& label)
{
    m_measurementLineItem->setLine(QLineF(startScenePos, endScenePos));
    m_measurementLineItem->setVisible(true);
    m_measurementTextItem->setText(label);
    m_measurementTextItem->setPos((startScenePos + endScenePos) / 2.0);
    m_measurementTextItem->setVisible(true);
}

void DicomGraphicsView::showPixelProbe(const QPointF& scenePos, const QString& label)
{
    constexpr qreal markerRadius = 8.0;
    m_probeHorizontalItem->setLine(
        scenePos.x() - markerRadius,
        scenePos.y(),
        scenePos.x() + markerRadius,
        scenePos.y());
    m_probeVerticalItem->setLine(
        scenePos.x(),
        scenePos.y() - markerRadius,
        scenePos.x(),
        scenePos.y() + markerRadius);
    m_probeHorizontalItem->setVisible(true);
    m_probeVerticalItem->setVisible(true);
    m_probeTextItem->setText(label);
    m_probeTextItem->setPos(scenePos + QPointF(10.0, 10.0));
    m_probeTextItem->setVisible(true);
}

void DicomGraphicsView::showAngleMeasurement(
    const QPointF& startScenePos,
    const QPointF& vertexScenePos,
    const QPointF& endScenePos,
    const QString& label)
{
    m_angleFirstLineItem->setLine(QLineF(vertexScenePos, startScenePos));
    m_angleSecondLineItem->setLine(QLineF(vertexScenePos, endScenePos));
    m_angleFirstLineItem->setVisible(true);
    m_angleSecondLineItem->setVisible(true);
    m_angleTextItem->setText(label);
    m_angleTextItem->setPos(vertexScenePos + QPointF(10.0, -10.0));
    m_angleTextItem->setVisible(true);
}

void DicomGraphicsView::clearMeasurementOverlays()
{
    m_hasDistanceAnchor = false;
    m_angleClickCount = 0;
    m_measurementLineItem->setVisible(false);
    m_measurementTextItem->setVisible(false);
    m_angleFirstLineItem->setVisible(false);
    m_angleSecondLineItem->setVisible(false);
    m_angleTextItem->setVisible(false);
    m_probeHorizontalItem->setVisible(false);
    m_probeVerticalItem->setVisible(false);
    m_probeTextItem->setVisible(false);
}

void DicomGraphicsView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton || m_toolMode == ToolMode::Pan)
    {
        QGraphicsView::mousePressEvent(event);
        return;
    }

    QPoint pixelPos;
    QPointF scenePos;
    if (!mapMouseToImage(event->pos(), pixelPos, scenePos))
    {
        QGraphicsView::mousePressEvent(event);
        return;
    }

    if (m_toolMode == ToolMode::PixelProbe)
    {
        emit pixelProbeRequested(pixelPos);
        event->accept();
        return;
    }

    if (m_toolMode == ToolMode::Distance)
    {
        if (!m_hasDistanceAnchor)
        {
            m_hasDistanceAnchor = true;
            m_distanceAnchorPixel = pixelPos;
            m_distanceAnchorScene = scenePos;
            m_measurementLineItem->setLine(QLineF(scenePos, scenePos));
            m_measurementLineItem->setVisible(true);
            m_measurementTextItem->setVisible(false);
        }
        else
        {
            emit distanceMeasurementRequested(m_distanceAnchorPixel, pixelPos);
            m_hasDistanceAnchor = false;
        }
        event->accept();
        return;
    }

    if (m_toolMode == ToolMode::Angle)
    {
        if (m_angleClickCount == 0)
        {
            m_angleStartPixel = pixelPos;
            m_angleStartScene = scenePos;
            m_angleClickCount = 1;
            m_angleFirstLineItem->setLine(QLineF(scenePos, scenePos));
            m_angleFirstLineItem->setVisible(true);
            m_angleSecondLineItem->setVisible(false);
            m_angleTextItem->setVisible(false);
        }
        else if (m_angleClickCount == 1)
        {
            m_angleVertexPixel = pixelPos;
            m_angleVertexScene = scenePos;
            m_angleClickCount = 2;
            m_angleFirstLineItem->setLine(QLineF(m_angleVertexScene, m_angleStartScene));
            m_angleSecondLineItem->setLine(QLineF(m_angleVertexScene, m_angleVertexScene));
            m_angleSecondLineItem->setVisible(true);
        }
        else
        {
            emit angleMeasurementRequested(m_angleStartPixel, m_angleVertexPixel, pixelPos);
            m_angleClickCount = 0;
        }
        event->accept();
        return;
    }

    QGraphicsView::mousePressEvent(event);
}

void DicomGraphicsView::wheelEvent(QWheelEvent* event)
{
    if (event->angleDelta().y() == 0)
    {
        QGraphicsView::wheelEvent(event);
        return;
    }

    const int stepCount = event->angleDelta().y() > 0 ? -1 : 1;
    emit wheelSliceNavigationRequested(stepCount);
    event->accept();
}

void DicomGraphicsView::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    updateOverlayGeometry();

    if (!m_image || !m_image->isValid() || m_scene->sceneRect().isEmpty())
    {
        return;
    }

    applyFitToView();
}

bool DicomGraphicsView::mapMouseToImage(const QPoint& viewPos, QPoint& pixelPos, QPointF& scenePos) const
{
    if (!m_pixmapItem || m_pixmapItem->pixmap().isNull())
    {
        return false;
    }

    scenePos = mapToScene(viewPos);
    const QPointF itemPos = m_pixmapItem->mapFromScene(scenePos);
    const QRectF imageRect = m_pixmapItem->boundingRect();
    if (!imageRect.contains(itemPos))
    {
        return false;
    }

    pixelPos.setX(std::clamp(static_cast<int>(itemPos.x()), 0, m_pixmapItem->pixmap().width() - 1));
    pixelPos.setY(std::clamp(static_cast<int>(itemPos.y()), 0, m_pixmapItem->pixmap().height() - 1));
    scenePos = m_pixmapItem->mapToScene(QPointF(pixelPos));
    return true;
}

void DicomGraphicsView::updatePixmap()
{
    if (!m_image || !m_image->isValid())
    {
        clearImage();
        return;
    }

    const QPixmap pixmap = m_image->pixmap();
    const QRectF previousSceneRect = m_scene->sceneRect();
    m_pixmapItem->setPixmap(QPixmap());
    m_pixmapItem->setPixmap(pixmap);
    m_scene->setSceneRect(pixmap.rect());
    clearMeasurementOverlays();
    if (m_fitToViewPending || previousSceneRect.size() != m_scene->sceneRect().size())
    {
        applyFitToView();
    }
    m_scene->update();
    viewport()->update();
    updateOverlayGeometry();
}

void DicomGraphicsView::applyFitToView()
{
    if (m_scene->sceneRect().isEmpty())
    {
        return;
    }

    fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    m_zoomFactor = 1.0;
    m_fitToViewPending = false;
}

void DicomGraphicsView::buildOverlayControls()
{
    m_toolOverlayWidget = new QWidget(viewport());
    m_toolOverlayWidget->setObjectName("viewerToolOverlay");
    m_toolOverlayWidget->setAttribute(Qt::WA_StyledBackground, true);

    auto* toolRootLayout = new QVBoxLayout(m_toolOverlayWidget);
    toolRootLayout->setContentsMargins(10, 8, 10, 8);
    toolRootLayout->setSpacing(6);

    auto* toolTitleLabel = new QLabel("Tools", m_toolOverlayWidget);
    toolTitleLabel->setObjectName("viewerOverlayTitleLabel");
    toolRootLayout->addWidget(toolTitleLabel);

    auto* toolLayout = new QHBoxLayout();
    toolLayout->setContentsMargins(0, 0, 0, 0);
    toolLayout->setSpacing(6);

    for (const auto& descriptor : kToolDescriptors)
    {
        auto* button = new QToolButton(m_toolOverlayWidget);
        button->setCheckable(true);
        button->setIcon(QIcon(QString::fromUtf8(descriptor.iconPath)));
        button->setIconSize(QSize(18, 18));
        button->setToolTip(QString::fromUtf8(descriptor.toolTip));
        button->setStatusTip(QString::fromUtf8(descriptor.toolTip));
        button->setProperty("toolMode", static_cast<int>(descriptor.mode));
        button->setObjectName("viewerToolButton");
        button->setAutoRaise(false);
        button->setMinimumSize(34, 34);
        connect(button, &QToolButton::clicked, this, [this, descriptor]() {
            setToolMode(descriptor.mode);
            emit toolModeSelected(descriptor.mode);
        });
        toolLayout->addWidget(button);
        m_toolButtons.append(button);
    }
    toolRootLayout->addLayout(toolLayout);

    m_cineOverlayWidget = new QWidget(viewport());
    m_cineOverlayWidget->setObjectName("viewerCineOverlay");
    m_cineOverlayWidget->setAttribute(Qt::WA_StyledBackground, true);

    auto* cineRootLayout = new QVBoxLayout(m_cineOverlayWidget);
    cineRootLayout->setContentsMargins(10, 8, 10, 8);
    cineRootLayout->setSpacing(6);

    auto* cineTitleLabel = new QLabel("Slices", m_cineOverlayWidget);
    cineTitleLabel->setObjectName("viewerOverlayTitleLabel");
    cineRootLayout->addWidget(cineTitleLabel);

    auto* cineLayout = new QHBoxLayout();
    cineLayout->setContentsMargins(0, 0, 0, 0);
    cineLayout->setSpacing(8);

    m_cinePlayButton = new QToolButton(m_cineOverlayWidget);
    m_cinePlayButton->setCheckable(true);
    m_cinePlayButton->setIcon(QIcon(":/icons/cine_play.svg"));
    m_cinePlayButton->setIconSize(QSize(18, 18));
    m_cinePlayButton->setObjectName("viewerCinePlayButton");
    m_cinePlayButton->setMinimumSize(34, 30);
    m_cinePlayButton->setToolTip("Play / Pause");
    m_cinePlayButton->setStatusTip("Play / Pause");
    connect(m_cinePlayButton, &QToolButton::toggled, this, [this](bool checked) {
        if (m_cinePlayButton)
        {
            m_cinePlayButton->setIcon(QIcon(checked ? ":/icons/cine_pause.svg" : ":/icons/cine_play.svg"));
        }
        emit cinePlaybackToggled(checked);
    });

    m_sliceSlider = new QSlider(Qt::Horizontal, m_cineOverlayWidget);
    m_sliceSlider->setMinimum(0);
    m_sliceSlider->setMaximum(0);
    m_sliceSlider->setToolTip("Slice navigation");
    m_sliceSlider->setStatusTip("Slice navigation");
    connect(m_sliceSlider, &QSlider::valueChanged, this, &DicomGraphicsView::sliceIndexSelected);

    m_sliceLabel = new QLabel("0 / 0", m_cineOverlayWidget);
    m_sliceLabel->setObjectName("viewerSliceLabel");

    cineLayout->addWidget(m_cinePlayButton);
    cineLayout->addWidget(m_sliceSlider, 1);
    cineLayout->addWidget(m_sliceLabel);
    cineRootLayout->addLayout(cineLayout);

    updateToolOverlaySelection();
    updateSliceNavigationLabel();
    updateOverlayGeometry();
}

void DicomGraphicsView::updateOverlayGeometry()
{
    const QRect viewRect = viewport()->rect();
    if (m_toolOverlayWidget)
    {
        const QSize toolSize = m_toolOverlayWidget->sizeHint();
        m_toolOverlayWidget->resize(toolSize);
        m_toolOverlayWidget->move(16, 16);
        m_toolOverlayWidget->raise();
    }

    if (m_cineOverlayWidget)
    {
        const int cineWidth = std::max(320, viewRect.width() - 32);
        const QSize cineHint = m_cineOverlayWidget->sizeHint();
        m_cineOverlayWidget->resize(cineWidth, cineHint.height());
        m_cineOverlayWidget->move(16, viewRect.height() - cineHint.height() - 16);
        m_cineOverlayWidget->raise();
    }
}

void DicomGraphicsView::updateToolOverlaySelection()
{
    for (auto* button : m_toolButtons)
    {
        if (!button)
        {
            continue;
        }

        const auto toolMode = static_cast<ToolMode>(button->property("toolMode").toInt());
        button->blockSignals(true);
        button->setChecked(toolMode == m_toolMode);
        button->blockSignals(false);
    }
}

void DicomGraphicsView::updateSliceNavigationLabel()
{
    if (!m_sliceLabel)
    {
        return;
    }

    const int currentDisplayIndex = m_totalSliceCount > 0 ? m_currentSliceIndex + 1 : 0;
    m_sliceLabel->setText(QString("%1 / %2").arg(currentDisplayIndex).arg(m_totalSliceCount));
}
