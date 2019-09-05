#include "MenuPlayerSystem.hpp"
#include "PhysicsSystem.hpp"

#include <chipmunk/chipmunk.h>

namespace
{
    const float Gravity = 8800.f;
    const float MaxVelocity = 3200.f;
    const float Acceleration = 8800.f;

    //const float speed = 280.f;
    const float initialJumpVelocity = 2600.f;// 1260.f;
    const float minJumpVelocity = -initialJumpVelocity * 0.2f;
}

MenuPlayerSystem::MenuPlayerSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(MenuPlayerSystem))
{
    requireComponent<MenuPlayer>();
    requireComponent<PhysicsObject>();
    requireComponent<InputComponent>();
}

//public
void MenuPlayerSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& player = entity.getComponent<MenuPlayer>();
        auto& input = entity.getComponent<InputComponent>();

        std::size_t idx = (input.currentInput + input.history.size() - 1) % input.history.size();
        while (input.lastUpdatedInput != idx)
        {
            auto delta = getDelta(input.history, input.lastUpdatedInput);
            processInput(input.history[input.lastUpdatedInput].input.mask, delta, entity);

            input.lastUpdatedInput = (input.lastUpdatedInput + 1) % input.history.size();
        }
        auto body = entity.getComponent<PhysicsObject>().getBody();
        cpBodySetVelocity(body, Convert::toPhysVec(player.velocity));
    }
}

void MenuPlayerSystem::reconcile()
{

}

//private
float MenuPlayerSystem::getDelta(const History& history, std::size_t idx)
{
    auto prevInput = (idx + history.size() - 1) % history.size();
    auto delta = history[idx].input.timestamp - history[prevInput].input.timestamp;

    return static_cast<float>(delta) / 1000000.f;
}

void MenuPlayerSystem::processInput(std::uint16_t mask, float dt, xy::Entity entity)
{
    auto& player = entity.getComponent<MenuPlayer>();
    if (!player.onSurface)
    {
        //apply gravity
        player.velocity.y += Gravity * dt;
        player.velocity.y = std::min(player.velocity.y, MaxVelocity);
    }
    else
    {
        //on the ground
        if (mask & InputFlag::Up &&
            (player.flags & MenuPlayer::CanJump))
        {
            player.velocity.y = -initialJumpVelocity;
            player.flags &= ~MenuPlayer::CanJump;
        }
    }

    player.velocity.x *= 0.8f;
    if (mask & InputFlag::Left)
    {
        player.velocity.x = std::max(-MaxVelocity, player.velocity.x - Acceleration * dt);
    }
    if (mask & InputFlag::Right)
    {
        player.velocity.x = std::min(MaxVelocity, player.velocity.x + Acceleration * dt);
    }
    if ((mask & InputFlag::Up) == 0)
    {
        player.flags |= MenuPlayer::CanJump;
    }
}