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
#include <array>

#include "InputParser.hpp"

struct Player final
{
    sf::Vector2f spawnPosition;
    float coolDown = 0.f;
    float respawn = 3.f;
    float drownTime = 0.f;
    sf::Vector2f previousMovement;
    sf::Vector2f deathPosition; //used by client to create effects

    float curseTimer = 0.f;

    enum Flags
    {
        CanDoAction = 0x1,
        Strafing = 0x2,
        Cursed = 0x4 //reverses controls
        //bit kludgy here as we also include
        //carriable flags from the 'Carrier' component
        //so remember if we add more here make sure
        //the carriable flags match up
        //also make sure to sync correctly in the PlayerSystem update
    };

    enum Direction
    {
        Up, Down, Left, Right
    };

    enum State
    {
        Alive, Dead
    };

    struct Sync final
    {
        float accel = 0.f;        
        std::uint16_t flags = CanDoAction;
        std::uint8_t direction = Direction::Down;
        std::uint8_t state = State::Alive;
    }sync;
    std::uint16_t previousInputFlags = 0;
    std::uint8_t playerNumber = 0;
};

struct ClientState;
class PlayerSystem final : public xy::System 
{
public:
    explicit PlayerSystem(xy::MessageBus&);

    void process(float) override;

    void reconcile(const ClientState&, xy::Entity);

private:

    sf::Vector2f parseInput(std::uint16_t, bool);
    float getDelta(const History&, std::size_t);
    sf::Vector2f processInput(Input, float, xy::Entity);

    void updateAnimation(sf::Vector2f direction, xy::Entity);

    void updateCollision(xy::Entity, float);
};