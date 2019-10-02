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
#include "MatrixPool.hpp"

#include <xyginext/core/State.hpp>
#include <xyginext/gui/GuiClient.hpp>

#include <xyginext/ecs/Scene.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/resources/Resource.hpp>
#include <xyginext/resources/ShaderResource.hpp>

#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include <array>
#include <deque>

struct SharedData;
struct ActorState;
struct SceneState;

namespace xy
{
    struct NetEvent;
}

struct Checkbox final
{
    std::uint8_t playerID = 0;
};

class MenuState final : public xy::State, public xy::GuiClient
{
public:
    MenuState(xy::StateStack&, xy::State::Context, SharedData&);
    ~MenuState();

    xy::StateID stateID() const override { return StateID::Menu; }
    bool handleEvent(const sf::Event&) override;
    void handleMessage(const xy::Message&) override;
    bool update(float) override;
    void draw() override;

private:
    xy::Scene m_uiScene;
    xy::Scene m_gameScene; //used for drawing background
    SharedData& m_sharedData;

    xy::TextureResource m_textureResource;
    xy::FontResource m_fontResource;
    xy::AudioResource m_audioResource;

    std::array<xy::Sprite, Menu::SpriteID::Count> m_sprites = {};
    std::array<sf::Uint32, Menu::CallbackID::Count> m_callbackIDs = {};

    struct ActiveMapping final
    {
        bool keybindActive = false;
        bool joybindActive = false;

        union
        {
            sf::Keyboard::Key* keyDest;
            std::uint32_t* joyButtonDest;
        };

        xy::Entity displayEntity;
    }m_activeMapping;
    void handleKeyMapping(const sf::Event&);
    void handleJoyMapping(const sf::Event&);

    bool m_gameLaunched;

    sf::String* m_activeString;
    sf::String m_seedDisplayString;
    sf::String m_chatInDisplayString;
    std::deque<sf::String> m_chatBuffer;
    static constexpr std::size_t MaxChatSize = 6;
    void updateTextInput(const sf::Event&);
    void applySeed();
    void randomiseSeed();
    void sendChat();
    void receiveChat(const xy::NetEvent&);
    void updateChatOutput(xy::Entity, float);

    MatrixPool m_matrixPool;
    xy::ShaderResource m_shaderResource;
    sf::RenderTexture m_seaBuffer;
    sf::Sprite m_seaSprite;
    void updateBackground(float);

    void updateHostInfo(std::uint64_t);

    void loadAssets();
    void createScene();
    void createBackground();
    void setLobbyView();

    std::vector<std::string> m_readmeStrings;
    void loadReadme();

    void buildLobby(sf::Font&);
    void buildOptions(sf::Font&);

    void buildNameEntry(sf::Font&);
    void buildJoinEntry(sf::Font&);

    xy::Entity addCheckbox(std::uint8_t);

    void sendPlayerData();
    void updateClientInfo(const xy::NetEvent&);

    void handlePacket(const xy::NetEvent&);

    void loadSettings();
    void saveSettings();

    void updateLoadingScreen(float, sf::RenderWindow&) override;
};