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

#include "ItemBar.hpp"

#include <xyginext/ecs/components/Drawable.hpp>

ItemBarSystem::ItemBarSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(ItemBarSystem))
{
    requireComponent<ItemBar>();
    requireComponent<xy::Drawable>();
}

//public
void ItemBarSystem::process(float)
{
    auto& entities = getEntities();
    
    for(auto entity : entities)
    {
        auto& drawable = entity.getComponent<xy::Drawable>();
        const auto& itemBar = entity.getComponent<ItemBar>();

        auto& verts = drawable.getVertices();
        verts.clear();

        //full rows
        auto rowCount = itemBar.itemCount / itemBar.xCount;
        float remainTop = 0.f;
        auto rect = itemBar.textureRect;

        for (auto y = 0; y < rowCount; ++y)
        {
            for (auto x = 0; x < itemBar.xCount; ++x)
            {
                float left = x * rect.width;
                float top = y * rect.height;

                verts.emplace_back(sf::Vector2f(left, top), sf::Vector2f(rect.left, rect.top));
                verts.emplace_back(sf::Vector2f(left + rect.width, top), sf::Vector2f(rect.left + rect.width, rect.top));
                verts.emplace_back(sf::Vector2f(left + rect.width, top + rect.height), sf::Vector2f(rect.left + rect.width, rect.top + rect.height));
                verts.emplace_back(sf::Vector2f(left, top + rect.height), sf::Vector2f(rect.left, rect.top + rect.height));
            }
            remainTop += rect.height;
        }

        //remaining icons
        rowCount = itemBar.itemCount % itemBar.xCount;
        for (auto i = 0; i < rowCount; ++i)
        {
            float left = i * rect.width;

            verts.emplace_back(sf::Vector2f(left, remainTop), sf::Vector2f(rect.left, rect.top));
            verts.emplace_back(sf::Vector2f(left + rect.width, remainTop), sf::Vector2f(rect.left + rect.width, rect.top));
            verts.emplace_back(sf::Vector2f(left + rect.width, remainTop + rect.height), sf::Vector2f(rect.left + rect.width, rect.top + rect.height));
            verts.emplace_back(sf::Vector2f(left, remainTop + rect.height), sf::Vector2f(rect.left, rect.top + rect.height));
        }

        drawable.updateLocalBounds();
    }
}