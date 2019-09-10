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

#include <SFML/Graphics/Glsl.hpp>

#include <vector>

namespace sf
{
    class Shader;
}

namespace xy
{
    class ShaderResource;
}

struct Torchlight final
{
    sf::Glsl::Vec3 colour;
    static constexpr float height = 32.f;
};

class TorchlightSystem final : public xy::System
{
public:
    TorchlightSystem(xy::MessageBus&, xy::ShaderResource&);

    void handleMessage(const xy::Message&) override;

    void process(float) override;

private:
    float m_dayTime; //updated by server but interpolated to know how bright to make lights

    std::vector<sf::Shader*> m_shaders;
    std::vector<float> m_noiseTable;
    std::size_t m_noiseIndex;
};
