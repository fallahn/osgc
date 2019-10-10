/*********************************************************************
(c) Matt Marchant 2019
http://trederia.blogspot.com

osgc - Open Source Game Collection - Zlib license.

This software is provided 'as-is', without any express or
implied warranty. In no event will the authors be held
liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute
it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented;
you must not claim that you wrote the original software.
If you use this software in a product, an acknowledgment
in the product documentation would be appreciated but
is not required.

2. Altered source versions must be plainly marked as such,
and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any
source distribution.
*********************************************************************/

#pragma once

#include <xyginext/core/State.hpp>
#include <xyginext/gui/GuiClient.hpp>
#include <xyginext/ecs/Scene.hpp>
#include <xyginext/resources/ResourceHandler.hpp>
#include <xyginext/resources/Resource.hpp>
#include <xyginext/audio/AudioScape.hpp>

#include <SFML/Graphics/Shader.hpp>

class Game;
namespace fe
{
    class LoadingScreen;
}

class BrowserState final : public xy::State, public xy::GuiClient
{
public:
    BrowserState(xy::StateStack&, xy::State::Context, Game&, fe::LoadingScreen&);
    ~BrowserState();

    bool handleEvent(const sf::Event &evt) override;
    
    void handleMessage(const xy::Message &) override;
    
    bool update(float dt) override;
    
    void draw() override;

    xy::StateID stateID() const override;

private:
    Game& m_gameInstance;
    fe::LoadingScreen& m_loadingScreen;

    xy::Scene m_scene;
    xy::ResourceHandler m_resources;

    std::size_t m_browserTargetIndex;
    std::vector<sf::Vector2f> m_browserTargets;

    std::vector<std::unique_ptr<sf::Texture>> m_slideshowTextures;
    std::size_t m_slideshowIndex;
    sf::Shader m_shader;

    xy::AudioResource m_audioResource;
    xy::AudioScape m_audioScape;

    bool m_locked; //locks the browser until the intro is finished
    bool m_quitShown;

    std::vector<std::pair<xy::Entity, std::string>> m_nodeList;
    sf::Vector2f m_basePosition; //initial node position
    
    struct Settings final
    {
        bool useSlideshow = false;
        bool lastSort = true;
    }m_settings;
    void loadSettings();
    void saveSettings();

    void initScene();
    void loadResources();
    void buildMenu();
    void buildSlideshow();

    void nextItem();
    void prevItem();
    void execItem();
    void enableItem();

    void showQuit();
    void hideQuit();

    void sortNodes(bool asc);

    void updateTextScene();
    void updateLoadingScreen(float, sf::RenderWindow&) override;
};
