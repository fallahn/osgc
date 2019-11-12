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

#include "PlayerDirector.hpp"
#include "MessageIDs.hpp"
#include "AnimationID.hpp"
#include "GameConst.hpp"
#include "WallData.hpp"
#include "CommandIDs.hpp"
#include "CameraTransportSystem.hpp"

#include <xyginext/ecs/Scene.hpp>
#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/BroadPhaseComponent.hpp>

#include <xyginext/util/Random.hpp>
#include <xyginext/util/Vector.hpp>

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>

namespace
{
    //TODO bindable keymap
    const sf::Keyboard::Key left = sf::Keyboard::A;
    const sf::Keyboard::Key right = sf::Keyboard::D;
    const sf::Keyboard::Key up = sf::Keyboard::W; //pressing up inspects something behind the player

    enum InputFlags
    {
        Up = 0x1,
        Left = 0x2,
        Right = 0x4
    };

    const float PlayerSpeed = 180.f;

    const std::array<sf::Vector2f, 4u> Offsets =
    {
        sf::Vector2f(0.f, -1.f),
        sf::Vector2f(1.f, 0.f),
        sf::Vector2f(0.f, 1.f),
        sf::Vector2f(-1.f, 0.f)
    };
    const float OffsetDistance = 128.f;

    const sf::FloatRect ClickArea(0.f, 736.f, 1920.f, 1080.f - 736.f);

    sf::Vector2f roomPosition(std::int32_t room)
    {
        auto x = room % GameConst::RoomsPerRow;
        auto y = room / GameConst::RoomsPerRow;
        return { x * GameConst::RoomWidth, y * GameConst::RoomWidth };
    }
}

PlayerDirector::PlayerDirector(const xy::Scene& uiScene)
    : m_uiScene         (uiScene),
    m_cameraLocked      (false),
    m_cameraDirection   (0),
    m_inputFlags        (0),
    m_currentRoom       (0),
    m_walkToTarget      (false),
    m_nextAnimTime      (sf::seconds(xy::Util::Random::value(5, 12)))
{

}

//public
void PlayerDirector::handleEvent(const sf::Event& evt)
{
#ifdef XY_DEBUG
    if (evt.type == sf::Event::KeyPressed)
    {
        switch (evt.key.code)
        {
        default: break;
        case left:
            m_inputFlags |= Left;
            break;
        case right:
            m_inputFlags |= Right;
            break;
        case up:
            m_inputFlags |= Up;
            break;
        }
    }
    else if (evt.type == sf::Event::KeyReleased)
    {
        switch (evt.key.code)
        {
        default: break;
        case left:
            m_inputFlags &= ~Left;
            break;
        case right:
            m_inputFlags &= ~Right;
            break;
        case up:
            m_inputFlags &= ~Up;
            break;
        }
    }
#endif

    if (evt.type == sf::Event::MouseButtonReleased)
    {
        const auto& cam = m_uiScene.getActiveCamera().getComponent<xy::Camera>();
        sf::View view(xy::DefaultSceneSize / 2.f, cam.getView());
        view.setViewport(cam.getViewport());
        auto pos = xy::App::getActiveInstance()->getRenderWindow()->mapPixelToCoords({ evt.mouseButton.x, evt.mouseButton.y }, view);
        
        if (ClickArea.contains(pos))
        {
            pos.x *= 2.f;
            pos.x -= xy::DefaultSceneSize.x;
            pos.x /= 4.f;

            auto roomPos = roomPosition(m_currentRoom);
            switch (m_cameraDirection)
            {
            default: break;
            case CameraEvent::N:
                roomPos.x += pos.x;
                break;
            case CameraEvent::E:
                roomPos.y += pos.x;
                break;
            case CameraEvent::S:
                roomPos.x -= pos.x;
                break;
            case CameraEvent::W:
                roomPos.y -= pos.x;
                break;
            }
            m_targetWalkPosition = roomPos;
            m_walkToTarget = true;
        }
    }
}

void PlayerDirector::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::CameraMessage)
    {
        const auto& data = msg.getData<CameraEvent>();
        if (data.type == CameraEvent::Locked)
        {
            m_cameraLocked = true;

            //snap player to target position
            m_playerEntity.getComponent<xy::Transform>().setPosition(m_targetScreenPosition);
        }
        else if (data.type == CameraEvent::Unlocked)
        {
            m_cameraLocked = false;

            if (data.direction != m_cameraDirection)
            {
                //we're rotating - set interp dest to centre of the room
                //on the new depth axis, and restore to previous position on width axis
                switch (data.direction)
                {
                default: break;
                case CameraEvent::N:
                case CameraEvent::S:
                    m_targetScreenPosition.x = m_realWorldPosition.x;
                    m_targetScreenPosition.y = (m_currentRoom / GameConst::RoomsPerRow) * GameConst::RoomWidth;
                    
                    break;
                case CameraEvent::E:
                case CameraEvent::W:
                    m_targetScreenPosition.y = m_realWorldPosition.y;
                    m_targetScreenPosition.x = (m_currentRoom % GameConst::RoomsPerRow) * GameConst::RoomWidth;
                    break;
                }
                //m_targetScreenPosition += Offsets[data.direction] * OffsetDistance;
            }
            //translating the camera is take care of by collision trigger, below

            m_cameraDirection = data.direction;
        }
    }
}

