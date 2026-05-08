#pragma once

#include <concepts>

/**
 * @brief Concept for floating-point mesh scalar buffers.
 */
template<typename T>
concept MeshScalar = std::floating_point<T>;
