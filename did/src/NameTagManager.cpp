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

#include "NameTagManager.hpp"
#include "GlobalConsts.hpp"
#include "SharedStateData.hpp"

#include <xyginext/resources/Resource.hpp>
#include <xyginext/core/Log.hpp>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Text.hpp>

namespace
{
    const sf::Uint32 LabelWidth = 320;
    const sf::Uint32 LabelHeight = 32;
}

NameTagManager::NameTagManager(SharedData& sd, xy::FontResource& fr)
    : m_font(fr.get(Global::BoldFont))
{
    for (auto i = 0u; i < m_textures.size(); ++i)
    {
        if (!m_textures[i].create(LabelWidth, LabelHeight))
        {
            xy::Logger::log("failed creating texture for name tag", xy::Logger::Type::Error);
        }
        else
        {
            updateName(sd.clientInformation.getClient(i).name, i);
        }
    }
}

//public
const sf::Texture& NameTagManager::getTexture(std::size_t idx) const
{
    return m_textures[idx].getTexture();
}

void NameTagManager::updateName(const sf::String& name, std::size_t idx)
{
    sf::Text text;
    text.setFont(m_font);
    text.setString(name);
    text.setCharacterSize(27);

    auto width = text.getLocalBounds().width;
    float move = LabelWidth - width;
    move /= 2.f;
    text.move(move, 0.f);

    m_textures[idx].clear({ 0, 0, 0, 120 });
    m_textures[idx].draw(text);
    m_textures[idx].display();
}