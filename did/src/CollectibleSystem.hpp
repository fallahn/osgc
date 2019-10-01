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

namespace Server
{
    struct SharedStateData;
}

struct Collectible final
{
    static constexpr float DespawnTime = 60.f;
    std::int32_t value = 0;
    enum class Type
    {
        Ammo,
        Coin,
        Food
    }type = Type::Ammo;
    float collectionTime = 1.f; //not collectible until 0
    float lifeTime = DespawnTime; //despawn after a while
    sf::Vector2f velocity;

    enum Despawn
    {
        TimeOut, Collected
    };
};

class CollectibleSystem final : public xy::System
{
public:
    CollectibleSystem(xy::MessageBus&, Server::SharedStateData&);

    void process(float) override;

private:
    Server::SharedStateData& m_sharedData;

    void onEntityAdded(xy::Entity) override;
};