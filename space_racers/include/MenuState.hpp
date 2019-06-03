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

#include "ResourceIDs.hpp"

#include <xyginext/core/ConsoleClient.hpp>
#include <xyginext/core/State.hpp>
#include <xyginext/core/ConfigFile.hpp>
#include <xyginext/ecs/Scene.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/resources/ResourceHandler.hpp>

#include <array>

struct SharedData;
class MenuState final : public xy::State, public xy::ConsoleClient
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
    xy::ResourceHolder m_resources;
    xy::ConfigFile m_settings;

    sf::String* m_activeString;

    std::array<xy::Sprite, SpriteID::Menu::Count> m_sprites;
    std::array<std::size_t, TextureID::Menu::Count> m_textureIDs;

    void initScene();
    void loadResources();
    void buildMenu();
    void buildNetworkMenu(xy::Entity, sf::Uint32, sf::Uint32);

    void updateTextInput(const sf::Event&);
};