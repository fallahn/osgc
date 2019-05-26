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
#include "ResourceIDs.hpp"
#include "InputParser.hpp"
#include "MapParser.hpp"

#include <xyginext/core/State.hpp>
#include <xyginext/ecs/Scene.hpp>
#include <xyginext/ecs/components/Sprite.hpp>

#include <vector>
#include <array>

struct VehicleData;
struct ActorData;
struct ActorUpdate;
struct ClientUpdate;
class RaceState final : public xy::State
{
public:
    RaceState(xy::StateStack&, xy::State::Context, SharedData&);

    bool handleEvent(const sf::Event&) override;
    void handleMessage(const xy::Message&) override;
    bool update(float) override;
    void draw() override;

    xy::StateID stateID() const override { return StateID::Race; }

private:
    SharedData& m_sharedData;
    xy::Scene m_gameScene;
    //sf::RenderTexture m_gameBuffer;
    //sf::RenderWindow m_lightingBuffer;
    //xy::Scene m_outputScene;
    xy::Scene m_uiScene;

    MapParser m_mapParser;
    xy::ResourceHolder m_resources;

    std::array<xy::Sprite, SpriteID::Game::Count> m_sprites;

    InputParser m_playerInput;
    sf::Clock m_pingClock;

    void initScene();
    void loadResources();
    void buildWorld();
    void buildUI();

    void handlePackets();

    void spawnVehicle(const VehicleData&);
    void spawnActor(const ActorData&);
    void updateActor(const ActorUpdate&);
    void reconcile(const ClientUpdate&);

    void resetNetVehicle(const VehicleData&);
    void explodeNetVehicle(std::uint32_t);
    void fallNetVehicle(std::uint32_t);
    void removeNetVehicle(std::uint32_t);
};