void PlayerDirector::process(float dt)
{
    if (m_playerEntity.isValid())
    {
        if (m_walkToTarget)
        {
            auto dir = m_targetWalkPosition - m_playerEntity.getComponent<xy::Transform>().getPosition();
            auto dist = xy::Util::Vector::lengthSquared(dir);
            if (dist > 3)
            {
                switch (m_cameraDirection)
                {
                default: break;
                case CameraEvent::N:
                    if (dir.x > 0)
                    {
                        m_inputFlags |= InputFlags::Right;
                        m_inputFlags &= ~InputFlags::Left;
                    }
                    else
                    {
                        m_inputFlags |= InputFlags::Left;
                        m_inputFlags &= ~InputFlags::Right;
                    }
                    break;
                case CameraEvent::E:
                    if (dir.y > 0)
                    {
                        m_inputFlags |= InputFlags::Right;
                        m_inputFlags &= ~InputFlags::Left;
                    }
                    else
                    {
                        m_inputFlags |= InputFlags::Left;
                        m_inputFlags &= ~InputFlags::Right;
                    }
                    break;
                case CameraEvent::S:
                    if (dir.x < 0)
                    {
                        m_inputFlags |= InputFlags::Right;
                        m_inputFlags &= ~InputFlags::Left;
                    }
                    else
                    {
                        m_inputFlags |= InputFlags::Left;
                        m_inputFlags &= ~InputFlags::Right;
                    }
                    break;
                case CameraEvent::W:
                    if (dir.y < 0)
                    {
                        m_inputFlags |= InputFlags::Right;
                        m_inputFlags &= ~InputFlags::Left;
                    }
                    else
                    {
                        m_inputFlags |= InputFlags::Left;
                        m_inputFlags &= ~InputFlags::Right;
                    }
                    break;
                }
            }
            else
            {
                m_walkToTarget = false;
                m_inputFlags = 0;
            }
        }


        sf::Vector2f velocity; //used below for updating animation
        if (m_cameraLocked)
        {
            //check input
            if (m_inputFlags != 0)
            {
                if (m_inputFlags & InputFlags::Left)
                {
                    switch (m_cameraDirection)
                    {
                    default: break;
                    case CameraEvent::N:
                        velocity.x += -1.f;
                        break;
                    case CameraEvent::E:
                        velocity.y += -1.f;
                        break;
                    case CameraEvent::S:
                        velocity.x += 1.f;
                        break;
                    case CameraEvent::W:
                        velocity.y += 1.f;
                        break;
                    }
                }

                if (m_inputFlags & InputFlags::Right)
                {
                    switch (m_cameraDirection)
                    {
                    default: break;
                    case CameraEvent::N:
                        velocity.x += 1.f;
                        break;
                    case CameraEvent::E:
                        velocity.y += 1.f;
                        break;
                    case CameraEvent::S:
                        velocity.x += -1.f;
                        break;
                    case CameraEvent::W:
                        velocity.y += -1.f;
                        break;
                    }
                }

                auto& tx = m_playerEntity.getComponent<xy::Transform>();
                tx.move(velocity * PlayerSpeed * dt);

                doCollision(); //this might mutate our position

                switch (m_cameraDirection)
                {
                default: break;
                case CameraEvent::N:
                case CameraEvent::S:
                    m_realWorldPosition.x = tx.getPosition().x;
                    break;
                case CameraEvent::E:
                case CameraEvent::W:
                    m_realWorldPosition.y = tx.getPosition().y;
                    break;
                }

                //TODO inspect stuff if UP is released
            }
        }
        else
        {
            //camera is moving so interp to dest point
            auto& tx = m_playerEntity.getComponent<xy::Transform>();
            auto dist = m_targetScreenPosition - tx.getPosition();
            tx.move(dist * dt * GameConst::CamRotateSpeed); //TODO decide if we should be using rot or trans speed
        }

        //update the player animations
        const auto& animMap = m_playerEntity.getComponent<AnimationMap>();
        auto& animation = m_playerEntity.getComponent<xy::SpriteAnimation>();
        if (animation.getAnimationIndex() == animMap[AnimationID::Idle]
            && m_animationTimer.getElapsedTime() > m_nextAnimTime)
        {
            m_nextAnimTime = sf::seconds(xy::Util::Random::value(6, 14));
            m_animationTimer.restart();
            animation.play(animMap[AnimationID::Scratch]);
        }
        else if (animation.getAnimationIndex() == animMap[AnimationID::Scratch]
            && animation.stopped())
        {
            animation.play(animMap[AnimationID::Idle]);
        }

        if (xy::Util::Vector::lengthSquared(velocity) == 0
            && xy::Util::Vector::lengthSquared(m_previousVelocity) != 0)
        {
            //we stop moving
            animation.play(animMap[AnimationID::Idle]);
            m_animationTimer.restart();
        }
        else if (xy::Util::Vector::lengthSquared(velocity) != 0
            && xy::Util::Vector::lengthSquared(m_previousVelocity) == 0)
        {
            if (m_inputFlags & InputFlags::Left)
            {
                animation.play(animMap[AnimationID::Left]);
            }
            else
            {
                animation.play(animMap[AnimationID::Right]);
            }
        }

        m_previousVelocity = velocity;
    }
}

