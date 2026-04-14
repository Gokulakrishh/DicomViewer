#pragma once

#include <concepts>

template<typename T>
concept VolumeVoxel = std::integral<T> || std::floating_point<T>;
