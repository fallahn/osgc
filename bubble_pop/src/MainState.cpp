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


#include "MainState.hpp"
#include "StateIDs.hpp"
#include "IniParse.hpp"
#include "GameConsts.hpp"
#include "CommandID.hpp"
#include "GameDirector.hpp"
#include "BubbleSystem.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/BitmapText.hpp>

#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/SpriteAnimator.hpp>
#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/ecs/systems/BitmapTextSystem.hpp>

#include <xyginext/graphics/BitmapFont.hpp>

#include <xyginext/core/Log.hpp>
#include <xyginext/gui/Gui.hpp>
#include <xyginext/graphics/SpriteSheet.hpp>
#include <xyginext/core/FileSystem.hpp>

namespace
{
    const std::size_t MaxLevels = 100;
}

MainState::MainState(xy::StateStack& ss, xy::State::Context ctx)
    : xy::State     (ss, ctx),
    m_scene         (ctx.appInstance.getMessageBus()),
    m_currentLevel  (0)
{
    launchLoadingScreen();
    initScene();
    loadResources();
    loadLevelData();
    buildArena();

    m_scene.getActiveCamera().getComponent<xy::Camera>().setView(ctx.defaultView.getSize());
    m_scene.getActiveCamera().getComponent<xy::Camera>().setViewport(ctx.defaultView.getViewport());

    quitLoadingScreen();
}

//public
bool MainState::handleEvent(const sf::Event& evt)
{
	//prevents events being forwarded if the console wishes to consume them
	if (xy::Nim::wantsKeyboard() || xy::Nim::wantsMouse())
    {
        return true;
    }

    m_playerInput.handleEvent(evt);
    m_scene.forwardEvent(evt);

    return true;
}

void MainState::handleMessage(const xy::Message& msg)
{
    m_scene.forwardMessage(msg);
}

bool MainState::update(float dt)
{
    m_playerInput.update(dt);
    m_scene.update(dt);
    return true;
}

void MainState::draw()
{
    auto& rw = getContext().renderWindow;

    rw.draw(m_scene);
}

xy::StateID MainState::stateID() const
{
    return StateID::Main;
}

//private
void MainState::initScene()
{
    auto& mb = getContext().appInstance.getMessageBus();

    m_scene.addSystem<BubbleSystem>(mb, m_nodeSet);
    m_scene.addSystem<xy::CommandSystem>(mb);
    m_scene.addSystem<xy::SpriteSystem>(mb);
    m_scene.addSystem<xy::SpriteAnimator>(mb);
    m_scene.addSystem<xy::BitmapTextSystem>(mb);
    m_scene.addSystem<xy::RenderSystem>(mb);

    m_scene.addDirector<GameDirector>(m_nodeSet, m_bubbleSprites);
}

void MainState::loadResources()
{
    xy::SpriteSheet spriteSheet;

    spriteSheet.loadFromFile("assets/sprites/red.spt", m_resources);
    m_bubbleSprites[BubbleID::Red] = spriteSheet.getSprite("ball");

    spriteSheet.loadFromFile("assets/sprites/green.spt", m_resources);
    m_bubbleSprites[BubbleID::Green] = spriteSheet.getSprite("ball");

    spriteSheet.loadFromFile("assets/sprites/blue.spt", m_resources);
    m_bubbleSprites[BubbleID::Blue] = spriteSheet.getSprite("ball");

    spriteSheet.loadFromFile("assets/sprites/cyan.spt", m_resources);
    m_bubbleSprites[BubbleID::Cyan] = spriteSheet.getSprite("ball");

    spriteSheet.loadFromFile("assets/sprites/magenta.spt", m_resources);
    m_bubbleSprites[BubbleID::Magenta] = spriteSheet.getSprite("ball");

    spriteSheet.loadFromFile("assets/sprites/yellow.spt", m_resources);
    m_bubbleSprites[BubbleID::Yellow] = spriteSheet.getSprite("ball");

    spriteSheet.loadFromFile("assets/sprites/orange.spt", m_resources);
    m_bubbleSprites[BubbleID::Orange] = spriteSheet.getSprite("ball");

    spriteSheet.loadFromFile("assets/sprites/grey.spt", m_resources);
    m_bubbleSprites[BubbleID::Grey] = spriteSheet.getSprite("ball");

    spriteSheet.loadFromFile("assets/sprites/misc.spt", m_resources);
    m_sprites[SpriteID::GameOver] = spriteSheet.getSprite("game_over");
    m_sprites[SpriteID::Pause] = spriteSheet.getSprite("paused");
    m_sprites[SpriteID::Title] = spriteSheet.getSprite("title");
    m_sprites[SpriteID::TopBar] = spriteSheet.getSprite("top_bar");

    m_textures[TextureID::Background] = m_resources.load<sf::Texture>("assets/images/background.png");
    m_textures[TextureID::RayGun] = m_resources.load<sf::Texture>("assets/images/shooter.png");
    m_textures[TextureID::GunMount] = m_resources.load<sf::Texture>("assets/images/shooter_circle.png");
}

