/*********************************************************************
(c) Matt Marchant 2019
http://trederia.blogspot.com

osgc - Open Source Game Collection - Zlib license.

This software is provided 'as-is', without any express or
implied warranty. In no event will the authors be held
liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute
it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented;
you must not claim that you wrote the original software.
If you use this software in a product, an acknowledgment
in the product documentation would be appreciated but
is not required.

2. Altered source versions must be plainly marked as such,
and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any
source distribution.
*********************************************************************/


#include "MenuState.hpp"
#include "StateIDs.hpp"
#include "GameConsts.hpp"
#include "ShapeUtils.hpp"
#include "Collision.hpp"
#include "Player.hpp"
#include "SharedStateData.hpp"
#include "NinjaStarSystem.hpp"
#include "MessageIDs.hpp"
#include "NinjaDirector.hpp"
#include "CommandIDs.hpp"
#include "ShakerSystem.hpp"
#include "PluginExport.hpp"
#include "FluidAnimationSystem.hpp"
#include "CrateSystem.hpp"
#include "SoundEffectsDirector.hpp"
#include "MovingPlatform.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/BroadPhaseComponent.hpp>
#include <xyginext/ecs/components/Callback.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/AudioListener.hpp>

#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/SpriteAnimator.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/CameraSystem.hpp>
#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>
#include <xyginext/ecs/systems/CallbackSystem.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/ecs/systems/AudioSystem.hpp>
#include <xyginext/ecs/systems/ParticleSystem.hpp>

#include <xyginext/gui/Gui.hpp>
#include <xyginext/detail/Operators.hpp>
#include <xyginext/graphics/SpriteSheet.hpp>
#include <xyginext/util/Rectangle.hpp>

#include <SFML/Graphics/Font.hpp>

namespace
{
#include "tilemap.inl"
#include "transition.inl"

    sf::FloatRect operator * (sf::FloatRect r, float f)
    {
        return { r.left * f, r.top * f, r.width * f, r.height * f };
    }

    struct MenuItem final
    {
        std::int32_t ID = -1;
        float coolDown = 0.f;
    };

    struct MenuCallback final
    {
        void operator () (xy::Entity e, float dt)
        {
            e.getComponent<MenuItem>().coolDown -= dt;
        }
    };
}

MenuState::MenuState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State         (ss, ctx),
    m_sharedData        (sd),
    m_backgroundScene   (ctx.appInstance.getMessageBus()),
    m_fontID            (0),
    m_playerInput       (sd.inputBinding)
{
    launchLoadingScreen();

    m_sharedData.reset();
    m_sharedData.inventory.ammo = 1;

    initScene();
    loadResources();
    buildBackground();
    buildMenu();

    m_backgroundScene.getActiveCamera().getComponent<xy::Camera>().setView(ctx.defaultView.getSize());
    m_backgroundScene.getActiveCamera().getComponent<xy::Camera>().setViewport(ctx.defaultView.getViewport());

    registerCommand("map", [&](const std::string params)
        {
            if (xy::FileSystem::fileExists(xy::FileSystem::getResourcePath() + "assets/maps/" + params + ".tmx"))
            {
                m_sharedData.nextMap = params + ".tmx";
                requestStackClear();
                requestStackPush(StateID::Game);
            }
            else
            {
                xy::Console::print(params + ": map not found");
            }
        });

    quitLoadingScreen();
}

//public
bool MenuState::handleEvent(const sf::Event& evt)
{
	//prevents events being forwarded if the console wishes to consume them
	if (xy::Nim::wantsKeyboard() || xy::Nim::wantsMouse())
    {
        return true;
    }

    if (evt.type == sf::Event::KeyReleased)
    {
        switch (evt.key.code)
        {
        default: break;
        case sf::Keyboard::F2:
        {
            xy::Command cmd;
            cmd.targetFlags = CommandID::World::DebugItem;
            cmd.action = [](xy::Entity e, float)
            {
                auto scale = e.getComponent<xy::Transform>().getScale();
                if (scale.x == 0)
                {
                    scale = { 1.f, 1.f };
                }
                else
                {
                    scale = {};
                }
                e.getComponent<xy::Transform>().setScale(scale);
            };
            m_backgroundScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
        }
        break;
        }
    }

    m_playerInput.handleEvent(evt);
    m_backgroundScene.forwardEvent(evt);

    return true;
}

