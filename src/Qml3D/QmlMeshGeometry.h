#pragma once

#include <QtQuick3D/QQuick3DGeometry>
#include <QVector3D>

class MeshSceneAdapter;

class QmlMeshGeometry : public QQuick3DGeometry
{
    Q_OBJECT
    Q_PROPERTY(QVector3D meshCenter READ meshCenter NOTIFY meshChanged)
    Q_PROPERTY(float boundingRadius READ boundingRadius NOTIFY meshChanged)

public:
    explicit QmlMeshGeometry(QQuick3DObject* parent = nullptr);

    [[nodiscard]] QVector3D meshCenter() const;
    [[nodiscard]] float boundingRadius() const;

    void setMesh(const MeshSceneAdapter& sceneAdapter);
    void clearMesh();

signals:
    void meshChanged();

private:
    void rebuildGeometry(const MeshSceneAdapter& sceneAdapter);

private:
    QVector3D m_meshCenter;
    float m_boundingRadius{0.0f};
};
