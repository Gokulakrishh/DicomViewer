#pragma once

#include <QDialog>

class QLabel;
class QVBoxLayout;
class QWidget;

/**
 * @brief Shared branded base class for application dialogs.
 *
 * Responsibilities:
 * - Provide consistent title/message/body layout.
 * - Keep individual dialogs focused on their specific controls.
 */
class AppDialogBase : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Creates the dialog base.
     * @param parent Optional Qt parent.
     */
    explicit AppDialogBase(QWidget* parent = nullptr);

protected:
    /** @brief Sets the visible dialog title text. */
    void setDialogTitleText(const QString& title);
    /** @brief Sets the visible dialog message text. */
    void setDialogMessageText(const QString& message);
    /** @brief Returns the layout used by derived dialogs for body controls. */
    QVBoxLayout* bodyLayout() const;

private:
    QLabel* m_brandLabel{nullptr};
    QLabel* m_titleLabel{nullptr};
    QLabel* m_messageLabel{nullptr};
    QVBoxLayout* m_bodyLayout{nullptr};
};
