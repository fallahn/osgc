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
#include "MessageIDs.hpp"
#include "NetworkClient.hpp"
#include "SharedStateData.hpp"
#include "Packet.hpp"
#include "SliderSystem.hpp"
#include "MenuUI.hpp"
#include "MixerChannels.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/UIHitBox.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Callback.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>

#include <xyginext/ecs/systems/UISystem.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>

#include <xyginext/graphics/SpriteSheet.hpp>
#include <xyginext/util/Random.hpp>

#include <SFML/Window/Mouse.hpp>

namespace
{
    
}

void MenuState::buildLobby(sf::Font& font)
{
    auto& fineFont = m_fontResource.get(Global::FineFont);
    
    auto mouseOver = m_callbackIDs[Menu::CallbackID::TextSelected];
    auto mouseOut = m_callbackIDs[Menu::CallbackID::TextUnselected];

    auto parentEntity = m_uiScene.createEntity();
    parentEntity.addComponent<xy::Transform>();
    if (!m_sharedData.netClient->connected()) //hide this until joining a game
    {
        parentEntity.getComponent<xy::Transform>().setPosition(Menu::OffscreenPosition);
    }
    parentEntity.addComponent<xy::CommandTarget>().ID = Menu::CommandID::LobbyNode;
    parentEntity.addComponent<Slider>().speed = Menu::SliderSpeed;

    //back button
    auto entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::BackButtonPosition);
    entity.addComponent<xy::Text>(font).setString("Leave");
    entity.getComponent<xy::Text>().setCharacterSize(Global::MediumTextSize);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Left);
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

            xy::AudioMixer::setPrefadeVolume(0.f, MixerChannel::Menu);

            xy::Command cmd;
            cmd.targetFlags = Menu::CommandID::MainNode;
            cmd.action = [](xy::Entity e, float)
            {
                e.getComponent<Slider>().target = {};
                e.getComponent<Slider>().active = true;
            };
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

            cmd.targetFlags = Menu::CommandID::LobbyItem;
            cmd.action = [&](xy::Entity entity, float)
            {
                m_uiScene.destroyEntity(entity);
            };
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

            //clear the chat history
            m_chatBuffer.clear();

            cmd.targetFlags = Menu::CommandID::ChatOutText;
            cmd.action = [](xy::Entity e, float) {e.getComponent<xy::Text>().setString(""); };
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

            //leave lobby
            m_sharedData.netClient->disconnect();

            //stop the server if hosting
            if (m_sharedData.gameServer->running())
            {
                m_sharedData.gameServer->stop();
                m_sharedData.clientInformation.setHostID(0);
            }
        }
    });
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //start button
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::StartButtonPositionHidden);
    entity.addComponent<xy::Text>(font).setString("Start");
    entity.getComponent<xy::Text>().setCharacterSize(Global::MediumTextSize);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Right);
    entity.getComponent<xy::Text>().setFillColour(Global::InnerTextColour);
    entity.getComponent<xy::Text>().setOutlineColour(Global::OuterTextColour);
    entity.getComponent<xy::Text>().setOutlineThickness(1.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::CommandTarget>().ID = Menu::CommandID::StartButton;
    entity.addComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseEnter] = mouseOver;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseExit] = mouseOut;
    entity.getComponent<xy::UIHitBox>().area = Menu::ButtonArea;
    entity.getComponent<xy::UIHitBox>().area.left = -entity.getComponent<xy::UIHitBox>().area.width;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseUp] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback([&](xy::Entity, sf::Uint64 flags)
    {
        if (flags & xy::UISystem::Flags::LeftMouse)
        {
            //apply the seed if it hasn't been already
            applySeed();

            //send a packet to server if we're host telling it to switch to game state
            m_sharedData.netClient->sendPacket(PacketID::StartGame, std::uint8_t(0), xy::NetFlag::Reliable, Global::ReliableChannel);
        }
    });
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());


    xy::SpriteSheet spriteSheet;
    spriteSheet.loadFromFile("assets/sprites/players.spt", m_textureResource);
    std::array<std::string, 4u> spriteNames =
    {
        "player_one",
        "player_two",
        "player_three",
        "player_four"
    };

    xy::SpriteSheet weaponSprites;
    weaponSprites.loadFromFile("assets/sprites/weapons.spt", m_textureResource);
    std::array<std::string, 4u> weaponNames =
    {
        "weapon_rodney",
        "weapon_jean",
        "weapon_rodney", "weapon_rodney"
    };

    std::array<std::string, 3u> animationNames =
    {
        "pistol_idle_front", "sword_idle_front", "shovel_idle_front"
    };

    //root node for avatars to move easily arrange them
    auto avatarEnt = m_uiScene.createEntity();
    avatarEnt.addComponent<xy::Transform>().setPosition(0.f, -100.f);
    parentEntity.getComponent<xy::Transform>().addChild(avatarEnt.getComponent<xy::Transform>());

    //player avatars
    float xPos = 40.f;
    const float xStride = (xy::DefaultSceneSize.x - (2 * xPos)) / 4.f;
    xPos += (xStride / 2.f);
    for (auto i = 0; i < 4; ++i)
    {
        //background
        entity = m_uiScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(xPos, 160.f);
        entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Far);
        entity.addComponent<xy::Sprite>() = m_sprites[Menu::SpriteID::PlayerFrame];
        auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
        entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, 0.f);
        avatarEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());
        
        //small icon/avatar
        entity = m_uiScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(xPos - 160.f, 740.f);
        entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
        entity.addComponent<xy::Sprite>() = m_sprites[Menu::SpriteID::Bot];
        entity.addComponent<xy::Callback>().active = true;
        entity.getComponent<xy::Callback>().function =
            [&,i](xy::Entity e, float)
        {
            if (m_sharedData.clientInformation.getClient(i).peerID == 0)
            {
                e.getComponent<xy::Sprite>().setTextureRect(m_sprites[Menu::SpriteID::Bot].getTextureRect());
            }
            else
            {
                e.getComponent<xy::Sprite>().setTextureRect(m_sprites[Menu::SpriteID::PlayerOne + i].getTextureRect());
            }
        };
        avatarEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

        //large avatar
        entity = m_uiScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(xPos - 12.f, 340.f);
        entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Mid);
        entity.addComponent<xy::Sprite>() = spriteSheet.getSprite(spriteNames[i]);
        entity.addComponent<xy::SpriteAnimation>().play(spriteSheet.getAnimationIndex("idle_down", spriteNames[i]));
        bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
        entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, 0.f);
        entity.getComponent<xy::Transform>().setScale(5.f, 5.f);
        avatarEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

        auto playerEnt = entity;
        entity = m_uiScene.createEntity();
        entity.addComponent<xy::Transform>();
        entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
        entity.addComponent<xy::Sprite>() = weaponSprites.getSprite(weaponNames[i]);
        entity.addComponent<xy::SpriteAnimation>().play(weaponSprites.getAnimationIndex(animationNames[xy::Util::Random::value(0,2)], weaponNames[i]));
        auto weaponBounds = entity.getComponent<xy::Sprite>().getTextureBounds();
        entity.getComponent<xy::Transform>().setOrigin(weaponBounds.width / 2.f, 0.f);
        entity.getComponent<xy::Transform>().setPosition(bounds.width * 0.56f, 14.f);
        playerEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());
        
        //player name
        entity = m_uiScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(xPos, 180.f);
        entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
        entity.addComponent<xy::Text>(fineFont).setAlignment(xy::Text::Alignment::Centre);
        entity.getComponent<xy::Text>().setCharacterSize(Global::LobbyTextSize - 8);
        entity.getComponent<xy::Text>().setFillColour(Global::InnerTextColour);
        entity.getComponent<xy::Text>().setOutlineColour(Global::OuterTextColour);
        entity.getComponent<xy::Text>().setOutlineThickness(1.f);
        entity.addComponent<xy::Callback>().active = true;
        entity.getComponent<xy::Callback>().function = [&, i](xy::Entity e, float)
        {
            e.getComponent<xy::Text>().setString(m_sharedData.clientInformation.getClient(i).name);
        };
        avatarEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

        //current score
        entity = m_uiScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(xPos, 240.f);
        entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
        entity.addComponent<xy::Text>(fineFont).setAlignment(xy::Text::Alignment::Centre);
        entity.getComponent<xy::Text>().setCharacterSize(Global::SmallTextSize + 10);
        //entity.getComponent<xy::Text>().setFillColour(Global::InnerTextColour);
        //entity.getComponent<xy::Text>().setOutlineColour(Global::OuterTextColour);
        entity.getComponent<xy::Text>().setOutlineThickness(1.f);
        entity.addComponent<xy::Callback>().active = true;
        entity.getComponent<xy::Callback>().function = [&, i](xy::Entity e, float)
        {
            const auto& client = m_sharedData.clientInformation.getClient(i);
            std::string str = "Games Played : " + std::to_string(client.gamesPlayed);
            str += "\nScore: " + std::to_string(client.score);
            
            e.getComponent<xy::Text>().setString(str);
        };
        avatarEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

        //add ready state checkbox
        entity = addCheckbox(i);
        entity.getComponent<xy::Transform>().setPosition(xPos + 80.f, 740.f); //TODO const some position values
        avatarEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

        entity = m_uiScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(xPos + 60.f, 740.f);
        entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
        entity.addComponent<xy::Text>(fineFont).setAlignment(xy::Text::Alignment::Right);
        entity.getComponent<xy::Text>().setString("Ready");
        entity.getComponent<xy::Text>().setCharacterSize(Global::LobbyTextSize);
        entity.getComponent<xy::Text>().setFillColour(Global::InnerTextColour);
        entity.getComponent<xy::Text>().setOutlineColour(Global::OuterTextColour);
        entity.getComponent<xy::Text>().setOutlineThickness(1.f);
        avatarEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

        xPos += xStride;
    }

    //tip text
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::SeedPosition);
    entity.getComponent<xy::Transform>().setOrigin(-30.f, 20.f);
    entity.getComponent<xy::Transform>().setScale(0.f, 0.f);
    entity.addComponent<xy::Text>(fineFont).setString("Change this phrase to change how the island is generated");
    entity.getComponent<xy::Text>().setCharacterSize(Global::SmallTextSize);
    entity.getComponent<xy::Text>().setFillColour(/*Global::InnerTextColour*/sf::Color::White);
    entity.getComponent<xy::Text>().setOutlineColour(/*Global::OuterTextColour*/sf::Color::Black);
    entity.getComponent<xy::Text>().setOutlineThickness(1.f);
    entity.addComponent<xy::Drawable>().setDepth(100);
    entity.addComponent<xy::Callback>().active = true;
    entity.getComponent<xy::Callback>().function =
        [](xy::Entity e, float)
    {
        auto mousePos = xy::App::getRenderWindow()->mapPixelToCoords(sf::Mouse::getPosition(*xy::App::getRenderWindow()));
        e.getComponent<xy::Transform>().setPosition(mousePos);
    };
    auto toolTip = entity;

    //plank background
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, (xy::DefaultSceneSize.y / 4.f) + 40.f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Far - 10);
    entity.addComponent<xy::Sprite>() = m_sprites[Menu::SpriteID::TitleBar];
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //seed text
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::SeedPosition);
    entity.addComponent<xy::Text>(fineFont).setString("Seed:                      (?)");
    entity.getComponent<xy::Text>().setCharacterSize(Global::LobbyTextSize);
    entity.getComponent<xy::Text>().setFillColour(Global::InnerTextColour);
    entity.getComponent<xy::Text>().setOutlineColour(Global::OuterTextColour);
    entity.getComponent<xy::Text>().setOutlineThickness(1.f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
    bounds = xy::Text::getLocalBounds(entity);
    auto charWidth = bounds.width / 31.f;
    bounds.left = charWidth * 28.f;
    bounds.width = charWidth * 3.f;
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseEnter] =
        m_uiScene.getSystem<xy::UISystem>().addMouseMoveCallback([&, toolTip](xy::Entity, sf::Vector2f) mutable
            {
                if (m_sharedData.clientInformation.getHostID() == m_sharedData.netClient->getPeer().getID())
                {
                    toolTip.getComponent<xy::Text>().setString("Change this phrase to change how the island is generated");
                    toolTip.getComponent<xy::Transform>().setScale(1.f, 1.f);
                }
            });

    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseExit] =
        m_uiScene.getSystem<xy::UISystem>().addMouseMoveCallback([toolTip](xy::Entity, sf::Vector2f) mutable
            {
                toolTip.getComponent<xy::Transform>().setScale(0.f, 0.f);
            });

    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //text background
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::SeedPosition.x + 130.f, Menu::SeedPosition.y);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Far);
    entity.addComponent<xy::Sprite>() = m_sprites[Menu::SpriteID::TextInput];
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseDown] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse //only editable if hosting
                    && m_sharedData.clientInformation.getHostID() == m_sharedData.netClient->getPeer().getID())
                {
                    m_activeString = &m_seedDisplayString;

                    xy::Command cmd;
                    cmd.targetFlags = Menu::CommandID::SeedText;
                    cmd.action = [](xy::Entity e, float) {e.getComponent<xy::Text>().setFillColour(sf::Color::Red); };
                    m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                }
            });
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    auto position = entity.getComponent<xy::Transform>().getPosition();
    position.x += bounds.width / 2.f;
    position.y += bounds.height / 2.f;

    //seed text
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(position);
    entity.getComponent<xy::Transform>().move(0.f, -bounds.height * 0.5f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
    entity.getComponent<xy::Drawable>().setCroppingArea({ -232.f, -30.f, 464.f, 120.f });
    entity.addComponent<xy::Text>(fineFont);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.getComponent<xy::Text>().setCharacterSize(Global::SmallTextSize + 14);
    entity.addComponent<xy::CommandTarget>().ID = Menu::CommandID::SeedText;
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //randomise button
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(/*Menu::SeedPosition.x + 700.f, Menu::SeedPosition.y*/xy::DefaultSceneSize.x / 2.f, 1020.f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
    entity.addComponent<xy::Sprite>() = m_sprites[Menu::SpriteID::RandomButton];
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse
                    && m_sharedData.clientInformation.getHostID() == m_sharedData.netClient->getPeer().getID())
                {
                    m_activeString = &m_seedDisplayString;
                    m_activeString->clear();
                    applySeed();
                    m_activeString = nullptr;
                }
            });
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseEnter] =
        m_uiScene.getSystem<xy::UISystem>().addMouseMoveCallback([&, toolTip](xy::Entity, sf::Vector2f) mutable
            {
                if (m_sharedData.clientInformation.getHostID() == m_sharedData.netClient->getPeer().getID())
                {
                    toolTip.getComponent<xy::Text>().setString("Click to randomise seed");
                    toolTip.getComponent<xy::Transform>().setScale(1.f, 1.f);
                }
            });

    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseExit] =
        m_uiScene.getSystem<xy::UISystem>().addMouseMoveCallback([toolTip](xy::Entity, sf::Vector2f) mutable
            {
                toolTip.getComponent<xy::Transform>().setScale(0.f, 0.f);
            });
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //chat input
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::ChatInputPosition);
    entity.addComponent<xy::Text>(fineFont).setString("Chat:");
    entity.getComponent<xy::Text>().setCharacterSize(Global::LobbyTextSize);
    entity.getComponent<xy::Text>().setFillColour(Global::InnerTextColour);
    entity.getComponent<xy::Text>().setOutlineColour(Global::OuterTextColour);
    entity.getComponent<xy::Text>().setOutlineThickness(1.f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //text background
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::ChatInputPosition.x + 130.f, Menu::ChatInputPosition.y);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Far);
    entity.addComponent<xy::Sprite>() = m_sprites[Menu::SpriteID::TextInput];
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseDown] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    m_activeString = &m_chatInDisplayString;

                    xy::Command cmd;
                    cmd.targetFlags = Menu::CommandID::ChatInText;
                    cmd.action = [](xy::Entity e, float) {e.getComponent<xy::Text>().setFillColour(sf::Color::Red); };
                    m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                }
            });
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());


    position = entity.getComponent<xy::Transform>().getPosition();
    position.x += bounds.width - 24.f;
    position.y += bounds.height / 2.f;

    //chat input text
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(position);
    entity.getComponent<xy::Transform>().move(0.f, -bounds.height * 0.5f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
    entity.getComponent<xy::Drawable>().setCroppingArea({ -444.f, -30.f, 444.f, 120.f });
    entity.addComponent<xy::Text>(fineFont);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Right);
    entity.getComponent<xy::Text>().setCharacterSize(Global::SmallTextSize + 14);
    entity.addComponent<xy::CommandTarget>().ID = Menu::CommandID::ChatInText;
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //blinkenlights
    m_textureResource.setFallbackColour(sf::Color::Red);
    auto& cursorTex = m_textureResource.get();
    sf::Vector2f texSize(cursorTex.getSize());
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(position);
    entity.getComponent<xy::Transform>().move(6.f, -18.f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
    entity.addComponent<xy::Sprite>(cursorTex);
    entity.getComponent<xy::Transform>().setScale(4.f / texSize.x, 36.f / texSize.y);
    entity.addComponent<xy::Callback>().active = true;
    entity.getComponent<xy::Callback>().function =
        [&](xy::Entity e, float dt)
    {
        if (m_activeString == &m_chatInDisplayString)
        {
            //flash
            static float timer = 0.f;
            timer += dt;
            if (timer > 0.25f)
            {
                timer = 0.f;
                auto c = e.getComponent<xy::Sprite>().getColour();
                if (c.a == 0)
                {
                    c = sf::Color::Red;
                }
                else
                {
                    c = sf::Color::Transparent;
                }
                e.getComponent<xy::Sprite>().setColour(c);
            }
        }
        else
        {
            e.getComponent<xy::Sprite>().setColour(sf::Color::Transparent);
        }
    };
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //chat button
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::ChatInputPosition.x + 620.f, Menu::ChatInputPosition.y);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
    entity.addComponent<xy::Sprite>() = m_sprites[Menu::SpriteID::ChatButton];
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    m_activeString = &m_chatInDisplayString;
                    sendChat();
                    m_activeString = nullptr;

                    xy::Command cmd;
                    cmd.targetFlags = Menu::CommandID::ChatInText;
                    cmd.action = [](xy::Entity e, float) {e.getComponent<xy::Text>().setFillColour(sf::Color::White); };
                    m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                }
            });
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::ChatBoxPosition);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Far);
    entity.addComponent<xy::Sprite>() = m_sprites[Menu::SpriteID::ChatBox];
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //chat output text
    auto& chatFont = m_fontResource.get("assets/fonts/ProggyClean.ttf");
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::ChatBoxPosition + sf::Vector2f(12.f, -6.f));
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
    entity.addComponent<xy::Text>(chatFont).setFillColour(sf::Color::White);
    entity.getComponent<xy::Text>().setCharacterSize(32);
    entity.addComponent<xy::AudioEmitter>() = m_audioScape.getEmitter("chat_message");
    bounds = m_sprites[Menu::SpriteID::ChatBox].getTextureBounds();
    bounds.width -= 24.f;
    //bounds.height -= 24.f;
    entity.getComponent<xy::Drawable>().setCroppingArea(bounds);
    entity.addComponent<xy::CommandTarget>().ID = Menu::CommandID::ChatOutText;
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //host icon
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(-100.f, -100.f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
    entity.addComponent<xy::Sprite>() = m_sprites[Menu::SpriteID::Host];
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, 0.f);
    entity.addComponent<xy::CommandTarget>().ID = Menu::CommandID::HostIcon;
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());
}

