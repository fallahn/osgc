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

#include "GameState.hpp"
#include "GameUI.hpp"
#include "SharedStateData.hpp"
#include "HealthBarSystem.hpp"
#include "Camera3D.hpp"
#include "GlobalConsts.hpp"
#include "AnimationCallbacks.hpp"
#include "CommandIDs.hpp"
#include "ServerMessages.hpp"
#include "AnimationSystem.hpp"
#include "BotSystem.hpp"
#include "ClientWeaponSystem.hpp"

#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/SpriteAnimator.hpp>
#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/ecs/systems/CameraSystem.hpp>
#include <xyginext/ecs/systems/CallbackSystem.hpp>
#include <xyginext/ecs/systems/UISystem.hpp>

#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Callback.hpp>
#include <xyginext/ecs/components/UIHitBox.hpp>
#include <xyginext/ecs/components/AudioEmitter.hpp>

#include <xyginext/graphics/SpriteSheet.hpp>
#include <xyginext/util/Random.hpp>

#include <sstream>

namespace
{
    const sf::Vector2f AmmoPosition(1420.f, 20.f);
    const sf::Vector2f TreasurePosition(/*AmmoPosition.x + 216.f*/20.f, AmmoPosition.y);
    const sf::Vector2f AmmoTextPosition(AmmoPosition.x + 100.f, AmmoPosition.y + 6.f);
    const sf::Vector2f TreasureTextPosition(TreasurePosition.x + 100.f, AmmoTextPosition.y);

    std::array<xy::Sprite, UI::SpriteID::Count> Avatars;

    const std::array<sf::Color, 4u> PlayerColours = 
    {
        sf::Color(128,25,8),
        sf::Color(51,128,8),
        sf::Color(86,8,128),
        sf::Color(8,110,128)        
    };

    const std::array<sf::Vector2f, 4u> TrophyPositions =
    {
        sf::Vector2f(90.f, 316.f),
        sf::Vector2f(1250.f, 316.f),
        sf::Vector2f(1250.f, 656.f),
        sf::Vector2f(90.f, 656.f)
    };

    class TreasureFlash final
    {
    public:
        void operator()(xy::Entity entity, float dt)
        {
            m_currentTime += dt;
            if (m_currentTime > FlashTime)
            {
                m_currentTime = 0.f;
                auto& sprite = entity.getComponent<xy::Sprite>();
                auto colour = sprite.getColour();
                colour.a = (colour.a == 0) ? 255 : 0;
                sprite.setColour(colour);
            }
        }

    private:
        float m_currentTime = 0.f;
        static constexpr float FlashTime = 0.6f;
    };

    class TimerUpdate final
    {
    public:
        explicit TimerUpdate(std::int32_t& timeSource)
            : m_timeSource(timeSource) {}
        void operator()(xy::Entity e, float dt)
        {
            m_currSec -= dt;
            if (m_currSec < 0)
            {
                m_currSec = 1.f;
                m_timeSource--;

                if (m_timeSource == 0)
                {
                    e.getComponent<xy::Callback>().active = false;
                }
            } 

            if (m_alpha < 1)
            {
                m_alpha = std::min(1.f, m_alpha + (dt * 4.f));
                auto c = UI::Colour::RedText;
                c.a = static_cast<sf::Uint8>(255.f * m_alpha);
                e.getComponent<xy::Text>().setFillColour(c);

                e.getComponent<xy::Transform>().move(0.f, 160.f * dt);
            }

            std::stringstream ss;
            ss << m_timeSource / 60 << ":" << std::setw(2) << std::setfill('0') << m_timeSource % 60;
            e.getComponent<xy::Text>().setString(ss.str());
        }

    private:
        std::int32_t& m_timeSource;
        float m_currSec = 1.f;
        float m_alpha = 0.f;
    };
}

