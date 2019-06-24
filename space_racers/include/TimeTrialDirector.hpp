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

#include "ResourceIDs.hpp"

#include <xyginext/ecs/Director.hpp>
#include <xyginext/ecs/Entity.hpp>
#include <xyginext/ecs/components/Sprite.hpp>

#include <SFML/System/Clock.hpp>

namespace xy
{
    class Message;
    class Scene;
    class ResourceHandler;
    class ShaderResource;
}

struct ResourceCollection final
{
    xy::Scene* gameScene = nullptr;;
    xy::ResourceHandler* resources = nullptr;
    xy::ShaderResource* shaders = nullptr;
    const std::array<xy::Sprite, SpriteID::Game::Count>* sprites = nullptr;
    const std::array<std::size_t, TextureID::Game::Count>* textureIDs = nullptr;
};

struct SharedData;
class TimeTrialDirector final : public xy::Director
{
public:
    TimeTrialDirector(ResourceCollection, const std::string&, std::int32_t, SharedData&);

    void handleMessage(const xy::Message &) override;

    void process(float) override;

private:
    sf::Clock m_lapClock;
    bool m_updateDisplay;
    float m_fastestLap;

    sf::Time m_pauseTime;

    ResourceCollection m_resources;
    std::string m_mapName;
    std::int32_t m_vehicleType;
    SharedData& m_sharedData;

    struct Point final
    {
        Point() = default;
        Point(sf::Vector2f position, float rot, float sc)
            : x(position.x), y(position.y), rotation(rot), scale(sc) {}

        float x = 0.f;
        float y = 0.f;
        float rotation = 0.f;
        float scale = 1.f;
    };
    static constexpr std::size_t MaxPoints = 60 * 120; //60Hz, 120 seconds
    std::vector<Point> m_recordedPoints;
    std::size_t m_recordingIndex;

    std::vector<Point> m_playbackPoints;
    std::size_t m_playbackIndex;

    xy::Entity m_ghostEntity;
    xy::Entity m_playerEntity;

    bool m_ghostEnabled;

    void createGhost();

    void loadGhost();
    void saveGhost();
    std::string getGhostPath() const;

    void updateScoreboard();
};