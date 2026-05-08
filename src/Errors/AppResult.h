#pragma once

#include "Errors/AppError.h"

#include <type_traits>
#include <utility>
#include <variant>

/**
 * @brief Small value-or-error result type for recoverable application workflows.
 *
 * Responsibilities:
 * - Return either a value or AppError without exceptions.
 * - Make error handling explicit at service boundaries.
 *
 * Assumptions:
 * - Callers check hasValue() or the bool conversion before accessing value().
 */
template<typename T>
class AppResult
{
public:
    /**
     * @brief Creates a successful result.
     * @param value Value convertible to T.
     */
    template<
        typename U,
        typename = std::enable_if_t<
            std::is_constructible_v<T, U&&> &&
            !std::is_same_v<std::decay_t<U>, AppResult> &&
            !std::is_same_v<std::decay_t<U>, AppError>>>
    AppResult(U&& value)
        : m_data(T(std::forward<U>(value)))
    {
    }

    /**
     * @brief Creates an error result.
     * @param error Application error payload.
     */
    AppResult(AppError error)
        : m_data(std::move(error))
    {
    }

    /**
     * @brief Reports whether the result contains a value.
     * @return True when value() can be accessed.
     */
    [[nodiscard]] bool hasValue() const
    {
        return std::holds_alternative<T>(m_data);
    }

    /**
     * @brief Boolean success check.
     * @return True when the result contains a value.
     */
    [[nodiscard]] explicit operator bool() const
    {
        return hasValue();
    }

    /**
     * @brief Returns the contained value.
     * @return Mutable contained value.
     */
    [[nodiscard]] T& value()
    {
        return std::get<T>(m_data);
    }

    /**
     * @brief Returns the contained value.
     * @return Contained value.
     */
    [[nodiscard]] const T& value() const
    {
        return std::get<T>(m_data);
    }

    /**
     * @brief Returns the contained error.
     * @return Mutable application error.
     */
    [[nodiscard]] AppError& error()
    {
        return std::get<AppError>(m_data);
    }

    /**
     * @brief Returns the contained error.
     * @return Application error.
     */
    [[nodiscard]] const AppError& error() const
    {
        return std::get<AppError>(m_data);
    }

private:
    std::variant<T, AppError> m_data;
};
