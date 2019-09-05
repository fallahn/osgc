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

#include "MiniMap.hpp"
#include "MessageIDs.hpp"
#include "Actor.hpp"
#include "GlobalConsts.hpp"

#include <xyginext/resources/Resource.hpp>
#include <xyginext/util/Vector.hpp>
#include <xyginext/util/Math.hpp>

namespace
{
    const float Scalar = 3.f;
    const float IconSize = 6.f;
    const float ViewRadius = 60.f;
    const float ViewRadiusSqr = ViewRadius * ViewRadius;

    const std::string MapShader = 
        R"(
        #version 120

        uniform sampler2D u_texture;

        const float threshold = 0.2 / 30.0;

        void main()
        {
            float factor = 1.0 / (threshold + 0.001);
            vec2 pos = floor(gl_TexCoord[0].xy * factor + 0.5) / factor;

            vec4 colour = texture2D(u_texture, pos);
            
            if(colour.a == 0)
            {
                colour = vec4(1.0);
            }
            colour.rgb = vec3(dot(vec3(0.3, 0.59, 0.11), colour.rgb));


            gl_FragColor = colour;
        })";
}

MiniMap::MiniMap(xy::TextureResource& tr)
    : m_textureResource (tr),
    m_localPlayer       (0)
{
    auto textureSize = sf::Vector2u(Global::IslandSize / Scalar);
    m_outputBuffer.create(textureSize.x, textureSize.y);
    m_backgroundTexture.create(textureSize.x, textureSize.y);

    m_backgroundSprite.setTexture(m_backgroundTexture.getTexture());
    m_backgroundSprite.setColor({ 230, 230, 230 }); //so halo won't bleach

    m_crossTexture = &m_textureResource.get("assets/images/cross.png");

    m_mapShader.loadFromMemory(MapShader, sf::Shader::Fragment);

    int i = 0;
    for (auto& p : m_playerPoints)
    {
        p.setFillColor(Global::PlayerColours[i++]);
        p.setRadius(IconSize);
        p.setOrigin(IconSize, IconSize);
    }

    m_halo.setRadius(ViewRadius);
    m_halo.setOrigin(ViewRadius, ViewRadius);
    m_halo.setFillColor({ 25, 25, 25 });
}

void MiniMap::setTexture(const sf::Texture& texture)
{    
    sf::Sprite scrollSpr(m_textureResource.get("assets/images/treasure_map.png"));
    scrollSpr.setScale(4.f, 4.f);
    scrollSpr.setTextureRect({ 0,0,128,128 });

    sf::Sprite spr(texture);
    spr.setScale({ 1.f / Scalar, 1.f / Scalar });

    m_mapShader.setUniform("u_texture", texture);
    sf::RenderStates states;
    states.shader = &m_mapShader;
    states.blendMode = sf::BlendMultiply;

    m_backgroundTexture.clear(sf::Color::Transparent);
    m_backgroundTexture.draw(scrollSpr);
    m_backgroundTexture.draw(spr, states);
    scrollSpr.setTextureRect({ 128,0,128,128 });
    m_backgroundTexture.draw(scrollSpr);
    m_backgroundTexture.display();
}

const sf::Texture& MiniMap::getTexture() const
{
    return m_outputBuffer.getTexture();
}

void MiniMap::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::MiniMapUpdate)
    {
        const auto& data = msg.getData<MiniMapEvent>();
        m_playerPoints[data.actorID - Actor::ID::PlayerOne].setPosition(data.position / Scalar);
    }
}

void MiniMap::updateTexture()
{
    //TODO update FOW texture

    m_outputBuffer.clear(sf::Color::Transparent);

    //draw background
    m_outputBuffer.draw(m_backgroundSprite);
    
    //draw halo over player
    m_outputBuffer.draw(m_halo, sf::BlendAdd);

    //crosses
    m_outputBuffer.draw(m_crossVerts.data(), m_crossVerts.size(), sf::Quads, m_crossTexture);

    for (const auto& p : m_playerPoints)
    {
        m_outputBuffer.draw(p);
    }

    //TODO draw FOW - requires some double buffering..

    m_outputBuffer.display();
}

void MiniMap::update()
{
    m_halo.setPosition(m_playerPoints[m_localPlayer].getPosition());

    for (auto i = 0u; i < m_playerPoints.size(); ++i)
    {
        if (i != m_localPlayer)
        {
            auto colour = Global::PlayerColours[i];

            //measure distance to point and set to transparent if too far
            auto len2 = xy::Util::Vector::lengthSquared(m_playerPoints[i].getPosition() - m_playerPoints[m_localPlayer].getPosition());
            if (len2 > ViewRadiusSqr)
            {
                float diff = len2 - ViewRadiusSqr;
                float alpha = 1.f - xy::Util::Math::clamp(diff / 2000.f, 0.f, 1.f);
                colour.a = static_cast<sf::Uint8>(255.f * alpha);
            }
            m_playerPoints[i].setFillColor(colour);
        }
    }
}

void MiniMap::setLocalPlayer(std::size_t p)
{
    m_localPlayer = p;

    //uuugghhh at least read something from the texture size....
    auto pos = Global::BoatPositions[p] / Scalar;

    if (pos.x < (xy::DefaultSceneSize.x / Scalar) / 2.f)
    {
        m_crossVerts.emplace_back(sf::Vector2f(-10.f, -10.f) + pos, sf::Vector2f(16.f, 0.f));
        m_crossVerts.emplace_back(sf::Vector2f(54.f, -10.f) + pos, sf::Vector2f(48.f, 0.f));
        m_crossVerts.emplace_back(sf::Vector2f(54.f, 22.f) + pos, sf::Vector2f(48.f, 16.f));
        m_crossVerts.emplace_back(sf::Vector2f(-10.f, 22.f) + pos, sf::Vector2f(16.f, 16.f));
    }
    else
    {
        m_crossVerts.emplace_back(sf::Vector2f(-54.f, -10.f) + pos, sf::Vector2f(16.f, 0.f));
        m_crossVerts.emplace_back(sf::Vector2f(10.f, -10.f) + pos, sf::Vector2f(48.f, 0.f));
        m_crossVerts.emplace_back(sf::Vector2f(10.f, 22.f) + pos, sf::Vector2f(48.f, 16.f));
        m_crossVerts.emplace_back(sf::Vector2f(-54.f, 22.f) + pos, sf::Vector2f(16.f, 16.f));
    }
}

void MiniMap::addCross(sf::Vector2f position)
{
    position /= Scalar;

    static const float CrossSize = 8.f;
    static const std::array<sf::Vector2f, 4u> CrossPositions =
    {
        sf::Vector2f(-CrossSize, -CrossSize),
        sf::Vector2f(CrossSize, -CrossSize),
        sf::Vector2f(CrossSize, CrossSize),
        sf::Vector2f(-CrossSize, CrossSize)
    };

    static const std::array<sf::Vector2f, 4u> UVs = 
    {
        sf::Vector2f(),
        sf::Vector2f(CrossSize * 2.f, 0.f),
        sf::Vector2f(CrossSize * 2.f, CrossSize * 2.f),
        sf::Vector2f(0.f, CrossSize * 2.f)
    };

    for (auto i = 0u; i < CrossPositions.size(); ++i)
    {
        m_crossVerts.emplace_back(position + CrossPositions[i], UVs[i]);
    }
}