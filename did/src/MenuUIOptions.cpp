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

#include "MenuState.hpp"
#include "GlobalConsts.hpp"
#include "SliderSystem.hpp"
#include "SharedStateData.hpp"
#include "KeyMapping.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/UIHitBox.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/Drawable.hpp>

#include <xyginext/ecs/systems/UISystem.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>

#include <SFML/Window/Event.hpp>

namespace
{
    std::array<sf::Keyboard::Key, 4u> ReservedKeys =
    {
        sf::Keyboard::Escape, sf::Keyboard::Tab,
        sf::Keyboard::P, sf::Keyboard::Pause
    };

    //yeah I know - but we might reserve more in the future...
    std::array<std::uint32_t, 1u> ReservedButtons =
    {
        7
    };

    const std::string defaultMessage("Click on an input to change it");
}

void MenuState::buildOptions(sf::Font& font)
{
    auto mouseOver = m_callbackIDs[Menu::CallbackID::TextSelected];
    auto mouseOut = m_callbackIDs[Menu::CallbackID::TextUnselected];


    auto parentEntity = m_uiScene.createEntity();
    parentEntity.addComponent<xy::Transform>().setPosition(Menu::OffscreenPosition);
    parentEntity.addComponent<xy::CommandTarget>().ID = Menu::CommandID::OptionsNode;
    parentEntity.addComponent<Slider>().speed = Menu::SliderSpeed;

    //background
    auto entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Sprite>(m_textureResource.get("assets/images/controls_bg.png"));
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Far);
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setPosition((xy::DefaultSceneSize.x - bounds.width) / 2.f, 300.f);
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());
    auto bgEntity = entity;

    //plank
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::TitlePosition);
    entity.addComponent<xy::Sprite>() = m_sprites[Menu::SpriteID::TitleBar];
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Far);
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //title
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::TitlePosition);
    entity.getComponent<xy::Transform>().move(0.f, -46.f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
    entity.addComponent<xy::Text>(font).setString("Controls");
    entity.getComponent<xy::Text>().setCharacterSize(Global::LargeTextSize);
    entity.getComponent<xy::Text>().setFillColour(Global::InnerTextColour);
    entity.getComponent<xy::Text>().setOutlineColour(Global::OuterTextColour);
    entity.getComponent<xy::Text>().setOutlineThickness(1.f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
    bounds = xy::Text::getLocalBounds(entity);
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());


    //back button
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::BackButtonPosition);
    entity.addComponent<xy::Text>(font).setString("Back");
    entity.getComponent<xy::Text>().setCharacterSize(Global::MediumTextSize);
    entity.getComponent<xy::Text>().setFillColour(Global::InnerTextColour);
    entity.getComponent<xy::Text>().setOutlineColour(Global::OuterTextColour);
    entity.getComponent<xy::Text>().setOutlineThickness(1.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseEnter] = mouseOver;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseExit] = mouseOut;
    entity.getComponent<xy::UIHitBox>().area = Menu::ButtonArea;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseUp] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback([&, parentEntity](xy::Entity, sf::Uint64 flags) mutable
    {
        if (flags & xy::UISystem::Flags::LeftMouse)
        {
            parentEntity.getComponent<Slider>().target = Menu::OffscreenPosition;
            parentEntity.getComponent<Slider>().active = true;

            xy::Command cmd;
            cmd.targetFlags = Menu::CommandID::MainNode;
            cmd.action = [](xy::Entity e, float)
            {
                e.getComponent<Slider>().target = {};
                e.getComponent<Slider>().active = true;
            };
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
        }
    });
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //advanced button
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::StartButtonPosition);
    entity.addComponent<xy::Text>(font).setString("Advanced");
    entity.getComponent<xy::Text>().setCharacterSize(Global::MediumTextSize);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Right);
    entity.getComponent<xy::Text>().setFillColour(Global::InnerTextColour);
    entity.getComponent<xy::Text>().setOutlineColour(Global::OuterTextColour);
    entity.getComponent<xy::Text>().setOutlineThickness(1.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseEnter] = mouseOver;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseExit] = mouseOut;
    entity.getComponent<xy::UIHitBox>().area = Menu::ButtonArea;
    entity.getComponent<xy::UIHitBox>().area.left = -entity.getComponent<xy::UIHitBox>().area.width;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseUp] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback([&](xy::Entity, sf::Uint64 flags)
    {
        if (flags & xy::UISystem::Flags::LeftMouse)
        {
            xy::Console::show();
        }
    });
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    auto& finefont = m_fontResource.get(Global::FineFont);

    //info string
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize.x / 2.f, 800.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Text>(finefont).setString(defaultMessage);
    entity.getComponent<xy::Text>().setCharacterSize(40);
    entity.getComponent<xy::Text>().setFillColour(Global::InnerTextColour);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::CommandTarget>().ID = Menu::CommandID::OptionsInfoText;
    /*entity.getComponent<xy::Text>().setOutlineColour(Global::OuterTextColour);
    entity.getComponent<xy::Text>().setOutlineThickness(2.f);*/
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //keybinds
    sf::Vector2f currPos(664.f, 29.f);
    const float verticalOffset = 40.5f;
    const sf::FloatRect buttonArea(-86.f, 0.f, 172.f, 36.f);
    
    auto addKeyWindow = [&, buttonArea](std::int32_t binding, sf::Vector2f position)
    {
        auto ent = m_uiScene.createEntity();
        ent.addComponent<xy::Transform>().setPosition(position);
        ent.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
        ent.addComponent<xy::Text>(finefont).setString(KeyMapping.at(m_sharedData.inputBinding.keys[binding]));
        ent.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
        ent.getComponent<xy::Text>().setFillColour(Global::InnerTextColour);
        ent.getComponent<xy::Text>().setCharacterSize(Global::SmallTextSize);
        ent.addComponent<xy::CommandTarget>().ID = Menu::CommandID::KeybindButton;
        ent.addComponent<std::int32_t>() = binding;

        ent.addComponent<xy::UIHitBox>().area = buttonArea;
        ent.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
            m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback(
                [&, ent, binding](xy::Entity, sf::Uint64 flags) mutable
                {
                    if (flags & xy::UISystem::LeftMouse)
                    {
                        m_activeMapping.keybindActive = true;
                        m_activeMapping.keyDest = &m_sharedData.inputBinding.keys[binding];
                        m_activeMapping.displayEntity = ent;

                        ent.getComponent<xy::Text>().setString("?");

                        xy::Command cmd;
                        cmd.targetFlags = Menu::CommandID::OptionsInfoText;
                        cmd.action = [](xy::Entity e, float)
                        {
                            e.getComponent<xy::Text>().setString("Press a key");
                        };
                        m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                    }
                });


        bgEntity.getComponent<xy::Transform>().addChild(ent.getComponent<xy::Transform>());
    };

    addKeyWindow(InputBinding::Up, currPos);
    currPos.y += verticalOffset;
    addKeyWindow(InputBinding::Down, currPos);
    currPos.y += verticalOffset;
    addKeyWindow(InputBinding::Left, currPos);
    currPos.y += verticalOffset;
    addKeyWindow(InputBinding::Right, currPos);
    currPos.y += verticalOffset * 1.4f;

    addKeyWindow(InputBinding::CarryDrop, currPos);
    currPos.y += verticalOffset;
    addKeyWindow(InputBinding::Action, currPos);
    currPos.y += verticalOffset;
    addKeyWindow(InputBinding::Strafe, currPos);
    currPos.y += verticalOffset;
    addKeyWindow(InputBinding::WeaponPrev, currPos);
    currPos.y += verticalOffset;
    addKeyWindow(InputBinding::WeaponNext, currPos);
    currPos.y += verticalOffset;
    addKeyWindow(InputBinding::ZoomMap, currPos);
    currPos.y += verticalOffset;
    addKeyWindow(InputBinding::ShowMap, currPos);

    //joybuttons
    currPos = { 1154.f, 29.f };
    auto addJoyWindow = [&, buttonArea](std::int32_t binding, sf::Vector2f position)
    {
        auto ent = m_uiScene.createEntity();
        ent.addComponent<xy::Transform>().setPosition(position);
        ent.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
        ent.addComponent<xy::Text>(finefont).setAlignment(xy::Text::Alignment::Centre);
        ent.getComponent<xy::Text>().setFillColour(Global::InnerTextColour);

        if (binding == -1)
        {
            ent.getComponent<xy::Text>().setString("DPad");
        }
        else
        {
            ent.getComponent<xy::Text>().setString(JoyMapping[m_sharedData.inputBinding.buttons[binding]]);
            ent.addComponent<xy::UIHitBox>().area = buttonArea;
            ent.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
                m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback(
                    [&, ent, binding](xy::Entity, sf::Uint64 flags) mutable
                    {
                        if (flags & xy::UISystem::LeftMouse)
                        {
                            m_activeMapping.joybindActive = true;
                            m_activeMapping.joyButtonDest = &m_sharedData.inputBinding.buttons[binding];
                            m_activeMapping.displayEntity = ent;

                            ent.getComponent<xy::Text>().setString("?");

                            xy::Command cmd;
                            cmd.targetFlags = Menu::CommandID::OptionsInfoText;
                            cmd.action = [](xy::Entity e, float)
                            {
                                e.getComponent<xy::Text>().setString("Press a button");
                            };
                            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                        }
                    });

            ent.addComponent<xy::CommandTarget>().ID = Menu::CommandID::JoybindButton;
            ent.addComponent<std::int32_t>() = binding;
        }

        bgEntity.getComponent<xy::Transform>().addChild(ent.getComponent<xy::Transform>());
    };

    addJoyWindow(-1, currPos);
    currPos.y += verticalOffset;
    addJoyWindow(-1, currPos);
    currPos.y += verticalOffset;
    addJoyWindow(-1, currPos);
    currPos.y += verticalOffset;
    addJoyWindow(-1, currPos);
    currPos.y += verticalOffset * 1.4f;

    addJoyWindow(InputBinding::CarryDrop, currPos);
    currPos.y += verticalOffset;
    addJoyWindow(InputBinding::Action, currPos);
    currPos.y += verticalOffset;
    addJoyWindow(InputBinding::Strafe, currPos);
    currPos.y += verticalOffset;
    addJoyWindow(InputBinding::WeaponPrev, currPos);
    currPos.y += verticalOffset;
    addJoyWindow(InputBinding::WeaponNext, currPos);
    currPos.y += verticalOffset;
    addJoyWindow(InputBinding::ZoomMap, currPos);
    currPos.y += verticalOffset;
    addJoyWindow(InputBinding::ShowMap, currPos);

    //reset buttons
    auto mouseEnter = m_uiScene.getSystem<xy::UISystem>().addMouseMoveCallback(
        [&](xy::Entity, sf::Vector2f)
        {
            xy::Command cmd;
            cmd.targetFlags = Menu::CommandID::OptionsInfoText;
            cmd.action = [](xy::Entity e, float) {e.getComponent<xy::Text>().setString("Reset Controls"); };
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
        });
    auto mouseExit = m_uiScene.getSystem<xy::UISystem>().addMouseMoveCallback(
        [&](xy::Entity, sf::Vector2f)
        {
            xy::Command cmd;
            cmd.targetFlags = Menu::CommandID::OptionsInfoText;
            cmd.action = [](xy::Entity e, float) {e.getComponent<xy::Text>().setString(defaultMessage); };
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
        });

    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(210.f, 448.f);
    entity.addComponent<xy::UIHitBox>().area = { 0.f, 0.f, 64.f, 64.f };
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseEnter] = mouseEnter;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseExit] = mouseExit;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&](xy::Entity, sf::Uint64 flags) 
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    m_sharedData.inputBinding.keys =
                    {
                        sf::Keyboard::M,
                        sf::Keyboard::Z,
                        sf::Keyboard::Space,
                        sf::Keyboard::LControl,
                        sf::Keyboard::E,
                        sf::Keyboard::Q,
                        sf::Keyboard::LShift,
                        sf::Keyboard::A,
                        sf::Keyboard::D,
                        sf::Keyboard::W,
                        sf::Keyboard::S
                    };

                    xy::Command cmd;
                    cmd.targetFlags = Menu::CommandID::KeybindButton;
                    cmd.action = [&](xy::Entity e, float)
                    {
                        e.getComponent<xy::Text>().setString(KeyMapping.at(m_sharedData.inputBinding.keys[e.getComponent<std::int32_t>()]));
                    };
                    m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                }
            });
    bgEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(1536.f, 448.f);
    entity.addComponent<xy::UIHitBox>().area = { 0.f, 0.f, 64.f, 64.f };
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseEnter] = mouseEnter;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseExit] = mouseExit;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    m_sharedData.inputBinding.buttons =
                    {
                        {6, 2, 0, 1, 5, 3, 4} //Back, X, A, B , LB, RB, Y
                    };

                    xy::Command cmd;
                    cmd.targetFlags = Menu::CommandID::JoybindButton;
                    cmd.action = [&](xy::Entity e, float)
                    {
                        e.getComponent<xy::Text>().setString(JoyMapping[m_sharedData.inputBinding.buttons[e.getComponent<std::int32_t>()]]);
                    };
                    m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                }
            });
    bgEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());
}