xy::Entity MenuState::addCheckbox(std::uint8_t playerID)
{
    std::array<std::string, 4u> emitterNames =
    {
        "ready_rodney", "ready_jean", "ready_helena", "ready_lars"
    };

    auto entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Mid);
    entity.addComponent<xy::Sprite>() = m_sprites[Menu::SpriteID::Checkbox];
    entity.addComponent<Checkbox>().playerID = playerID;
    entity.addComponent<xy::AudioEmitter>() = m_audioScape.getEmitter(emitterNames[playerID]);

    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] = m_callbackIDs[Menu::CallbackID::CheckboxClicked];

    //callback updates appearance based on current status
    entity.addComponent<xy::Callback>().active = true;
    entity.getComponent<xy::Callback>().function =
        [&](xy::Entity e, float)
    {
        auto checked = m_sharedData.clientInformation.getClient(e.getComponent<Checkbox>().playerID).ready;
        auto bounds = e.getComponent<xy::Sprite>().getTextureRect();
        if (checked)
        {
            if (bounds.top != bounds.height)
            {
                //freshly checked
                e.getComponent<xy::AudioEmitter>().play();
            }
            bounds.top = bounds.height;
        }
        else
        {
            bounds.top = 0.f;
        }
        e.getComponent<xy::Sprite>().setTextureRect(bounds);
    };

    return entity;
}