void MenuState::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::StarMessage)
    {
        const auto& data = msg.getData<StarEvent>();
        if (data.type == StarEvent::HitItem &&
            data.entityHit.getComponent<xy::BroadphaseComponent>().getFilterFlags() & CollisionShape::Text)
        {
            auto entity = data.entityHit;
            auto& item = entity.getComponent<MenuItem>();
            
            switch (item.ID)
            {
            default:break;
            case MenuID::NewGame:
            case MenuID::Continue:
                if (item.coolDown < 0)
                {
                    m_sharedData.menuID = item.ID;
                    item.coolDown = 5.f;
                    requestStackPush(StateID::MenuConfirm);
                }
                break;
            case MenuID::Options:
                if (item.coolDown < 0)
                {
                    xy::Console::show();
                    item.coolDown = 5.f;
                }
                break;
            case MenuID::Quit:
                requestStackClear();
                requestStackPush(StateID::ParentState);
                break;
            }
        }
    }

    m_backgroundScene.forwardMessage(msg);
}

bool MenuState::update(float dt)
{
    m_playerInput.update();
    m_backgroundScene.update(dt);
    return true;
}

void MenuState::draw()
{
    auto& rw = getContext().renderWindow;
    rw.draw(m_backgroundScene);
}

xy::StateID MenuState::stateID() const
{
    return StateID::MainMenu;
}

//private
void MenuState::initScene()
{
    auto& mb = getContext().appInstance.getMessageBus();

    m_backgroundScene.addSystem<xy::CallbackSystem>(mb);
    m_backgroundScene.addSystem<xy::CommandSystem>(mb);
    m_backgroundScene.addSystem<xy::DynamicTreeSystem>(mb);
    m_backgroundScene.addSystem<PlayerSystem>(mb, m_sharedData);
    m_backgroundScene.addSystem<CrateSystem>(mb, m_sharedData);
    m_backgroundScene.addSystem<NinjaStarSystem>(mb);
    m_backgroundScene.addSystem<ShakerSystem>(mb);
    m_backgroundScene.addSystem<FluidAnimationSystem>(mb);
    m_backgroundScene.addSystem<xy::TextSystem>(mb);
    m_backgroundScene.addSystem<xy::SpriteAnimator>(mb);
    m_backgroundScene.addSystem<xy::SpriteSystem>(mb);
    m_backgroundScene.addSystem<xy::CameraSystem>(mb);
    m_backgroundScene.addSystem<xy::RenderSystem>(mb);
    m_backgroundScene.addSystem<xy::ParticleSystem>(mb);
    m_backgroundScene.addSystem<xy::AudioSystem>(mb);

    m_backgroundScene.addSystem<MovingPlatformSystem>(mb);

    m_backgroundScene.addDirector<NinjaDirector>(m_sprites, m_particleEmitters, m_sharedData);
    m_backgroundScene.addDirector<SFXDirector>(m_sharedData);

    m_backgroundScene.getActiveListener().getComponent<xy::AudioListener>().setDepth(1000.f);
}

