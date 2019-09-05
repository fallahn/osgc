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

#include <SFML/Graphics/Transformable.hpp>
#include <SFML/Graphics/Vertex.hpp>

#include <array>

//single parrot within a flock
struct Parrot final : public sf::Transformable
{
    sf::Vector2f velocity; //randomised when component initialised
    std::array<sf::Vertex, 4u> vertices = {};
    sf::FloatRect textureRect = {0.f, 0.f, 32.f, 42.f};
    std::size_t currentFrame = 0;
    float currentTime = 0.f;
    static constexpr std::size_t MaxFrames = 5; //TODO this should really be loaded from sprite sheet
    static constexpr float TimePerFrame = 1.f / 24.f;
};

//parrot flock component
struct ParrotFlock final
{
    //float timeToRelease = 2.f;
    enum
    {
        Flying,
        Roosting
    }state = Roosting;

    static constexpr std::size_t ParrotCount = 4;
    std::array<Parrot, ParrotCount> parrots = {};
    std::size_t roostCount = ParrotCount;
   
    void start()
    {
        //if (state == Roosting && timeToRelease < 0)
        //conditional should be entirely server side
        {
            state = Flying;
            roostCount = 0;
        }
    }

    void reset()
    {
        roostCount = ParrotCount;
        state = Roosting;
        //timeToRelease = 5.f;
    }
};

class ParrotSystem final : public xy::System
{
public:
    explicit ParrotSystem(xy::MessageBus&);

    void process(float) override;

    void onEntityAdded(xy::Entity) override;

private:

};
