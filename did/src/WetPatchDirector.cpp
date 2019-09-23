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

#include "WetPatchDirector.hpp"
#include "MessageIDs.hpp"
#include "QuadTreeFilter.hpp"
#include "InventorySystem.hpp"
#include "PlayerSystem.hpp"
#include "GlobalConsts.hpp"
#include "AnimationSystem.hpp"
#include "Actor.hpp"
#include "ServerSharedStateData.hpp"
#include "PacketTypes.hpp"
#include "Packet.hpp"
#include "Server.hpp"

#include <xyginext/ecs/Scene.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>

namespace
{
    const float OffsetDistance = 12.f;
}

DigDirector::DigDirector(Server::SharedStateData& sd)
    : m_sharedData(sd)
{

}

//public
void DigDirector::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::PlayerMessage)
    {
        const auto& data = msg.getData<PlayerEvent>();
        if (data.action == PlayerEvent::DidAction)
        {
            if (data.entity.getComponent<Inventory>().weapon == Inventory::Shovel)
            {
                const auto& tx = data.entity.getComponent<xy::Transform>();

                auto position = tx.getWorldPosition();
                auto direction = data.entity.getComponent<Player>().sync.direction;
                switch (direction)
                {
                default: break;
                case Player::Left:
                    position.x -= OffsetDistance;
                    break;
                case Player::Right:
                    position.x += OffsetDistance;
                    break;
                case Player::Up:
                    position.y -= OffsetDistance;
                    break;
                case Player::Down:
                    position.y += OffsetDistance;
                    break;
                }

                //fetch nearby holes from quad tree
                auto area = Global::PlayerBounds;
                area.left += position.x;
                area.top += position.y;
                auto holes = getScene().getSystem<xy::DynamicTreeSystem>().query(area, QuadTreeFilter::WetPatch);

                //check if dig point is inside hole bounds
                for (auto e : holes)
                {
                    //if (e.hasComponent<WetPatch>()) //hurrah for quad tree filters!
                    {
                        static const sf::FloatRect area(0.f, 0.f, Global::TileSize, Global::TileSize);

                        auto patchArea = e.getComponent<xy::Transform>().getWorldTransform().transformRect(area);

                        //raise message if necessary
                        if (patchArea.contains(position))
                        {
                            //and modify wet patch properties
                            auto& wetPatch = e.getComponent<WetPatch>();
                            wetPatch.digCount++;
                            wetPatch.digCount %= WetPatch::DigsPerStage;
                            if (wetPatch.digCount == 0 &&
                                wetPatch.state != WetPatch::State::OneHundred)
                            {
                                wetPatch.state = WetPatch::State(static_cast<int>(wetPatch.state) + 1);

                                //send sync packet so clients can update their wet patches
                                HoleState sync;
                                sync.state = wetPatch.state;
                                sync.serverID = e.getIndex();
                                m_sharedData.gameServer->broadcastData(PacketID::PatchSync, sync, xy::NetFlag::Reliable, Global::AnimationChannel);

                                //raise spawn message if hole fully dug
                                if (wetPatch.state == WetPatch::OneHundred)
                                {
                                    auto position = e.getComponent<xy::Transform>().getPosition();
                                    position.x += (Global::TileSize / 2.f);
                                    position.y += (Global::TileSize / 2.f);

                                    /*if (e.getComponent<Actor>().id != Actor::ID::SkellySpawn)
                                    {
                                        auto* msg = postMessage<ActorEvent>(MessageID::ActorMessage);
                                        msg->position = position;

                                        if (e.getComponent<Actor>().id == Actor::ID::TreasureSpawn)
                                        {
                                            msg->id = Actor::ID::Treasure;
                                        }
                                        else
                                        {
                                            msg->id = Actor::ID::Ammo;
                                        }
                                        msg->type = ActorEvent::RequestSpawn;
                                    }*/

                                    //spawn an item
                                    auto* msg = postMessage<ActorEvent>(MessageID::ActorMessage);
                                    msg->position = position;
                                    msg->type = ActorEvent::RequestSpawn;

                                    switch (e.getComponent<Actor>().id)
                                    {
                                    default:
                                    case Actor::ID::CoinSpawn:
                                        msg->id = Actor::ID::Coin;
                                        break;
                                    case Actor::ID::TreasureSpawn:
                                        msg->id = Actor::ID::Treasure;
                                        break;
                                    case Actor::ID::AmmoSpawn:
                                        msg->id = Actor::ID::Ammo;
                                        break;
                                    case Actor::ID::MineSpawn:
                                        msg->id = Actor::ID::Explosion;
                                        break;
                                    }

                                    //raise message to say there's a new hole
                                    auto* msg2 = postMessage<MapEvent>(MessageID::MapMessage);
                                    msg2->position = position;
                                    msg2->type = MapEvent::HoleAdded;
                                }

                                e.getComponent<AnimationModifier>().nextAnimation = e.getComponent<AnimationModifier>().currentAnimation + 1;
                            }

                            if (wetPatch.state != WetPatch::State::OneHundred)
                            {
                                //create an actor for dig effects
                                auto* msg = postMessage<ActorEvent>(MessageID::ActorMessage);
                                msg->position = position;
                                msg->type = ActorEvent::RequestSpawn;
                                msg->id = Actor::ID::DirtSpray;
                            }
                        }
                    }
                }
            }
        }
    }
}

void DigDirector::handleEvent(const sf::Event&){}
void DigDirector::process(float) {}