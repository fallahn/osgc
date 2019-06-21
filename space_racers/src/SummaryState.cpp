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

#include "SummaryState.hpp"
#include "VehicleSystem.hpp"
#include "GameConsts.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Camera.hpp>

#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/SpriteAnimator.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>

#include <xyginext/gui/Gui.hpp>
#include <xyginext/graphics/SpriteSheet.hpp>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Window/Event.hpp>

namespace
{
    //small delay before input works
    const sf::Time delayTime = sf::seconds(0.5f);

    const std::string NeonFragment = 
        R"(
        #version 120
        
        uniform sampler2D u_diffuseMap;
        uniform sampler2D u_neonMap;

        float blend(float bg, float fg)
        {
            //overlay mode
            return bg < 0.5 ? (2.0 * bg * fg) : (1.0 - 2.0 * (1.0 - bg) * (1.0 - fg));
        }

        void main()
        {
            vec4 diffuse = texture2D(u_diffuseMap, gl_TexCoord[0].xy);
            vec4 neon = texture2D(u_neonMap, gl_TexCoord[0].xy);

            neon.r = blend(neon.r, gl_Color.r);
            neon.g = blend(neon.g, gl_Color.g);
            neon.b = blend(neon.b, gl_Color.b);

            diffuse.rgb += neon.rgb;
            gl_FragColor = diffuse;
        })";
}


SummaryState::SummaryState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State (ss, ctx),
    m_sharedData(sd),
    m_scene     (ctx.appInstance.getMessageBus())
{
    initScene();
    buildMenu();
}

//public
bool SummaryState::handleEvent(const sf::Event& evt)
{
    if (xy::Nim::wantsKeyboard() || xy::Nim::wantsMouse())
    {
        return true;
    }

    if (evt.type == sf::Event::KeyPressed
        && m_delayClock.getElapsedTime() > delayTime)
    {
        requestStackClear();

        if (m_sharedData.netClient)
        {
            requestStackPush(StateID::Lobby);
        }
        else
        {
            requestStackPush(StateID::MainMenu);
        }
    }

    m_scene.forwardEvent(evt);
    return false;
}

void SummaryState::handleMessage(const xy::Message& msg)
{
    m_scene.forwardMessage(msg);
}

bool SummaryState::update(float dt)
{
    m_scene.update(dt);
    return true;
}

void SummaryState::draw()
{
    auto& rw = getContext().renderWindow;
    rw.draw(m_scene);
}

//private
void SummaryState::initScene()
{
    auto& mb = getContext().appInstance.getMessageBus();

    m_scene.addSystem<xy::TextSystem>(mb);
    m_scene.addSystem<xy::SpriteSystem>(mb);
    m_scene.addSystem<xy::SpriteAnimator>(mb);
    m_scene.addSystem<xy::RenderSystem>(mb);

    auto view = getContext().defaultView;
    m_scene.getActiveCamera().getComponent<xy::Camera>().setView(view.getSize());
    m_scene.getActiveCamera().getComponent<xy::Camera>().setViewport(view.getViewport());

    //TODO ideally pre-load this somewhere to reduce loading times
    m_textureIDs[Textures::Podium] = m_resources.load<sf::Texture>("assets/images/podium.png");
    m_textureIDs[Textures::CarNeon] = m_resources.load<sf::Texture>("assets/images/menu_car_neon.png");
    m_textureIDs[Textures::BikeNeon] = m_resources.load<sf::Texture>("assets/images/menu_bike_neon.png");
    m_textureIDs[Textures::ShipNeon] = m_resources.load<sf::Texture>("assets/images/menu_ship_neon.png");

    xy::SpriteSheet spriteSheet;
    spriteSheet.loadFromFile("assets/sprites/menu_bike.spt", m_resources);
    m_sprites[Sprites::Bike] = spriteSheet.getSprite("bike");

    spriteSheet.loadFromFile("assets/sprites/menu_car.spt", m_resources);
    m_sprites[Sprites::Car] = spriteSheet.getSprite("car");

    spriteSheet.loadFromFile("assets/sprites/menu_ship.spt", m_resources);
    m_sprites[Sprites::Ship] = spriteSheet.getSprite("ship");

    m_neonShader.loadFromMemory(NeonFragment, sf::Shader::Fragment);
}

void SummaryState::buildMenu()
{
    //background
    sf::Color c(0, 0, 0, 220);

    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(-10);
    auto& verts = entity.getComponent<xy::Drawable>().getVertices();

    verts.emplace_back(sf::Vector2f(), c);
    verts.emplace_back(sf::Vector2f(xy::DefaultSceneSize.x, 0.f), c);
    verts.emplace_back(xy::DefaultSceneSize, c);
    verts.emplace_back(sf::Vector2f(0.f, xy::DefaultSceneSize.y), c);

    entity.getComponent<xy::Drawable>().updateLocalBounds();

    //podium
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.addComponent<xy::Drawable>().setDepth(-5);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[Textures::Podium]));
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);

    auto& font = m_sharedData.resources.get<sf::Font>(m_sharedData.fontID);

    if (m_sharedData.netClient)
    {
        buildNetworkPlayers();
    }
    else
    {
        buildLocalPlayers();
    }

    //quit message
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, 380.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Text>(font).setString("Press Any Key To Continue");
    entity.getComponent<xy::Text>().setCharacterSize(96);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
}

