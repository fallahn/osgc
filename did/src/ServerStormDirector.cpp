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

#include "ServerStormDirector.hpp"
#include "ServerRandom.hpp"
#include "ServerSharedStateData.hpp"
#include "Server.hpp"
#include "Packet.hpp"
#include "GlobalConsts.hpp"
#include "MessageIDs.hpp"
#include "Actor.hpp"

#include <xyginext/util/Random.hpp>

#include <array>

namespace
{
    const std::array<float, 12> WeatherTimes = 
    {
        110.f, 40.f, 150.f, 40.f, 130.f, 50.f,
        170.f, 50.f, 310.f, 50.f, 140.f, 60.f
    };

    const std::array<float, 12> LightningTimes = 
    {
        4.f, 6.f, 5.f, 8.f, 5.f, 9.f,
        3.f, 7.f, 6.f, 4.f, 5.f, 7.f
    };
}

StormDirector::StormDirector(Server::SharedStateData& sd)
    : m_sharedData      (sd),
    m_strikePointIndex  (0),
    m_strikeTimeIndex   (Server::getRandomInt(0, LightningTimes.size() - 1)),
    m_state             (StormDirector::Dry),
    m_stateIndex        (Server::getRandomInt(0, WeatherTimes.size() - 1))
{
    //poisson sample strike points
    m_strikePoints = xy::Util::Random::poissonDiscDistribution(
        { 0.f, 0.f, static_cast<float>(Global::TileCountX), static_cast<float>(Global::TileCountY) },
        8.f, 18, Server::rndEngine);

    //scale up from tile coords
    for (auto& point : m_strikePoints)
    {
        point *= Global::TileSize;
    }
}

//public
void StormDirector::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::SceneMessage)
    {
        const auto& data = msg.getData<SceneEvent>();
        if (data.type == SceneEvent::WeatherRequested)
        {
            setWeather(static_cast<State>(data.id));            
        }
    }
}

void StormDirector::process(float)
{
    if (m_stateClock.getElapsedTime().asSeconds() >
        WeatherTimes[m_stateIndex])
    {
        m_stateIndex = (m_stateIndex + 1) % WeatherTimes.size();
        
        auto state = (m_state == Dry) ? static_cast<State>(Wet + Server::getRandomInt(0, 1)) : Dry;
        setWeather(state);
    }

    if (m_state == State::Stormy)
    {
        //LIGHTNING!!
        if (m_strikeClock.getElapsedTime().asSeconds() >
            LightningTimes[m_strikeTimeIndex])
        {
            m_strikeClock.restart();
            m_strikeTimeIndex = (m_strikeTimeIndex + 1) % LightningTimes.size();

            auto pos = m_strikePoints[m_strikePointIndex];
            m_strikePointIndex = (m_strikePointIndex + 1) % m_strikePoints.size();

            auto* msg = postMessage<ActorEvent>(MessageID::ActorMessage);
            msg->type = ActorEvent::RequestSpawn;
            msg->id = Actor::ID::Lightning;
            msg->position = pos;
        }
    }
}

//private
void StormDirector::setWeather(State state)
{
    m_state = state;

    m_stateClock.restart();
    m_strikeClock.restart();

    m_sharedData.gameServer->broadcastData(PacketID::WeatherUpdate, std::uint8_t(m_state), xy::NetFlag::Reliable, Global::ReliableChannel);
    LOG("Set weather to " + std::to_string(m_state), xy::Logger::Type::Info);

    auto* msg = postMessage<SceneEvent>(MessageID::SceneMessage);
    msg->type = SceneEvent::WeatherChanged;
    msg->id = m_state;
}