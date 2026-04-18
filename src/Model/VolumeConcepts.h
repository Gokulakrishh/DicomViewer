#pragma once

#include <concepts>
#include <cstdint>

template<typename T>
concept VolumeVoxel = std::integral<T> || std::floating_point<T>;

template<typename T>
concept MaskVoxel = std::same_as<T, std::uint8_t> || std::same_as<T, bool>;
