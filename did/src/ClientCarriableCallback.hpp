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

#include "CarriableSystem.hpp"
#include "PlayerSystem.hpp"
#include "GlobalConsts.hpp"
#include "InterpolationSystem.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/util/Vector.hpp>

class ClientCarriableCallback final
{
public:
    explicit ClientCarriableCallback(sf::Vector2f initialPos) : m_lastPosition(initialPos) {}

    void operator()(xy::Entity entity, float dt)
    {
        const auto& carriable = entity.getComponent<Carriable>();
        if (carriable.carried && carriable.parentEntity.isValid())
        {
            auto direction = carriable.parentEntity.getComponent<Actor>().direction;
            sf::Vector2f position;
            switch (direction)
            {
            case Player::Left:
                position.x = -Global::CarryOffset;
                position.y -= 0.5f;
                break;
            case Player::Right:
                position.x = Global::CarryOffset;
                position.y += 0.5f;
                break;
            case Player::Up:
                position.y = -Global::CarryOffset;
                break;
            case Player::Down:
                position.y = Global::CarryOffset;
                break;
            }
            position *= carriable.offsetMultiplier;
            position += carriable.parentEntity.getComponent<xy::Transform>().getPosition();

            //interpolate between last position and target to make turning smoother
            auto& tx = entity.getComponent<xy::Transform>();

            auto dir = position - m_lastPosition;
            auto len2 = xy::Util::Vector::lengthSquared(dir);
            if (len2 > 64.f && len2 < 1024.f)
            {
                position = tx.getPosition() + (dir * (dt * 20.f));
            }

            entity.getComponent<InterpolationComponent>().resetPosition(position);
            tx.setPosition(position);

            m_lastPosition = position;
        }
    }

private:
    sf::Vector2f m_lastPosition;
};