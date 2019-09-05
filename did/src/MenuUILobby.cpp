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

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/UIHitBox.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/Drawable.hpp>

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
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Right);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::Selected] = mouseOver;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::Unselected] = mouseOut;
    entity.getComponent<xy::UIHitBox>().area = Menu::ButtonArea;
    entity.getComponent<xy::UIHitBox>().area.left = -entity.getComponent<xy::UIHitBox>().area.width;
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

            if (m_sharedData.lobbyID.IsLobby())
            {
                SteamMatchmaking()->LeaveLobby(m_sharedData.lobbyID);
            }

            if (m_sharedData.serverID.IsValid())
            {
                SteamNetworking()->CloseP2PSessionWithUser(m_sharedData.serverID);
            }

            m_sharedData.lobbyID = {};
            m_sharedData.serverID = {};
            m_sharedData.host = {};

            //quit our local server
            auto* msg = getContext().appInstance.getMessageBus().post<SystemEvent>(MessageID::SystemMessage);
            msg->action = SystemEvent::RequestStopServer;

            m_pingServer = false;
        }
    });
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //start button
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::StartButtonPosition);
    entity.addComponent<xy::Text>(font).setString("Start");
    entity.getComponent<xy::Text>().setCharacterSize(Global::MediumTextSize);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::Selected] = mouseOver;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::Unselected] = mouseOut;
    entity.getComponent<xy::UIHitBox>().area = Menu::ButtonArea;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseUp] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback([&, parentEntity](xy::Entity, sf::Uint64 flags) mutable
    {
        if (flags & xy::UISystem::Flags::LeftMouse)
        {
            if (SteamMatchmaking()->GetLobbyOwner(m_sharedData.lobbyID) == SteamUser()->GetSteamID())
            {
                //check all lobby members are ready
                auto memberCount = SteamMatchmaking()->GetNumLobbyMembers(m_sharedData.lobbyID);
                for (auto i = 0; i < memberCount; ++i)
                {
                    auto member = SteamMatchmaking()->GetLobbyMemberByIndex(m_sharedData.lobbyID, i);
                    std::string readyState = SteamMatchmaking()->GetLobbyMemberData(m_sharedData.lobbyID, member, "ready");
                    if (readyState != "true")
                    {
                        return;
                    }
                }

                //raise a message telling all lobby members to start the game
                const auto buns = Menu::ChatMessageID::StartGame;
                SteamMatchmaking()->SendLobbyChatMsg(m_sharedData.lobbyID, &buns, 1);

                //send a packet to server if we're host telling it to switch to game state
                sendData(PacketID::StartGame, std::uint8_t(0), m_sharedData.serverID, EP2PSend::k_EP2PSendReliable);

                //set the lobby not joinable while playing the game
                SteamMatchmaking()->SetLobbyJoinable(m_sharedData.lobbyID, false);
            }
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

    //seed text
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::SeedPosition);
    entity.addComponent<xy::Text>(fineFont).setString("Seed: ");
    entity.getComponent<xy::Text>().setCharacterSize(Global::LobbyTextSize);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::CommandTarget>().ID = Menu::CommandID::SeedText; //TODO move this to textbox text
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());
    //TODO text input box

    //friends text
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::FriendsPosition);
    entity.addComponent<xy::Text>(fineFont).setString("Friends Only");
    entity.getComponent<xy::Text>().setCharacterSize(Global::LobbyTextSize);
    entity.addComponent<xy::Drawable>();
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());
    
    entity = addCheckbox();
    entity.getComponent<xy::Transform>().setPosition(Menu::FriendsPosition.x + 360.f, Menu::FriendsPosition.y + 8.f);
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());
}

xy::Entity MenuState::addCheckbox()
{
    auto entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Mid);
    entity.addComponent<xy::Sprite>() = m_sprites[Menu::SpriteID::Checkbox];
    entity.addComponent<std::uint8_t>() = 0; //this is in lieu of a bool, we might replace this with a proper struct

    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] = m_callbackIDs[Menu::CallbackID::CheckboxClicked];

    return entity;
}
