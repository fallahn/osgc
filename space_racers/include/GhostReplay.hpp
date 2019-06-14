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

//TODO this could be structured a bit more.. 'generically', but right now
//it's only designed for time trial ghosts.

#pragma once

#include "ResourceIDs.hpp"

#include <xyginext/ecs/Entity.hpp>
#include <xyginext/ecs/components/Sprite.hpp>

#include <vector>
#include <array>

namespace xy
{
    class Message;
    class Scene;
    class ResourceHandler;
    class ShaderResource;
}

struct ResourceCollection final
{
    xy::ResourceHandler* resources = nullptr;
    xy::ShaderResource* shaders = nullptr;
    const std::array<xy::Sprite, SpriteID::Game::Count>* sprites = nullptr;
    const std::array<std::size_t, TextureID::Game::Count>* textureIDs = nullptr;
};

class GhostReplay final
{
public:
    GhostReplay(xy::Scene&);

    void setPlayerEntity(xy::Entity);

    void setResources(ResourceCollection rc) { m_resources = rc; }

    void handleMessage(const xy::Message&);

    void update();

private:

    xy::Scene& m_scene;
    ResourceCollection m_resources;

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

    bool m_enabled;

    void createGhost();
};