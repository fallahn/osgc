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
#include "SharedStateData.hpp"
#include "MessageIDs.hpp"
#include "SliderSystem.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/UIHitBox.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Callback.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>

#include <xyginext/ecs/systems/UISystem.hpp>

#include <xyginext/util/Random.hpp>

namespace
{
    const std::vector<std::string> names =
    {
        "Cleftwisp", "Old Sam", "Lou Baker",
        "Geoff Strongman", "Issy Soux", "Grumbles",
        "Harriet Cobbler", "Queasy Jo", "E. Claire",
        "Pro Moist", "Hannah Theresa", "Shrinkwrap",
        "Fat Tony", "Player Five", "Mr Pump",
        "Tenacious Kaye", "The Hammer", "Doris Baardgezicht"
    };
    std::size_t randNameIndex = 0;

    const std::array<std::string, 4u> playerTextureNames =
    {
        "assets/images/player_one.png",
        "assets/images/player_two.png",
        "assets/images/player_three.png",
        "assets/images/player_four.png"
    };
}

void MenuState::initHats()
{
    //load the normal player texture into the first slot - 
    //this will be the eqivalent of having no hat.
    m_hatTextureIDs.push_back(m_resources.load<sf::Texture>("assets/images/player.png"));

    //check hat directory for textures
    std::map<std::int32_t, std::string> textures;

    xy::ConfigFile cfg;
    if (cfg.loadFromFile(xy::FileSystem::getResourcePath() + "assets/images/hats/hatfig.hat"))
    {
        const auto& objs = cfg.getObjects();
        for (const auto& obj : objs)
        {
            const auto& properties = obj.getProperties();
            std::string file;
            std::int32_t id = -1;
            for (const auto& prop : properties)
            {
                if (prop.getName() == "file")
                {
                    file = prop.getValue<std::string>();
                }
                else if (prop.getName() == "id")
                {
                    id = prop.getValue<std::int32_t>();
                }
            }
            //if valid and ID doesn't exist...
            if (!file.empty() && id > -1
                && textures.count(id) == 0)
            {
                textures.insert(std::make_pair(id, file));
            }
        }
    }

    //std::sort(textures.begin(), textures.end(),
    //    [](const std::pair<std::int32_t, std::string&> a, const std::pair<std::int32_t, std::string>& b) {return a.first < b.first; });
    for(const auto& [i, t] : textures)
    {
        m_hatTextureIDs.push_back(m_resources.load<sf::Texture>("assets/images/hats/" + t));
    }

    for (auto i = 0u; i < playerTextureNames.size(); ++i)
    {
        m_playerTextureIDs[i] = m_resources.load<sf::Texture>(playerTextureNames[i]);
    }

    //init the target render texures
    auto defaultTexture = m_resources.get<sf::Texture>(m_hatTextureIDs[0]);
    auto size = defaultTexture.getSize();
    m_playerPreviewTexture.create(size.x, size.y);

    for (auto& t : m_sharedData.playerSprites)
    {
        t = std::make_shared<sf::RenderTexture>();
        t->create(size.x, size.y);
        updateHatTexture(defaultTexture, defaultTexture, *t);
    }

    //check the loaded index is in range
    if (m_hatIndex >= m_hatTextureIDs.size())
    {
        m_hatIndex = 0;
    }

    updateHatTexture(defaultTexture, m_resources.get<sf::Texture>(m_hatTextureIDs[m_hatIndex]), m_playerPreviewTexture);
}

void MenuState::updateHatTexture(const sf::Texture& a, const sf::Texture& b, sf::RenderTexture& target)
{
    target.clear(sf::Color::Transparent);
    target.draw(sf::Sprite(a));
    target.draw(sf::Sprite(b));
    target.display();
}