//private
void SummaryState::buildLocalPlayers()
{
    std::array<sf::Vector2f, 4u> positions =
    {
        sf::Vector2f(xy::DefaultSceneSize.x / 2.f, 620.f),
        sf::Vector2f(1250.f, 710.f),
        sf::Vector2f(670.f, 780.f),
        sf::Vector2f(-1000.f, -1000.f),
    };

    for (auto i = 0u; i < m_sharedData.localPlayers.size(); ++i)
    {
        auto entity = m_scene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(positions[m_sharedData.localPlayers[i].position]);
        entity.addComponent<xy::Drawable>().setShader(&m_neonShader);
        entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_diffuseMap");
        entity.addComponent<xy::Sprite>();
        entity.addComponent<xy::SpriteAnimation>().play(0);

        switch (m_sharedData.localPlayers[i].vehicle)
        {
        default:break;
        case Vehicle::Car:
            entity.getComponent<xy::Sprite>() = m_sprites[Sprites::Car];
            entity.getComponent<xy::Drawable>().bindUniform("u_neonMap", m_resources.get<sf::Texture>(m_textureIDs[Textures::CarNeon]));
            break;
        case Vehicle::Bike:
            entity.getComponent<xy::Sprite>() = m_sprites[Sprites::Bike];
            entity.getComponent<xy::Drawable>().bindUniform("u_neonMap", m_resources.get<sf::Texture>(m_textureIDs[Textures::BikeNeon]));
            break;
        case Vehicle::Ship:
            entity.getComponent<xy::Sprite>() = m_sprites[Sprites::Ship];
            entity.getComponent<xy::Drawable>().bindUniform("u_neonMap", m_resources.get<sf::Texture>(m_textureIDs[Textures::ShipNeon]));
            break;
        }
        auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
        entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height); 
        entity.getComponent<xy::Sprite>().setColour(GameConst::PlayerColour::Light[i]);
    }
    m_sharedData.localPlayers = {};
}

void SummaryState::buildNetworkPlayers()
{
    auto& font = m_sharedData.resources.get<sf::Font>(m_sharedData.fontID);

    //make a copy of thte player info and erase any found in race positions...
    //those who are left didn't get a podium finish.
    auto playerInfo = m_sharedData.playerInfo;
    float verticalOffset = -280.f;
    if (m_sharedData.racePositions[0] != 0)
    {
        auto peerID = m_sharedData.racePositions[0];

        auto entity = m_scene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
        entity.getComponent<xy::Transform>().move(0.f, verticalOffset);
        entity.addComponent<xy::Drawable>();
        entity.addComponent<xy::Text>(font).setString(playerInfo[peerID].name + " 50 points");
        entity.getComponent<xy::Text>().setCharacterSize(96);
        entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
        m_sharedData.playerInfo[peerID].score += 50;
        verticalOffset += 160.f;
        playerInfo.erase(peerID);
    }
    if (m_sharedData.racePositions[1] != 0)
    {
        auto peerID = m_sharedData.racePositions[1];

        auto entity = m_scene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
        entity.getComponent<xy::Transform>().move(0.f, verticalOffset);
        entity.addComponent<xy::Drawable>();
        entity.addComponent<xy::Text>(font).setString(playerInfo[peerID].name + " 30 points");
        entity.getComponent<xy::Text>().setCharacterSize(96);
        entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
        m_sharedData.playerInfo[peerID].score += 30;
        verticalOffset += 160.f;
        playerInfo.erase(peerID);
    }
    if (m_sharedData.racePositions[2] != 0)
    {
        auto peerID = m_sharedData.racePositions[2];

        auto entity = m_scene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
        entity.getComponent<xy::Transform>().move(0.f, verticalOffset);
        entity.addComponent<xy::Drawable>();
        entity.addComponent<xy::Text>(font).setString(playerInfo[peerID].name + " 10 points");
        entity.getComponent<xy::Text>().setCharacterSize(96);
        entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
        m_sharedData.playerInfo[peerID].score += 10;
        verticalOffset += 160.f;
        playerInfo.erase(peerID);
    }


    for (const auto& p : playerInfo)
    {
        auto entity = m_scene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
        entity.getComponent<xy::Transform>().move(0.f, verticalOffset);
        entity.addComponent<xy::Drawable>();
        entity.addComponent<xy::Text>(font).setString(p.second.name + " DNF");
        entity.getComponent<xy::Text>().setCharacterSize(96);
        entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
        verticalOffset += 160.f;
    }

    m_sharedData.racePositions = {};
}