void MenuState::loadResources()
{
    m_textureIDs[TextureID::Menu::Background] = m_resources.load<sf::Texture>("assets/images/gearboy/background.png");
    
    xy::SpriteSheet spriteSheet;
    spriteSheet.loadFromFile("assets/sprites/gearboy/player.spt", m_resources);
    m_sprites[SpriteID::GearBoy::Player] = spriteSheet.getSprite("player");

    m_playerAnimations[AnimID::Player::Idle] = spriteSheet.getAnimationIndex("idle", "player");
    m_playerAnimations[AnimID::Player::Jump] = spriteSheet.getAnimationIndex("jump", "player");
    m_playerAnimations[AnimID::Player::Run] = spriteSheet.getAnimationIndex("run", "player");
    m_playerAnimations[AnimID::Player::Die] = spriteSheet.getAnimationIndex("die", "player");

    spriteSheet.loadFromFile("assets/sprites/gearboy/projectile.spt", m_resources);
    m_sprites[SpriteID::GearBoy::Star] = spriteSheet.getSprite("projectile");

    spriteSheet.loadFromFile("assets/sprites/gearboy/smoke_puff.spt", m_resources);
    m_sprites[SpriteID::GearBoy::SmokePuff] = spriteSheet.getSprite("smoke_puff");

    spriteSheet.loadFromFile("assets/sprites/gearboy/lava.spt", m_resources);
    m_sprites[SpriteID::GearBoy::Lava] = spriteSheet.getSprite("lava");
    auto* tex = m_sprites[SpriteID::GearBoy::Lava].getTexture();
    if (tex) //uuugghhhhhh
    {
        const_cast<sf::Texture*>(tex)->setRepeated(true);
    }

    spriteSheet.loadFromFile("assets/sprites/gearboy/water.spt", m_resources);
    m_sprites[SpriteID::GearBoy::Water] = spriteSheet.getSprite("water");
    tex = m_sprites[SpriteID::GearBoy::Water].getTexture();
    if (tex) //uuugghhhhhhghghghghghdddhhhhhh
    {
        const_cast<sf::Texture*>(tex)->setRepeated(true);
    }

    spriteSheet.loadFromFile("assets/sprites/gearboy/crate.spt", m_resources);
    m_sprites[SpriteID::GearBoy::Crate] = spriteSheet.getSprite("crate");

    m_shaders.preload(ShaderID::TileMap, tilemapFrag, sf::Shader::Fragment);
    m_shaders.preload(ShaderID::PixelTransition, PixelateFrag, sf::Shader::Fragment);
    m_sharedData.transitionContext.shader = &m_shaders.get(ShaderID::PixelTransition);

    m_particleEmitters[ParticleID::Crate].loadFromFile("assets/particles/gearboy/crate.xyp", m_resources);

    m_fontID = m_resources.load<sf::Font>(FontID::GearBoyFont);
}

