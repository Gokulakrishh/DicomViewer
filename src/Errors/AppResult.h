#pragma once

#include "Errors/AppError.h"

#include <type_traits>
#include <utility>
#include <variant>

template<typename T>
class AppResult
{
public:
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

    AppResult(AppError error)
        : m_data(std::move(error))
    {
    }

    [[nodiscard]] bool hasValue() const
    {
        return std::holds_alternative<T>(m_data);
    }

    [[nodiscard]] explicit operator bool() const
    {
        return hasValue();
    }

    [[nodiscard]] T& value()
    {
        return std::get<T>(m_data);
    }

    [[nodiscard]] const T& value() const
    {
        return std::get<T>(m_data);
    }

    [[nodiscard]] AppError& error()
    {
        return std::get<AppError>(m_data);
    }

    [[nodiscard]] const AppError& error() const
    {
        return std::get<AppError>(m_data);
    }

private:
    std::variant<T, AppError> m_data;
};