void PlayerDirector::setPlayerEntity(xy::Entity entity)
{
    m_playerEntity = entity; 
    m_realWorldPosition = entity.getComponent<xy::Transform>().getPosition();
    m_targetScreenPosition = m_realWorldPosition;
}

//private
void PlayerDirector::playAnimation(std::size_t id)
{

}

void PlayerDirector::doCollision()
{
    //in theory we know which room we're in
    //so we ought to be able to pick the 4 triggers
    //belonging to it rather than query the dynamic tree

    const auto pos = m_playerEntity.getComponent<xy::Transform>().getPosition();
    const sf::FloatRect queryBounds(pos.x - 64.f, pos.y - 64.f, 128.f, 128.f);
    auto nearby = getScene().getSystem<xy::DynamicTreeSystem>().query(queryBounds);
    for (auto otherEnt : nearby)
    {
        auto otherPos = otherEnt.getComponent<xy::Transform>().getWorldPosition();
        auto otherBounds = otherEnt.getComponent<xy::BroadphaseComponent>().getArea();
        otherBounds.left += otherPos.x;
        otherBounds.top += otherPos.y;

        sf::FloatRect intersection;
        if (queryBounds.intersects(otherBounds, intersection))
        {
            m_walkToTarget = false;
            m_inputFlags = 0;

            const auto& wallData = otherEnt.getComponent<WallData>();
            if (wallData.passable)
            {
                m_targetScreenPosition = wallData.targetPoint;
                m_realWorldPosition = wallData.targetPoint;

                //trigger cam shift based on current direction
                //and update new room number
                auto shiftCam = [&](bool moveLeft)
                {
                    xy::Command cmd;
                    cmd.targetFlags = CommandID::Camera;
                    cmd.action = [moveLeft](xy::Entity e, float)
                    {
                        e.getComponent<CameraTransport>().move(moveLeft);
                    };
                    getScene().getSystem<xy::CommandSystem>().sendCommand(cmd);
                };

                switch (m_cameraDirection)
                {
                default: break;
                case CameraEvent::N:
                    if (pos.x > otherPos.x)
                    {
                        m_currentRoom++;
                        shiftCam(false);
                    }
                    else
                    {
                        m_currentRoom--;
                        shiftCam(true);
                    }
                    break;
                case CameraEvent::E:
                    if (pos.y > otherPos.y)
                    {
                        m_currentRoom += GameConst::RoomsPerRow;
                        shiftCam(false);
                    }
                    else
                    {
                        m_currentRoom -= GameConst::RoomsPerRow;
                        shiftCam(true);
                    }
                    break;
                case CameraEvent::S:
                    if (pos.x < otherPos.x)
                    {
                        m_currentRoom--;
                        shiftCam(false);
                    }
                    else
                    {
                        m_currentRoom++;
                        shiftCam(true);
                    }
                    break;
                case CameraEvent::W:
                    if (pos.y < otherPos.y)
                    {
                        m_currentRoom -= GameConst::RoomsPerRow;
                        shiftCam(false);
                    }
                    else
                    {
                        m_currentRoom += GameConst::RoomsPerRow;
                        shiftCam(true);
                    }
                    break;
                }
                m_cameraLocked = false; //pre-empt this so we don't get double collisions.
            }
            else //solid collision
            {
                switch (m_cameraDirection)
                {
                default: break;
                case CameraEvent::N:
                    if (pos.x < otherPos.x)
                    {
                        m_playerEntity.getComponent<xy::Transform>().move(intersection.width, 0.f);
                    }
                    else
                    {
                        m_playerEntity.getComponent<xy::Transform>().move(-intersection.width, 0.f);
                    }
                    break;
                case CameraEvent::E:
                    if (pos.y < otherPos.y)
                    {
                        m_playerEntity.getComponent<xy::Transform>().move(0.f, intersection.height);
                    }
                    else
                    {
                        m_playerEntity.getComponent<xy::Transform>().move(0.f, -intersection.height);
                    }
                    break;
                case CameraEvent::S:
                    if (pos.x < otherPos.x)
                    {
                        m_playerEntity.getComponent<xy::Transform>().move(intersection.width, 0.f);
                    }
                    else
                    {
                        m_playerEntity.getComponent<xy::Transform>().move(-intersection.width, 0.f);
                    }
                    break;
                case CameraEvent::W:
                    if (pos.y < otherPos.y)
                    {
                        m_playerEntity.getComponent<xy::Transform>().move(0.f, intersection.height);
                    }
                    else
                    {
                        m_playerEntity.getComponent<xy::Transform>().move(0.f, -intersection.height);
                    }
                    break;
                }
            }

            break;
        }
    }
}