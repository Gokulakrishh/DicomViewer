#pragma once

#include "ViewerTools/Measurements/MeasurementTypes.h"

#include <algorithm>
#include <QFont>
#include <QPainter>
#include <QWidget>

/**
 * @brief Qt overlay for display-space measurement annotations.
 *
 * Responsibilities:
 * - Render measurement geometry, ROI fills, points, and labels.
 * - Keep overlay drawing independent of measurement persistence.
 */
class MeasurementOverlayWidget final : public QWidget
{
public:
    /** @brief Creates the measurement overlay. */
    explicit MeasurementOverlayWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_TranslucentBackground);
    }

    /** @brief Replaces the display measurements and repaints. */
    void setMeasurements(const QVector<DisplayMeasurement>& measurements)
    {
        m_measurements = measurements;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        for (const auto& measurement : m_measurements)
        {
            if (measurement.points.size() < 2)
            {
                continue;
            }

            QColor color = measurement.color;
            if (measurement.preview)
            {
                color.setAlpha(170);
            }

            QPen pen(color, measurement.preview ? 1.5 : 2.0);
            if (measurement.preview)
            {
                pen.setStyle(Qt::DashLine);
            }
            painter.setPen(pen);

            for (int index = 1; index < measurement.points.size(); ++index)
            {
                painter.drawLine(measurement.points[index - 1], measurement.points[index]);
            }
            if (measurement.closedShape)
            {
                painter.drawLine(measurement.points.last(), measurement.points.first());
            }

            if (measurement.filled)
            {
                QColor fillColor = color;
                fillColor.setAlpha(measurement.preview ? 35 : 55);
                painter.setBrush(fillColor);
                painter.drawPolygon(QPolygonF(measurement.points));
                painter.setBrush(color);
            }

            painter.setBrush(color);
            for (const QPointF& point : measurement.points)
            {
                painter.drawEllipse(point, 3.5, 3.5);
            }

            if (!measurement.label.isEmpty())
            {
                drawLabel(painter, measurement);
            }
        }
    }

private:
    static void drawLabel(QPainter& painter, const DisplayMeasurement& measurement)
    {
        const QPointF anchor = measurement.labelAnchor.isNull()
            ? (measurement.points.last() + QPointF(8.0, -8.0))
            : measurement.labelAnchor;
        painter.setFont(QFont(painter.font().family(), 10));

        const QFontMetrics metrics(painter.font());
        const QStringList lines = measurement.label.split('\n');
        int maxWidth = 0;
        int totalHeight = 0;
        for (const QString& line : lines)
        {
            const QRect lineRect = metrics.boundingRect(line);
            maxWidth = std::max(maxWidth, lineRect.width());
            totalHeight += lineRect.height();
        }
        QRectF labelRect(anchor, QSizeF(maxWidth + 10.0, totalHeight + 6.0));
        labelRect.translate(0.0, -labelRect.height());

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 190));
        painter.drawRoundedRect(labelRect, 3.0, 3.0);

        painter.setPen(measurement.color);
        painter.drawText(labelRect, Qt::AlignCenter, measurement.label);
    }

private:
    QVector<DisplayMeasurement> m_measurements;
};
