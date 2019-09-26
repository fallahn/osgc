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

#include <xyginext/ecs/System.hpp>

#include <SFML/System/Vector2.hpp>

#include <vector>

class TreeSegment final
{
public:
    explicit TreeSegment(float length);

    sf::Vector2f getEnd() const;

    void setRotation(float);

    void setPosition(sf::Vector2f);

    sf::Vector2f getPosition() const;

    sf::Vector2f getNormal() const;

private:
    float m_length;
    float m_rotation;
    sf::Vector2f m_position;
    mutable sf::Vector2f m_normal;
};

class TreeBranch final
{
public:
    TreeBranch(float length, std::size_t segmentCount, sf::Vector2f basePosition);

    void update(float windStrength);

    const std::vector<TreeSegment>& getSegments() const;

    sf::Vector2f getBasePosition() const;

    float getLength() const;

    void setBasePosition(sf::Vector2f);

    float getTextureOffset() const { return m_textureOffset; }

private:
    std::vector<TreeSegment> m_segments;
    sf::Vector2f m_basePosition;
    float m_rotation;
    float m_length;
    float m_textureOffset;
};

struct Tree final
{
    std::vector<TreeBranch> branches;
    float width = 0.f;
    float height = 96.f; //total 128 when you include the leaves
    std::size_t indexOffset = 0;
};


class FoliageSystem final : public xy::System
{
public:
    explicit FoliageSystem(xy::MessageBus&);

    void process(float) override;

    void setWindStrength(float);

    void onEntityAdded(xy::Entity) override;

private:
    float m_windStrength;

    std::vector<float> m_windTable;
    std::size_t m_windIndex;
};