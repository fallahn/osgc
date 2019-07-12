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

#include "UIDirector.hpp"
#include "MessageIDs.hpp"
#include "CommandIDs.hpp"
#include "SharedStateData.hpp"
#include "EnemySystem.hpp"

#include <xyginext/ecs/Scene.hpp>
#include <xyginext/ecs/components/Text.hpp>

#include <sstream>

UIDirector::UIDirector(SharedData& sd)
    : m_sharedData  (sd)
{
    sd.roundTime = 160;
}

//public
void UIDirector::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::PlayerMessage)
    {
        const auto& data = msg.getData<PlayerEvent>();
        switch (data.type)
        {
        default: break;
        case PlayerEvent::Died:
        {
            m_sharedData.inventory.lives--;

            updateLives();

            if (m_sharedData.inventory.lives == 0)
            {
                auto* msg = postMessage<GameEvent>(MessageID::GameMessage);
                msg->type = GameEvent::LivesExpired;
            }
        }
            break;
        case PlayerEvent::GotCoin:
        {
            m_sharedData.inventory.coins = std::min(Inventory::MaxCoins, m_sharedData.inventory.coins + 1);

            if ((m_sharedData.inventory.coins % 100) == 0)
            {
                auto* msg = postMessage<PlayerEvent>(MessageID::PlayerMessage);
                msg->type = PlayerEvent::GotLife;
            }

            xy::Command cmd;
            cmd.targetFlags = CommandID::UI::CoinText;
            cmd.action = [&](xy::Entity e, float)
            {
                std::stringstream ss;
                ss << "x " << std::setw(3) << std::setfill('0') << m_sharedData.inventory.coins;

                e.getComponent<xy::Text>().setString(ss.str());
            };
            sendCommand(cmd);
        }
            break;
        case PlayerEvent::GotAmmo:
        {
            m_sharedData.inventory.ammo = std::min(m_sharedData.inventory.ammo + 5, Inventory::MaxAmmo);

            updateAmmo();
        }
            break;

        case PlayerEvent::GotShield:
        {
            m_sharedData.inventory.score += 100;
            updateScore();
        }
            break;
        case PlayerEvent::GotLife:
        {
            m_sharedData.inventory.lives = std::min(Inventory::MaxLives, m_sharedData.inventory.lives + 1);
            updateLives();
        }
            break;
        case PlayerEvent::Shot:
            m_sharedData.inventory.ammo = std::max(0, m_sharedData.inventory.ammo - 1);
            updateAmmo();
            break;
        }
    }
    else if (msg.id == MessageID::EnemyMessage)
    {
        const auto& data = msg.getData<EnemyEvent>();
        switch (data.type)
        {
        default: break;
        case EnemyEvent::Died:
            if (data.entity.getComponent<Enemy>().type == Enemy::Bird)
            {
                m_sharedData.inventory.score += 200;
            }
            else if (data.entity.getComponent<Enemy>().type == Enemy::Egg)
            {
                m_sharedData.inventory.score += 0; //eggs die on their own.
            }
            else
            {
                m_sharedData.inventory.score += 150;
            }
            updateScore();
            break;
        }
    }
}

void UIDirector::process(float)
{
    updateTimer();
}

//private
void UIDirector::updateTimer()
{
    if (m_roundClock.getElapsedTime() > sf::seconds(1.f))
    {
        m_roundClock.restart();
        
        if (m_sharedData.roundTime > 0)
        {
            m_sharedData.roundTime--;

            xy::Command cmd;
            cmd.targetFlags = CommandID::UI::TimeText;
            cmd.action = [&](xy::Entity e, float)
            {
                e.getComponent<xy::Text>().setString(std::to_string(m_sharedData.roundTime));
            };
            sendCommand(cmd);

            if (m_sharedData.roundTime == 30)
            {
                auto* msg = postMessage<GameEvent>(MessageID::GameMessage);
                msg->type = GameEvent::TimerWarning;
            }
        }
        else
        {
            auto* msg = postMessage<GameEvent>(MessageID::GameMessage);
            msg->type = GameEvent::TimerExpired;
        }
    }
}

void UIDirector::updateScore()
{
    xy::Command cmd;
    cmd.targetFlags = CommandID::UI::ScoreText;
    cmd.action = [&](xy::Entity e, float)
    {
        e.getComponent<xy::Text>().setString("SCORE " + std::to_string(m_sharedData.inventory.score));
    };
    sendCommand(cmd);
}

void UIDirector::updateLives()
{
    xy::Command cmd;
    cmd.targetFlags = CommandID::UI::LivesText;
    cmd.action = [&](xy::Entity e, float)
    {
        e.getComponent<xy::Text>().setString(std::to_string(m_sharedData.inventory.lives));
    };
    sendCommand(cmd);
}

void UIDirector::updateAmmo()
{
    xy::Command cmd;
    cmd.targetFlags = CommandID::UI::AmmoText;
    cmd.action = [&](xy::Entity e, float)
    {
        std::stringstream ss;
        ss << "x " << std::setw(2) << std::setfill('0') << m_sharedData.inventory.ammo;

        e.getComponent<xy::Text>().setString(ss.str());
    };
    sendCommand(cmd);
}