void MenuState::buildBackground()
{
    if (m_mapLoader.load("menu.tmx"))
    {
        const float pixelScale = GameConst::PixelsPerTile / m_mapLoader.getTileSize();
        m_backgroundScene.getDirector<NinjaDirector>().setSpriteScale(pixelScale);
        const auto& layers = m_mapLoader.getLayers();

        //for each layer create a drawable in the scene
        std::int32_t startDepth = GameConst::BackgroundDepth + 2;
        for (const auto& layer : layers)
        {
            auto entity = m_backgroundScene.createEntity();
            entity.addComponent<xy::Transform>();
            entity.addComponent<xy::Drawable>().setDepth(startDepth++);
            entity.getComponent<xy::Drawable>().setTexture(layer.indexMap);
            entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::TileMap));
            entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_indexMap");
            entity.getComponent<xy::Drawable>().bindUniform("u_tileSet", *layer.tileSet);
            entity.getComponent<xy::Drawable>().bindUniform("u_indexSize", sf::Vector2f(layer.indexMap->getSize()));
            entity.getComponent<xy::Drawable>().bindUniform("u_tileCount", layer.tileSetSize);

            auto& verts = entity.getComponent<xy::Drawable>().getVertices();
            sf::Vector2f texCoords(layer.indexMap->getSize());

            verts.emplace_back(sf::Vector2f(), sf::Vector2f());
            verts.emplace_back(sf::Vector2f(layer.layerSize.x, 0.f), sf::Vector2f(texCoords.x, 0.f));
            verts.emplace_back(layer.layerSize, texCoords);
            verts.emplace_back(sf::Vector2f(0.f, layer.layerSize.y), sf::Vector2f(0.f, texCoords.y));

            entity.getComponent<xy::Drawable>().updateLocalBounds();
            entity.getComponent<xy::Transform>().setScale(pixelScale, pixelScale);
        }

        //background
        auto entity = m_backgroundScene.createEntity();
        entity.addComponent<xy::Transform>().setScale(pixelScale, pixelScale);
        entity.addComponent<xy::Drawable>().setDepth(GameConst::BackgroundDepth);
        entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Menu::Background]));

        entity = m_backgroundScene.createEntity();
        entity.addComponent<xy::Transform>().setScale(pixelScale, pixelScale);
        entity.addComponent<xy::Drawable>().setDepth(GameConst::BackgroundDepth + 1);
        entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Menu::Background]));
        entity.getComponent<xy::Sprite>().setTextureRect({ 0.f, 272.f, 480.f, 86.f });

        //player
        entity = m_backgroundScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(m_mapLoader.getSpawnPoint() * pixelScale);
        entity.getComponent<xy::Transform>().setScale(pixelScale, pixelScale);
        entity.addComponent<xy::Drawable>();
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::GearBoy::Player];
        entity.addComponent<xy::SpriteAnimation>().play(0);
        auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
        entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height);
        
        auto& collision = entity.addComponent<CollisionBody>();
        collision.shapes[0].aabb = GameConst::Gearboy::PlayerBounds;
        collision.shapes[0].type = CollisionShape::Player;
        collision.shapes[0].collisionFlags = CollisionGroup::PlayerFlags;

        collision.shapes[1].aabb = GameConst::Gearboy::PlayerFoot;
        collision.shapes[1].type = CollisionShape::Foot;
        collision.shapes[1].collisionFlags = CollisionGroup::PlayerFlags;

        collision.shapes[2].aabb = GameConst::Gearboy::PlayerLeftHand;
        collision.shapes[2].type = CollisionShape::LeftHand;
        collision.shapes[2].collisionFlags = CollisionGroup::PlayerFlags;

        collision.shapes[3].aabb = GameConst::Gearboy::PlayerRightHand;
        collision.shapes[3].type = CollisionShape::RightHand;
        collision.shapes[3].collisionFlags = CollisionGroup::PlayerFlags;

        collision.shapeCount = 4;

        auto aabb = xy::Util::Rectangle::combine(GameConst::Gearboy::PlayerBounds, GameConst::Gearboy::PlayerFoot);
        aabb = xy::Util::Rectangle::combine(GameConst::Gearboy::PlayerLeftHand, aabb);
        aabb = xy::Util::Rectangle::combine(GameConst::Gearboy::PlayerRightHand, aabb);

        entity.addComponent<xy::BroadphaseComponent>().setArea(aabb);
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(CollisionShape::Player | CollisionShape::Foot | CollisionShape::LeftHand | CollisionShape::RightHand);
        entity.addComponent<Player>().animations = m_playerAnimations;
        m_playerInput.setPlayerEntity(entity);

