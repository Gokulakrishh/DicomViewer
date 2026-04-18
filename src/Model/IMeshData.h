#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

class IMeshData
{
public:
    virtual ~IMeshData() = default;

    [[nodiscard]] virtual std::size_t vertexCount() const = 0;
    [[nodiscard]] virtual std::size_t triangleCount() const = 0;
    [[nodiscard]] virtual bool hasVertexNormals() const = 0;

    [[nodiscard]] virtual std::array<double, 3> vertexPosition(std::size_t index) const = 0;
    [[nodiscard]] virtual std::array<double, 3> vertexNormal(std::size_t index) const = 0;
    [[nodiscard]] virtual std::array<std::uint32_t, 3> triangleIndices(std::size_t index) const = 0;

    [[nodiscard]] virtual std::array<double, 3> minBounds() const = 0;
    [[nodiscard]] virtual std::array<double, 3> maxBounds() const = 0;
};
