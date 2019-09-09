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

#pragma once

#include "StateIDs.hpp"
#include "MenuUI.hpp"
#include "InputParser.hpp"

#include <xyginext/core/State.hpp>

#include <xyginext/ecs/Scene.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/resources/Resource.hpp>

#include <steam/steam_api.h>

#include <array>

struct Packet;
struct SharedData;
struct ActorState;
struct SceneState;

namespace xy
{
    struct NetEvent;
}

class MenuState final : public xy::State
{
public:
    MenuState(xy::StateStack&, xy::State::Context, SharedData&);

    xy::StateID stateID() const override { return StateID::Menu; }
    bool handleEvent(const sf::Event&) override;
    void handleMessage(const xy::Message&) override;
    bool update(float) override;
    void draw() override;

private:
    xy::Scene m_uiScene;
    xy::Scene m_gameScene;
    SharedData& m_sharedData;

    xy::TextureResource m_textureResource;
    xy::FontResource m_fontResource;

    std::array<xy::Sprite, Menu::SpriteID::Count> m_sprites = {};
    std::array<sf::Uint32, Menu::CallbackID::Count> m_callbackIDs = {};

    bool m_pingServer;
    sf::Clock m_pingClock;

    bool m_gameLaunched;

    void loadAssets();
    void createScene();
    void setLobbyView();

    void buildBrowser(sf::Font&);
    void buildLobby(sf::Font&);
    void buildOptions(sf::Font&);

    xy::Entity addCheckbox();

    //lobby callbacks and call results
    STEAM_CALLBACK(MenuState, onLobbyEnter, LobbyEnter_t);
    STEAM_CALLBACK(MenuState, onLobbyDataUpdate, LobbyDataUpdate_t);
    STEAM_CALLBACK(MenuState, onLobbyChatUpdate, LobbyChatUpdate_t);
    STEAM_CALLBACK(MenuState, onChatMessage, LobbyChatMsg_t);
    CCallResult<MenuState, LobbyCreated_t> m_lobbyCreated;
    void onLobbyCreated(LobbyCreated_t*, bool);

    CCallResult<MenuState, LobbyMatchList_t> m_lobbyListResult;
    void onLobbyListRecieved(LobbyMatchList_t*, bool);

    CCallResult<MenuState, LobbyEnter_t> m_lobbyJoined;
    void onLobbyJoined(LobbyEnter_t*, bool);

    void refreshLobbyView();
    void refreshBrowserView();

    void handlePacket(const xy::NetEvent&);
};