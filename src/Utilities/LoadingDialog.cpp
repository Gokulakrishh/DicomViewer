#include "Utilities/LoadingDialog.h"

#include <QApplication>
#include <QProgressBar>
#include <QVBoxLayout>

LoadingDialog::LoadingDialog(QWidget* parent)
    : AppDialogBase(parent)
{
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 0);
    m_progressBar->setTextVisible(false);
    bodyLayout()->addWidget(m_progressBar);
}

void LoadingDialog::show(const QString& title, const QString& message)
{
    setDialogTitleText(title);
    setDialogMessageText(message);
    QDialog::show();
    processUiEvents();
}

void LoadingDialog::setMessage(const QString& message)
{
    setDialogMessageText(message);
    processUiEvents();
}

void LoadingDialog::close()
{
    QDialog::close();
}

void LoadingDialog::processUiEvents() const
{
    if (qApp)
    {
        qApp->processEvents();
    }
}