void MenuState::buildNameEntry(sf::Font& largeFont)
{
    auto parentEnt = m_uiScene.createEntity();
    parentEnt.addComponent<xy::Transform>().setPosition(Menu::OffscreenPosition);
    parentEnt.addComponent<xy::CommandTarget>().ID = Menu::CommandID::HostNameNode;
    parentEnt.addComponent<Slider>().speed = Menu::SliderSpeed;

    auto sharedEnt = m_uiScene.createEntity();
    sharedEnt.addComponent<xy::Transform>().setPosition(Menu::OffscreenPosition);
    sharedEnt.addComponent<xy::CommandTarget>().ID = Menu::CommandID::HostNameNode | Menu::CommandID::JoinNameNode;
    sharedEnt.addComponent<Slider>().speed = Menu::SliderSpeed;

    //title
    auto entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::TitlePosition);
    entity.addComponent<xy::Sprite>() = m_sprites[Menu::SpriteID::TitleBar];
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Far);
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    parentEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::TitlePosition);
    entity.getComponent<xy::Transform>().move(0.f, -46.f);
    entity.addComponent<xy::Text>(largeFont).setString("Host Game");
    entity.getComponent<xy::Text>().setCharacterSize(Global::LargeTextSize);
    entity.getComponent<xy::Text>().setFillColour(Global::InnerTextColour);
    entity.getComponent<xy::Text>().setOutlineColour(Global::OuterTextColour);
    //entity.getComponent<xy::Text>().setOutlineThickness(1.f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
    bounds = xy::Text::getLocalBounds(entity);
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    parentEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //name input
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, 320.f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Far);
    entity.addComponent<xy::Sprite>() = m_sprites[Menu::SpriteID::TextInput];
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseDown] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    m_activeString = &m_sharedData.clientName;

                    xy::Command cmd;
                    cmd.targetFlags = Menu::CommandID::NameText;
                    cmd.action = [](xy::Entity e, float) {e.getComponent<xy::Text>().setFillColour(sf::Color::Red); };
                    m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                }
            });
    sharedEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    auto& font = m_resources.get<sf::Font>(m_fontIDs[Menu::FontID::Fine]);

    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, bounds.height * 2.5f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
    entity.addComponent<xy::Text>(font).setString("Enter Your Name");
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.getComponent<xy::Text>().setCharacterSize(Global::MediumTextSize);
    entity.getComponent<xy::Text>().setFillColour(Global::InnerTextColour);
    entity.getComponent<xy::Text>().setOutlineColour(Global::OuterTextColour);
    entity.getComponent<xy::Text>().setOutlineThickness(2.f);
    sharedEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, (-bounds.height * 0.5f) + 320.f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
    entity.getComponent<xy::Drawable>().setCroppingArea({ -232.f, -30.f, 464.f, 120.f });
    entity.addComponent<xy::Text>(font).setString(m_sharedData.clientName);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.getComponent<xy::Text>().setCharacterSize(Global::SmallTextSize + 14);
    entity.addComponent<xy::CommandTarget>().ID = Menu::CommandID::NameText;
    sharedEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //buttons to scroll through random names
     //left arrow
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(-284.f, 320.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>() = m_sprites[Menu::SpriteID::ArrowLeft];
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    randNameIndex = (randNameIndex + (names.size() - 1)) % names.size();
                    m_sharedData.clientName = names[randNameIndex];

                    xy::Command cmd;
                    cmd.targetFlags = Menu::CommandID::NameText;
                    cmd.action = [&](xy::Entity e, float) {e.getComponent<xy::Text>().setString(names[randNameIndex]); };
                    m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                }
            });
    sharedEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //right arrow
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(284.f, 320.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>() = m_sprites[Menu::SpriteID::ArrowRight];
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    randNameIndex = (randNameIndex + 1) % names.size();
                    m_sharedData.clientName = names[randNameIndex];

                    xy::Command cmd;
                    cmd.targetFlags = Menu::CommandID::NameText;
                    cmd.action = [&](xy::Entity e, float) {e.getComponent<xy::Text>().setString(names[randNameIndex]); };
                    m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                }
            });
    sharedEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());


    //selected avatar
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().setScale(5.f, 5.f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Mid);
    entity.addComponent<xy::Sprite>() = m_avatarSprites[m_spriteIndex];
    entity.getComponent<xy::Sprite>().setTexture(m_playerPreviewTexture.getTexture(), false);
    entity.addComponent<xy::SpriteAnimation>().play(1);
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    auto avatarEnt = entity;
    sharedEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
    entity.addComponent<xy::Sprite>() = m_weaponSprites[m_spriteIndex];
    entity.addComponent<xy::SpriteAnimation>().play(22);
    auto weaponBounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(weaponBounds.width / 2.f, 0.f);
    entity.getComponent<xy::Transform>().setPosition(bounds.width * 0.56f, 14.f);
    entity.addComponent<xy::Callback>().active = true;
    entity.getComponent<xy::Callback>().userData = std::make_any<std::uint8_t>(m_spriteIndex);
    entity.getComponent<xy::Callback>().function =
        [&, avatarEnt](xy::Entity e, float) mutable
    {
        auto& spriteIdx = std::any_cast<std::uint8_t&>(e.getComponent<xy::Callback>().userData);
        if (spriteIdx != m_spriteIndex)
        {
            spriteIdx = m_spriteIndex;
            avatarEnt.getComponent<xy::Sprite>() = m_avatarSprites[spriteIdx];
            avatarEnt.getComponent<xy::Sprite>().setTexture(m_playerPreviewTexture.getTexture(), false);
            auto frame = e.getComponent<xy::Sprite>().getTextureRect();
            e.getComponent<xy::Sprite>() = m_weaponSprites[spriteIdx];
            e.getComponent<xy::Sprite>().setTextureRect(frame);
        }
    };
    avatarEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //left arrow
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(-224.f, 80.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>() = m_sprites[Menu::SpriteID::ArrowLeft];
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    m_spriteIndex = (m_spriteIndex + 3) % 4;
                    m_spriteSounds[m_spriteIndex].getComponent<xy::AudioEmitter>().play();
                }
            });
    sharedEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //right arrow
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(224.f, 80.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>() = m_sprites[Menu::SpriteID::ArrowRight];
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    m_spriteIndex = (m_spriteIndex + 1) % 4;
                    m_spriteSounds[m_spriteIndex].getComponent<xy::AudioEmitter>().play();
                }
            });
    sharedEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //hat select arrows
    if (m_hatTextureIDs.size() > 1)
    {
        //left arrow
        entity = m_uiScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
        entity.getComponent<xy::Transform>().move(-224.f, -60.f);
        entity.addComponent<xy::Drawable>();
        entity.addComponent<xy::Sprite>() = m_sprites[Menu::SpriteID::ArrowLeft];
        bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
        entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
        entity.addComponent<xy::UIHitBox>().area = bounds;
        entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
            m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback(
                [&](xy::Entity, sf::Uint64 flags)
                {
                    if (flags & xy::UISystem::LeftMouse)
                    {
                        m_hatIndex = (m_hatIndex + (m_hatTextureIDs.size() - 1)) % m_hatTextureIDs.size();
                        updateHatTexture(m_resources.get<sf::Texture>(m_hatTextureIDs[0]), m_resources.get<sf::Texture>(m_hatTextureIDs[m_hatIndex]), m_playerPreviewTexture);
                    }
                });
        sharedEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

        //right arrow
        entity = m_uiScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
        entity.getComponent<xy::Transform>().move(224.f, -60.f);
        entity.addComponent<xy::Drawable>();
        entity.addComponent<xy::Sprite>() = m_sprites[Menu::SpriteID::ArrowRight];
        bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
        entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
        entity.addComponent<xy::UIHitBox>().area = bounds;
        entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
            m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback(
                [&](xy::Entity, sf::Uint64 flags)
                {
                    if (flags & xy::UISystem::LeftMouse)
                    {
                        m_hatIndex = (m_hatIndex + 1) % m_hatTextureIDs.size();
                        updateHatTexture(m_resources.get<sf::Texture>(m_hatTextureIDs[0]), m_resources.get<sf::Texture>(m_hatTextureIDs[m_hatIndex]), m_playerPreviewTexture);
                    }
                });
        sharedEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());
    }


    //back background
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::BackButtonPosition);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Far);
    entity.addComponent<xy::Sprite>() = m_sprites[Menu::SpriteID::ButtonBackground];
    parentEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    auto textPos = Menu::BackButtonPosition;
    textPos.x += bounds.width / 2.f;
    textPos.y += 8.f;
    bounds.left -= bounds.width / 2.f;

    //back button
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(textPos);
    entity.addComponent<xy::Text>(largeFont).setString("Back");
    entity.getComponent<xy::Text>().setCharacterSize(Global::MediumTextSize);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Left);
    entity.getComponent<xy::Text>().setFillColour(Global::ButtonTextColour);
    entity.getComponent<xy::Text>().setOutlineColour(Global::OuterTextColour);
    //entity.getComponent<xy::Text>().setOutlineThickness(1.f);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
    entity.addComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseEnter] = m_callbackIDs[Menu::CallbackID::TextSelected];
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseExit] = m_callbackIDs[Menu::CallbackID::TextUnselected];
    entity.getComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseUp] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback([&, parentEnt, sharedEnt](xy::Entity, sf::Uint64 flags) mutable
            {
                if (flags & xy::UISystem::Flags::LeftMouse)
                {
                    parentEnt.getComponent<Slider>().target = Menu::OffscreenPosition;
                    parentEnt.getComponent<Slider>().active = true;

                    sharedEnt.getComponent<Slider>().target = Menu::OffscreenPosition;
                    sharedEnt.getComponent<Slider>().active = true;

                    xy::Command cmd;
                    cmd.targetFlags = Menu::CommandID::MainNode;
                    cmd.action = [](xy::Entity e, float)
                    {
                        e.getComponent<Slider>().target = {};
                        e.getComponent<Slider>().active = true;
                    };
                    m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                    m_activeString = nullptr;

                    cmd.targetFlags = Menu::CommandID::NameText;
                    cmd.action = [](xy::Entity e, float) {e.getComponent<xy::Text>().setFillColour(sf::Color::White); };
                    m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                    if (m_sharedData.clientName.isEmpty())
                    {
                        m_sharedData.clientName = names[xy::Util::Random::value(0, names.size() - 1)];
                    }
                }
            });
    parentEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //next background
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::StartButtonPosition);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Far);
    entity.addComponent<xy::Sprite>() = m_sprites[Menu::SpriteID::ButtonBackground];
    parentEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    textPos = Menu::StartButtonPosition;
    textPos.x += bounds.width / 2.f;
    textPos.y += 8.f;
    bounds.left -= bounds.width / 2.f;

    //next button
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(textPos);
    entity.addComponent<xy::Text>(largeFont).setString("Next");
    entity.getComponent<xy::Text>().setCharacterSize(Global::MediumTextSize);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.getComponent<xy::Text>().setFillColour(Global::ButtonTextColour);
    entity.getComponent<xy::Text>().setOutlineColour(Global::OuterTextColour);
    //entity.getComponent<xy::Text>().setOutlineThickness(1.f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
    entity.addComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseEnter] = m_callbackIDs[Menu::CallbackID::TextSelected];
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseExit] = m_callbackIDs[Menu::CallbackID::TextUnselected];
    entity.getComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseUp] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback([&](xy::Entity, sf::Uint64 flags) mutable
            {
                if (flags & xy::UISystem::Flags::LeftMouse)
                {
                    if (!m_sharedData.clientName.isEmpty())
                    {
                        auto* msg = getContext().appInstance.getMessageBus().post<SystemEvent>(MessageID::SystemMessage);
                        msg->action = SystemEvent::RequestStartServer;

                        m_activeString = nullptr;
                        //xy::AudioMixer::setPrefadeVolume(1.f, MixerChannel::Menu);

                        xy::Command cmd;
                        cmd.targetFlags = Menu::CommandID::NameText;
                        cmd.action = [](xy::Entity e, float) {e.getComponent<xy::Text>().setFillColour(sf::Color::White); };
                        m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                    }
                }
            });
    parentEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());
}

