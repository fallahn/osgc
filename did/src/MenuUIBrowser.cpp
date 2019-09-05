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

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/UIHitBox.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/Callback.hpp>

#include <xyginext/ecs/systems/UISystem.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>

#include <SFML/System/Clock.hpp>

namespace
{
    const sf::Vector2f ItemOffset(80.f, 148.f);
}

void MenuState::buildBrowser(sf::Font& font)
{
    auto mouseOver = m_callbackIDs[Menu::CallbackID::TextSelected];
    auto mouseOut = m_callbackIDs[Menu::CallbackID::TextUnselected];

    auto parentEntity = m_uiScene.createEntity();
    parentEntity.addComponent<xy::Transform>().setPosition(Menu::OffscreenPosition);
    parentEntity.addComponent<xy::CommandTarget>().ID = Menu::CommandID::BrowserNode;

    //back button
    auto entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::BackButtonPosition);
    entity.addComponent<xy::Text>(font).setString("Back");
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

            cmd.targetFlags = Menu::CommandID::BrowserItem;
            cmd.action = [&](xy::Entity entity, float)
            {
                m_uiScene.destroyEntity(entity);
            };
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
        }
    });
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //refresh button
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::StartButtonPosition);
    entity.addComponent<xy::Text>(font).setString("Refresh");
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
            refreshBrowserView();
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
}

void MenuState::refreshBrowserView()
{
    xy::Command cmd;
    cmd.targetFlags = Menu::CommandID::BrowserItem;
    cmd.action = [&](xy::Entity entity, float)
    {
        m_uiScene.destroyEntity(entity);
    };
    m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

    auto& font = m_fontResource.get(Global::FineFont);

    auto textEntity = m_uiScene.createEntity();
    textEntity.addComponent<xy::Transform>().setPosition(ItemOffset);
    textEntity.addComponent<xy::Text>(font).setString("Refreshing...");
    textEntity.addComponent<xy::Drawable>();
    textEntity.addComponent<xy::CommandTarget>();
    textEntity.addComponent<xy::Callback>().active = true;
    textEntity.getComponent<xy::Callback>().function = 
        [](xy::Entity entity, float)
    {
        entity.getComponent<xy::CommandTarget>().ID = Menu::CommandID::BrowserItem;
        entity.getComponent<xy::Callback>().active = false;
    };

    m_lobbyListResult.Set(SteamMatchmaking()->RequestLobbyList(), this, &MenuState::onLobbyListRecieved);
}

void MenuState::onLobbyListRecieved(LobbyMatchList_t* list, bool failed)
{
    if (!failed)
    {
        xy::Command cmd;
        cmd.targetFlags = Menu::CommandID::BrowserItem;
        cmd.action = [&](xy::Entity entity, float)
        {
            m_uiScene.destroyEntity(entity);
        };
        m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

        sf::FloatRect bounds = m_sprites[Menu::SpriteID::BrowserItem].getTextureRect(); //not yet cropped
        bounds.height /= 3.f;

        auto& font = m_fontResource.get(Global::FineFont);
        auto callback = [](xy::Entity entity, float)
        {
            entity.getComponent<xy::CommandTarget>().ID = Menu::CommandID::BrowserItem;
            entity.getComponent<xy::Callback>().active = false;
        };

        for (auto i = 0; i < list->m_nLobbiesMatching; ++i)
        {
            auto lobbyID = SteamMatchmaking()->GetLobbyByIndex(i);
            auto memberCount = SteamMatchmaking()->GetNumLobbyMembers(lobbyID);

            auto itemEntity = m_uiScene.createEntity();
            itemEntity.addComponent<xy::Transform>().setPosition(ItemOffset);
            itemEntity.getComponent<xy::Transform>().move(0.f, bounds.height * i);
            itemEntity.addComponent<xy::CommandTarget>();
            itemEntity.addComponent<xy::Callback>().active = true;
            itemEntity.getComponent<xy::Callback>().function = callback;

            auto spriteEntity = m_uiScene.createEntity();
            spriteEntity.addComponent<CSteamID>() = lobbyID;
            spriteEntity.addComponent<xy::Drawable>();
            spriteEntity.addComponent<xy::Sprite>() = m_sprites[Menu::SpriteID::BrowserItem];
            spriteEntity.getComponent<xy::Sprite>().setTextureRect(bounds);
            spriteEntity.addComponent<xy::UIHitBox>().area = bounds;
            spriteEntity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::Selected] = m_callbackIDs[Menu::CallbackID::BrowserItemSelected];
            spriteEntity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::Unselected] = m_callbackIDs[Menu::CallbackID::BrowserItemDeselected];
            spriteEntity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseDown] = m_callbackIDs[Menu::CallbackID::BrowserItemClicked];
            spriteEntity.addComponent<sf::Clock>();
            spriteEntity.addComponent<xy::CommandTarget>();
            spriteEntity.addComponent<xy::Callback>().active = true;
            spriteEntity.getComponent<xy::Callback>().function = callback;
            spriteEntity.addComponent<xy::Transform>();
            itemEntity.getComponent<xy::Transform>().addChild(spriteEntity.getComponent<xy::Transform>());

            auto textEntity = m_uiScene.createEntity();
            textEntity.addComponent<xy::Transform>().setPosition(12.f, 10.f);
            textEntity.addComponent<xy::Text>(font).setString("Players: " + std::to_string(memberCount) + "/4");
            textEntity.addComponent<xy::Drawable>();
            textEntity.addComponent<xy::CommandTarget>();
            textEntity.addComponent<xy::Callback>().active = true;
            textEntity.getComponent<xy::Callback>().function = callback;
            itemEntity.getComponent<xy::Transform>().addChild(textEntity.getComponent<xy::Transform>());

            //TODO can we query lobby server for ping? Lobby needs server IP/port available
            //so we can query it with SteamMatchmakingServers()->PingServer(ip, port, &queryHandler);
        }

        if (list->m_nLobbiesMatching == 0)
        {
            //show a message saying no games were found
            auto textEntity = m_uiScene.createEntity();
            textEntity.addComponent<xy::Transform>().setPosition(ItemOffset);
            textEntity.addComponent<xy::Text>(font).setString("No Games Found");
            textEntity.addComponent<xy::Drawable>();
            textEntity.addComponent<xy::CommandTarget>();
            textEntity.addComponent<xy::Callback>().active = true;
            textEntity.getComponent<xy::Callback>().function = callback;
        }
    }
}