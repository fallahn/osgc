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

#include "GameDirector.hpp"
#include "NodeSet.hpp"
#include "BubbleSystem.hpp"
#include "GameConsts.hpp"
#include "CommandID.hpp"
#include "MessageIDs.hpp"
#include "IniParse.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/Callback.hpp>

#include <xyginext/ecs/Scene.hpp>

#include <xyginext/util/Random.hpp>

GameDirector::GameDirector(NodeSet& ns, const std::array<xy::Sprite, BubbleID::Count>& sprites)
    : m_nodeSet         (ns),
    m_sprites           (sprites),
    m_currentLevel      (0),
    m_bubbleGeneration  (0)
{

}

//public
void GameDirector::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::BubbleMessage)
    {
        const auto& data = msg.getData<BubbleEvent>();
        switch (data.type)
        {
        default: break;
        case BubbleEvent::Fired:
        {
            xy::Command cmd;
            cmd.targetFlags = CommandID::Bubble;
            cmd.action = [&,data](xy::Entity e, float)
            {
                auto& bubble = e.getComponent<Bubble>();
                if (bubble.state == Bubble::State::Mounted)
                {
                    bubble.state = Bubble::State::Firing;
                    bubble.velocity = data.velocity;

                    auto& tx = e.getComponent<xy::Transform>();
                    m_nodeSet.gunNode.getComponent<xy::Transform>().removeChild(tx);
                    tx.setPosition(m_nodeSet.gunNode.getComponent<xy::Transform>().getPosition());
                    m_nodeSet.rootNode.getComponent<xy::Transform>().addChild(tx);

                    mountBubble(); //only mount a bubble if we fired one
                }
            };
            sendCommand(cmd);
        }
            break;
        }
    }
    else if (msg.id == MessageID::GameMessage)
    {
        const auto& data = msg.getData<GameEvent>();
        switch (data.type)
        {
        default: break;
        case GameEvent::RoundCleared:
        case GameEvent::RoundFailed:
            loadNextLevel(data.generation);
            break;
        }
    }
}

void GameDirector::process(float)
{
    //track round time and lower bar
    //TODO fix the wrap around of this index
    if (m_roundTimer.getElapsedTime() > m_levels[m_currentLevel - 1].interval)
    {
        m_roundTimer.restart();

        m_nodeSet.barNode.getComponent<xy::Transform>().move(0.f, Const::RowHeight);

        //TODO raise message
    }
}

void GameDirector::loadLevelData()
{
    static const std::string section("Bubble Puzzle 97 - Level");
    static const std::size_t BallArraySize = 64;

    auto path = xy::FileSystem::getResourcePath() + "assets/levels/";

    auto files = xy::FileSystem::listFiles(path);
    for (const auto& file : files)
    {
        if (xy::FileSystem::getFileExtension(file) == ".ini")
        {
            IniParse iniFile(path + file);
            if (iniFile.getData().empty())
            {
                continue;
            }

            LevelData levelData;
            levelData.name = iniFile.getValueString(section, "name");
            levelData.author = iniFile.getValueString(section, "author");
            levelData.comment = iniFile.getValueString(section, "comment");

            levelData.level = iniFile.getValueInt(section, "stage");
            levelData.interval = sf::seconds(iniFile.getValueFloat(section, "interval"));
            levelData.gunMove = iniFile.getValueInt(section, "gunmove") == 1;

            for (auto i = 8; i > 0; --i)
            {
                auto str = iniFile.getValueString(section, "line" + std::to_string(i));
                if (!str.empty())
                {
                    auto row = xy::Util::String::tokenize(str, ' ');

                    for (auto s : row)
                    {
                        if (s == "R")
                        {
                            levelData.ballArray.push_back(BubbleID::Red);
                        }
                        else if (s == "G")
                        {
                            levelData.ballArray.push_back(BubbleID::Green);
                        }
                        else if (s == "B")
                        {
                            levelData.ballArray.push_back(BubbleID::Blue);
                        }
                        else if (s == "C")
                        {
                            levelData.ballArray.push_back(BubbleID::Cyan);
                        }
                        else if (s == "P")
                        {
                            levelData.ballArray.push_back(BubbleID::Magenta);
                        }
                        else if (s == "Y")
                        {
                            levelData.ballArray.push_back(BubbleID::Yellow);
                        }
                        else if (s == "O")
                        {
                            levelData.ballArray.push_back(BubbleID::Orange);
                        }
                        else if (s == "E")
                        {
                            levelData.ballArray.push_back(BubbleID::Grey);
                        }
                        else if (s == "-")
                        {
                            levelData.ballArray.push_back(-1);
                        }
                    }
                }
            }

            auto order = iniFile.getValueString(section, "order");
            for (auto c : order)
            {
                if (c == 'R')
                {
                    levelData.orderArray.push_back(BubbleID::Red);
                }
                else if (c == 'G')
                {
                    levelData.orderArray.push_back(BubbleID::Green);
                }
                else if (c == 'B')
                {
                    levelData.orderArray.push_back(BubbleID::Blue);
                }
                else if (c == 'C')
                {
                    levelData.orderArray.push_back(BubbleID::Cyan);
                }
                else if (c == 'P')
                {
                    levelData.orderArray.push_back(BubbleID::Magenta);
                }
                else if (c == 'Y')
                {
                    levelData.orderArray.push_back(BubbleID::Yellow);
                }
                else if (c == 'O')
                {
                    levelData.orderArray.push_back(BubbleID::Orange);
                }
                else if (c == 'E')
                {
                    levelData.orderArray.push_back(BubbleID::Grey);
                }
            }
            //reverse this as we'll be popping from the back
            std::reverse(levelData.orderArray.begin(), levelData.orderArray.end());

            if (levelData.level != 0 &&
                levelData.interval != sf::Time::Zero &&
                levelData.ballArray.size() == BallArraySize)
            {
                m_levels.push_back(levelData);
            }
        }
    }

    std::sort(m_levels.begin(), m_levels.end(),
        [](const LevelData& a, const LevelData& b)
        {
            return a.level < b.level;
        });
}

