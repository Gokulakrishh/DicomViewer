#pragma once

#include <cstdint>
#include <memory>
#include <vector>

class IMeshData;

class MeshSceneAdapter
{
public:
    void setMesh(std::shared_ptr<IMeshData> mesh);
    void clear();

    [[nodiscard]] bool hasMesh() const;
    [[nodiscard]] const std::shared_ptr<IMeshData>& mesh() const;
    [[nodiscard]] const std::vector<float>& positionBuffer() const;
    [[nodiscard]] const std::vector<float>& normalBuffer() const;
    [[nodiscard]] const std::vector<std::uint32_t>& indexBuffer() const;

private:
    void rebuildBuffers();

private:
    std::shared_ptr<IMeshData> m_mesh;
    std::vector<float> m_positionBuffer;
    std::vector<float> m_normalBuffer;
    std::vector<std::uint32_t> m_indexBuffer;
};
