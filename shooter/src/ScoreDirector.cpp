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

#include "ScoreDirector.hpp"
#include "MessageIDs.hpp"
#include "CommandIDs.hpp"
#include "CollisionTypes.hpp"
#include "GameConsts.hpp"
#include "Alien.hpp"
#include "Human.hpp"

#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/BroadPhaseComponent.hpp>
#include <xyginext/ecs/components/Callback.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>
#include <xyginext/ecs/Scene.hpp>

#include <SFML/Graphics/Font.hpp>

namespace
{
    const std::int32_t BuildingScore = -50;
    const std::int32_t HumanScore = -50;
    const std::int32_t HumanScoreByAlien = -150;
    const std::int32_t ScorpionScore = 100;
    const std::int32_t BeetleScore = 120;
    const std::int32_t AmmoScore = 20;
    const std::int32_t BatteryScore = 40;
    const std::int32_t DeathScore = -50;
    const std::int32_t DestroyCollectibleScore = -10;
}

ScoreDirector::ScoreDirector(sf::Font& font)
    : m_font        (font),
    m_score         (0),
    m_alienCount    (Alien::NumberPerRound),
    m_humanCount    (Human::NumberPerRound)
{

}

//public
void ScoreDirector::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::DroneMessage)
    {
        const auto& data = msg.getData<DroneEvent>();
        switch (data.type)
        {
        default: break;
        case DroneEvent::Died:
            spawnScoreItem(data.position, DeathScore);
            break;
        case DroneEvent::GotAmmo:
            spawnScoreItem(data.position, AmmoScore);
            break;
        case DroneEvent::GotBattery:
            spawnScoreItem(data.position, BatteryScore);
            break;
        }
    }
    else if (msg.id == MessageID::BombMessage)
    {
        const auto& data = msg.getData<BombEvent>();
        switch (data.type)
        {
        case BombEvent::Exploded:
            break;
        case BombEvent::DamagedBuilding:
            spawnScoreItem(data.position, BuildingScore);
            break;
        case BombEvent::DestroyedCollectible:
            spawnScoreItem(data.position, DestroyCollectibleScore);
            break;
        case BombEvent::KilledBeetle:
            spawnScoreItem(data.position, BeetleScore);
            updateAlienCount();
            break;
        case BombEvent::KilledScorpion:
            spawnScoreItem(data.position, ScorpionScore);
            updateAlienCount();
            break;
        case BombEvent::KilledHuman:
            spawnScoreItem(data.position, HumanScore);
            updateHumanCount();
            break;
        default: break;
        }
    }
    else if (msg.id == MessageID::HumanMessage)
    {
        const auto& data = msg.getData<HumanEvent>();
        if (data.type == HumanEvent::Died)
        {
            spawnScoreItem(data.position, HumanScoreByAlien);
            updateHumanCount();
        }
    }
}

//private
void ScoreDirector::spawnScoreItem(sf::Vector2f position, std::int32_t score)
{
    m_score = std::max(0, m_score + score);

    //update score view
    xy::Command cmd;
    cmd.targetFlags = CommandID::ScoreText;
    cmd.action = [&](xy::Entity entity, float)
    {
        entity.getComponent<xy::Text>().setString("Score: " + std::to_string(m_score));
    };
    sendCommand(cmd);

    //spawn on screen doohickey
    sf::Color textColour = score > 0 ? sf::Color::Green : sf::Color::Red;

    auto entity = getScene().createEntity();
    entity.addComponent<xy::Transform>().setPosition(position);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Text>(m_font).setString(std::to_string(score));
    entity.getComponent<xy::Text>().setFillColour(textColour);
    entity.getComponent<xy::Text>().setOutlineThickness(2.f);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::Callback>().active = true;
    entity.getComponent<xy::Callback>().userData = std::make_any<float>(2.f);
    entity.getComponent<xy::Callback>().function =
        [&](xy::Entity e, float dt)
    {
        static const float MaxTime = 2.f;

        float& currentTime = std::any_cast<float&>(e.getComponent<xy::Callback>().userData);
        currentTime = std::max(0.f, currentTime - dt);

        sf::Uint8 alpha = static_cast<sf::Uint8>(255.f * (currentTime / MaxTime));
        auto colour = e.getComponent<xy::Text>().getFillColour();
        colour.a = alpha;
        e.getComponent<xy::Text>().setFillColour(colour);
        e.getComponent<xy::Text>().setOutlineColour({ 0,0,0,alpha });

        e.getComponent<xy::Transform>().move(0.f, -160.f * dt);

        if (alpha == 0)
        {
            getScene().destroyEntity(e);
        }
    };
}

void ScoreDirector::updateAlienCount()
{
    m_alienCount--;

    //send command to UI
    xy::Command cmd;
    cmd.targetFlags = CommandID::AlienCount;
    cmd.action = [&](xy::Entity e, float)
    {
        e.getComponent<xy::Text>().setString(std::to_string(m_alienCount));
    };
    sendCommand(cmd);

    if (m_alienCount == 0)
    {
        //end the game
        auto* msg = postMessage<GameEvent>(MessageID::GameMessage);
        msg->type = GameEvent::StateChange;
        msg->reason = GameEvent::NoAliensLeft;
    }
}

void ScoreDirector::updateHumanCount()
{
    m_humanCount--;

    //send command to UI
    xy::Command cmd;
    cmd.targetFlags = CommandID::HumanCount;
    cmd.action = [&](xy::Entity e, float)
    {
        e.getComponent<xy::Text>().setString(std::to_string(m_humanCount));
    };
    sendCommand(cmd);

    if (m_humanCount == 0)
    {
        //end the game
        auto* msg = postMessage<GameEvent>(MessageID::GameMessage);
        msg->type = GameEvent::StateChange;
        msg->reason = GameEvent::NoHumansLeft;
    }
}