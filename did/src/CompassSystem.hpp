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

#include <SFML/Graphics/Shader.hpp>

#include <array>

struct Compass final
{
    //std::int32_t playerID = -1;
    xy::Entity parent;
};

class CompassSystem final :public xy::System
{
public:
    explicit CompassSystem(xy::MessageBus&);

    void handleMessage(const xy::Message&) override;

    void process(float) override;

    void onEntityAdded(xy::Entity) override;

private:
    std::array<sf::Vector2f, 4u> m_playerPoints;
    sf::Shader m_shader;
    std::pair<std::uint32_t, std::int32_t> m_uniform;

    std::size_t m_prepCount;
    void prepShader();
};