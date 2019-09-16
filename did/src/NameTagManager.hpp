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

#include <SFML/Graphics/RenderTexture.hpp>

#include <array>

namespace sf
{
    class String;
    class Font;
}

namespace xy
{
    class FontResource;
}

struct SharedData;

class NameTagManager final
{
public:
    explicit NameTagManager(SharedData&, xy::FontResource&);

    const sf::Texture& getTexture(std::size_t) const;
    void updateName(const sf::String&, std::size_t);

private:
    std::array<sf::RenderTexture, 4u> m_textures;
    sf::Font& m_font;
};
