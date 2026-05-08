#pragma once

#include <QPainter>
#include <QPoint>
#include <QWidget>

/**
 * @brief Lightweight Qt overlay that draws crosshair guide lines.
 *
 * Responsibilities:
 * - Render crosshair lines over the image viewport.
 * - Ignore mouse events so viewer tools receive interaction normally.
 */
class CrosshairOverlayWidget : public QWidget
{
public:
    /** @brief Creates the crosshair overlay. */
    explicit CrosshairOverlayWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_AlwaysStackOnTop);
    }

    /** @brief Sets crosshair position in widget coordinates. */
    void setPosition(const QPoint& position)
    {
        m_position = position;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        if (m_position.x() < 0 || m_position.y() < 0)
        {
            return;
        }

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, false);

        QPen pen(QColor(72, 164, 255, 200));
        pen.setWidth(2);
        painter.setPen(pen);
        painter.drawLine(0, m_position.y(), width(), m_position.y());
        painter.drawLine(m_position.x(), 0, m_position.x(), height());
    }

private:
    QPoint m_position{-1, -1};
};