void MenuState::handleKeyMapping(const sf::Event& evt)
{
    if (evt.type == sf::Event::KeyReleased)
    {
        //check existing bindings (but not the requested one)
        //to see if key code already bound
        bool keyFree = true;
        for (auto i = 0u; i < m_sharedData.inputBinding.keys.size(); ++i)
        {
            if ((m_sharedData.inputBinding.keys[i] != *m_activeMapping.keyDest
                && m_sharedData.inputBinding.keys[i] == evt.key.code)
                || std::find(ReservedKeys.begin(), ReservedKeys.end(), evt.key.code) != ReservedKeys.end())
            {
                //already bound!
                xy::Command cmd;
                cmd.targetFlags = Menu::CommandID::OptionsInfoText;
                cmd.action = [evt](xy::Entity e, float)
                {
                    e.getComponent<xy::Text>().setString(KeyMapping.at(evt.key.code) + " is already bound!");
                };
                m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                keyFree = false;
                break;
            }
        }

        if (keyFree)
        {
            *m_activeMapping.keyDest = evt.key.code;
            m_activeMapping.keyDest = nullptr;
            m_activeMapping.keybindActive = false;

            auto ent = m_activeMapping.displayEntity;
            m_activeMapping.displayEntity = {};

            ent.getComponent<xy::Text>().setString(KeyMapping.at(evt.key.code));

            xy::Command cmd;
            cmd.targetFlags = Menu::CommandID::OptionsInfoText;
            cmd.action = [](xy::Entity e, float)
            {
                e.getComponent<xy::Text>().setString(defaultMessage);
            };
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
        }
    }
}