#ifdef XY_DEBUG
        /*entity.addComponent<xy::Callback>().active = true;
        entity.getComponent<xy::Callback>().function = [aabb](xy::Entity e, float)
        {
            const auto& tx = e.getComponent<xy::Transform>();
            auto b = boundsToWorldSpace(aabb, tx);
            auto c = tx.getWorldTransform().transformRect(aabb);
            c.left += tx.getOrigin().x * tx.getScale().x;
            c.top += tx.getOrigin().y * tx.getScale().y;

            auto pos = tx.getPosition();
            DPRINT("Pos", std::to_string(pos.x) + ", " + std::to_string(pos.y));
            DPRINT("aabb", std::to_string(aabb.left) + ", " + std::to_string(aabb.top) + ", " + std::to_string(aabb.width) + ", " + std::to_string(aabb.height));
            DPRINT("b", std::to_string(b.left) + ", " + std::to_string(b.top) + ", " + std::to_string(b.width) + ", " + std::to_string(b.height));
            DPRINT("c", std::to_string(c.left) + ", " + std::to_string(c.top) + ", " + std::to_string(c.width) + ", " + std::to_string(c.height));
        };*/

        //player debug bounds
        auto debugEnt = m_backgroundScene.createEntity();
        debugEnt.addComponent<xy::Transform>().setPosition(entity.getComponent<xy::Transform>().getOrigin());
        debugEnt.getComponent<xy::Transform>().move(GameConst::Gearboy::PlayerBounds.left, GameConst::Gearboy::PlayerBounds.top);
        Shape::setRectangle(debugEnt.addComponent<xy::Drawable>(), { GameConst::Gearboy::PlayerBounds.width, GameConst::Gearboy::PlayerBounds.height }, sf::Color::Red);
        debugEnt.getComponent<xy::Transform>().setScale(0.f, 0.f);
        debugEnt.addComponent<xy::CommandTarget>().ID = CommandID::World::DebugItem;
        entity.getComponent<xy::Transform>().addChild(debugEnt.getComponent<xy::Transform>());

        debugEnt = m_backgroundScene.createEntity();
        debugEnt.addComponent<xy::Transform>().setPosition(entity.getComponent<xy::Transform>().getOrigin());
        debugEnt.getComponent<xy::Transform>().move(GameConst::Gearboy::PlayerFoot.left, GameConst::Gearboy::PlayerFoot.top);
        Shape::setRectangle(debugEnt.addComponent<xy::Drawable>(), { GameConst::Gearboy::PlayerFoot.width, GameConst::Gearboy::PlayerFoot.height }, sf::Color::Blue);
        debugEnt.getComponent<xy::Transform>().setScale(0.f, 0.f);
        debugEnt.addComponent<xy::CommandTarget>().ID = CommandID::World::DebugItem;
        entity.getComponent<xy::Transform>().addChild(debugEnt.getComponent<xy::Transform>());

        debugEnt = m_backgroundScene.createEntity();
        debugEnt.addComponent<xy::Transform>().setPosition(entity.getComponent<xy::Transform>().getOrigin());
        debugEnt.getComponent<xy::Transform>().move(GameConst::Gearboy::PlayerLeftHand.left, GameConst::Gearboy::PlayerLeftHand.top);
        Shape::setRectangle(debugEnt.addComponent<xy::Drawable>(), { GameConst::Gearboy::PlayerLeftHand.width, GameConst::Gearboy::PlayerLeftHand.height }, sf::Color::Blue);
        debugEnt.getComponent<xy::Transform>().setScale(0.f, 0.f);
        debugEnt.addComponent<xy::CommandTarget>().ID = CommandID::World::DebugItem;
        entity.getComponent<xy::Transform>().addChild(debugEnt.getComponent<xy::Transform>());

        debugEnt = m_backgroundScene.createEntity();
        debugEnt.addComponent<xy::Transform>().setPosition(entity.getComponent<xy::Transform>().getOrigin());
        debugEnt.getComponent<xy::Transform>().move(GameConst::Gearboy::PlayerRightHand.left, GameConst::Gearboy::PlayerRightHand.top);
        Shape::setRectangle(debugEnt.addComponent<xy::Drawable>(), { GameConst::Gearboy::PlayerRightHand.width, GameConst::Gearboy::PlayerRightHand.height }, sf::Color::Blue);
        debugEnt.getComponent<xy::Transform>().setScale(0.f, 0.f);
        debugEnt.addComponent<xy::CommandTarget>().ID = CommandID::World::DebugItem;
        entity.getComponent<xy::Transform>().addChild(debugEnt.getComponent<xy::Transform>());

        debugEnt = m_backgroundScene.createEntity();
        debugEnt.addComponent<xy::Transform>().setPosition(entity.getComponent<xy::Transform>().getOrigin());
        debugEnt.getComponent<xy::Transform>().move(aabb.left, aabb.top);
        Shape::setRectangle(debugEnt.addComponent<xy::Drawable>(), { aabb.width, aabb.height }, sf::Color::Yellow);
        debugEnt.getComponent<xy::Transform>().setScale(0.f, 0.f);
        debugEnt.addComponent<xy::CommandTarget>().ID = CommandID::World::DebugItem;
        entity.getComponent<xy::Transform>().addChild(debugEnt.getComponent<xy::Transform>());
