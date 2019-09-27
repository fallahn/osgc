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

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/UIHitBox.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/Drawable.hpp>

#include <xyginext/ecs/systems/UISystem.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>

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
    entity.addComponent<xy::Sprite>(m_textureResource.get("assets/images/browser_window.png"));
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Far);
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setPosition((xy::DefaultSceneSize.x - bounds.width) / 2.f, 120.f);
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //title
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize.x / 2.f, 30.f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
    entity.addComponent<xy::Text>(font).setString("Controls");
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.getComponent<xy::Text>().setCharacterSize(Global::LargeTextSize);
    entity.getComponent<xy::Text>().setFillColour(Global::InnerTextColour);
    entity.getComponent<xy::Text>().setOutlineColour(Global::OuterTextColour);
    entity.getComponent<xy::Text>().setOutlineThickness(1.f);
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


    auto helpString =
R"(W - Up
S - Down
A - Left
D - Right

LCtrl/Controller B - Pick up/drop
Space/Controller A - Use Item
LShift/Controller LB - Strafe

Q/Controller RB - Previous Item
E/Controller Y - Next Item
Z/Controller X - Show/Zoom Map
M/Controller Back - Show Map
Tab - Show Scoreboard

Esc/P/Pause/Controller Start - Show options Menu)";

    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(120.f, 180.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Text>(m_fontResource.get(Global::FineFont));
    entity.getComponent<xy::Text>().setString(helpString);
    entity.getComponent<xy::Text>().setCharacterSize(40);
    entity.getComponent<xy::Text>().setFillColour(Global::InnerTextColour);
    /*entity.getComponent<xy::Text>().setOutlineColour(Global::OuterTextColour);
    entity.getComponent<xy::Text>().setOutlineThickness(2.f);*/
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());
}