void GameDirector::activateLevel()
{
    if (m_levels.empty())
    {
        xy::Logger::log("No levels loaded!", xy::Logger::Type::Error);
        return;
    }
    m_bubbleGeneration++;

    auto origin = Const::BubbleSize / 2.f;

    auto createBubble = [&, origin](int idx, sf::Vector2f pos)->xy::Entity
    {
        auto entity = getScene().createEntity();
        entity.addComponent<xy::Transform>().setPosition(pos + origin);
        entity.getComponent<xy::Transform>().setOrigin(origin);
        entity.addComponent<xy::Drawable>().setDepth(1);
        entity.addComponent<xy::Sprite>() = m_sprites[idx];
        entity.addComponent<Bubble>().state = Bubble::State::Suspended;
        entity.getComponent<Bubble>().colourType = idx;
        entity.addComponent<xy::CommandTarget>().ID = CommandID::Bubble;

        //tag bubbles so next time we know which bubbles to clear
        entity.addComponent<std::size_t>() = m_bubbleGeneration;
        return entity;
    };
    getScene().getSystem<BubbleSystem>().resetGrid();


    //just to pick a first colour at random
    std::int32_t colour = 0;
    const auto& bubbles = m_levels[m_currentLevel].ballArray;
    for (auto i = 0; i < bubbles.size(); ++i)
    {
        if (bubbles[i] != -1)
        {
            auto pos = tileToWorldCoord(i);

            auto entity = createBubble(bubbles[i], pos);
            entity.getComponent<Bubble>().gridIndex = i;
            m_nodeSet.barNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

            colour = bubbles[i];
        }
    }

    auto queue = m_levels[m_currentLevel].orderArray;
    
    //mount first bubble
    if (!queue.empty())
    {
        colour = queue.back();
        queue.pop_back();
    }

    auto entity = createBubble(colour, m_nodeSet.gunNode.getComponent<xy::Transform>().getOrigin() - origin);
    entity.getComponent<Bubble>().state = Bubble::State::Mounted;
    m_nodeSet.gunNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //check game rule if gun mount should be moving
    m_nodeSet.gunNode.getComponent<xy::Transform>().setPosition(Const::GunPosition);
    m_nodeSet.gunNode.getComponent<xy::Callback>().active = m_levels[m_currentLevel].gunMove;

    //set active queue
    m_queue = queue;
    queueBubble();

    m_currentLevel = (m_currentLevel + 1) % m_levels.size();
    m_roundTimer.restart();
}

//private
void GameDirector::queueBubble()
{
    const auto& activeColours = getScene().getSystem<BubbleSystem>().getActiveColours();

    auto id = 0;
    if (!m_queue.empty())
    {
        id = m_queue.back();
        m_queue.pop_back();
    }
    else if (activeColours.size() > 1)
    {
        id = activeColours[xy::Util::Random::value(0, activeColours.size() - 1)];
    }

    //place bubble at spawn point
    auto origin = Const::BubbleSize / 2.f;
    auto entity = getScene().createEntity();
    entity.addComponent<xy::Transform>().setPosition(Const::BubbleQueuePosition);
    entity.getComponent<xy::Transform>().setOrigin(origin);
    entity.addComponent<xy::Drawable>().setDepth(1);
    entity.addComponent<xy::Sprite>() = m_sprites[id];
    entity.addComponent<Bubble>().state = Bubble::State::Queued;
    entity.getComponent<Bubble>().colourType = id;
    entity.addComponent<xy::CommandTarget>().ID = CommandID::Bubble;
    entity.addComponent<std::size_t>() = m_bubbleGeneration;

    m_nodeSet.rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());
}

void GameDirector::mountBubble()
{
    xy::Command cmd;
    cmd.targetFlags = CommandID::Bubble;
    cmd.action = [&](xy::Entity e, float)
    {
        if (e.getComponent<Bubble>().state == Bubble::State::Queued)
        {
            e.getComponent<Bubble>().state = Bubble::State::Queuing;
            queueBubble();
        }
    };
    sendCommand(cmd);
}

void GameDirector::loadNextLevel(std::size_t generation)
{
    if (generation == m_bubbleGeneration)
    {
        auto level = m_bubbleGeneration;
        xy::Command cmd;
        cmd.targetFlags = CommandID::Bubble;
        cmd.action = [&, level](xy::Entity e, float)
        {
            if (e.getComponent<std::size_t>() == level)
            {
                getScene().destroyEntity(e);
            }
        };
        sendCommand(cmd);

        m_nodeSet.barNode.getComponent<xy::Transform>().setPosition(Const::BarPosition);
        activateLevel();
    }
}