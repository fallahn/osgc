/*
Handles rendering of the current collection of meshes, and texture baking.
*/

#pragma once

#include "structures.h"

#include <string>
#include <memory>

class Scene final
{
public:
    Scene() {};

    bool init();
    void draw(const glm::mat4& view, const glm::mat4& projection, bool = true) const;
    void destroy();
    bool bake(const std::string&, const std::array<float, 3>&) const;

    std::vector<std::unique_ptr<Mesh>>& getMeshes() { return m_meshes; }
    const std::vector<std::unique_ptr<Mesh>>& getMeshes() const { return m_meshes; }

    const std::string& getProgress() const { return m_progressString; }

    void setLightmapSize(std::int32_t w, std::int32_t h) { m_lightmapWidth = w; m_lightmapHeight = h; }

    void saveLightmap(const std::string&);

    void createMeasureMesh(const std::string& path);

    void removeMeasureMesh();

    void setzUp(bool z) { m_zUp = z; }

    bool getzUp() const { return m_zUp; }

private:
    GLuint m_programID = 0;
    GLint m_textureUniform = 0;
    GLint m_projectionUniform = 0;
    GLint m_viewUniform = 0;
    GLint m_modelUniform = 0;

    std::int32_t m_lightmapWidth = 0;
    std::int32_t m_lightmapHeight = 0;

    std::vector<std::unique_ptr<Mesh>> m_meshes;
    std::unique_ptr<Mesh> m_measureMesh;

    mutable std::string m_progressString;

    bool m_zUp = true;
};

using scene_t = Scene;