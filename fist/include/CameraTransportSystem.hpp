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

#include "GameConst.hpp"

#include <xyginext/ecs/System.hpp>

struct CameraTransport final
{
    CameraTransport(std::int32_t = 0);

    void move(bool left);

    void rotate(bool left);

    float getCurrentRotation() const { return -currentRotation; }

    sf::Vector2f getCurrentDirection() const { return currentDirection; }

private:
    sf::Vector2f targetPosition;
    sf::Vector2f currentPosition;
    
    sf::Vector2f currentDirection = sf::Vector2f(0.f, -1.f);

    float targetRotation = 0.f;
    float currentRotation = 0.f;
    std::size_t m_currentRotationTarget = 0;

    bool active = false;

    friend class CameraTransportSystem;
};

class CameraTransportSystem final : public xy::System
{
public:
    explicit CameraTransportSystem(xy::MessageBus&);

    void process(float) override;

private:

};