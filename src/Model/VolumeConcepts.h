#pragma once

#include <concepts>
#include <cstdint>

/**
 * @brief Concept for scalar voxel values accepted by VolumeData.
 */
template<typename T>
concept VolumeVoxel = std::integral<T> || std::floating_point<T>;

/**
 * @brief Concept for binary/label mask voxel values.
 */
template<typename T>
concept MaskVoxel = std::same_as<T, std::uint8_t> || std::same_as<T, bool>;
