#pragma once

#include <QDialog>

class QLabel;
class QVBoxLayout;
class QWidget;

class AppDialogBase : public QDialog
{
    Q_OBJECT

public:
    explicit AppDialogBase(QWidget* parent = nullptr);

protected:
    void setDialogTitleText(const QString& title);
    void setDialogMessageText(const QString& message);
    QVBoxLayout* bodyLayout() const;

private:
    QLabel* m_brandLabel{nullptr};
    QLabel* m_titleLabel{nullptr};
    QLabel* m_messageLabel{nullptr};
    QVBoxLayout* m_bodyLayout{nullptr};
};
