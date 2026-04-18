#include "Qml3D/QmlMeshGeometry.h"

#include "Model/IMeshData.h"
#include "Qml3D/MeshSceneAdapter.h"

#include <QByteArray>
#include <QtGlobal>
#include <algorithm>
#include <cmath>

QmlMeshGeometry::QmlMeshGeometry(QQuick3DObject* parent)
    : QQuick3DGeometry(parent)
{
}

QVector3D QmlMeshGeometry::meshCenter() const
{
    return m_meshCenter;
}

float QmlMeshGeometry::boundingRadius() const
{
    return m_boundingRadius;
}

void QmlMeshGeometry::setMesh(const MeshSceneAdapter& sceneAdapter)
{
    rebuildGeometry(sceneAdapter);
}

void QmlMeshGeometry::clearMesh()
{
    clear();
    m_meshCenter = {};
    m_boundingRadius = 0.0f;
    emit meshChanged();
}

void QmlMeshGeometry::rebuildGeometry(const MeshSceneAdapter& sceneAdapter)
{
    clear();

    if (!sceneAdapter.hasMesh() || !sceneAdapter.mesh())
    {
        m_meshCenter = {};
        m_boundingRadius = 0.0f;
        emit meshChanged();
        return;
    }

    const auto& positions = sceneAdapter.positionBuffer();
    const auto& normals = sceneAdapter.normalBuffer();
    const auto& indices = sceneAdapter.indexBuffer();
    const bool hasNormals = !normals.empty();
    const int stride = hasNormals ? static_cast<int>(sizeof(float) * 6) : static_cast<int>(sizeof(float) * 3);
    const std::size_t vertexCount = sceneAdapter.mesh()->vertexCount();

    QByteArray vertexData;
    vertexData.resize(static_cast<qsizetype>(vertexCount * static_cast<std::size_t>(stride)));
    char* vertexBytes = vertexData.data();

    for (std::size_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
    {
        const std::size_t positionOffset = vertexIndex * 3U;
        float* vertex = reinterpret_cast<float*>(vertexBytes + (vertexIndex * static_cast<std::size_t>(stride)));
        vertex[0] = positions[positionOffset + 0U];
        vertex[1] = positions[positionOffset + 1U];
        vertex[2] = positions[positionOffset + 2U];

        if (hasNormals)
        {
            const std::size_t normalOffset = vertexIndex * 3U;
            vertex[3] = normals[normalOffset + 0U];
            vertex[4] = normals[normalOffset + 1U];
            vertex[5] = normals[normalOffset + 2U];
        }
    }

    QByteArray indexData(
        reinterpret_cast<const char*>(indices.data()),
        static_cast<qsizetype>(indices.size() * sizeof(std::uint32_t)));

    setStride(stride);
    setPrimitiveType(QQuick3DGeometry::PrimitiveType::Triangles);
    setVertexData(vertexData);
    setIndexData(indexData);
    addAttribute(QQuick3DGeometry::Attribute::Semantic::PositionSemantic, 0, QQuick3DGeometry::Attribute::ComponentType::F32Type);
    if (hasNormals)
    {
        addAttribute(
            QQuick3DGeometry::Attribute::Semantic::NormalSemantic,
            static_cast<int>(sizeof(float) * 3),
            QQuick3DGeometry::Attribute::ComponentType::F32Type);
    }
    addAttribute(
        QQuick3DGeometry::Attribute::Semantic::IndexSemantic,
        0,
        QQuick3DGeometry::Attribute::ComponentType::U32Type);

    const auto minimumBounds = sceneAdapter.mesh()->minBounds();
    const auto maximumBounds = sceneAdapter.mesh()->maxBounds();
    const QVector3D minimum(
        static_cast<float>(minimumBounds[0]),
        static_cast<float>(minimumBounds[1]),
        static_cast<float>(minimumBounds[2]));
    const QVector3D maximum(
        static_cast<float>(maximumBounds[0]),
        static_cast<float>(maximumBounds[1]),
        static_cast<float>(maximumBounds[2]));
    setBounds(minimum, maximum);

    m_meshCenter = (minimum + maximum) * 0.5f;
    const QVector3D halfExtents = (maximum - minimum) * 0.5f;
    m_boundingRadius = std::max(
        1.0f,
        std::sqrt(
            (halfExtents.x() * halfExtents.x()) +
            (halfExtents.y() * halfExtents.y()) +
            (halfExtents.z() * halfExtents.z())));

    emit meshChanged();
}
