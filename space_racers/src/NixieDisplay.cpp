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

#include "NixieDisplay.hpp"

#include <xyginext/ecs/components/Drawable.hpp>

#include <SFML/Graphics/Texture.hpp>

namespace
{
    const sf::Vector2f frameCount(4.f, 3.f);
}

NixieSystem::NixieSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(NixieSystem))
{
    requireComponent<xy::Drawable>();
    requireComponent<Nixie>();
}

//public
void NixieSystem::process(float)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& nixie = entity.getComponent<Nixie>();
        std::uint32_t flags = (nixie.lowerValue << 4) | (nixie.upperValue);
        if (flags != nixie.dirtyFlags)
        {
            auto& drawable = entity.getComponent<xy::Drawable>();
            auto& verts = drawable.getVertices();
            verts.clear();

            auto frameSize = sf::Vector2f(drawable.getTexture()->getSize());
            frameSize.x /= frameCount.x;
            frameSize.y /= frameCount.y;

            static auto getFrame = [&](std::uint32_t val)->sf::FloatRect
            {
                auto left = (val % static_cast<std::uint32_t>(frameCount.x)) * frameSize.x;
                auto top = (val / static_cast<std::uint32_t>(frameCount.x)) * frameSize.y;
                return { left, top, frameSize.x, frameSize.y };
            };

            for (auto i = 0u; i < nixie.digitCount; ++i)
            {
                switch (i)
                {
                case 0:
                {
                    std::uint32_t val = nixie.lowerValue % 10;
                    auto frame = getFrame(val);

                    verts.emplace_back(sf::Vector2f(), sf::Vector2f(frame.left, frame.top));
                    verts.emplace_back(sf::Vector2f(frame.width, 0.f), sf::Vector2f(frame.left + frame.width, frame.top));
                    verts.emplace_back(sf::Vector2f(frame.width, frame.height), sf::Vector2f(frame.left + frame.width, frame.top + frame.height));
                    verts.emplace_back(sf::Vector2f(0.f, frame.height), sf::Vector2f(frame.left, frame.top + frame.height));
                }
                    break;
                case 1:
                {
                    std::uint32_t val = std::min(9, nixie.lowerValue / 10);
                    auto frame = getFrame(val);

                    verts.emplace_back(sf::Vector2f(-frame.width, 0.f), sf::Vector2f(frame.left, frame.top));
                    verts.emplace_back(sf::Vector2f(), sf::Vector2f(frame.left + frame.width, frame.top));
                    verts.emplace_back(sf::Vector2f(0.f, frame.height), sf::Vector2f(frame.left + frame.width, frame.top + frame.height));
                    verts.emplace_back(sf::Vector2f(-frame.width, frame.height), sf::Vector2f(frame.left, frame.top + frame.height));
                }
                    break;
                case 2:
                {
                    std::uint32_t val = 10; //colon position
                    auto frame = getFrame(val);

                    verts.emplace_back(sf::Vector2f(-frame.width * 2.f, 0.f), sf::Vector2f(frame.left, frame.top));
                    verts.emplace_back(sf::Vector2f(-frame.width * 1.f, 0.f), sf::Vector2f(frame.left + frame.width, frame.top));
                    verts.emplace_back(sf::Vector2f(-frame.width * 1.f, frame.height), sf::Vector2f(frame.left + frame.width, frame.top + frame.height));
                    verts.emplace_back(sf::Vector2f(-frame.width * 2.f, frame.height), sf::Vector2f(frame.left, frame.top + frame.height));

                    val = nixie.upperValue % 10;
                    frame = getFrame(val);

                    verts.emplace_back(sf::Vector2f(-frame.width * 3.f, 0.f), sf::Vector2f(frame.left, frame.top));
                    verts.emplace_back(sf::Vector2f(-frame.width * 2.f, 0.f), sf::Vector2f(frame.left + frame.width, frame.top));
                    verts.emplace_back(sf::Vector2f(-frame.width * 2.f, frame.height), sf::Vector2f(frame.left + frame.width, frame.top + frame.height));
                    verts.emplace_back(sf::Vector2f(-frame.width * 3.f, frame.height), sf::Vector2f(frame.left, frame.top + frame.height));
                }
                    break;
                case 3:
                {
                    std::uint32_t val = std::min(9, nixie.upperValue / 10);
                    auto frame = getFrame(val);

                    verts.emplace_back(sf::Vector2f(-frame.width * 4.f, 0.f), sf::Vector2f(frame.left, frame.top));
                    verts.emplace_back(sf::Vector2f(-frame.width * 3.f, 0.f), sf::Vector2f(frame.left + frame.width, frame.top));
                    verts.emplace_back(sf::Vector2f(-frame.width * 3.f, frame.height), sf::Vector2f(frame.left + frame.width, frame.top + frame.height));
                    verts.emplace_back(sf::Vector2f(-frame.width * 4.f, frame.height), sf::Vector2f(frame.left, frame.top + frame.height));
                }
                    break;
                default: break;
                }
            }

            drawable.updateLocalBounds();
        }
        nixie.dirtyFlags = flags;
    }
}