void MainState::loadLevelData()
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

void MainState::buildArena()
{
    sf::Vector2f playArea(m_resources.get<sf::Texture>(m_textures[TextureID::Background]).getSize());
    
    m_nodeSet.rootNode = m_scene.createEntity();
    m_nodeSet.rootNode.addComponent<xy::Transform>().setScale(2.f, 2.f);
    m_nodeSet.rootNode.getComponent<xy::Transform>().setPosition((xy::DefaultSceneSize.x - playArea.x) / 4.f, 60.f);

    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(-10);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textures[TextureID::Background]));
    m_nodeSet.rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Const::GunPosition);
    entity.addComponent<xy::Drawable>().setDepth(1);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textures[TextureID::GunMount]));
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    m_nodeSet.rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    m_nodeSet.gunNode = entity;

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(m_nodeSet.gunNode.getComponent<xy::Transform>().getOrigin());
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textures[TextureID::RayGun]));
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(11.f, bounds.height / 2.f);
    m_nodeSet.gunNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    m_playerInput.setPlayerEntity(entity);

    m_nodeSet.barNode = m_scene.createEntity();
    m_nodeSet.barNode.addComponent<xy::Transform>().setPosition(Const::BarPosition);
    m_nodeSet.barNode.addComponent<xy::CommandTarget>().ID = CommandID::TopBar;
    m_nodeSet.rootNode.getComponent<xy::Transform>().addChild(m_nodeSet.barNode.getComponent<xy::Transform>());

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(0.f, -m_sprites[SpriteID::TopBar].getTextureRect().height);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::TopBar];
    m_nodeSet.barNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());   

    activateLevel();
}

void MainState::activateLevel()
{
    if (m_levels.empty())
    {
        xy::Logger::log("No levels loaded!", xy::Logger::Type::Error);
        return;
    }

    auto origin = Const::BubbleSize / 2.f;

    auto createBubble = [&,origin](int idx, sf::Vector2f pos)->xy::Entity
    {
        auto entity = m_scene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(pos + origin);
        entity.getComponent<xy::Transform>().setOrigin(origin);
        entity.addComponent<xy::Drawable>().setDepth(1);
        entity.addComponent<xy::Sprite>() = m_bubbleSprites[idx];
        entity.addComponent<Bubble>().state = Bubble::State::Suspended;
        entity.addComponent<xy::CommandTarget>().ID = CommandID::Bubble;

        //TODO tag bubbles with level ID so next time we know which bubbles to clear
        return entity;
    };
    m_scene.getSystem<BubbleSystem>().resetGrid();

    const auto& bubbles = m_levels[m_currentLevel].ballArray;
    for (auto i = 0; i < bubbles.size(); ++i)
    {
        if (bubbles[i] != -1)
        {
            auto pos = tileToWorldCoord(i);
                        
            auto entity = createBubble(bubbles[i], pos);
            entity.getComponent<Bubble>().gridIndex = i;
            m_nodeSet.barNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());
        }
    }

    auto queue = m_levels[m_currentLevel].orderArray;
    //mount first bubble
    auto entity = createBubble(queue.back(), m_nodeSet.gunNode.getComponent<xy::Transform>().getOrigin() - origin);
    entity.getComponent<Bubble>().state = Bubble::State::Mounted;
    m_nodeSet.gunNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());
    queue.pop_back();

    //set director queue. This also queues first bubble
    m_scene.getDirector<GameDirector>().setQueue(queue);

    m_currentLevel = (m_currentLevel + 1) % m_levels.size();
}