void GameState::loadUI()
{
    auto& mb = getContext().appInstance.getMessageBus();
    m_uiScene.addSystem<HealthBarSystem>(mb);
    m_uiScene.addSystem<xy::CommandSystem>(mb);
    m_uiScene.addSystem<xy::UISystem>(mb);
    m_uiScene.addSystem<xy::CallbackSystem>(mb);
    m_uiScene.addSystem<xy::CameraSystem>(mb);
    m_uiScene.addSystem<xy::SpriteSystem>(mb);
    m_uiScene.addSystem<xy::SpriteAnimator>(mb);
    m_uiScene.addSystem<xy::TextSystem>(mb);
    m_uiScene.addSystem<xy::RenderSystem>(mb);
    
    m_uiScene.setSystemActive<xy::UISystem>(false);

    auto view = getContext().defaultView;
    auto cameraEntity = m_uiScene.getActiveCamera();
    auto& camera = cameraEntity.getComponent<xy::Camera>();
    camera.setView(view.getSize());
    camera.setViewport(view.getViewport());
    cameraEntity.getComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);


    xy::SpriteSheet spriteSheet;
    spriteSheet.loadFromFile("assets/sprites/lobby.spt", m_textureResource);
    Avatars[UI::SpriteID::Bot] = spriteSheet.getSprite("bot_icon");
    Avatars[UI::SpriteID::PlayerOneIcon] = spriteSheet.getSprite("player_one");
    Avatars[UI::SpriteID::PlayerTwoIcon] = spriteSheet.getSprite("player_two");
    Avatars[UI::SpriteID::PlayerThreeIcon] = spriteSheet.getSprite("player_three");
    Avatars[UI::SpriteID::PlayerFourIcon] = spriteSheet.getSprite("player_four");


    spriteSheet.loadFromFile("assets/sprites/hud.spt", m_textureResource);

    //inventory display
    auto entity = m_uiScene.createEntity();
    entity.addComponent<xy::Sprite>() = spriteSheet.getSprite("weapons");
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.addComponent<xy::Transform>().setPosition({ (xy::DefaultSceneSize.x - bounds.width) / 2.f, 10.f });
    entity.addComponent<xy::Drawable>();
    auto& uiTx = entity.getComponent<xy::Transform>();
    
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(bounds.width / 3.f * 2.f, 0.f);
    entity.addComponent<xy::Sprite>() = spriteSheet.getSprite("highlight");
    entity.addComponent<xy::Drawable>().setDepth(1);
    entity.addComponent<xy::CommandTarget>().ID = UI::CommandID::WeaponSlot;
    uiTx.addChild(entity.getComponent<xy::Transform>());

    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(TreasurePosition);
    entity.addComponent<xy::Sprite>() = spriteSheet.getSprite("treasure");
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Callback>().function = TreasureFlash();
    entity.addComponent<xy::CommandTarget>().ID = UI::CommandID::TreasureIcon;

    auto& font = m_fontResource.get(Global::BoldFont);
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(/*AmmoTextPosition*/106.f, 96.f);
    entity.addComponent<xy::Text>(font).setString("5");
    entity.getComponent<xy::Text>().setCharacterSize(30);
    entity.getComponent<xy::Text>().setFillColour(sf::Color::Black);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::CommandTarget>().ID = UI::CommandID::Ammo;
    uiTx.addChild(entity.getComponent<xy::Transform>());

    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(TreasureTextPosition);
    entity.addComponent<xy::Text>(font).setString("0");
    entity.getComponent<xy::Text>().setCharacterSize(70);
    entity.getComponent<xy::Text>().setFillColour(UI::Colour::ScoreText);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::CommandTarget>().ID = UI::CommandID::Treasure;

    //light banner across top to make text stand out
    sf::Color bannerColour(0, 0, 0, 90);
    const float bannerOffset = 1188.f;
    const float bannerTop = 14.f;
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(-4);
    auto& verts = entity.getComponent<xy::Drawable>().getVertices();
    verts.emplace_back(sf::Vector2f(bannerOffset, bannerTop), bannerColour);
    verts.emplace_back(sf::Vector2f(xy::DefaultSceneSize.x - bannerTop, bannerTop), bannerColour);
    verts.emplace_back(sf::Vector2f(bannerOffset, 10.f), bannerColour);
    verts.emplace_back(sf::Vector2f(xy::DefaultSceneSize.x - bannerTop, 10.f), bannerColour);
    verts.emplace_back(sf::Vector2f(bannerOffset, 80.f), sf::Color::Transparent);
    verts.emplace_back(sf::Vector2f(xy::DefaultSceneSize.x - bannerTop, 80.f), sf::Color::Transparent);
    entity.getComponent<xy::Drawable>().setPrimitiveType(sf::PrimitiveType::TriangleStrip);
    entity.getComponent<xy::Drawable>().updateLocalBounds();

    //player avatar icons
    auto controlEntity = m_uiScene.createEntity();
    controlEntity.addComponent<xy::Transform>().setPosition(Global::DefaultHealthPosition);
    controlEntity.addComponent<sf::Vector2f>() = Global::DefaultHealthPosition;
    controlEntity.addComponent<xy::Callback>().function = UISlideCallback(getContext().appInstance.getMessageBus());
    controlEntity.addComponent<xy::CommandTarget>().ID = UI::CommandID::InfoBar;

    const std::array<int, 4u> treasureIDs = 
    { UI::CommandID::CarryIconOne,UI::CommandID::CarryIconTwo,
        UI::CommandID::CarryIconThree, UI::CommandID::CarryIconFour
    };

    const std::array<int, 4u> skullIDs =
    {
        UI::CommandID::DeadIconOne, UI::CommandID::DeadIconTwo,
        UI::CommandID::DeadIconThree, UI::CommandID::DeadIconFour
    };

    auto& controlTx = controlEntity.getComponent<xy::Transform>();
    for (int i = Actor::ID::PlayerOne; i <= Actor::ID::PlayerFour; ++i)
    {
        auto idx = i - Actor::ID::PlayerOne;

        sf::Vector2f position(((xy::DefaultSceneSize.x / 4.f) * idx) + 20.f, 0.f);

        //avatar
        entity = m_uiScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(position);
        entity.addComponent<xy::Sprite>() = Avatars[UI::SpriteID::Bot];
        entity.addComponent<xy::Drawable>();
        entity.addComponent<xy::Callback>().active = true;
        entity.getComponent<xy::Callback>().function =
            [&, idx](xy::Entity e, float)
        {
            if (m_sharedData.clientInformation.getClient(idx).peerID > 0)
            {
                e.getComponent<xy::Sprite>().setTextureRect(Avatars[idx].getTextureRect());
            }
            else
            {
                e.getComponent<xy::Sprite>().setTextureRect(Avatars[UI::SpriteID::Bot].getTextureRect());
            }
        };

        controlTx.addChild(entity.getComponent<xy::Transform>());

        //name
        entity = m_uiScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(position + sf::Vector2f(84.f, 0.f));
        entity.addComponent<xy::Sprite>(m_nameTagManager.getTexture(idx));
        entity.addComponent<xy::Drawable>();
        controlTx.addChild(entity.getComponent<xy::Transform>());

        //health bar
        entity = m_uiScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(position + sf::Vector2f(84.f, 42.f));
        entity.addComponent<HealthBar>().colour = PlayerColours[idx];
        entity.getComponent<HealthBar>().size = { 320.f, 22.f };
        entity.addComponent<xy::Drawable>();
        entity.addComponent<xy::CommandTarget>().ID = UI::CommandID::Health;
        entity.addComponent<std::uint32_t>() = i;
        controlTx.addChild(entity.getComponent<xy::Transform>());

        //treasure icon
        entity = m_uiScene.createEntity();
        entity.addComponent<xy::Transform>().setScale(0.5f, 0.5f);
        entity.getComponent<xy::Transform>().setPosition(position + sf::Vector2f(390.f, 10.f));
        entity.addComponent<xy::Drawable>().setDepth(2);
        entity.addComponent<xy::Sprite>() = spriteSheet.getSprite("treasure");
        entity.getComponent<xy::Sprite>().setColour(sf::Color::Transparent);
        entity.addComponent<xy::CommandTarget>().ID = treasureIDs[idx];
        controlTx.addChild(entity.getComponent<xy::Transform>());

        //dead icon
        entity = m_uiScene.createEntity();
        entity.addComponent<xy::Transform>().setScale(2.f, 2.f);
        entity.getComponent<xy::Transform>().setPosition(position);
        entity.addComponent<xy::Drawable>().setDepth(2);
        entity.addComponent<xy::Sprite>() = spriteSheet.getSprite("skull");
        entity.getComponent<xy::Sprite>().setColour(sf::Color::Transparent);
        entity.addComponent<xy::CommandTarget>().ID = skullIDs[idx];
        controlTx.addChild(entity.getComponent<xy::Transform>());
    }
    

    //curtain
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.getComponent<xy::Transform>().setScale(4.f, 4.f);
    entity.addComponent<xy::Drawable>().setDepth(20000);
    entity.addComponent<xy::Sprite>(m_textureResource.get("assets/images/red_drapes.png"));
    entity.addComponent<xy::CommandTarget>().ID = UI::CommandID::Curtain;
    entity.addComponent<xy::Callback>().function =
        [&](xy::Entity ent, float dt)
    {
        ent.getComponent<xy::Transform>().move(0.f, -420.f * dt);

        auto yPos = ent.getComponent<xy::Transform>().getPosition().y;

        if (yPos < -xy::DefaultSceneSize.y * 1.05f)
        {
            m_uiScene.destroyEntity(ent);
            //toggleUI();
            m_gameScene.getSystem<Camera3DSystem>().setActive(true);
            
            xy::Command cmd;
            cmd.targetFlags = CommandID::LoopedSound;
            cmd.action = [](xy::Entity ent, float) {ent.getComponent<xy::AudioEmitter>().play(); };
            m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
        }
    };

    //minimap
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Global::MapOffPosition);
    entity.getComponent<xy::Transform>().setScale(0.5f, 0.5f);
    auto tBounds = entity.addComponent<xy::Sprite>(m_miniMap.getTexture()).getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(0.f, tBounds.height / 2.f);
    entity.addComponent<xy::Drawable>().setDepth(-1);
    entity.addComponent<xy::CommandTarget>().ID = UI::CommandID::MiniMap;
    entity.addComponent<MiniMapData>().targetPosition = entity.getComponent<xy::Transform>().getPosition();
    entity.addComponent<xy::Callback>().function = 
        [](xy::Entity ent, float dt)
    {
        static const float totalMovement = xy::Util::Vector::length(Global::MapOnPosition - Global::MapZoomPosition);

        const auto& data = ent.getComponent<MiniMapData>();
        //auto scale = ent.getComponent<xy::Transform>().getScale();

        auto& tx = ent.getComponent<xy::Transform>();
        auto position = tx.getPosition();
        auto movement = data.targetPosition - position;
        auto len2 = xy::Util::Vector::lengthSquared(movement);
        if (len2 < 5)
        {
            tx.setPosition(data.targetPosition);
            tx.setScale(data.targetScale, data.targetScale);
            ent.getComponent<xy::Callback>().active = false;
        }
        else
        {
            tx.move(movement * 14.f * dt);

            auto currMovement = xy::Util::Vector::length(ent.getComponent<xy::Transform>().getPosition() - Global::MapOnPosition);
            auto ratio = currMovement / totalMovement;
            float scale = 0.5f + (ratio * 1.5f);
            tx.setScale(scale, scale);
        }
    };

    //end of round time
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize.x / 2.f, 108.f);
    entity.addComponent<xy::Drawable>().setDepth(20);
    entity.addComponent<xy::Text>(m_fontResource.get(Global::BoldFont));
    entity.getComponent<xy::Text>().setString("3:00");
    entity.getComponent<xy::Text>().setFillColour(sf::Color::Transparent);
    entity.getComponent<xy::Text>().setCharacterSize(80);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);

    entity.addComponent<xy::Callback>().function = TimerUpdate(m_roundTime);
    entity.addComponent<xy::CommandTarget>().ID = UI::CommandID::RoundTimer;

    loadRoundEnd();
}

