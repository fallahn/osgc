/*********************************************************************

Copyright 2019 Matt Marchant

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*********************************************************************/

#include "Sprite3D.hpp"
#include "GlobalConsts.hpp"
#include "Camera3D.hpp"
#include "HealthBarSystem.hpp"

#include "glad/glad.h"

#include "glm/gtc/matrix_transform.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/Drawable.hpp>

#include <xyginext/ecs/Scene.hpp>
#include <xyginext/core/App.hpp>

#include <SFML/Graphics/Shader.hpp>

Sprite3DSystem::Sprite3DSystem(xy::MessageBus& mb, const std::vector<sf::Shader*>& spriteShaders)
    : xy::System    (mb, typeid(Sprite3DSystem)),
    m_spriteShaders (spriteShaders)
{
    requireComponent<Sprite3D>();
    requireComponent<xy::Drawable>();
    requireComponent<xy::Transform>();
}

//public
void Sprite3DSystem::process(float)
{
    auto& entities = getEntities();
    for (auto& entity : entities)
    {
        const auto& tx = entity.getComponent<xy::Transform>();
        auto position = tx.getWorldPosition();
        auto& spr = entity.getComponent<Sprite3D>();
        auto& drawable = entity.getComponent<xy::Drawable>();

        //we're also flipping the sprites vertically with a negative scale  
        float height = tx.getTransform().transformRect(drawable.getLocalBounds()).height;

        //SFML 'optimises' by pre-transforming verts if there are 4 or fewer
        auto& matrix = spr.getMatrix();
        if (spr.needsCorrection)
        {
            matrix = glm::translate(glm::mat4(1.f), glm::vec3(0.f, height - position.y - spr.verticalOffset, position.y));
        }
        else
        {
            auto scale = tx.getScale(); //sadly not world (combined) scale

            matrix = glm::translate(glm::mat4(1.f), glm::vec3(position.x, -spr.verticalOffset, position.y));
            matrix = glm::scale(matrix, glm::vec3(scale.x, scale.y, (scale.x + scale.y) / 2.f));
        }


        if (drawable.getDepth() > Global::MaxSortingDepth)
        {
            sf::Int32 zDepth = static_cast<sf::Int32>((position.y));
            drawable.setDepth(zDepth);
        }
    }

    //update the shader with the camera world position
    auto camEnt = getScene()->getActiveCamera();
    const auto& camera = camEnt.getComponent<Camera3D>();
    auto camPos = camera.worldPosition.z - (Global::PlayerCameraOffset - 30.f);

    //for (auto shader : m_spriteShaders)
    //{
    //    //compensate for the offset value in the shader - see Global::PlayerCameraOffset
    //    shader->setUniform("u_cameraWorldPosition", camPos);

    //    //DPRINT("Cam pos", std::to_string(tx.y));
    //}

    for (auto [program, uniform] : m_uniformMap)
    {
        glUseProgram(program);
        glUniform1f(uniform, camPos);        
    }
    glUseProgram(0);
}

void Sprite3DSystem::onEntityAdded(xy::Entity)
{
    //we do this here as a delayed operation
    //because apparently at the time of this system's
    //construction the shaders return incorrect uniform locations
    if (m_uniformMap.empty())
    {
        for (const auto* s : m_spriteShaders)
        {
            glUseProgram(s->getNativeHandle());
            auto loc = glGetUniformLocation(s->getNativeHandle(), "u_cameraWorldPosition");
            m_uniformMap.insert(std::make_pair(s->getNativeHandle(), loc));
        }
    }
}