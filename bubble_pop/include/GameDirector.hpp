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
#include "LevelData.hpp"
#include "Animations.hpp"

#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/Director.hpp>
#include <xyginext/graphics/BitmapFont.hpp>

#include <SFML/System/Clock.hpp>

#include <array>

struct NodeSet;
class GameDirector final : public xy::Director
{
public:
    GameDirector(NodeSet&, const std::array<xy::Sprite, BubbleID::Count>&, const std::array<AnimationMap<AnimID::Bubble::Count>, BubbleID::Count>&);

    void handleMessage(const xy::Message&) override;
    void process(float) override;

    void loadLevelData();
    void activateLevel();

    void reset();

private:
    NodeSet& m_nodeSet;
    const std::array<xy::Sprite, BubbleID::Count>& m_sprites;
    const std::array<AnimationMap<AnimID::Bubble::Count>, BubbleID::Count>& m_animationMaps;

    std::vector<LevelData> m_levels;
    std::size_t m_currentLevel;
    std::size_t m_bubbleGeneration;
    std::vector<std::int32_t> m_queue;

    sf::Clock m_roundTimer;
    sf::Clock m_scoreTimer;

    std::size_t m_score;
    std::size_t m_previousScore;

    void queueBubble();
    void mountBubble();

    void loadNextLevel(std::size_t);
    void updateScoreDisplay();
};