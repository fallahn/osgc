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

#include "ParrotLauncherSystem.hpp"
#include "CollisionBounds.hpp"
#include "Packet.hpp"
#include "ServerSharedStateData.hpp"
#include "Server.hpp"
#include "MessageIDs.hpp"

ParrotLauncherSystem::ParrotLauncherSystem(xy::MessageBus& mb, Server::SharedStateData& sd)
    : xy::System(mb, typeid(ParrotLauncherSystem)),
    m_sharedData(sd),
    m_dayTime   (0.f)
{
    requireComponent<ParrotLauncher>();
    requireComponent<CollisionComponent>();
}

//public
void ParrotLauncherSystem::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::MapMessage)
    {
        const auto& data = msg.getData<MapEvent>();
        if (data.type == MapEvent::DayNightUpdate)
        {
            //track cycle so only spawn at day
            m_dayTime = data.value;
        }
    }
}

void ParrotLauncherSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& launcher = entity.getComponent<ParrotLauncher>();
        if (launcher.nextLaunchTime > 0)
        {
            launcher.nextLaunchTime -= dt;
        }
        else
        {
            if (launcher.launched)
            {
                launcher.currentLaunchTime -= dt;
                if (launcher.currentLaunchTime < 0)
                {
                    launcher.launched = false;
                    launcher.nextLaunchTime = 5.f;
                }
            }
            else
            {
                //check for collision and launch
                const auto& collision = entity.getComponent<CollisionComponent>();
                for (auto i = 0u; i < collision.manifoldCount; ++i)
                {
                    if (collision.manifolds[i].ID == ManifoldID::Player
                        && isDay())
                    {
                        launcher.launched = true;
                        launcher.currentLaunchTime = 12.f;
                        m_sharedData.gameServer->broadcastData(PacketID::ParrotLaunch, entity.getIndex(), xy::NetFlag::Reliable);
                    }
                }
            }
        }
    }
}

//private
bool ParrotLauncherSystem::isDay() const
{
    return (m_dayTime > 0.25f && m_dayTime < 0.5f) || (m_dayTime > 0.75f && m_dayTime < 1.f);
}