#endif //XY_DEBUG


        const auto& collisionShapes = m_mapLoader.getCollisionShapes();
        const auto& platPaths = m_mapLoader.getPlatformPaths();
        for (const auto& shape : collisionShapes)
        {
            entity = m_backgroundScene.createEntity();
            entity.addComponent<xy::Transform>().setPosition(shape.aabb.left * pixelScale, shape.aabb.top * pixelScale);
            entity.addComponent<CollisionBody>().shapes[0] = shape;
            //convert from world to local
            entity.getComponent<CollisionBody>().shapes[0].aabb.left = 0;
            entity.getComponent<CollisionBody>().shapes[0].aabb.top = 0;
            entity.getComponent<CollisionBody>().shapes[0].aabb.width *= pixelScale;
            entity.getComponent<CollisionBody>().shapes[0].aabb.height *= pixelScale;
            entity.getComponent<CollisionBody>().shapeCount = 1;
            entity.addComponent<xy::BroadphaseComponent>().setArea(entity.getComponent<CollisionBody>().shapes[0].aabb);
            entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(shape.type);

            if (shape.ID > -1)
            {
                switch (shape.type)
                {
                default: break;
                case CollisionShape::MPlat:
                    if (platPaths.count(shape.ID) == 0)
                    {
                        //missing path, kill platform
                        m_backgroundScene.destroyEntity(entity);
                    }
                    else
                    {
                        //convert path to world scale
                        auto path = platPaths.at(shape.ID);
                        std::transform(path.begin(), path.end(), path.begin(),
                            [pixelScale](sf::Vector2f v) { return v * pixelScale; });

                        //move plat to beginning
                        entity.getComponent<xy::Transform>().setPosition(path[0]);

                        //add path
                        entity.addComponent<MovingPlatform>().path = path;

                        //set texture
                        entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(TextureID::Menu::MovingPlatform));
                        entity.getComponent<xy::Sprite>().setTextureRect(entity.getComponent<CollisionBody>().shapes[0].aabb);
                        entity.addComponent<xy::Drawable>();
                    }
                    break;
                case CollisionShape::Fluid:
                {
                    sf::Vector2f size(shape.aabb.width, shape.aabb.height);
                    size *= pixelScale;

                    entity.addComponent<xy::Drawable>();
                    auto& verts = entity.getComponent<xy::Drawable>().getVertices();
                    verts.emplace_back(sf::Vector2f());
                    verts.emplace_back(sf::Vector2f(size.x, 0.f), sf::Vector2f(size.x, 0.f));
                    verts.emplace_back(size, size);
                    verts.emplace_back(sf::Vector2f(0.f, size.y), sf::Vector2f(0.f, size.y));

                    entity.getComponent<xy::Drawable>().updateLocalBounds();
                    std::int32_t spriteID;
                    if (shape.ID == 0)
                    {
                        spriteID = SpriteID::GearBoy::Lava;
                    }
                    else
                    {
                        spriteID = SpriteID::GearBoy::Water;
                    }
                    entity.getComponent<xy::Drawable>().setTexture(m_sprites[spriteID].getTexture());
                    entity.addComponent<Fluid>().frameSize = m_sprites[spriteID].getTextureBounds().height;
                    if (m_sprites[spriteID].getAnimations()[0].framerate != 0)
                    {
                        entity.getComponent<Fluid>().frameTime = 1.f / m_sprites[spriteID].getAnimations()[0].framerate;
                    }
                }
                    break;
                case CollisionShape::Checkpoint:
                    entity.addComponent<xy::CommandTarget>().ID = CommandID::World::CheckPoint;
                    break;
                }
            }

#ifdef XY_DEBUG
            debugEnt = m_backgroundScene.createEntity();
            debugEnt.addComponent<xy::CommandTarget>().ID = CommandID::World::DebugItem;
            debugEnt.addComponent<xy::Transform>();
            Shape::setRectangle(debugEnt.addComponent<xy::Drawable>(), { shape.aabb.width * pixelScale, shape.aabb.height * pixelScale }, sf::Color::Red);
            debugEnt.getComponent<xy::Transform>().setScale(0.f, 0.f); //press F2 to toggle visibility
            entity.getComponent<xy::Transform>().addChild(debugEnt.getComponent<xy::Transform>());
#endif //XY_DEBUG
        }

        const auto& props = m_mapLoader.getProps();
        for (const auto& [id, bounds] : props)
        {
            if (id == PropID::CrateSpawn)
            {
                spawnCrate({ bounds.left * pixelScale, bounds.top * pixelScale });
            }
        }

        m_backgroundScene.getSystem<PlayerSystem>().setBounds({ sf::Vector2f(), m_mapLoader.getMapSize() * pixelScale });
    }
}

