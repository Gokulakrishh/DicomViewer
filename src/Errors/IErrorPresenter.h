#pragma once

#include "Errors/AppError.h"

class QWidget;

/**
 * @brief UI boundary for presenting application errors.
 *
 * Responsibilities:
 * - Present recoverable errors to the user.
 * - Keep service code independent of Qt dialog implementation details.
 *
 * Assumptions:
 * - Presentation is separate from error auditing.
 */
class IErrorPresenter
{
public:
    virtual ~IErrorPresenter() = default;

    /**
     * @brief Sets the parent widget used for modal presentation.
     * @param parent Optional Qt parent widget.
     */
    virtual void setParentWidget(QWidget* parent) = 0;

    /**
     * @brief Presents an application error to the user.
     * @param error Structured error payload.
     */
    virtual void present(const AppError& error) const = 0;
};
