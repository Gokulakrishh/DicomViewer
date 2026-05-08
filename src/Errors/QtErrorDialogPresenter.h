#pragma once

#include "Errors/IErrorPresenter.h"

class QWidget;

/**
 * @brief Qt dialog-based application error presenter.
 *
 * Responsibilities:
 * - Convert AppError payloads into user-visible warning/error dialogs.
 * - Keep parent widget ownership outside service code.
 */
class QtErrorDialogPresenter final : public IErrorPresenter
{
public:
    /**
     * @brief Creates a Qt error presenter.
     * @param parent Optional dialog parent widget.
     */
    explicit QtErrorDialogPresenter(QWidget* parent = nullptr);

    /**
     * @brief Updates the dialog parent widget.
     * @param parent Optional Qt parent widget.
     */
    void setParentWidget(QWidget* parent) override;

    /**
     * @brief Presents an application error dialog.
     * @param error Structured error payload.
     */
    void present(const AppError& error) const override;

private:
    QWidget* m_parent{nullptr};
};