void GameState::loadRoundEnd()
{
    //this is loaded at the beginning so it's ready to be shown immediately
    //once the game has ended
    auto parentEntity = m_uiScene.createEntity();
    parentEntity.addComponent<xy::Transform>();
    parentEntity.addComponent<xy::Sprite>(m_textureResource.get("assets/images/summary_window.png"));
    parentEntity.addComponent<xy::Drawable>();
    auto bounds = parentEntity.getComponent<xy::Sprite>().getTextureBounds();
    parentEntity.getComponent<xy::Transform>().setPosition(UI::RoundScreenOffPosition);
    parentEntity.addComponent<xy::CommandTarget>().ID = UI::CommandID::RoundEndMenu;
    parentEntity.addComponent<sf::Vector2f>() = UI::RoundScreenOnPosition;
    parentEntity.addComponent<xy::Callback>().function = UISlideCallback(getContext().appInstance.getMessageBus());

    auto entity = m_uiScene.createEntity();
    entity.addComponent<xy::Sprite>(m_summaryTexture.getTexture());
    entity.addComponent<xy::Drawable>();
    parentEntity.getComponent<xy::Transform>().addChild(entity.addComponent<xy::Transform>());

    //treasures remaining
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(UI::TreasureScorePosition);
    entity.addComponent<xy::Text>(m_fontResource.get(Global::BoldFont));
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.getComponent<xy::Text>().setCharacterSize(56);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::CommandTarget>().ID = UI::CommandID::TreasureScore;
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //trophy sprites
    std::array<int, 4u> IDs = { UI::CommandID::TrophyOne, UI::CommandID::TrophyTwo, UI::CommandID::TrophyThree, UI::CommandID::TrophyFour };
    xy::SpriteSheet spriteSheet;
    spriteSheet.loadFromFile("assets/sprites/trophies.spt", m_textureResource);
    for (auto i = 0u; i < TrophyPositions.size(); ++i)
    {
        entity = m_uiScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(TrophyPositions[i]);
        entity.addComponent<xy::Drawable>().setDepth(2);
        entity.addComponent<xy::Sprite>() = spriteSheet.getSprite("trophy");
        entity.addComponent<xy::SpriteAnimation>().play(3);
        auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
        entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
        entity.getComponent<xy::Transform>().setScale(2.f, 2.f);
        entity.addComponent<xy::CommandTarget>().ID = IDs[i];
        parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());
    }
}

