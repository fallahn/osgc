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

#include "TextCrawl.hpp"

#include <xyginext/core/State.hpp>
#include <xyginext/ecs/Scene.hpp>
#include <xyginext/resources/ResourceHandler.hpp>
#include <xyginext/core/ConfigFile.hpp>

#include <SFML/Graphics/Shader.hpp>
#include <SFML/Window/Keyboard.hpp>

struct SharedData;
class MenuState final : public xy::State
{
public:
    MenuState(xy::StateStack&, xy::State::Context, SharedData&);
    ~MenuState();

    bool handleEvent(const sf::Event&) override;

    void handleMessage(const xy::Message&) override;

    bool update(float) override;

    void draw() override;

    xy::StateID stateID() const override;

private:
    SharedData& m_sharedData;
    xy::Scene m_scene;
    TextCrawl m_textCrawl;
    sf::Shader m_crawlShader;
    xy::ResourceHandler m_resources;

    bool m_menuActive;

    xy::ConfigFile m_configSettings;

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

    std::vector<std::string> m_highScores;
    std::vector<std::string> m_scoreTitles;
    std::size_t m_scoreIndex;

    void initScene();
    void loadAssets();
    void buildMenu();
    void buildStarfield();
    void buildHelp();
    void buildDifficultySelect();
    void buildHighScores();
    void saveSettings();

    void updateLoadingScreen(float, sf::RenderWindow&) override;
};