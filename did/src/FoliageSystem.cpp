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

#include "FoliageSystem.hpp"
#include "fastnoise/FastNoiseSIMD.h"
using fn = FastNoiseSIMD;

#include <xyginext/ecs/components/Drawable.hpp>

#include <xyginext/util/Vector.hpp>
#include <xyginext/util/Math.hpp>
#include <xyginext/util/Random.hpp>

namespace
{
    const sf::Vector2f NormalVec(0.f, 1.f);
    const float DefaultAngle = -4.f;
    const float MaxAngle = 30.f;

    const int NoiseTableSize = 256;
    const float TreeWidth = 6.f;

    const float LeafWidth = 128.f;
    const float LeafHeight = 96.f;
    const float TrunkLeft = 128.f;
    const float TrunkRight = 148.f;
    const float TrunkHeight = 192.f;
    const float LeafScale = 0.85f;
}

TreeSegment::TreeSegment(float length)
    : m_length(length),
    m_rotation(0.f)
{

}

sf::Vector2f TreeSegment::getEnd() const
{
    auto vec = xy::Util::Vector::rotate(NormalVec, m_rotation);
    m_normal = { -vec.y, vec.x };
    return (vec * m_length);
}

void TreeSegment::setRotation(float rotation)
{
    m_rotation = rotation;
}

void TreeSegment::setPosition(sf::Vector2f position)
{
    m_position = position;
}

sf::Vector2f TreeSegment::getPosition() const
{
    return m_position;
}

sf::Vector2f TreeSegment::getNormal() const
{
    return m_normal;
}

TreeBranch::TreeBranch(float length, std::size_t segmentCount, sf::Vector2f basePosition)
    : m_basePosition(basePosition),
    m_rotation      (0.f),
    m_length        (length),
    m_textureOffset (0.f)
{
    float segLength = length / segmentCount;
    for (auto i = 0u; i < segmentCount; ++i)
    {
        m_segments.emplace_back(segLength);
        m_segments.back().setRotation(DefaultAngle);
    }

    m_textureOffset += xy::Util::Random::value(0, 1) == 0 ? 0.f : LeafHeight;
}

void TreeBranch::update(float windstrength)
{
    //add wind
    m_rotation = xy::Util::Math::clamp(m_rotation + windstrength, -MaxAngle, MaxAngle);

    //add some value to make it look like it springs back
    m_rotation *= 0.99f;
    //if (m_rotation > DefaultAngle /*+ 0.1f*/)
    //{
    //    m_rotation -= 0.1f;
    //}
    //else if (m_rotation < DefaultAngle/* - 0.1f*/)
    //{
    //    m_rotation += 0.1f;
    //}

    //update all positions
    sf::Vector2f basePosition = m_basePosition;
    float rotation = m_rotation;
    for (auto& segment : m_segments)
    {
        basePosition += segment.getEnd();

        segment.setRotation(rotation);
        segment.setPosition(basePosition);
        rotation += m_rotation;
    }
}

const std::vector<TreeSegment>& TreeBranch::getSegments() const
{
    return m_segments;
}

sf::Vector2f TreeBranch::getBasePosition() const
{
    return m_basePosition;
}

float TreeBranch::getLength() const
{
    return m_length;
}

void TreeBranch::setBasePosition(sf::Vector2f position)
{
    m_basePosition = position;
}

//----Actual system----//
FoliageSystem::FoliageSystem(xy::MessageBus& mb)
    : xy::System    (mb, typeid(FoliageSystem)),
    m_windStrength  (0.06f),
    m_windIndex     (0)
{
    requireComponent<xy::Drawable>();
    requireComponent<Tree>();

    //windy wind
    auto noise = fn::NewFastNoiseSIMD();
    noise->SetFractalOctaves(5);
    noise->SetFrequency(0.015f);
    noise->SetFractalGain(0.2f);

    auto windNoise = noise->GetSimplexFractalSet(0, 0, 0, NoiseTableSize, 1, 1);
    for (auto i = 0u; i < NoiseTableSize; ++i)
    {
        m_windTable.push_back((windNoise[i] - 0.5f) * 0.25f);
    }
    fn::FreeNoiseSet(windNoise);
}

