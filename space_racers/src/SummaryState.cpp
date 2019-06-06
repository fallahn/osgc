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

#include <SFML/Graphics/Font.hpp>
#include <SFML/Window/Event.hpp>

namespace
{
    //small delay before input works
    const sf::Time delayTime = sf::seconds(0.5f);
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
    if (evt.type == sf::Event::KeyPressed
        && m_delayClock.getElapsedTime() > delayTime)
    {
        requestStackClear();
        requestStackPush(StateID::Lobby);
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

    //if we stash this in shared resources it should remain loaded after the first time
    //this state is show and (I think!!) the resource manager is smart enough to know this
    m_textureIDs[TextureID::Menu::Podium] = m_sharedData.resources.load<sf::Texture>("assets/images/podium.png");

    auto view = getContext().defaultView;
    m_scene.getActiveCamera().getComponent<xy::Camera>().setView(view.getSize());
    m_scene.getActiveCamera().getComponent<xy::Camera>().setViewport(view.getViewport());
}

void SummaryState::buildMenu()
{
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

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.addComponent<xy::Drawable>().setDepth(-5);
    entity.addComponent<xy::Sprite>(m_sharedData.resources.get<sf::Texture>(m_textureIDs[TextureID::Menu::Podium]));
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);

    auto& font = m_sharedData.resources.get<sf::Font>(m_sharedData.fontID);

    //make a copy of thte player info and erase any found in race positions...
    //those who are left didn't get a podium finish.
    auto playerInfo = m_sharedData.playerInfo;
    float verticalOffset = -280.f;
    if (m_sharedData.racePositions[0] != 0)
    {
        auto peerID = m_sharedData.racePositions[0];

        entity = m_scene.createEntity();
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

        entity = m_scene.createEntity();
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

        entity = m_scene.createEntity();
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
        entity = m_scene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
        entity.getComponent<xy::Transform>().move(0.f, verticalOffset);
        entity.addComponent<xy::Drawable>();
        entity.addComponent<xy::Text>(font).setString(p.second.name + " DNF");
        entity.getComponent<xy::Text>().setCharacterSize(96);
        entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
        verticalOffset += 160.f;
    }

    m_sharedData.racePositions = {};

    //quit message
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, 380.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Text>(font).setString("Press Any Key To Continue");
    entity.getComponent<xy::Text>().setCharacterSize(96);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
}