void GameState::toggleUI()
{
    xy::Command cmd;
    cmd.targetFlags = CommandID::NameTag;
    cmd.action = [&](xy::Entity entity, float)
    {
        sf::Vector2f target;
        auto colour = entity.getComponent<xy::Sprite>().getColour();
        if (colour.a == 255)
        {
            colour.a = 0;
            target = Global::HiddenHealthPosition;
        }
        else
        {
            colour.a = 255;
            target = Global::DefaultHealthPosition;
        }
        entity.getComponent<xy::Sprite>().setColour(colour);

        xy::Command cmd2;
        cmd2.targetFlags = UI::CommandID::InfoBar;
        cmd2.action = [&, target](xy::Entity ent, float)
        {
            ent.getComponent<sf::Vector2f>() = target;
            ent.getComponent<xy::Callback>().active = true;
        };
        m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd2);
    };
    m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
}

void GameState::showRoundEnd(const RoundSummary& summary)
{
    for (auto i = 0u; i < 4; ++i)
    {
        m_sharedData.clientInformation.getClient(i).score += summary.stats[i].roundXP;
        m_sharedData.clientInformation.getClient(i).gamesPlayed++;
    }
    
    //send message to menu items with stat info
    m_summaryTexture.update(summary, m_sharedData.clientInformation, true);

    //sort the results to see who gets which trophy
    auto summaryCopy = summary;
    std::sort(std::begin(summaryCopy.stats), std::end(summaryCopy.stats),
        [](const RoundStat& l, const RoundStat& r)
    {
        return l.roundXP > r.roundXP;
    });
    auto winnerID = summaryCopy.stats[0].id;

    //update trophies
    for (auto i = 0; i < 4; ++i)
    {
        xy::Command c;
        switch (summaryCopy.stats[i].id)
        {
        default: break;
        case Actor::ID::PlayerOne:
            c.targetFlags = UI::CommandID::TrophyOne;
            break;
        case Actor::ID::PlayerTwo:
            c.targetFlags = UI::CommandID::TrophyTwo;
            break;
        case Actor::ID::PlayerThree:
            c.targetFlags = UI::CommandID::TrophyThree;
            break;
        case Actor::ID::PlayerFour:
            c.targetFlags = UI::CommandID::TrophyFour;
            break;
        }
        c.action = [i](xy::Entity e, float)
        {
            e.getComponent<xy::SpriteAnimation>().play(i);
        };
        m_uiScene.getSystem<xy::CommandSystem>().sendCommand(c);
    }

    //send message to start menu display
    xy::Command cmd;
    cmd.targetFlags = UI::CommandID::RoundEndMenu;
    cmd.action = [](xy::Entity entity, float)
    {
        entity.getComponent<xy::Callback>().active = true;
    };
    m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

    cmd.targetFlags = UI::CommandID::TreasureScore;
    cmd.action = [&](xy::Entity entity, float)
    {
        entity.getComponent<xy::Text>().setFont(m_fontResource.get(Global::TitleFont));
        entity.getComponent<xy::Text>().setString("Winner!");
        entity.getComponent<xy::Text>().setCharacterSize(120);
        entity.getComponent<xy::Text>().setFillColour(Global::InnerTextColour);
        entity.getComponent<xy::Text>().setOutlineColour(Global::OuterTextColour);
        entity.getComponent<xy::Text>().setOutlineThickness(1.f);
    };
    m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

    //hide the clock
    cmd.targetFlags = UI::CommandID::RoundTimer;
    cmd.action = [](xy::Entity e, float)
    {
        e.getComponent<xy::Callback>().active = false;
        e.getComponent<xy::Text>().setFillColour(sf::Color::Transparent);
    };
    m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

    //set the camera to zoom onto the winner
    cmd.targetFlags = CommandID::NetInterpolator | CommandID::LocalPlayer;
    cmd.action = [&, winnerID](xy::Entity entity, float)
    {
        auto id = entity.getComponent<Actor>().id;
        if (id == winnerID)
        {
            m_gameScene.getSystem<Camera3DSystem>().enableChase(true);
            m_gameScene.getActiveCamera().getComponent<Camera3D>().target = entity;
        }

        //stop animation
        if ((id >= Actor::ID::PlayerOne && id <= Actor::ID::PlayerFour) || id == Actor::ID::Skeleton)
        {
            entity.getComponent<AnimationModifier>().nextAnimation = AnimationID::IdleDown;
        }
    };
    m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

    //disable any client side bots
    cmd.targetFlags = CommandID::LocalPlayer;
    cmd.action = [](xy::Entity ent, float)
    {
        ent.getComponent<Bot>().enabled = false;
    };
    m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

    //disable unnecessary systems - why is this unnecessary?
    //disabling it breaks the weapon animations
    //m_gameScene.setSystemActive<ClientWeaponSystem>(false);
}