void MenuState::buildMenu()
{
    auto& font = m_resources.get<sf::Font>(m_fontID);

    auto entity = m_backgroundScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(140.f, 100.f);
    entity.addComponent<xy::Drawable>().setDepth(GameConst::TextDepth);
    entity.addComponent<xy::Text>(font).setString("Back To Computer!");
    entity.getComponent<xy::Text>().setCharacterSize(/*GameConst::UI::LargeTextSize*/96);
    entity.getComponent<xy::Text>().setFillColour(GameConst::Gearboy::colours[0]);
    entity.getComponent<xy::Text>().setOutlineColour(GameConst::Gearboy::colours[2]);
    entity.getComponent<xy::Text>().setOutlineThickness(4.f);

    entity = m_backgroundScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(1100.f, 300.f);
    entity.addComponent<xy::Drawable>().setDepth(GameConst::TextDepth);
    entity.addComponent<xy::Text>(font).setString("New Game");
    entity.getComponent<xy::Text>().setCharacterSize(GameConst::UI::MediumTextSize);
    entity.getComponent<xy::Text>().setFillColour(GameConst::Gearboy::colours[0]);
    entity.getComponent<xy::Text>().setOutlineColour(GameConst::Gearboy::colours[2]);
    entity.getComponent<xy::Text>().setOutlineThickness(2.f);
    entity.addComponent<xy::BroadphaseComponent>().setArea(xy::Text::getLocalBounds(entity));
    entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(CollisionShape::Text);
    entity.addComponent<Shaker>();
    entity.addComponent<MenuItem>().ID = MenuID::NewGame;
    entity.addComponent<xy::Callback>().active = true;
    entity.getComponent<xy::Callback>().function = MenuCallback();

    entity = m_backgroundScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(1200.f, 390.f);
    entity.addComponent<xy::Drawable>().setDepth(GameConst::TextDepth);
    entity.addComponent<xy::Text>(font).setString("Continue");
    entity.getComponent<xy::Text>().setCharacterSize(GameConst::UI::MediumTextSize);
    entity.getComponent<xy::Text>().setFillColour(GameConst::Gearboy::colours[0]);
    entity.getComponent<xy::Text>().setOutlineColour(GameConst::Gearboy::colours[2]);
    entity.getComponent<xy::Text>().setOutlineThickness(2.f);
    entity.addComponent<xy::BroadphaseComponent>().setArea(xy::Text::getLocalBounds(entity));
    entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(CollisionShape::Text);
    entity.addComponent<Shaker>();
    entity.addComponent<MenuItem>().ID = MenuID::Continue;
    entity.addComponent<xy::Callback>().active = true;
    entity.getComponent<xy::Callback>().function = MenuCallback();

    entity = m_backgroundScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(800.f, 700.f);
    entity.addComponent<xy::Drawable>().setDepth(GameConst::TextDepth);
    entity.addComponent<xy::Text>(font).setString("Options");
    entity.getComponent<xy::Text>().setCharacterSize(GameConst::UI::MediumTextSize);
    entity.getComponent<xy::Text>().setFillColour(GameConst::Gearboy::colours[0]);
    entity.getComponent<xy::Text>().setOutlineColour(GameConst::Gearboy::colours[2]);
    entity.getComponent<xy::Text>().setOutlineThickness(2.f);
    entity.addComponent<xy::BroadphaseComponent>().setArea(xy::Text::getLocalBounds(entity));
    entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(CollisionShape::Text);
    entity.addComponent<Shaker>();
    entity.addComponent<MenuItem>().ID = MenuID::Options;
    entity.addComponent<xy::Callback>().active = true;
    entity.getComponent<xy::Callback>().function = MenuCallback();

    entity = m_backgroundScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(220.f, 770.f);
    entity.addComponent<xy::Drawable>().setDepth(GameConst::TextDepth);
    entity.addComponent<xy::Text>(font).setString("Quit");
    entity.getComponent<xy::Text>().setCharacterSize(GameConst::UI::MediumTextSize);
    entity.getComponent<xy::Text>().setFillColour(GameConst::Gearboy::colours[0]);
    entity.getComponent<xy::Text>().setOutlineColour(GameConst::Gearboy::colours[2]);
    entity.getComponent<xy::Text>().setOutlineThickness(2.f);
    entity.addComponent<xy::BroadphaseComponent>().setArea(xy::Text::getLocalBounds(entity));
    entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(CollisionShape::Text);
    entity.addComponent<Shaker>();
    entity.addComponent<MenuItem>().ID = MenuID::Quit;
}

