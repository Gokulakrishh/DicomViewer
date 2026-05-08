#pragma once

#include <QtQuick3D/QQuick3DGeometry>
#include <QVector3D>

class MeshSceneAdapter;

/**
 * @brief Quick3D geometry object backed by MeshSceneAdapter buffers.
 *
 * Responsibilities:
 * - Upload mesh buffers into QQuick3DGeometry.
 * - Expose center/radius values used by the QML camera scene.
 */
class QmlMeshGeometry : public QQuick3DGeometry
{
    Q_OBJECT
    Q_PROPERTY(QVector3D meshCenter READ meshCenter NOTIFY meshChanged)
    Q_PROPERTY(float boundingRadius READ boundingRadius NOTIFY meshChanged)

public:
    /** @brief Creates the QML mesh geometry object. */
    explicit QmlMeshGeometry(QQuick3DObject* parent = nullptr);

    /** @brief Returns the mesh center in scene coordinates. */
    [[nodiscard]] QVector3D meshCenter() const;
    /** @brief Returns the mesh bounding radius. */
    [[nodiscard]] float boundingRadius() const;

    /** @brief Updates geometry from prepared mesh scene buffers. */
    void setMesh(const MeshSceneAdapter& sceneAdapter);
    /** @brief Clears uploaded mesh geometry. */
    void clearMesh();

signals:
    void meshChanged();

private:
    void rebuildGeometry(const MeshSceneAdapter& sceneAdapter);

private:
    QVector3D m_meshCenter;
    float m_boundingRadius{0.0f};
};
