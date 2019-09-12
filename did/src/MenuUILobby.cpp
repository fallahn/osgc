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

#include "MenuState.hpp"
#include "GlobalConsts.hpp"
#include "MessageIDs.hpp"
#include "NetworkClient.hpp"
#include "SharedStateData.hpp"
#include "Packet.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/UIHitBox.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Callback.hpp>

#include <xyginext/ecs/systems/UISystem.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>

void MenuState::buildLobby(sf::Font& font)
{
    auto& fineFont = m_fontResource.get(Global::FineFont);
    
    auto mouseOver = m_callbackIDs[Menu::CallbackID::TextSelected];
    auto mouseOut = m_callbackIDs[Menu::CallbackID::TextUnselected];

    auto parentEntity = m_uiScene.createEntity();
    parentEntity.addComponent<xy::Transform>().setPosition(Menu::OffscreenPosition);
    parentEntity.addComponent<xy::CommandTarget>().ID = Menu::CommandID::LobbyNode;

    //back button
    auto entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::BackButtonPosition);
    entity.addComponent<xy::Text>(font).setString("Leave");
    entity.getComponent<xy::Text>().setCharacterSize(Global::MediumTextSize);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Left);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseEnter] = mouseOver;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseExit] = mouseOut;
    entity.getComponent<xy::UIHitBox>().area = Menu::ButtonArea;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseUp] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback([&, parentEntity](xy::Entity, sf::Uint64 flags) mutable
    {
        if (flags & xy::UISystem::Flags::LeftMouse)
        {
            parentEntity.getComponent<xy::Transform>().setPosition(Menu::OffscreenPosition);

            xy::Command cmd;
            cmd.targetFlags = Menu::CommandID::MainNode;
            cmd.action = [](xy::Entity e, float)
            {
                e.getComponent<xy::Transform>().setPosition({});
            };
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

            cmd.targetFlags = Menu::CommandID::LobbyItem;
            cmd.action = [&](xy::Entity entity, float)
            {
                m_uiScene.destroyEntity(entity);
            };
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

            //leave lobby
            m_sharedData.netClient->disconnect();

            //quit our local server - does nothing if we're not host :)
            auto* msg = getContext().appInstance.getMessageBus().post<SystemEvent>(MessageID::SystemMessage);
            msg->action = SystemEvent::RequestStopServer;

            m_pingServer = false;
        }
    });
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //start button - TODO only add this if we're hosting
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::StartButtonPosition);
    entity.addComponent<xy::Text>(font).setString("Start");
    entity.getComponent<xy::Text>().setCharacterSize(Global::MediumTextSize);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Right);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseEnter] = mouseOver;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseExit] = mouseOut;
    entity.getComponent<xy::UIHitBox>().area = Menu::ButtonArea;
    entity.getComponent<xy::UIHitBox>().area.left = -entity.getComponent<xy::UIHitBox>().area.width;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseUp] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback([&, parentEntity](xy::Entity, sf::Uint64 flags) mutable
    {
        if (flags & xy::UISystem::Flags::LeftMouse)
        {
            //send a packet to server if we're host telling it to switch to game state
            m_sharedData.netClient->sendPacket(PacketID::StartGame, std::uint8_t(0), xy::NetFlag::Reliable, Global::ReliableChannel);
        }
    });
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //background window
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Sprite>(m_textureResource.get("assets/images/browser_window.png"));
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Far);
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setPosition((xy::DefaultSceneSize.x - bounds.width) / 2.f, 120.f);
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //player avatars
    float xPos = 20.f;
    const float xStride = (xy::DefaultSceneSize.x - (2 * xPos)) / 4.f;
    xPos += (xStride / 2.f);
    for (auto i = 0; i < 4; ++i)
    {
        //TODO character avatar as well as small icon/avatar
        entity = m_uiScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(xPos, 200.f);
        entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
        entity.addComponent<xy::Text>(fineFont).setAlignment(xy::Text::Alignment::Centre);
        entity.getComponent<xy::Text>().setCharacterSize(Global::LobbyTextSize);
        entity.addComponent<xy::Callback>().active = true;
        entity.getComponent<xy::Callback>().function = [&, i](xy::Entity e, float)
        {
            e.getComponent<xy::Text>().setString(m_sharedData.clientInformation.getClient(i).name);
        };
        parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

        //add ready state checkbox
        entity = addCheckbox(i);
        entity.getComponent<xy::Transform>().setPosition(xPos + 80.f, 740.f); //TODO const some position values
        parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

        entity = m_uiScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(xPos + 60.f, 740.f);
        entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
        entity.addComponent<xy::Text>(fineFont).setAlignment(xy::Text::Alignment::Right);
        entity.getComponent<xy::Text>().setString("Ready");
        entity.getComponent<xy::Text>().setCharacterSize(Global::LobbyTextSize);
        parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

        xPos += xStride;
    }

    //seed text
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::SeedPosition);
    entity.addComponent<xy::Text>(fineFont).setString("Seed: ");
    entity.getComponent<xy::Text>().setCharacterSize(Global::LobbyTextSize);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::CommandTarget>().ID = Menu::CommandID::SeedText; //TODO move this to textbox text
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());
    //TODO text input box
}

xy::Entity MenuState::addCheckbox(std::uint8_t playerID)
{
    auto entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Mid);
    entity.addComponent<xy::Sprite>() = m_sprites[Menu::SpriteID::Checkbox];
    entity.addComponent<Checkbox>().playerID = playerID;

    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] = m_callbackIDs[Menu::CallbackID::CheckboxClicked];

    //callback updates appearance based on current status
    entity.addComponent<xy::Callback>().active = true;
    entity.getComponent<xy::Callback>().function =
        [&](xy::Entity e, float)
    {
        auto checked = m_sharedData.clientInformation.getClient(e.getComponent<Checkbox>().playerID).ready;
        auto bounds = e.getComponent<xy::Sprite>().getTextureRect();
        if (checked)
        {
            bounds.top = bounds.height;
        }
        else
        {
            bounds.top = 0.f;
        }
        e.getComponent<xy::Sprite>().setTextureRect(bounds);
    };

    return entity;
}
