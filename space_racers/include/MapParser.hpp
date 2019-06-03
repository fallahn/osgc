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

#include "LightningSystem.hpp"

#include <tmxlite/Map.hpp>

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <string>
#include <array>
#include <vector>

namespace MapConst
{
    enum
    {
        Neon, Detail, Track, Normal, Count
    };
    static const std::array<std::string, Count> TileLayers =
    {
        "neon", "detail", "track", "normal"
    };
}

namespace sf
{
    class RenderTarget;
    class RenderTexture;
}

namespace xy
{
    class Scene;
}

namespace tmx
{
    class TileLayer;
}

class MapParser final
{
public:
    explicit MapParser(xy::Scene&);

    bool load(const std::string&);

    std::int32_t getWaypointCount() const
    {
        return m_waypointCount;
    }

    sf::Vector2f getStartPosition() const
    {
        return m_startPosition;
    }

    sf::Vector2f getSize() const
    {
        return m_size;
    }

    const std::vector<Lightning>& getBarriers() const
    {
        return m_barriers;
    }

    void renderLayers(std::array<sf::RenderTexture, 2u>&) const;

private:
    xy::Scene& m_scene;

    std::int32_t m_waypointCount;
    sf::Vector2f m_startPosition;
    sf::Vector2f m_size;

    std::vector<Lightning> m_barriers;

    tmx::Map m_map;
    enum LayerID
    {
        Track, Detail,
        Normal,
        Neon,

        Count
    };
    std::array<const tmx::TileLayer*, Count> m_layers = {};

    struct TilesetInfo final
    {
        std::vector<std::unique_ptr<sf::Texture>> textures;
        const std::vector<tmx::Tileset>* tileSets = nullptr;
    };

    void renderLayer(sf::RenderTarget&, const tmx::TileLayer*, const TilesetInfo&) const;
};