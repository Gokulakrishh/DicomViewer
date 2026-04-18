#pragma once

#include "Model/MeshData.h"

#include <cmath>

namespace Math3D
{
template<MeshScalar TScalar>
[[nodiscard]] inline Vec3<TScalar> add(const Vec3<TScalar>& a, const Vec3<TScalar>& b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

template<MeshScalar TScalar>
[[nodiscard]] inline Vec3<TScalar> subtract(const Vec3<TScalar>& a, const Vec3<TScalar>& b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

template<MeshScalar TScalar>
[[nodiscard]] inline Vec3<TScalar> multiply(const Vec3<TScalar>& value, TScalar scalar)
{
    return {value.x * scalar, value.y * scalar, value.z * scalar};
}

template<MeshScalar TScalar>
[[nodiscard]] inline TScalar dot(const Vec3<TScalar>& a, const Vec3<TScalar>& b)
{
    return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

template<MeshScalar TScalar>
[[nodiscard]] inline Vec3<TScalar> cross(const Vec3<TScalar>& a, const Vec3<TScalar>& b)
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x};
}

template<MeshScalar TScalar>
[[nodiscard]] inline TScalar lengthSquared(const Vec3<TScalar>& value)
{
    return dot(value, value);
}

template<MeshScalar TScalar>
[[nodiscard]] inline TScalar length(const Vec3<TScalar>& value)
{
    return static_cast<TScalar>(std::sqrt(lengthSquared(value)));
}

template<MeshScalar TScalar>
[[nodiscard]] inline Vec3<TScalar> normalize(const Vec3<TScalar>& value)
{
    const TScalar magnitude = length(value);
    if (magnitude <= static_cast<TScalar>(0))
    {
        return {static_cast<TScalar>(0), static_cast<TScalar>(0), static_cast<TScalar>(1)};
    }

    return {value.x / magnitude, value.y / magnitude, value.z / magnitude};
}

template<MeshScalar TScalar>
[[nodiscard]] inline Vec3<TScalar> lerp(const Vec3<TScalar>& a, const Vec3<TScalar>& b, TScalar t)
{
    return add(a, multiply(subtract(b, a), t));
}
}
