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

#include "Camera3D.hpp"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/util/Vector.hpp>
#include <xyginext/util/Wavetable.hpp>

namespace
{

}

Camera3DSystem::Camera3DSystem(xy::MessageBus& mb)
    : xy::System    (mb, typeid(Camera3DSystem))
{
    requireComponent<Camera3D>();
}

//public
void Camera3DSystem::process(float)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& tx = entity.getComponent<xy::Transform>();
        auto& camera = entity.getComponent<Camera3D>();

        //we can save a matrix inv op here because we only want the translation, and
        //just use the inverse positions EXCEPT for the y position, as SFML inverts
        //it when drawing top to bottom Y coords - hence the scale op underneath!!
        //camera.viewMatrix = camera.rotationMatrix;
        camera.viewMatrix = glm::translate(/*camera.viewMatrix*/glm::mat4(1.f), glm::vec3(-tx.getPosition().x, tx.getPosition().y, -camera.depth));
        camera.viewMatrix = glm::scale(camera.viewMatrix, glm::vec3(1.f, -1.f, 1.f));

        //these are updated by the camera transport system
        //camera.viewMatrix *= camera.postRotationMatrix * camera.postTranslationMatrix;
        //camera.worldPosition = glm::inverse(camera.viewMatrix) * glm::vec4(0.f, 0.f, 0.f, 1.f);

        //xy::Console::printStat("cam pos", std::to_string(camera.worldPosition.x) + ", " + std::to_string(camera.worldPosition.y));

        camera.viewProjectionMatrix = camera.projectionMatrix * camera.viewMatrix;
    }
}