void GameState::showEndButton()
{
    xy::App::setMouseCursorVisible(true);
    m_uiScene.setSystemActive<xy::UISystem>(true);

    auto entity = m_uiScene.createEntity();
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>(m_textureResource.get("assets/images/buttons.png"));
    auto bounds = entity.getComponent<xy::Sprite>().getTextureRect();
    entity.addComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseUp] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&](xy::Entity entity, std::uint64_t flags)
    {
        if (flags & xy::UISystem::Flags::LeftMouse)
        {
            requestStackClear();
            requestStackPush(StateID::Menu);
        }
    });
    //not used but slider callback requires it
    entity.addComponent<xy::CommandTarget>().ID = 0;
    entity.addComponent<xy::Callback>().function = UISlideCallback(getContext().appInstance.getMessageBus());
    entity.getComponent<xy::Callback>().active = true;
    entity.addComponent<sf::Vector2f>() = { xy::DefaultSceneSize.x / 2.f, 942.f };
    entity.getComponent<xy::Transform>().setPosition(xy::DefaultSceneSize.x / 2.f, xy::DefaultSceneSize.y + 64.f);
}

void GameState::showServerMessage(std::int32_t messageID)
{
    sf::String msgString;

    switch (messageID)
    {
    default: break;
    case Server::Message::OneTreasureRemaining:
        msgString = Server::messages[Server::Message::OneTreasureRemaining];
        break;
    case Server::Message::PlayerOneScored:
        msgString = m_sharedData.clientInformation.getClient(0).name 
            + Server::messages[xy::Util::Random::value(Server::Message::PlayerOneScored, Server::Message::PlayerFourScored)];
        break;
    case Server::Message::PlayerTwoScored:
        msgString = m_sharedData.clientInformation.getClient(1).name
            + Server::messages[xy::Util::Random::value(Server::Message::PlayerOneScored, Server::Message::PlayerFourScored)];
        break;
    case Server::Message::PlayerThreeScored:
        msgString = m_sharedData.clientInformation.getClient(2).name
            + Server::messages[xy::Util::Random::value(Server::Message::PlayerOneScored, Server::Message::PlayerFourScored)];
        break;
    case Server::Message::PlayerFourScored:
        msgString = m_sharedData.clientInformation.getClient(3).name
            + Server::messages[xy::Util::Random::value(Server::Message::PlayerOneScored, Server::Message::PlayerFourScored)];
        break;
        //-------------------------
    case Server::Message::PlayerOneDrowned:
        msgString = m_sharedData.clientInformation.getClient(0).name
            + Server::messages[xy::Util::Random::value(Server::Message::PlayerOneDrowned, Server::Message::PlayerFourDrowned)];
        break;
    case Server::Message::PlayerTwoDrowned:
        msgString = m_sharedData.clientInformation.getClient(1).name
            + Server::messages[xy::Util::Random::value(Server::Message::PlayerOneDrowned, Server::Message::PlayerFourDrowned)];
        break;
    case Server::Message::PlayerThreeDrowned:
        msgString = m_sharedData.clientInformation.getClient(2).name
            + Server::messages[xy::Util::Random::value(Server::Message::PlayerOneDrowned, Server::Message::PlayerFourDrowned)];
        break;
    case Server::Message::PlayerFourDrowned:
        msgString = m_sharedData.clientInformation.getClient(3).name
            + Server::messages[xy::Util::Random::value(Server::Message::PlayerOneDrowned, Server::Message::PlayerFourDrowned)];
        break;
        //------------------------
    case Server::Message::PlayerOneDiedFromWeapon:
        msgString = m_sharedData.clientInformation.getClient(0).name
            + Server::messages[xy::Util::Random::value(Server::Message::PlayerOneDiedFromWeapon, Server::Message::PlayerFourDiedFromWeapon)];
        break;
    case Server::Message::PlayerTwoDiedFromWeapon:
        msgString = m_sharedData.clientInformation.getClient(1).name
            + Server::messages[xy::Util::Random::value(Server::Message::PlayerOneDiedFromWeapon, Server::Message::PlayerFourDiedFromWeapon)];
        break;
    case Server::Message::PlayerThreeDiedFromWeapon:
        msgString = m_sharedData.clientInformation.getClient(2).name
            + Server::messages[xy::Util::Random::value(Server::Message::PlayerOneDiedFromWeapon, Server::Message::PlayerFourDiedFromWeapon)];
        break;
    case Server::Message::PlayerFourDiedFromWeapon:
        msgString = m_sharedData.clientInformation.getClient(3).name
            + Server::messages[xy::Util::Random::value(Server::Message::PlayerOneDiedFromWeapon, Server::Message::PlayerFourDiedFromWeapon)];
        break;
        //------------------------
    case Server::Message::PlayerOneDiedFromSkeleton:
        msgString = m_sharedData.clientInformation.getClient(0).name
            + Server::messages[xy::Util::Random::value(Server::Message::PlayerOneDiedFromSkeleton, Server::Message::PlayerFourDiedFromSkeleton)];
        break;
    case Server::Message::PlayerTwoDiedFromSkeleton:
        msgString = m_sharedData.clientInformation.getClient(1).name
            + Server::messages[xy::Util::Random::value(Server::Message::PlayerOneDiedFromSkeleton, Server::Message::PlayerFourDiedFromSkeleton)];
        break;
    case Server::Message::PlayerThreeDiedFromSkeleton:
        msgString = m_sharedData.clientInformation.getClient(2).name
            + Server::messages[xy::Util::Random::value(Server::Message::PlayerOneDiedFromSkeleton, Server::Message::PlayerFourDiedFromSkeleton)];
        break;
    case Server::Message::PlayerFourDiedFromSkeleton:
        msgString = m_sharedData.clientInformation.getClient(3).name
            + Server::messages[xy::Util::Random::value(Server::Message::PlayerOneDiedFromSkeleton, Server::Message::PlayerFourDiedFromSkeleton)];
        break;
        //------------------------
    case Server::Message::PlayerOneExploded:
        msgString = m_sharedData.clientInformation.getClient(0).name
            + Server::messages[xy::Util::Random::value(Server::Message::PlayerOneExploded, Server::Message::PlayerFourExploded)];
        break;
    case Server::Message::PlayerTwoExploded:
        msgString = m_sharedData.clientInformation.getClient(1).name
            + Server::messages[xy::Util::Random::value(Server::Message::PlayerOneExploded, Server::Message::PlayerFourExploded)];
        break;
    case Server::Message::PlayerThreeExploded:
        msgString = m_sharedData.clientInformation.getClient(2).name
            + Server::messages[xy::Util::Random::value(Server::Message::PlayerOneExploded, Server::Message::PlayerFourExploded)];
        break;
    case Server::Message::PlayerFourExploded:
        msgString = m_sharedData.clientInformation.getClient(3).name
            + Server::messages[xy::Util::Random::value(Server::Message::PlayerOneExploded, Server::Message::PlayerFourExploded)];
        break;
        //-------------------------
    case Server::Message::PlayerOneDiedFromBees:
        msgString = m_sharedData.clientInformation.getClient(0).name
            + Server::messages[xy::Util::Random::value(Server::Message::PlayerOneDiedFromBees, Server::Message::PlayerFourDiedFromBees)];
        break;
    case Server::Message::PlayerTwoDiedFromBees:
        msgString = m_sharedData.clientInformation.getClient(1).name
            + Server::messages[xy::Util::Random::value(Server::Message::PlayerOneDiedFromBees, Server::Message::PlayerFourDiedFromBees)];
        break;
    case Server::Message::PlayerThreeDiedFromBees:
        msgString = m_sharedData.clientInformation.getClient(2).name
            + Server::messages[xy::Util::Random::value(Server::Message::PlayerOneDiedFromBees, Server::Message::PlayerFourDiedFromBees)];
        break;
    case Server::Message::PlayerFourDiedFromBees:
        msgString = m_sharedData.clientInformation.getClient(3).name
            + Server::messages[xy::Util::Random::value(Server::Message::PlayerOneDiedFromBees, Server::Message::PlayerFourDiedFromBees)];
        break;
    }

    if (messageID == Server::Message::OneTreasureRemaining)
    {
        //BIG
        auto entity = m_uiScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize.x, 310.f);
        entity.addComponent<xy::Text>(m_fontResource.get(Global::TitleFont));
        entity.getComponent<xy::Text>().setString(msgString);
        entity.getComponent<xy::Text>().setCharacterSize(80);
        entity.getComponent<xy::Text>().setFillColour(UI::Colour::MessageText);
        entity.addComponent<xy::Drawable>();
        auto width = xy::Text::getLocalBounds(entity).width;
        entity.addComponent<xy::Callback>().function = 
            [&, width](xy::Entity ent, float dt)
        {
            ent.getComponent<xy::Transform>().move(-530.f * dt, 0.f);
            if (ent.getComponent<xy::Transform>().getPosition().x < -width)
            {
                m_uiScene.destroyEntity(ent);
            }
        };
        entity.getComponent<xy::Callback>().active = true;

        //tell the treasure icon to flash
        xy::Command cmd;
        cmd.targetFlags = UI::CommandID::TreasureIcon;
        cmd.action = [](xy::Entity entity, float)
        {
            entity.getComponent<xy::Callback>().active = true;
        };
        m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

        //and start clock display
        cmd.targetFlags = UI::CommandID::RoundTimer;
        cmd.action = [](xy::Entity e, float)
        {
            e.getComponent<xy::Callback>().active = true;
        };
        m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
    }
    else
    {
        //small.
        printMessage(msgString);
    }
}

