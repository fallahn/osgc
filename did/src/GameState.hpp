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
#include "FoliageGenerator.hpp"
#include "IslandGenerator.hpp"
#include "Server.hpp"
#include "Actor.hpp"
#include "InputParser.hpp"
#include "ResourceIDs.hpp"
#include "MatrixPool.hpp"
#include "NameTagManager.hpp"
#include "SummaryTexture.hpp"
#include "PathFinder.hpp"
#include "MiniMap.hpp"
#include "PacketTypes.hpp"

#include <xyginext/core/State.hpp>
#include <xyginext/core/ConsoleClient.hpp>
#include <xyginext/ecs/Scene.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/resources/Resource.hpp>
#include <xyginext/resources/ShaderResource.hpp>
#include <xyginext/audio/AudioScape.hpp>

#include <queue>

struct Packet;
struct SharedData;

namespace xy
{
    struct NetEvent;
}

struct MiniMapData final
{
    sf::Vector2f targetPosition;
    float targetScale = 0.5f;
};

class GameState final : public xy::State, public xy::ConsoleClient
{
public:
    GameState(xy::StateStack&, xy::State::Context, SharedData&);

    xy::StateID stateID() const override { return StateID::Game; }

    bool handleEvent(const sf::Event&) override;

    void handleMessage(const xy::Message&) override;

    bool update(float) override;

    void draw() override;

private:
    xy::AudioResource m_audioResource;  //needs to outlive the scene!

    xy::Scene m_gameScene;
    xy::Scene m_uiScene;
    SharedData& m_sharedData;
    InputParser m_inputParser;

    xy::TextureResource m_textureResource;
    xy::ShaderResource m_shaderResource;
    xy::FontResource m_fontResource;
    NameTagManager m_nameTagManager;
    SummaryTexture m_summaryTexture;
    MiniMap m_miniMap;

    MatrixPool m_modelMatrices;
    FoliageGenerator m_foliageGenerator;
    IslandGenerator m_islandGenerator;

    PathFinder m_pathFinder;

    std::array<xy::Sprite, SpriteID::Count> m_sprites;
    std::array<AnimationMap, SpriteID::Count> m_animationMaps;
    xy::AudioScape m_audioScape;

    std::queue<sf::String> m_messageQueue;
    bool m_canShowMessage;

    std::int32_t m_roundTime;

    bool m_roundOver;
    bool m_updateSummary;
    RoundSummary m_summaryStats;

    void loadResources();
    void loadUI();
    void loadRoundEnd();
    void loadScene(const TileArray&);
    bool m_sceneLoaded;

    void handlePacket(const xy::NetEvent&);

    void spawnActor(Actor, sf::Vector2f, std::int32_t timestamp, bool localPlayer = false);
    void updateCarriable(const CarriableState&);
    void updateInventory(InventoryState);
    void updateScene(SceneState);
    void updateConnection(ConnectionState);

    void spawnGhost(xy::Entity, sf::Vector2f);
    void toggleUI();
    void showRoundEnd(const RoundSummary&);
    void showEndButton();
    void showServerMessage(std::int32_t);
    void printMessage(const sf::String&);
    void processMessageQueue();

    void createSplash(sf::Vector2f);
    void createPlayerPuff(sf::Vector2f);

    void plotPath(const std::vector<sf::Vector2f>&);
    void loadAudio();

    void handleDisconnect();

    void updateLoadingScreen(float, sf::RenderWindow&) override;
};
