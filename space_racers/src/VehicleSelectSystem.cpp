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

#include "VehicleSelectSystem.hpp"
#include "MenuConsts.hpp"

#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/util/Math.hpp>

#include <array>

namespace
{
    const std::array<float, 3u> targets = { -90.f, -45.f, -135.f };
}

VehicleSelectSystem::VehicleSelectSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(VehicleSelectSystem))
{
    requireComponent<xy::Drawable>();
    requireComponent<VehicleSelect>();
}

//public
void VehicleSelectSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities) 
    {
        auto& select = entity.getComponent<VehicleSelect>();
        auto rotation = xy::Util::Math::shortestRotation(select.rotation, targets[select.index]) * (dt * 5.f);
        select.rotation += rotation;

        sf::Transform tx;
        tx.rotate(select.rotation);

        
        const sf::FloatRect needleRect(0.f, 536.f, 192.f, 27.f);
        const sf::Vector2f needlePos(258.5f, 254.f);

        auto& drawable = entity.getComponent<xy::Drawable>();
        auto& vertices = drawable.getVertices();

        sf::Vertex v(sf::Vector2f(0.f, - needleRect.height / 2.f), sf::Vector2f(needleRect.left, needleRect.top));
        v.position = tx.transformPoint(v.position);
        v.position += needlePos;
        vertices[4] = v;

        v = sf::Vertex(sf::Vector2f(needleRect.width, -needleRect.height / 2.f), sf::Vector2f(needleRect.width, needleRect.top));
        v.position = tx.transformPoint(v.position);
        v.position += needlePos;
        vertices[5] = v;

        v = sf::Vertex(sf::Vector2f(needleRect.width, needleRect.height / 2.f), sf::Vector2f(needleRect.width, needleRect.top + needleRect.height));
        v.position = tx.transformPoint(v.position);
        v.position += needlePos;
        vertices[6] = v;

        v = sf::Vertex(sf::Vector2f(0.f, needleRect.height / 2.f), sf::Vector2f(needleRect.left, needleRect.top + needleRect.height));
        v.position = tx.transformPoint(v.position);
        v.position += needlePos;
        vertices[7] = v;

        drawable.updateLocalBounds();
    }
}

//private
void VehicleSelectSystem::onEntityAdded(xy::Entity entity)
{
    const sf::FloatRect bigRect = MenuConst::VehicleSelectArea;

    auto& verts = entity.getComponent<xy::Drawable>().getVertices();
    
    //background
    verts.emplace_back(sf::Vector2f(), sf::Vector2f(0.f, bigRect.height));
    verts.emplace_back(sf::Vector2f(bigRect.width, 0.f), sf::Vector2f(bigRect.width, bigRect.height));
    verts.emplace_back(sf::Vector2f(bigRect.width, bigRect.height), sf::Vector2f(bigRect.width, bigRect.height * 2.f));
    verts.emplace_back(sf::Vector2f(0.f, bigRect.height), sf::Vector2f(0.f, bigRect.height * 2.f));

    //needle
    verts.emplace_back(sf::Vector2f(), sf::Vector2f());
    verts.emplace_back(sf::Vector2f(), sf::Vector2f());
    verts.emplace_back(sf::Vector2f(), sf::Vector2f());
    verts.emplace_back(sf::Vector2f(), sf::Vector2f());

    //foreground
    verts.emplace_back(sf::Vector2f(), sf::Vector2f());
    verts.emplace_back(sf::Vector2f(bigRect.width, 0.f), sf::Vector2f(bigRect.width, 0.f));
    verts.emplace_back(sf::Vector2f(bigRect.width, bigRect.height), sf::Vector2f(bigRect.width, bigRect.height));
    verts.emplace_back(sf::Vector2f(0.f, bigRect.height), sf::Vector2f(0.f, bigRect.height));
}