void GameState::printMessage(const sf::String& msgString)
{
    m_messageQueue.push(msgString);
}

void GameState::processMessageQueue()
{
    if (m_canShowMessage && !m_messageQueue.empty())
    {
        auto msgString = m_messageQueue.front();
        m_messageQueue.pop();

        auto entity = m_uiScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize.x - 40.f, -20.f);
        entity.addComponent<xy::Text>(m_fontResource.get(Global::FineFont));
        entity.getComponent<xy::Text>().setString(msgString);
        entity.getComponent<xy::Text>().setCharacterSize(32);
        entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Right);
        //entity.getComponent<xy::Text>().setFillColour(MessageColour);
        entity.addComponent<xy::Drawable>();
        entity.addComponent<xy::Callback>().function = ServerMessageCallback(m_uiScene, getContext().appInstance.getMessageBus());
        entity.getComponent<xy::Callback>().active = true;
        entity.addComponent<xy::CommandTarget>().ID = UI::CommandID::ServerMessage;

        m_canShowMessage = false;
    }
}

void GameState::plotPath(const std::vector<sf::Vector2f>& points)
{
    auto entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>();
    auto& dwb = entity.addComponent<xy::Drawable>();
    for (auto p : points)
    {
        dwb.getVertices().emplace_back(p, sf::Color::Red, p);
    }
    dwb.updateLocalBounds();
    dwb.setPrimitiveType(sf::LineStrip);
    dwb.setShader(&m_shaderResource.get(ShaderID::PlaneShader));
    m_textureResource.setFallbackColour(sf::Color::Red);
    dwb.setTexture(&m_textureResource.get("red"));
    dwb.setDepth(15000);
}