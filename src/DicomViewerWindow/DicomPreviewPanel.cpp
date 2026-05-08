#include "DicomViewerWindow/DicomPreviewPanel.h"

#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollArea>
#include <QVBoxLayout>

#include <algorithm>
#include <functional>

namespace
{
QString clippedText(const QString& value, int maxLength)
{
    const QString trimmedValue = value.trimmed();
    if (trimmedValue.size() <= maxLength)
    {
        return trimmedValue;
    }

    return trimmedValue.left(maxLength - 3) + "...";
}

class PreviewTile final : public QFrame
{
public:
    explicit PreviewTile(const DicomPreviewItem& item, QWidget* parent = nullptr)
        : QFrame(parent),
          m_item(item)
    {
    }

    std::function<void(const DicomPreviewItem&)> onDoubleClicked;

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton && onDoubleClicked)
        {
            onDoubleClicked(m_item);
            event->accept();
            return;
        }

        QFrame::mousePressEvent(event);
    }

    void mouseDoubleClickEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton && onDoubleClicked)
        {
            onDoubleClicked(m_item);
            event->accept();
            return;
        }

        QFrame::mouseDoubleClickEvent(event);
    }

private:
    DicomPreviewItem m_item;
};
}

DicomPreviewPanel::DicomPreviewPanel(QWidget* parent)
    : QWidget(parent)
{
    buildUi();
}

void DicomPreviewPanel::showItems(
    const QString& title,
    const DicomPreviewItems& items,
    const QString& emptyText)
{
    clearItems();

    if (m_titleLabel)
    {
        m_titleLabel->setText(title.trimmed().isEmpty() ? QString("Preview") : title);
    }

    if (items.isEmpty())
    {
        auto* emptyLabel = new QLabel(emptyText, m_gridWidget);
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setMinimumHeight(kThumbnailSize);
        m_gridLayout->addWidget(emptyLabel, 0, 0, 1, kColumns);
        return;
    }

    for (int index = 0; index < items.size(); ++index)
    {
        addTile(items.at(index), index / kColumns, index % kColumns);
    }
}

void DicomPreviewPanel::showSinglePreview(
    const QString& title,
    const QPixmap& pixmap,
    const QString& badgeText,
    const QString& emptyText)
{
    DicomPreviewItems items;
    if (!pixmap.isNull())
    {
        DicomPreviewItem item;
        item.title = title;
        item.badgeText = badgeText;
        item.pixmap = pixmap;
        items.append(item);
    }

    showItems(title, items, emptyText);
}

void DicomPreviewPanel::buildUi()
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(6);

    m_titleLabel = new QLabel("Preview", this);
    m_titleLabel->setStyleSheet("font-weight: 700;");
    rootLayout->addWidget(m_titleLabel);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    m_gridWidget = new QWidget(scrollArea);
    m_gridLayout = new QGridLayout(m_gridWidget);
    m_gridLayout->setContentsMargins(0, 0, 0, 0);
    m_gridLayout->setHorizontalSpacing(8);
    m_gridLayout->setVerticalSpacing(10);
    m_gridLayout->setColumnStretch(0, 1);
    m_gridLayout->setColumnStretch(1, 1);

    scrollArea->setWidget(m_gridWidget);
    rootLayout->addWidget(scrollArea, 1);
}

void DicomPreviewPanel::clearItems()
{
    if (!m_gridLayout)
    {
        return;
    }

    while (QLayoutItem* item = m_gridLayout->takeAt(0))
    {
        if (QWidget* widget = item->widget())
        {
            widget->deleteLater();
        }
        delete item;
    }
}

void DicomPreviewPanel::addTile(const DicomPreviewItem& item, int row, int column)
{
    auto* tile = new PreviewTile(item, m_gridWidget);
    tile->setFrameShape(QFrame::NoFrame);
    tile->setMinimumWidth(kThumbnailSize);
    tile->setMaximumWidth(kThumbnailSize + 18);
    tile->setCursor(Qt::PointingHandCursor);
    tile->onDoubleClicked = [this](const DicomPreviewItem& previewItem) {
        emit itemDoubleClicked(previewItem);
    };

    auto* layout = new QVBoxLayout(tile);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    auto* imageLabel = new QLabel(tile);
    imageLabel->setFixedSize(kThumbnailSize, kThumbnailSize);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    imageLabel->setPixmap(composeThumbnail(item.pixmap, item.badgeText));
    layout->addWidget(imageLabel, 0, Qt::AlignHCenter);

    if (!item.title.trimmed().isEmpty())
    {
        auto* titleLabel = new QLabel(clippedText(item.title, 42), tile);
        titleLabel->setWordWrap(true);
        titleLabel->setAlignment(Qt::AlignCenter);
        titleLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        layout->addWidget(titleLabel);
    }

    if (!item.subtitle.trimmed().isEmpty())
    {
        auto* subtitleLabel = new QLabel(clippedText(item.subtitle, 44), tile);
        subtitleLabel->setWordWrap(true);
        subtitleLabel->setAlignment(Qt::AlignCenter);
        subtitleLabel->setStyleSheet("color: palette(mid);");
        subtitleLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        layout->addWidget(subtitleLabel);
    }

    m_gridLayout->addWidget(tile, row, column, Qt::AlignTop | Qt::AlignHCenter);
}

QPixmap DicomPreviewPanel::composeThumbnail(const QPixmap& source, const QString& badgeText) const
{
    QPixmap thumbnail(kThumbnailSize, kThumbnailSize);
    thumbnail.fill(QColor(24, 24, 24));

    QPainter painter(&thumbnail);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    if (source.isNull())
    {
        painter.setPen(QColor(180, 180, 180));
        painter.drawText(thumbnail.rect(), Qt::AlignCenter, "No preview");
    }
    else
    {
        const QPixmap scaled = source.scaled(
            QSize(kThumbnailSize, kThumbnailSize),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation);
        const QPoint topLeft(
            (kThumbnailSize - scaled.width()) / 2,
            (kThumbnailSize - scaled.height()) / 2);
        painter.drawPixmap(topLeft, scaled);
    }

    const QString badge = badgeText.trimmed();
    if (!badge.isEmpty())
    {
        const QFontMetrics metrics(painter.font());
        const int badgeWidth = std::min(kThumbnailSize - 8, metrics.horizontalAdvance(badge) + 12);
        const QRect badgeRect(
            kThumbnailSize - badgeWidth - 5,
            kThumbnailSize - metrics.height() - 9,
            badgeWidth,
            metrics.height() + 5);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 190));
        painter.drawRoundedRect(badgeRect, 5, 5);
        painter.setPen(Qt::white);
        painter.drawText(badgeRect, Qt::AlignCenter, badge);
    }

    return thumbnail;
}
