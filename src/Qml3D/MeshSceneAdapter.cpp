#include "Qml3D/MeshSceneAdapter.h"

#include "Model/IMeshData.h"

void MeshSceneAdapter::setMesh(std::shared_ptr<IMeshData> mesh)
{
    m_mesh = std::move(mesh);
    rebuildBuffers();
}

void MeshSceneAdapter::clear()
{
    m_mesh.reset();
    m_positionBuffer.clear();
    m_normalBuffer.clear();
    m_indexBuffer.clear();
}

bool MeshSceneAdapter::hasMesh() const
{
    return static_cast<bool>(m_mesh);
}

const std::shared_ptr<IMeshData>& MeshSceneAdapter::mesh() const
{
    return m_mesh;
}

const std::vector<float>& MeshSceneAdapter::positionBuffer() const
{
    return m_positionBuffer;
}

const std::vector<float>& MeshSceneAdapter::normalBuffer() const
{
    return m_normalBuffer;
}

const std::vector<std::uint32_t>& MeshSceneAdapter::indexBuffer() const
{
    return m_indexBuffer;
}

void MeshSceneAdapter::rebuildBuffers()
{
    m_positionBuffer.clear();
    m_normalBuffer.clear();
    m_indexBuffer.clear();

    if (!m_mesh)
    {
        return;
    }

    m_positionBuffer.reserve(m_mesh->vertexCount() * 3U);
    if (m_mesh->hasVertexNormals())
    {
        m_normalBuffer.reserve(m_mesh->vertexCount() * 3U);
    }
    m_indexBuffer.reserve(m_mesh->triangleCount() * 3U);

    for (std::size_t vertexIndex = 0; vertexIndex < m_mesh->vertexCount(); ++vertexIndex)
    {
        const auto position = m_mesh->vertexPosition(vertexIndex);
        m_positionBuffer.push_back(static_cast<float>(position[0]));
        m_positionBuffer.push_back(static_cast<float>(position[1]));
        m_positionBuffer.push_back(static_cast<float>(position[2]));

        if (m_mesh->hasVertexNormals())
        {
            const auto normal = m_mesh->vertexNormal(vertexIndex);
            m_normalBuffer.push_back(static_cast<float>(normal[0]));
            m_normalBuffer.push_back(static_cast<float>(normal[1]));
            m_normalBuffer.push_back(static_cast<float>(normal[2]));
        }
    }

    for (std::size_t triangleIndex = 0; triangleIndex < m_mesh->triangleCount(); ++triangleIndex)
    {
        const auto triangle = m_mesh->triangleIndices(triangleIndex);
        m_indexBuffer.push_back(triangle[0]);
        m_indexBuffer.push_back(triangle[1]);
        m_indexBuffer.push_back(triangle[2]);
    }
}