void MenuState::spawnCrate(sf::Vector2f position)
{
    const float scale = GameConst::PixelsPerTile / m_mapLoader.getTileSize();

    auto entity = m_backgroundScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(position);
    entity.getComponent<xy::Transform>().setScale(scale, scale);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::GearBoy::Crate];
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);

    bounds.left -= bounds.width / 2.f;
    bounds.top -= bounds.height / 2.f;
    
    entity.addComponent<CollisionBody>().shapeCount = 2;
    entity.getComponent<CollisionBody>().shapes[0].aabb = bounds;
    entity.getComponent<CollisionBody>().shapes[0].type = CollisionShape::Crate;
    entity.getComponent<CollisionBody>().shapes[0].collisionFlags = CollisionGroup::CrateFlags;

    sf::FloatRect foot(bounds.left + 2.f, bounds.height / 2.f, bounds.width - 4.f, 4.f);
    entity.getComponent<CollisionBody>().shapes[1].aabb = foot;
    entity.getComponent<CollisionBody>().shapes[1].type = CollisionShape::Foot;
    entity.getComponent<CollisionBody>().shapes[1].collisionFlags = CollisionGroup::CrateFlags;

    entity.addComponent<xy::BroadphaseComponent>().setArea(xy::Util::Rectangle::combine(bounds, foot));
    entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(CollisionShape::Crate);

    entity.addComponent<Crate>().spawnPosition = position;
    entity.addComponent<xy::ParticleEmitter>().settings = m_particleEmitters[ParticleID::Crate];

#ifdef XY_DEBUG
    auto debugEnt = m_backgroundScene.createEntity();
    debugEnt.addComponent<xy::Transform>().setPosition(entity.getComponent<xy::Transform>().getOrigin());
    debugEnt.getComponent<xy::Transform>().move(bounds.left, bounds.top);
    Shape::setRectangle(debugEnt.addComponent<xy::Drawable>(), { bounds.width, bounds.height }, sf::Color::Red);
    debugEnt.getComponent<xy::Transform>().setScale(0.f, 0.f);
    debugEnt.addComponent<xy::CommandTarget>().ID = CommandID::World::DebugItem;
    entity.getComponent<xy::Transform>().addChild(debugEnt.getComponent<xy::Transform>());

    debugEnt = m_backgroundScene.createEntity();
    debugEnt.addComponent<xy::Transform>().setPosition(entity.getComponent<xy::Transform>().getOrigin());
    debugEnt.getComponent<xy::Transform>().move(foot.left, foot.top);
    Shape::setRectangle(debugEnt.addComponent<xy::Drawable>(), { foot.width, foot.height }, sf::Color::Blue);
    debugEnt.getComponent<xy::Transform>().setScale(0.f, 0.f);
    debugEnt.addComponent<xy::CommandTarget>().ID = CommandID::World::DebugItem;
    entity.getComponent<xy::Transform>().addChild(debugEnt.getComponent<xy::Transform>());

    auto global =xy::Util::Rectangle::combine(bounds, foot);
    debugEnt = m_backgroundScene.createEntity();
    debugEnt.addComponent<xy::Transform>().setPosition(entity.getComponent<xy::Transform>().getOrigin());
    debugEnt.getComponent<xy::Transform>().move(global.left, global.top);
    Shape::setRectangle(debugEnt.addComponent<xy::Drawable>(), { global.width, global.height }, sf::Color::Yellow);
    debugEnt.getComponent<xy::Transform>().setScale(0.f, 0.f);
    debugEnt.addComponent<xy::CommandTarget>().ID = CommandID::World::DebugItem;
    entity.getComponent<xy::Transform>().addChild(debugEnt.getComponent<xy::Transform>());
#endif //XY_DEBUG
}

void MenuState::updateLoadingScreen(float dt, sf::RenderWindow& rw)
{
    m_sharedData.loadingScreen.update(dt);
    rw.draw(m_sharedData.loadingScreen);
}