void MenuState::handleJoyMapping(const sf::Event& evt)
{
    //check we didn't hit this by mistake and enable escape to cancel
        //otherwise a user will be buggered if they have no controller!
    if (evt.type == sf::Event::KeyReleased
        && evt.key.code == sf::Keyboard::Escape)
    {
        m_activeMapping.displayEntity.getComponent<xy::Text>().setString(JoyMapping[*m_activeMapping.joyButtonDest]);
        m_activeMapping.joyButtonDest = nullptr;
        m_activeMapping.displayEntity = { };
        m_activeMapping.joybindActive = false;

        xy::Command cmd;
        cmd.targetFlags = Menu::CommandID::OptionsInfoText;
        cmd.action = [](xy::Entity e, float)
        {
            e.getComponent<xy::Text>().setString(defaultMessage);
        };
        m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
    }
    else if (evt.type == sf::Event::JoystickButtonReleased
        && evt.joystickButton.joystickId == 0)
    {
        //check not existing binding
        bool buttonFree = true;
        for (auto i = 0u; i < m_sharedData.inputBinding.buttons.size(); ++i)
        {
            if ((m_sharedData.inputBinding.buttons[i] != *m_activeMapping.joyButtonDest
                && m_sharedData.inputBinding.buttons[i] == evt.joystickButton.button)
                || std::find(ReservedButtons.begin(), ReservedButtons.end(), evt.joystickButton.button) != ReservedButtons.end())
            {
                //already bound!
                xy::Command cmd;
                cmd.targetFlags = Menu::CommandID::OptionsInfoText;
                cmd.action = [evt](xy::Entity e, float)
                {
                    e.getComponent<xy::Text>().setString(JoyMapping[evt.joystickButton.button] + " is already bound!");
                };
                m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                buttonFree = false;
                break;
            }
        }

        if (buttonFree)
        {
            *m_activeMapping.joyButtonDest = evt.joystickButton.button;
            m_activeMapping.joyButtonDest = nullptr;
            m_activeMapping.joybindActive = false;

            auto ent = m_activeMapping.displayEntity;
            m_activeMapping.displayEntity = {};

            ent.getComponent<xy::Text>().setString(JoyMapping[evt.joystickButton.button]);

            xy::Command cmd;
            cmd.targetFlags = Menu::CommandID::OptionsInfoText;
            cmd.action = [](xy::Entity e, float)
            {
                e.getComponent<xy::Text>().setString(defaultMessage);
            };
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
        }
    }
}