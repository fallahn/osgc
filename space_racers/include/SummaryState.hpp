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
#include <xyginext/core/State.hpp>
#include <xyginext/ecs/Scene.hpp>
#include <xyginext/resources/ResourceHandler.hpp>
#include <xyginext/ecs/components/Sprite.hpp>

#include <SFML/System/Clock.hpp>
#include <SFML/Graphics/Shader.hpp>

class SummaryState final : public xy::State
{
public:
    SummaryState(xy::StateStack&, xy::State::Context, SharedData&);

    bool handleEvent(const sf::Event&) override;

    void handleMessage(const xy::Message&) override;

    bool update(float) override;

    void draw() override;

    xy::StateID stateID() const override { return StateID::Summary; }

private:
    SharedData& m_sharedData;

    xy::ResourceHandler m_resources;

    enum Textures
    {
        Podium, Title,
        CarNeon, BikeNeon, ShipNeon,
        Size
    };
    std::array<std::size_t, Textures::Size> m_textureIDs;

    enum Sprites
    {
        Car, Bike, Ship, Count
    };
    std::array<xy::Sprite, Sprites::Count> m_sprites;

    xy::Scene m_scene;

    sf::Clock m_delayClock;
    sf::Shader m_neonShader;

    void initScene();
    void buildMenu();

    void buildLocalPlayers();
    void buildNetworkPlayers();
};