void MenuState::buildJoinEntry(sf::Font& largeFont)
{
    auto parentEnt = m_uiScene.createEntity();
    parentEnt.addComponent<xy::Transform>().setPosition(Menu::OffscreenPosition);
    parentEnt.addComponent<xy::CommandTarget>().ID = Menu::CommandID::JoinNameNode;
    parentEnt.addComponent<Slider>().speed = Menu::SliderSpeed;

    //title bar
    auto entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::TitlePosition);
    entity.addComponent<xy::Sprite>() = m_sprites[Menu::SpriteID::TitleBar];
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Far);
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    parentEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::TitlePosition);
    entity.getComponent<xy::Transform>().move(0.f, -46.f);
    entity.addComponent<xy::Text>(largeFont).setString("Join Game");
    entity.getComponent<xy::Text>().setCharacterSize(Global::LargeTextSize);
    entity.getComponent<xy::Text>().setFillColour(Global::InnerTextColour);
    entity.getComponent<xy::Text>().setOutlineColour(Global::OuterTextColour);
    //entity.getComponent<xy::Text>().setOutlineThickness(1.f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
    bounds = xy::Text::getLocalBounds(entity);
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    parentEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //remote address box
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Far);
    entity.addComponent<xy::Sprite>() = m_sprites[Menu::SpriteID::TextInput];
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, (bounds.height * 1.4f) + 320.f);
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseDown] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    m_activeString = &m_sharedData.remoteIP;

                    xy::Command cmd;
                    cmd.targetFlags = Menu::CommandID::IPText;
                    cmd.action = [](xy::Entity e, float) {e.getComponent<xy::Text>().setFillColour(sf::Color::Red); };
                    m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                    cmd.targetFlags = Menu::CommandID::NameText;
                    cmd.action = [](xy::Entity e, float) {e.getComponent<xy::Text>().setFillColour(sf::Color::White); };
                    m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                }
            });
    parentEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    auto& font = m_resources.get<sf::Font>(m_fontIDs[Menu::FontID::Fine]);
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, (bounds.height * 0.9f) + 320.f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
    entity.getComponent<xy::Drawable>().setCroppingArea({ -232.f, -30.f, 464.f, 120.f });
    entity.addComponent<xy::Text>(font).setString(m_sharedData.remoteIP);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.getComponent<xy::Text>().setCharacterSize(Global::SmallTextSize + 14);
    entity.addComponent<xy::CommandTarget>().ID = Menu::CommandID::IPText;
    parentEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //back background
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::BackButtonPosition);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Far);
    entity.addComponent<xy::Sprite>() = m_sprites[Menu::SpriteID::ButtonBackground];
    parentEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    auto textPos = Menu::BackButtonPosition;
    textPos.x += bounds.width / 2.f;
    textPos.y += 8.f;
    bounds.left -= bounds.width / 2.f;

    //back button
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(textPos);
    entity.addComponent<xy::Text>(largeFont).setString("Back");
    entity.getComponent<xy::Text>().setCharacterSize(Global::MediumTextSize);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.getComponent<xy::Text>().setFillColour(Global::ButtonTextColour);
    entity.getComponent<xy::Text>().setOutlineColour(Global::OuterTextColour);
    //entity.getComponent<xy::Text>().setOutlineThickness(1.f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
    entity.addComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseEnter] = m_callbackIDs[Menu::CallbackID::TextSelected];
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseExit] = m_callbackIDs[Menu::CallbackID::TextUnselected];
    entity.getComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseUp] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback([&, parentEnt](xy::Entity, sf::Uint64 flags) mutable
            {
                if (flags & xy::UISystem::Flags::LeftMouse)
                {
                    parentEnt.getComponent<Slider>().target = Menu::OffscreenPosition;
                    parentEnt.getComponent<Slider>().active = true;

                    xy::Command cmd;
                    cmd.targetFlags = Menu::CommandID::MainNode;
                    cmd.action = [](xy::Entity e, float)
                    {
                        e.getComponent<Slider>().target = {};
                        e.getComponent<Slider>().active = true;
                    };
                    m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                    m_activeString = nullptr;

                    cmd.targetFlags = Menu::CommandID::NameText | Menu::CommandID::IPText;
                    cmd.action = [](xy::Entity e, float) {e.getComponent<xy::Text>().setFillColour(sf::Color::White); };
                    m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                    if (m_sharedData.clientName.isEmpty())
                    {
                        m_sharedData.clientName = names[xy::Util::Random::value(0, names.size() - 1)];
                    }

                    if (m_sharedData.remoteIP.isEmpty())
                    {
                        m_sharedData.remoteIP = "127.0.0.1";
                    }

                    //hide shared node
                    cmd.targetFlags = Menu::CommandID::JoinNameNode;
                    cmd.action = [](xy::Entity e, float)
                    {
                        e.getComponent<Slider>().target = Menu::OffscreenPosition;
                        e.getComponent<Slider>().active = true;
                    };
                    m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                }
            });
    parentEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //next background
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::StartButtonPosition);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Far);
    entity.addComponent<xy::Sprite>() = m_sprites[Menu::SpriteID::ButtonBackground];
    parentEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    textPos = Menu::StartButtonPosition;
    textPos.x += bounds.width / 2.f;
    textPos.y += 8.f;
    bounds.left -= bounds.width / 2.f;

    //next button
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(textPos);
    entity.addComponent<xy::Text>(largeFont).setString("Join");
    entity.getComponent<xy::Text>().setCharacterSize(Global::MediumTextSize);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.getComponent<xy::Text>().setFillColour(Global::ButtonTextColour);
    entity.getComponent<xy::Text>().setOutlineColour(Global::OuterTextColour);
    //entity.getComponent<xy::Text>().setOutlineThickness(1.f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
    entity.addComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseEnter] = m_callbackIDs[Menu::CallbackID::TextSelected];
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseExit] = m_callbackIDs[Menu::CallbackID::TextUnselected];
    entity.getComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseUp] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback([&](xy::Entity, sf::Uint64 flags) mutable
            {
                if (flags & xy::UISystem::Flags::LeftMouse)
                {
                    if (!m_sharedData.clientName.isEmpty()
                        && !m_sharedData.remoteIP.isEmpty())
                    {
                        m_activeString = nullptr;

                        //xy::AudioMixer::setPrefadeVolume(1.f, MixerChannel::Menu);

                        xy::Command cmd;
                        cmd.targetFlags = Menu::CommandID::NameText | Menu::CommandID::IPText;
                        cmd.action = [](xy::Entity e, float) {e.getComponent<xy::Text>().setFillColour(sf::Color::White); };
                        m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                        if (m_sharedData.netClient->connect(m_sharedData.remoteIP.toAnsiString(), Global::GamePort))
                        {
                            //goto lobby
                            cmd.targetFlags = Menu::CommandID::LobbyNode;
                            cmd.action = [](xy::Entity e, float)
                            {
                                e.getComponent<Slider>().target = {};
                                e.getComponent<Slider>().active = true;
                            };
                            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                            cmd.targetFlags = Menu::CommandID::JoinNameNode;
                            cmd.action = [](xy::Entity e, float)
                            {
                                e.getComponent<Slider>().target = Menu::OffscreenPosition;
                                e.getComponent<Slider>().active = true;
                            };
                            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                        }
                        else
                        {
                            m_sharedData.error = "Could not connect to server: attempt timed out";
                            requestStackPush(StateID::Error);
                        }
                    }
                }
            });
    parentEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

}
