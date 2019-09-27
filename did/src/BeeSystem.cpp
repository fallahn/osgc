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

#include "BeeSystem.hpp"
#include "AnimationSystem.hpp"
#include "QuadTreeFilter.hpp"
#include "Actor.hpp"
#include "CollisionBounds.hpp"
#include "MessageIDs.hpp"
#include "DecoySystem.hpp"
#include "PlayerSystem.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>
#include <xyginext/ecs/Scene.hpp>
#include <xyginext/util/Vector.hpp>

/*
Bees chase players until they reach a certain radius
from their home position or the target jumps into water
at which point they switch to a homing state.
Bees have to home for at least 5 seconds before they follow
again. They will also attack decoys (which take precedent over
players) damaging the decoy.

Bees sleep at night and hide when raining
*/

namespace
{
    const sf::FloatRect QueryArea(-64.f, 0.f, 128.f, 128.f);
    const float MaxBeeRadius = (480.f * 480.f); //max radius from hive
    const float MoveSpeed = 72.f; //player speed is 80
    const float StingRadius = 16.f * 16.f;
    const float DecoyDamage = 5.f;
}

BeeSystem::BeeSystem(xy::MessageBus& mb)
    : xy::System    (mb, typeid(BeeSystem)),
    m_dayPosition   (0.f)
{
    requireComponent<Bee>();
    requireComponent<xy::Transform>();
    requireComponent<AnimationModifier>();
}

void BeeSystem::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::SceneMessage)
    {
        const auto& data = msg.getData<SceneEvent>();
        if (data.type == SceneEvent::WeatherChanged)
        {
            m_weather = static_cast<StormDirector::State>(data.id);
        }
    }
    else if (msg.id == MessageID::MapMessage)
    {
        const auto& data = msg.getData<MapEvent>();
        if (data.type == MapEvent::DayNightUpdate)
        {
            //track cycle so only spawn at night
            m_dayPosition = data.value;
        }
    }
}

