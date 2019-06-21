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

#include "EliminationDotSystem.hpp"
#include "GameConsts.hpp"
#include "MessageIDs.hpp"
#include "CommandIDs.hpp"

#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/ecs/Scene.hpp>

#include <SFML/Graphics/Texture.hpp>

EliminationPointSystem::EliminationPointSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(EliminationPointSystem)),
    m_suddenDeath(false)
{
    requireComponent<EliminationDot>();
    requireComponent<xy::Drawable>();
}

//public
void EliminationPointSystem::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::GameMessage)
    {
        const auto& data = msg.getData<GameEvent>();
        if (data.type == GameEvent::PlayerScored
            && !m_suddenDeath)
        {
            xy::Command cmd;
            cmd.targetFlags = CommandID::EliminationDot;
            cmd.action = [data](xy::Entity e, float)
            {
                if (e.getComponent<EliminationDot>().ID == data.playerID)
                {
                    auto& verts = e.getComponent<xy::Drawable>().getVertices();
                    for (auto i = 0; i < data.score * 4; ++i)
                    {
                        verts[i].color = GameConst::PlayerColour::Light[data.playerID];
                    }
                }
            };
            getScene()->getSystem<xy::CommandSystem>().sendCommand(cmd);
        }
        else if (data.type == GameEvent::SuddenDeath)
        {
            m_suddenDeath = true;

            xy::Command cmd;
            cmd.targetFlags = CommandID::EliminationDot;
            cmd.action = [](xy::Entity e, float)
            {
                auto& verts = e.getComponent<xy::Drawable>().getVertices();
                for (auto i = 0u; i < verts.size(); ++i)
                {
                    verts[i].color = sf::Color(127, 127, 127);
                }
            };
            getScene()->getSystem<xy::CommandSystem>().sendCommand(cmd);
        }
    }
}

void EliminationPointSystem::process(float)
{

}

//private
void EliminationPointSystem::onEntityAdded(xy::Entity entity)
{
    auto id = entity.getComponent<EliminationDot>().ID;

    auto& drawable = entity.getComponent<xy::Drawable>();
    auto size = sf::Vector2f(drawable.getTexture()->getSize());
    auto& verts = drawable.getVertices();

    const float spacing = size.y -8.f;
    float position = 0.f;

    for (auto i = 0; i < 4; ++i)
    {
        verts.emplace_back(sf::Vector2f(0.f, position), GameConst::PlayerColour::Dark[id], sf::Vector2f());
        verts.emplace_back(sf::Vector2f(size.x, position), GameConst::PlayerColour::Dark[id], sf::Vector2f(size.x, 0.f));
        verts.emplace_back(sf::Vector2f(size.x, position + size.y), GameConst::PlayerColour::Dark[id], size);
        verts.emplace_back(sf::Vector2f(0.f, position + size.y), GameConst::PlayerColour::Dark[id], sf::Vector2f(0.f, size.y));

        position += spacing;
    }

    drawable.updateLocalBounds();
}