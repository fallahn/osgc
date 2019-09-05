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

#pragma once

#include <xyginext/ecs/System.hpp>

#include <vector>

namespace sf
{
    class Shader;
}

namespace xy
{
    class ShaderResource;
}

struct ShadowCaster final
{
    xy::Entity parent;
};

class ShadowCastSystem final : public xy::System
{
public:
    ShadowCastSystem(xy::MessageBus&, xy::ShaderResource&);

    void process(float) override;

    void onEntityAdded(xy::Entity) override;

    void addLight(xy::Entity);

private:

    std::vector<xy::Entity> m_lights;
    sf::Shader* m_shader;

    std::pair<sf::Vector2f, float> getRay(sf::Vector2f, sf::Vector2f);
};