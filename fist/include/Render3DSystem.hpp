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

#include <SFML/Graphics/Drawable.hpp>

#include <array>

class Render3DSystem final : public xy::System, public sf::Drawable
{
public:
    explicit Render3DSystem(xy::MessageBus&);

    void process(float) override;

    void setFOV(float);

    void setCamera(xy::Entity cam) { m_camera = cam; }

private:

    struct Frustum final
    {
        //points are 2 directions indicating frustum sides
        sf::Vector2f left;
        sf::Vector2f right;
    } m_frustum;

    std::vector<std::pair<xy::Entity, float>> m_drawList;

    xy::Entity m_camera;

    float lineSide(sf::Vector2f, sf::Vector2f);
    void draw(sf::RenderTarget&, sf::RenderStates) const override;
};