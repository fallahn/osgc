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

#include "ServerWeaponDirector.hpp"
#include "MessageIDs.hpp"
#include "PlayerSystem.hpp"
#include "InventorySystem.hpp"
#include "QuadTreeFilter.hpp"
#include "CollisionBounds.hpp"
#include "Actor.hpp"
#include "SkeletonSystem.hpp"
#include "ServerSharedStateData.hpp"
#include "Packet.hpp"
#include "PacketTypes.hpp"
#include "Server.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>
#include <xyginext/ecs/Scene.hpp>

#include <array>

namespace
{
    const float WeaponWidth = 1.f;
    const float WeaponHalfWidth = WeaponWidth / 2.f;
    const float SwordLength = 22.f;
    const float PistolLength = 256.f;

    //indexed by Player::Direction enum
    //up, down, left, right
    const std::array<sf::FloatRect,4u> swords = 
    {
        sf::FloatRect(-WeaponHalfWidth, 0.f, WeaponWidth, -SwordLength),
        sf::FloatRect(-WeaponHalfWidth, 0.f, WeaponWidth, SwordLength),
        sf::FloatRect(0.f, -WeaponHalfWidth, -SwordLength, WeaponWidth),
        sf::FloatRect(0.f, -WeaponHalfWidth, SwordLength, WeaponWidth)
    };

    const std::array<sf::FloatRect, 4u> pistols = 
    {
        sf::FloatRect(-WeaponHalfWidth, 0.f, WeaponWidth, -PistolLength),
        sf::FloatRect(-WeaponHalfWidth, 0.f, WeaponWidth, PistolLength),
        sf::FloatRect(0.f, -WeaponHalfWidth, -PistolLength, WeaponWidth),
        sf::FloatRect(0.f, -WeaponHalfWidth, PistolLength, WeaponWidth)
    };

}

ServerWeaponDirector::ServerWeaponDirector(Server::SharedStateData& sd)
    : m_sharedData(sd)
{

}

//public
void ServerWeaponDirector::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::PlayerMessage)
    {
        const auto& data = msg.getData<PlayerEvent>();
        if (data.action == PlayerEvent::DidAction)
        {
            //let the clients know someone did something
            SceneState state;
            state.action = SceneState::PlayerDidAction;
            state.serverID = data.entity.getIndex();
            m_sharedData.gameServer->broadcastData(PacketID::SceneUpdate, state);

            auto weapon = data.entity.getComponent<Inventory>().weapon;
            sf::FloatRect hitbox;
            if (weapon == Inventory::Shovel)
            {
                return;
            }
            else if (weapon == Inventory::Pistol)
            {
                hitbox = pistols[data.entity.getComponent<Player>().sync.direction];
            }
            else if (weapon == Inventory::Sword)
            {
                hitbox = swords[data.entity.getComponent<Player>().sync.direction];
            }
            hitbox = data.entity.getComponent<xy::Transform>().getWorldTransform().transformRect(hitbox);

            auto entities = getScene().getSystem<xy::DynamicTreeSystem>().query(hitbox, QuadTreeFilter::Player | QuadTreeFilter::Skeleton | QuadTreeFilter::Barrel);
            for (auto entity : entities)
            {
                if (entity != data.entity)
                {
                    auto bounds = entity.getComponent<CollisionComponent>().bounds;
                    bounds = entity.getComponent<xy::Transform>().getWorldTransform().transformRect(bounds);

                    if (bounds.intersects(hitbox))
                    {
                        auto* newMsg = postMessage<PlayerEvent>(MessageID::PlayerMessage);
                        newMsg->action = (weapon == Inventory::Pistol) ? PlayerEvent::PistolHit : PlayerEvent::SwordHit;
                        newMsg->entity = entity;
                        newMsg->data = data.entity.getComponent<Actor>().id;

                        if (entity.getComponent<Actor>().id == Actor::ID::Skeleton)
                        {
                            entity.getComponent<Inventory>().lastDamager = data.entity.getComponent<Actor>().id;
                            
                            auto& skeleton = entity.getComponent<Skeleton>();
                            float strength = (weapon == Inventory::Pistol) ? 20.f : 60.f;

                            switch (data.entity.getComponent<Player>().sync.direction)
                            {
                            default: break;
                            case Player::Up:
                                skeleton.velocity.y = -strength;
                                break;
                            case Player::Down:
                                skeleton.velocity.y = strength;
                                break;
                            case Player::Left:
                                skeleton.velocity.x = -strength;
                                break;
                            case Player::Right:
                                skeleton.velocity.x = strength;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}

//private
