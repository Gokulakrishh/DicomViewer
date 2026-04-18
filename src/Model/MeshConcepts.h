#pragma once

#include <concepts>

template<typename T>
concept MeshScalar = std::floating_point<T>;