void FoliageSystem::process(float dt)
{
    m_windIndex = (m_windIndex + 1) % NoiseTableSize;
    
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& tree = entity.getComponent<Tree>();
        auto& drawable = entity.getComponent<xy::Drawable>();
        auto& verts = drawable.getVertices();
        verts.clear();

        auto offsetIndex = (m_windIndex + tree.indexOffset) % NoiseTableSize;
        auto offsetStride = (NoiseTableSize / tree.branches.size()) / 2;
        float width = TreeWidth;

        for (auto& branch : tree.branches)
        {
            branch.update(m_windTable[offsetIndex] * m_windStrength);
            offsetIndex = (offsetIndex + offsetStride) % NoiseTableSize;
            
            float yCoord = TrunkHeight;
            float coordStride = yCoord / branch.getSegments().size();

            sf::Vector2f lastPos = branch.getBasePosition();
            verts.emplace_back(sf::Vector2f(lastPos.x - width, lastPos.y)); //extra for degen tri
            verts.emplace_back(sf::Vector2f(lastPos.x - width, lastPos.y), sf::Vector2f(TrunkLeft, yCoord));
            verts.emplace_back(sf::Vector2f(lastPos.x + width, lastPos.y), sf::Vector2f(TrunkRight, yCoord));

            yCoord -= coordStride;

            for (const auto& p : branch.getSegments())
            {
                verts.emplace_back(p.getPosition() + (p.getNormal() * width), sf::Vector2f(TrunkLeft, yCoord));
                verts.emplace_back(p.getPosition() - (p.getNormal() * width), sf::Vector2f(TrunkRight, yCoord));

                //width *= 0.8f;
                yCoord -= coordStride;
            }
            auto dgen = verts.back();
            verts.push_back(dgen);

            //quad for leaves at top
            float leafWidth = (LeafWidth / 2.f) * (branch.getLength() / tree.height);
            float leafHeight = (LeafHeight / 2.f) * (branch.getLength() / tree.height);
            float textureOffset = branch.getTextureOffset();
            auto topPos = branch.getSegments().back().getPosition();
            verts.emplace_back(topPos - sf::Vector2f(-leafWidth, -leafHeight) * LeafScale);
            verts.emplace_back(topPos - sf::Vector2f(-leafWidth, -leafHeight) * LeafScale, sf::Vector2f(0.f, textureOffset));
            verts.emplace_back(topPos - sf::Vector2f(leafWidth, -leafHeight) * LeafScale, sf::Vector2f(LeafWidth, textureOffset));
            verts.emplace_back(topPos - sf::Vector2f(-leafWidth, leafHeight) * LeafScale, sf::Vector2f(0.f, LeafHeight+ textureOffset));
            verts.emplace_back(topPos - sf::Vector2f(leafWidth, leafHeight) * LeafScale, sf::Vector2f(LeafWidth, LeafHeight + textureOffset));
            verts.emplace_back(topPos - sf::Vector2f(leafWidth, leafHeight) * LeafScale);

            width = TreeWidth;
        }
        drawable.updateLocalBounds();
    }
}

void FoliageSystem::setWindStrength(float strength)
{
    m_windStrength = xy::Util::Math::clamp(strength, 0.f, 1.f);
}

void FoliageSystem::onEntityAdded(xy::Entity entity)
{
    float spacing = xy::Util::Random::value(60.f, 88.f);
    auto& tree = entity.getComponent<Tree>();
    std::size_t count = static_cast<std::size_t>(tree.width / spacing);

    for (auto i = 0u; i < count; ++i)
    {
        tree.branches.emplace_back(tree.height + xy::Util::Random::value(-40.f, 0.f), 4,
            sf::Vector2f(i * (spacing - xy::Util::Random::value(-(spacing / 15.f), (spacing / 6.f))), 0.f));
        tree.indexOffset = xy::Util::Random::value(20, NoiseTableSize);
    }

    //TODO make sure last tree is approx 80 units from the end
    auto backPos = tree.branches.back().getBasePosition();
    auto diff = tree.width - backPos.x;
    if (((diff - 80.f) > 20.f) || diff < 80.f)
    {
        backPos.x = tree.width - 80.f;
        tree.branches.back().setBasePosition(backPos);
    }
}