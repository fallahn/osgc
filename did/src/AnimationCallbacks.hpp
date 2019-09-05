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

#include "Sprite3D.hpp"
#include "MessageIDs.hpp"
#include "CollisionBounds.hpp"
#include "GameUI.hpp"
#include "AnimationSystem.hpp"

#include <xyginext/util/Wavetable.hpp>
#include <xyginext/util/Vector.hpp>
#include <xyginext/ecs/Entity.hpp>
#include <xyginext/ecs/Scene.hpp>
#include <xyginext/ecs/components/Callback.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/AudioEmitter.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>

namespace Wave
{
    static std::vector<float> sine;
}

class CollectibleAnimationCallback final
{
public:
    CollectibleAnimationCallback()
    {
        if (Wave::sine.empty())
        {
            Wave::sine = xy::Util::Wavetable::sine(0.65f);

            for (auto& f : Wave::sine)
            {
                f -= 1.f;
                f *= 2.f;
            }
        }
    }

    void operator()(xy::Entity entity, float)
    {
        entity.getComponent<Sprite3D>().verticalOffset = Wave::sine[m_currentIndex];
        m_currentIndex = (m_currentIndex + 1) % Wave::sine.size();
    }

private:
    std::size_t m_currentIndex = 0;
};

class RedFlashCallback final
{
public:
    static constexpr float FlashTime = 0.1f;
    RedFlashCallback() = default;

    void operator ()(xy::Entity entity, float dt)
    {
        m_time -= dt;
        if (m_time < 0)
        {
            m_time = FlashTime;
            entity.getComponent<xy::Sprite>().setColour(sf::Color::White);
            entity.getComponent<xy::Callback>().active = false;
        }
    }

private:
    float m_time = FlashTime;
};

class GhostFloatCallback final
{
public:
    static constexpr float FadeTime = 2.f;
    static constexpr float MoveSpeed = 10.f;

    explicit GhostFloatCallback(xy::Scene& scene) : m_scene(scene) {}

    void operator()(xy::Entity entity, float dt)
    {
        entity.getComponent<Sprite3D>().verticalOffset -= MoveSpeed * dt;

        float alpha = std::max(0.f, m_fadeTime / FadeTime);

        sf::Uint8 a = static_cast<sf::Uint8>(alpha * 255.f);
        sf::Color c(255, 255, 255, a);
        entity.getComponent<xy::Sprite>().setColour(c);

        m_fadeTime -= dt;
        if (alpha <= 0)
        {
            m_scene.destroyEntity(entity);
        }
    }

private:
    float m_fadeTime = FadeTime;
    xy::Scene& m_scene;
};

class UISlideCallback final
{
public:
    explicit UISlideCallback(xy::MessageBus& mb) : m_messageBus(mb) {};

    void operator() (xy::Entity entity, float dt)
    {
        auto& tx = entity.getComponent<xy::Transform>();
        
        auto dest = entity.getComponent<sf::Vector2f>();
        auto position = tx.getPosition();
        auto travel = dest - position;

        static const float speed = 10.f;

        if (xy::Util::Vector::lengthSquared(travel) < 4.f)
        {
            tx.setPosition(dest);
            entity.getComponent<xy::Callback>().active = false;

            auto* msg = m_messageBus.post<SceneEvent>(MessageID::SceneMessage);
            msg->type = SceneEvent::SlideFinished;
            msg->id = entity.getComponent<xy::CommandTarget>().ID;
        }
        else
        {
            tx.move(travel * dt * speed);
        }
    }

private:
    xy::MessageBus& m_messageBus;
};

class ServerMessageCallback final
{
public:
    explicit ServerMessageCallback(xy::Scene& scene, xy::MessageBus& mb)
        : m_scene(scene), m_messageBus(mb) {}

    void operator () (xy::Entity entity, float dt)
    {
        switch (m_state)
        {
        default:
        case In:
        {
            entity.getComponent<xy::Transform>().move(0.f, InSpeed * dt);
            if (entity.getComponent<xy::Transform>().getPosition().y > 10.f)
            {
                m_state = Hold;

                //raise a message to say we finished moving in
                auto* msg = m_messageBus.post<SceneEvent>(MessageID::SceneMessage);
                msg->type = SceneEvent::MessageDisplayFinished;
            }

            //pushes existing messages down
            auto yPos = entity.getComponent<xy::Transform>().getPosition().y;
            yPos += xy::Text::getLocalBounds(entity).height;

            xy::Command cmd;
            cmd.targetFlags = UI::CommandID::ServerMessage;
            cmd.action = [yPos, entity](xy::Entity e, float t)
            {
                if (e == entity) return; //don't move ourselves...

                auto& tx = e.getComponent<xy::Transform>();
                if (tx.getPosition().y < yPos)
                {
                    tx.move(0.f, InSpeed * t);
                }
            };
            m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
        }
            break;
        case Hold:
            m_time -= dt;
            if (m_time < 0)
            {
                m_state = Out;
            }
            break;
        case Out:
            entity.getComponent<xy::Transform>().move(0.f, OutSpeed * dt);
            alpha = std::max(0.f, alpha - (dt * 3.f));
            
            auto colour = entity.getComponent<xy::Text>().getFillColour();
            colour.a = static_cast<sf::Uint8>(255.f * alpha);
            entity.getComponent<xy::Text>().setFillColour(colour);

            if (alpha == 0)
            {
                m_scene.destroyEntity(entity);
            }
            break;
        }
    }

private:
    enum State
    {
        In, Hold, Out
    }m_state = In;

    float m_time = 2.5f;
    xy::Scene& m_scene;
    xy::MessageBus& m_messageBus;
    float alpha = 1.f;

    static constexpr float InSpeed = 220.f;
    static constexpr float OutSpeed = 40.f;
};

class FadeoutCallback final
{
public:

    void operator()(xy::Entity entity, float dt)
    {
        fadeTime = std::max(0.f, fadeTime - dt);
        float alpha = 255.f * fadeTime;
        entity.getComponent<xy::Sprite>().setColour({ 255, 255, 255, static_cast<sf::Uint8>(alpha) });
    }
private:
    float fadeTime = 1.f;
};

class BarrelAudioCallback final
{
public:

    void operator() (xy::Entity entity, float)
    {
        auto offset = entity.getComponent<CollisionComponent>().water;
        if (offset == 0 && offset != m_lastOffset)
        {
            entity.getComponent<xy::AudioEmitter>().play();
            entity.getComponent<xy::Callback>().active = false;
        }
        m_lastOffset = offset;
    }
private:
    float m_lastOffset = -1000.f;
};

class FlameAnimationCallback final
{
public:
    explicit FlameAnimationCallback(xy::Scene& scene) : m_scene(scene) {}
    void operator() (xy::Entity entity, float dt)
    {
        if (entity.getComponent<xy::Transform>().getDepth() == 0)
        {
            if (m_depth == 1)
            {
                m_depth = 0;
                entity.getComponent<AnimationModifier>().nextAnimation = AnimationID::Die;
                entity.getComponent<Sprite3D>().verticalOffset = 0.f;
                entity.getComponent<xy::Transform>().setPosition(m_worldPosition);
                entity.getComponent<xy::Transform>().setScale(0.5f, -0.5f);
            }
            m_dieTime -= dt;

            if (m_dieTime < 0)
            {
                m_scene.destroyEntity(entity);
            }
        }

        m_worldPosition = entity.getComponent<xy::Transform>().getWorldPosition();
    }

private:
    xy::Scene& m_scene;
    float m_dieTime = 1.f;
    std::size_t m_depth = 1;
    sf::Vector2f m_worldPosition;
};