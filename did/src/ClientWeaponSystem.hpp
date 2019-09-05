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

//updates the weapon sprites client side

#include <xyginext/ecs/System.hpp>
#include <xyginext/ecs/Entity.hpp>

#include <array>

struct ClientWeapon final
{
    xy::Entity parent;
    sf::Uint8 prevWeapon = 10;
    sf::Uint8 nextWeapon = 2;
    sf::Uint8 prevDirection = 10; //force immediate update
    bool prevWalking = false;
    float spriteOffset = 0.f; //default vertical offset of sprite t orestor animation
};

namespace xy
{
    class SpriteSheet;
}

class ClientWeaponSystem final : public xy::System
{
public:
    explicit ClientWeaponSystem(xy::MessageBus&);

    void handleMessage(const xy::Message&) override;

    void process(float) override;

    void setAnimations(const xy::SpriteSheet&);

private:
    enum WeaponAnimationID
    {
        SwordSide,
        SwordFront,
        SwordIdleSide,
        SwordIdleFront,
        SwordIdleBack,
        SwordWalkSide,
        SwordWalkFront,
        SwordWalkBack,
        PistolSide,
        PistolBack,
        PistolFront,
        PistolIdleSide,
        PistolIdleBack,
        PistolIdleFront,
        PistolWalkSide,
        PistolWalkBack,
        PistolWalkFront,
        ShovelSide,
        ShovelBack,
        ShovelFront,
        ShovelIdleSide,
        ShovelIdleBack,
        ShovelIdleFront,
        ShovelWalkSide,
        ShovelWalkBack,
        ShovelWalkFront,
        NoneSide,
        NoneFront,
        //Hidden,
        Count
    };
    std::array<std::size_t, WeaponAnimationID::Count> m_animationIDs = {};

    //there are only ever 4 players so we'll double the size just
    std::array<sf::Uint32, 16u> m_entityQueue;
    std::size_t m_queueSize;
};