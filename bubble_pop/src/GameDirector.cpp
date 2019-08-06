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

#include "GameDirector.hpp"
#include "NodeSet.hpp"
#include "BubbleSystem.hpp"
#include "GameConsts.hpp"
#include "CommandID.hpp"
#include "MessageIDs.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>

#include <xyginext/ecs/Scene.hpp>

#include <xyginext/util/Random.hpp>

GameDirector::GameDirector(NodeSet& ns, const std::array<xy::Sprite, BubbleID::Count>& sprites)
    : m_nodeSet (ns),
    m_sprites   (sprites)
{

}

//public
void GameDirector::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::BubbleMessage)
    {
        const auto& data = msg.getData<BubbleEvent>();
        switch (data.type)
        {
        default: break;
        case BubbleEvent::Fired:
        {
            xy::Command cmd;
            cmd.targetFlags = CommandID::Bubble;
            cmd.action = [&,data](xy::Entity e, float)
            {
                auto& bubble = e.getComponent<Bubble>();
                if (bubble.state == Bubble::State::Mounted)
                {
                    bubble.state = Bubble::State::Firing;
                    bubble.velocity = data.velocity;

                    auto& tx = e.getComponent<xy::Transform>();
                    m_nodeSet.gunNode.getComponent<xy::Transform>().removeChild(tx);
                    tx.setPosition(m_nodeSet.gunNode.getComponent<xy::Transform>().getPosition());
                    m_nodeSet.rootNode.getComponent<xy::Transform>().addChild(tx);

                    mountBubble(); //only mount a bubble if we fired one
                }
            };
            sendCommand(cmd);
        }
            break;
        }
    }
}

void GameDirector::process(float)
{
    //TODO track round time and lower bar

    //TODO check bubble collision with red area on bar move
}

void GameDirector::setQueue(const std::vector<std::int32_t>& q)
{
    m_queue = q;
    queueBubble();
}

//private
void GameDirector::queueBubble()
{
    auto id = xy::Util::Random::value(BubbleID::Red, BubbleID::Grey);
    if (!m_queue.empty())
    {
        id = m_queue.back();
        m_queue.pop_back();
    }

    //place bubble at spawn point
    auto origin = Const::BubbleSize / 2.f;
    auto entity = getScene().createEntity();
    entity.addComponent<xy::Transform>().setPosition(Const::BubbleQueuePosition);
    entity.getComponent<xy::Transform>().setOrigin(origin);
    entity.addComponent<xy::Drawable>().setDepth(1);
    entity.addComponent<xy::Sprite>() = m_sprites[id];
    entity.addComponent<Bubble>().state = Bubble::State::Queued;
    entity.addComponent<xy::CommandTarget>().ID = CommandID::Bubble;

    m_nodeSet.rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());
}

void GameDirector::mountBubble()
{
    xy::Command cmd;
    cmd.targetFlags = CommandID::Bubble;
    cmd.action = [&](xy::Entity e, float)
    {
        if (e.getComponent<Bubble>().state == Bubble::State::Queued)
        {
            e.getComponent<Bubble>().state = Bubble::State::Queuing;
            queueBubble();
        }
    };
    sendCommand(cmd);
}