#pragma once

#include "structures.h"

#include <string>

class Scene final
{
public:
    Scene() {};

    bool init();
    void draw(const glm::mat4& view, const glm::mat4& projection) const;
    void destroy();
    bool bake(const std::string&, const std::array<float, 3>&) const;

    std::vector<std::unique_ptr<Mesh>>& getMeshes() { return m_meshes; }

    const std::string& getProgress() const { return m_progressString; }

    void setLightmapSize(std::int32_t w, std::int32_t h) { m_lightmapWidth = w; m_lightmapHeight = h; }

private:
    GLuint m_programID = 0;
    GLint m_textureUniform = 0;
    GLint m_projectionUniform = 0;
    GLint m_viewUniform = 0;
    GLint m_modelUniform = 0;

    std::int32_t m_lightmapWidth = 0;
    std::int32_t m_lightmapHeight = 0;

    std::vector<std::unique_ptr<Mesh>> m_meshes;

    mutable std::string m_progressString;
};

using scene_t = Scene;