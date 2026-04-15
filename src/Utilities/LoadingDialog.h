#pragma once

#include "Utilities/AppDialogBase.h"

class QProgressBar;

class LoadingDialog final : public AppDialogBase
{
    Q_OBJECT

public:
    explicit LoadingDialog(QWidget* parent = nullptr);

    void show(const QString& title, const QString& message);
    void setMessage(const QString& message);
    void setProgressRange(int minimum, int maximum);
    void setProgressValue(int value);
    void close();

private:
    void processUiEvents() const;

private:
    QProgressBar* m_progressBar{nullptr};
};
