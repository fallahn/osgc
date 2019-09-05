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

#include "BoatSystem.hpp"
#include "MessageIDs.hpp"
#include "PlayerSystem.hpp"
#include "AnimationSystem.hpp"

BoatSystem::BoatSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(BoatSystem))
{
    requireComponent<Boat>();
}

//public
void BoatSystem::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::PlayerMessage)
    {
        const auto& data = msg.getData<PlayerEvent>();
        if (data.action == PlayerEvent::StashedTreasure)
        {
            auto playerNumber = data.entity.getComponent<Player>().playerNumber;

            auto& entities = getEntities();

            for (auto& e : entities)
            {
                auto boatNumber = e.getComponent<Boat>().playerNumber;
                
                if (boatNumber == playerNumber)
                {
                    e.getComponent<AnimationModifier>().nextAnimation++;
                    break;
                }
            }

        }
        else if (data.action == PlayerEvent::StoleTreasure)
        {
            auto& entities = getEntities();
            for (auto& e : entities)
            {
                auto boatNumber = e.getComponent<Boat>().playerNumber;

                if (boatNumber == data.data)
                {
                    e.getComponent<AnimationModifier>().nextAnimation--;
                    break;
                }
            }
        }
    }
}