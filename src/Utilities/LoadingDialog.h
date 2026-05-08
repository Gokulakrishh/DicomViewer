#pragma once

#include "Utilities/AppDialogBase.h"

class QProgressBar;

/**
 * @brief Modal progress dialog for long DICOM import/loading operations.
 *
 * Responsibilities:
 * - Present status text and progress range/value.
 * - Keep UI responsive during controlled long-running workflows.
 */
class LoadingDialog final : public AppDialogBase
{
    Q_OBJECT

public:
    /** @brief Creates the loading dialog. */
    explicit LoadingDialog(QWidget* parent = nullptr);

    /** @brief Shows the dialog with title and message. */
    void show(const QString& title, const QString& message);
    /** @brief Updates the visible progress message. */
    void setMessage(const QString& message);
    /** @brief Sets the progress range. */
    void setProgressRange(int minimum, int maximum);
    /** @brief Sets the current progress value. */
    void setProgressValue(int value);
    /** @brief Closes the dialog. */
    void close();

private:
    void processUiEvents() const;

private:
    QProgressBar* m_progressBar{nullptr};
};
