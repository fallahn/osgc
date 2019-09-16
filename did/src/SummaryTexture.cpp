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

#include "SummaryTexture.hpp"
#include "PacketTypes.hpp"
#include "ClientInfoManager.hpp"
#include "GlobalConsts.hpp"

#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include <xyginext/resources/Resource.hpp>

namespace
{
    const sf::Uint32 TextureWidth = 1664;
    const sf::Uint32 TextureHeight = 768;

    std::array<sf::Vector2f, 4u> NamePositions = 
    {
        sf::Vector2f(154.f, 88.f),
        sf::Vector2f(1314.f, 88.f),
        sf::Vector2f(1314.f, 428.f),
        sf::Vector2f(154.f, 428.f)
    };

    std::array<sf::Vector2f, 4u> AvatarPositions =
    {
        sf::Vector2f(90.f, 112.f),
        sf::Vector2f(1250.f, 112.f),
        sf::Vector2f(1250.f, 452.f),
        sf::Vector2f(90.f, 452.f)
    };

    std::array<sf::IntRect, 5u> AvatarRects =
    {
        sf::IntRect(128, 0, 64, 64),
        sf::IntRect(192, 0, 64, 64),
        sf::IntRect(192, 64, 64, 64),
        sf::IntRect(128, 64, 64, 64),
        sf::IntRect(0, 64, 64, 64)
    };
}

SummaryTexture::SummaryTexture(xy::FontResource& fr)
    : m_font(fr.get(Global::FineFont))
{
    m_texture.create(TextureWidth, TextureHeight);

    m_avatarTexture.loadFromFile(xy::FileSystem::getResourcePath() + "assets/images/lobby.png");
}

//public
void SummaryTexture::update(const RoundSummary& summary, const ClientInfoManager& manager, bool roundEnd)
{
    sf::Sprite sprite;
    sprite.setOrigin(32.f, 32.f);
    sf::Text text;
    text.setFont(m_font);
    text.setCharacterSize(36);

    //TODO other text properties

    m_texture.clear(sf::Color::Transparent);

    for (auto i = 0u; i < NamePositions.size(); ++i)
    {
        const auto& info = manager.getClient(summary.stats[i].id - Actor::ID::PlayerOne);
        
        sf::String str = info.name;
        str += "\n\nTreasure: " + std::to_string(summary.stats[i].treasure);
        str += "\nEnemies Beaten: " + std::to_string(summary.stats[i].foesSlain);
        str += "\nShots Fired: " + std::to_string(summary.stats[i].shotsFired);
        str += "\nLives Lost: " + std::to_string(summary.stats[i].livesLost);
        if (roundEnd)
        {
            str += "\nXP Earned: " + std::to_string(summary.stats[i].roundXP);
        }
        
        text.setString(str);
        text.setPosition(NamePositions[i]);
        m_texture.draw(text);

        sprite.setTexture(m_avatarTexture);
        if (manager.getClient(i).peerID > 0)
        {
            sprite.setTextureRect(AvatarRects[i]);
        }
        else
        {
            sprite.setTextureRect(AvatarRects.back());
        }
        sprite.setPosition(AvatarPositions[i]);
        m_texture.draw(sprite);
    }

    m_texture.display();
}

const sf::Texture& SummaryTexture::getTexture() const
{
    return m_texture.getTexture();
}