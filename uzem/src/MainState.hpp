/*********************************************************************
(c) Jonny Paton 2018

xygineXT - Zlib license.

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

#include "avr8.h"

#include <xyginext/core/State.hpp>
#include <xyginext/core/ConfigFile.hpp>
#include <xyginext/gui/GuiClient.hpp>
#include <xyginext/resources/ResourceHandler.hpp>
#include <xyginext/resources/ShaderResource.hpp>

#include <SFML/System/Thread.hpp>
#include <SFML/Graphics/Text.hpp>


class MainState final : public xy::State, public xy::GuiClient
{
public:
    MainState(xy::StateStack&, xy::State::Context);

    ~MainState();

    bool handleEvent(const sf::Event &evt) override;
    
    void handleMessage(const xy::Message &) override;
    
    bool update(float dt) override;
    
    void draw() override;
    
    xy::StateID stateID() const override;

private:
    avr8 m_uzebox;
    sf::View m_view;

    xy::ConfigFile m_config;

    bool m_showOptions;
    bool m_hideHelpText;
    bool m_textureSmoothing;
    std::string m_romInfo;

    xy::ResourceHandler m_resources;
    xy::ShaderResource m_shaders;
    sf::Text m_helpText;

    sf::Shader* m_activeShader;
    std::vector<std::pair<sf::Shader*, std::string>> m_postShaders;
    std::size_t m_shaderIndex;

    sf::Thread m_thread;
    std::atomic_bool m_runEmulation;
    void emulate();

    void openRom();
    void closeRom();

    void loadResources();
    void updateView();
};