void BeeSystem::process(float dt)
{
    bool dayTime = isDayTime();

    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& bee = entity.getComponent<Bee>();

        if (bee.state == Bee::State::Awake)
        {
            if (bee.target.isValid() && !bee.target.destroyed())
            {
                //we must be chasing a player or decoy so
                //move to target
                move(entity, bee.target.getComponent<xy::Transform>().getPosition(), dt);

                //check distance from home and quit chasing if too far
                //set scan time negative so longer gap before starting to scan again
                auto dir = bee.homePosition - entity.getComponent<xy::Transform>().getPosition();
                auto len2 = xy::Util::Vector::lengthSquared(dir);
                if (len2 > MaxBeeRadius)
                {
                    bee.target = {};
                    bee.nextScanTime = -5.f;
                    continue;
                }

                //check target not in water (else quit) and in stinging range
                bee.nextStingTime += dt;
                if (bee.target.getComponent<Actor>().id != Actor::ID::Decoy)
                {
                    if (bee.target.getComponent<CollisionComponent>().water > 0.f
                        || bee.target.getComponent<Player>().sync.state == Player::State::Dead)
                    {
                        bee.target = {};
                        bee.nextScanTime = -5.f;
                        continue;
                    }

                    //if close enough hurt the target
                    auto targetPos = bee.target.getComponent<xy::Transform>().getPosition();
                    auto beePos = entity.getComponent<xy::Transform>().getPosition();
                    if (xy::Util::Vector::lengthSquared(targetPos - beePos) < StingRadius && bee.nextStingTime > Bee::StingTime)
                    {
                        bee.nextStingTime = 0.f;
                        auto* msg = postMessage<PlayerEvent>(MessageID::PlayerMessage);
                        msg->action = PlayerEvent::BeeHit;
                        msg->entity = bee.target;
                        msg->position = entity.getComponent<xy::Transform>().getPosition();

                        //if we reset here then there's a chance we scan for new decoys etc
                        bee.target = {};
                        bee.nextScanTime = 4.f;
                    }
                }
                else
                {
                    //damage decoy
                    auto targetPos = bee.target.getComponent<xy::Transform>().getPosition();
                    auto beePos = entity.getComponent<xy::Transform>().getPosition();
                    if (xy::Util::Vector::lengthSquared(targetPos - beePos) < StingRadius && bee.nextStingTime > Bee::StingTime)
                    {
                        bee.nextStingTime = 0.f;
                        bee.target.getComponent<Decoy>().health -= DecoyDamage;
                        //TODO this should raise some sort of message?
                    }
                }
            }
            else
            {
                auto& tx = entity.getComponent<xy::Transform>();
                auto position = tx.getPosition();

                bee.nextScanTime += dt;
                if (bee.nextScanTime > Bee::ScanTime)
                {
                    bee.nextScanTime = 0.f;

                    //scan area for trespassers / potential targets
                    //remember decoys come first
                    auto bounds = QueryArea;
                    bounds.left += position.x;
                    bounds.top += position.y;
                    auto result = getScene()->getSystem<xy::DynamicTreeSystem>().query(bounds, QuadTreeFilter::Player | QuadTreeFilter::Decoy);

                    //sort decoys to top
                    if (!result.empty())
                    {
                        std::sort(result.begin(), result.end(), 
                            [](const xy::Entity a, const xy::Entity b)
                        {
                            return a.getComponent<Actor>().id > b.getComponent<Actor>().id; 
                        });

                        //ignore dead players
                        result.erase(std::remove_if(result.begin(), result.end(), 
                            [](const xy::Entity e) 
                            {
                                return (e.getComponent<Actor>().id != Actor::ID::Decoy
                                    && e.getComponent<Player>().sync.state == Player::Dead);
                            }), result.end());

                        bee.target = result.empty() ? xy::Entity() : result[0];
                        continue;
                    }
                }

                //move towards home
                move(entity, bee.homePosition, dt);
            }

            if (!dayTime)
            {
                //time to sleep
                bee.state = Bee::State::Sleeping;
                entity.getComponent<AnimationModifier>().nextAnimation = AnimationID::WalkDown;
            }
            else
            {
                //check the weather if day time
                if (m_weather != StormDirector::Dry)
                {
                    bee.state = Bee::State::Sleeping;
                    entity.getComponent<AnimationModifier>().nextAnimation = AnimationID::WalkDown;
                }
            }
        }
        else if (bee.state == Bee::State::Waking)
        {
            //this state just allows some time for the wake animation to play
            bee.wakeTime += dt;
            if (bee.wakeTime > Bee::WakeDuration)
            {
                bee.state = Bee::State::Awake;
                entity.getComponent<AnimationModifier>().nextAnimation = AnimationID::IdleDown;
            }
        }
        else if(bee.state == Bee::State::Sleeping)
        {
            //move towards home
            move(entity, bee.homePosition, dt);

            //check if turned day time
            if (dayTime && m_weather == StormDirector::Dry)
            {
                //wake up
                bee.state = Bee::State::Waking;
                bee.wakeTime = 0.f;
                entity.getComponent<AnimationModifier>().nextAnimation = AnimationID::WalkUp;
            }
        }
    }
}

void BeeSystem::onEntityAdded(xy::Entity entity)
{
    entity.getComponent<Bee>().homePosition = entity.getComponent<xy::Transform>().getPosition();
}

//private
void BeeSystem::move(xy::Entity entity, sf::Vector2f target, float dt)
{
    auto& tx = entity.getComponent<xy::Transform>();
    auto position = tx.getPosition();

    auto dir = target - position;
    
    if (xy::Util::Vector::lengthSquared(dir) != 0)
    {
        tx.move(xy::Util::Vector::normalise(dir) * MoveSpeed * dt);
    }

    //update animation
    if (entity.getComponent<Bee>().state == Bee::State::Awake)
    {
        if (xy::Util::Vector::lengthSquared(dir) > 16)
        {
            if (dir.x < 0)
            {
                entity.getComponent<AnimationModifier>().nextAnimation = AnimationID::WalkLeft;
            }
            else if (dir.x > 0)
            {
                entity.getComponent<AnimationModifier>().nextAnimation = AnimationID::WalkRight;
            }
        }
        else
        {
            entity.getComponent<AnimationModifier>().nextAnimation = AnimationID::IdleDown;
        }
    }
}

bool BeeSystem::isDayTime() const
{
    return (m_dayPosition > 0.25f && m_dayPosition < 0.5f) || (m_dayPosition > 0.75f && m_dayPosition < 1.f);
}