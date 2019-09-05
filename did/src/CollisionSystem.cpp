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

#include "CollisionSystem.hpp"
#include "QuadTreeFilter.hpp"
#include "GlobalConsts.hpp"
#include "Operators.hpp"

#include <xyginext/ecs/Scene.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/BroadPhaseComponent.hpp>
#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>
#include <xyginext/util/Vector.hpp>
#include <xyginext/util/Math.hpp>

namespace
{
    //this is because the graphical representation has been offset by
    //the tile centres because of marching squares
    const sf::Vector2f CollisionOffset = { Global::TileSize / 2.f, Global::TileSize / 2.f };
    const sf::FloatRect IslandRect({Global::TileSize, Global::TileSize}, Global::IslandSize);

    const float BoatRadiusSqr = Global::BoatRadius * Global::BoatRadius;
    const sf::FloatRect BoatSearchArea(-(Global::BoatRadius / 2.f), -(Global::BoatRadius / 2.f), Global::BoatRadius, Global::BoatRadius);

    enum TileType //this is a bit kludgy, we should have a unified tile Type enum
    {
        Water, Sand, Solid = Sand + 2
    };
}

CollisionSystem::CollisionSystem(xy::MessageBus& mb, bool isServer)
    : xy::System(mb, typeid(CollisionSystem)),
    m_server    (isServer)
{
    requireComponent<CollisionComponent>();
    requireComponent<xy::Transform>();
    requireComponent<xy::BroadphaseComponent>();
}

//public
void CollisionSystem::handleMessage(const xy::Message&)
{

}

void CollisionSystem::process(float)
{
    m_collisions.clear();

    auto& entities = getEntities();

    for (auto entity : entities)
    {
        auto& collision = entity.getComponent<CollisionComponent>();
        collision.manifoldCount = 0;

        if (collision.collidesTerrain)
        {
            terrainCollision(entity);
        }

        /*if (collision.collidesBoat)
        {
            boatCollision(entity);
        }*/

        actorCollision(entity);
    }
    narrowPhase();

    //resolveAll();
}

void CollisionSystem::queryState(xy::Entity entity)
{
    XY_ASSERT(entity.hasComponent<CollisionComponent>() && entity.hasComponent<xy::Transform>(), "Nope!");
    entity.getComponent<CollisionComponent>().manifoldCount = 0;

    m_collisions.swap(m_tempCollisions);

    terrainCollision(entity);
    actorCollision(entity);
    boatCollision(entity);
    narrowPhaseQuery(entity);
    //resolveAll();

    m_collisions.clear();
    m_tempCollisions.swap(m_collisions);
}

//private
void CollisionSystem::terrainCollision(xy::Entity entity)
{
    auto& collision = entity.getComponent<CollisionComponent>();
    
    //get tile from current position
    const auto& tx = entity.getComponent<xy::Transform>();
    const auto position = tx.getWorldPosition() + (tx.getOrigin() * tx.getScale()) + CollisionOffset;
    const std::size_t x = static_cast<std::size_t>(position.x / Global::TileSize);
    const std::size_t y = static_cast<std::size_t>(position.y / Global::TileSize);
    //const sf::Vector2f tilePosition = { position.x - (x * Global::TileSize), position.y - (y * Global::TileSize) }; //position inside tile

    /*if (!m_server)
    {
        auto p1 = tx.getPosition();
        auto p2 = tx.getWorldPosition() + (tx.getOrigin() * tx.getScale());
        xy::App::printStat("Local", std::to_string(p1.x) + ", " + std::to_string(p1.y));
        xy::App::printStat("World", std::to_string(p2.x) + ", " + std::to_string(p2.y));
    }*/

    //used to calc how far away from land we are
    std::int32_t waterCount = 0;

    //test surrounding tiles to see if they are solid or water
    for (auto j = y - 1; j < y + 2; ++j)
    {
        for (auto i = x - 1; i < x + 2; ++i)
        {
            if ((i == x && j == y)) //and skip current tile
            {
                continue;
            }

            auto testTile = tileAt(i, j);

            //create manifold from any solid tiles
            if (testTile == TileType::Solid)                
            {
                if (collision.manifoldCount < CollisionComponent::MaxManifolds)
                {
                    sf::FloatRect tileBounds(i * Global::TileSize, j * Global::TileSize, Global::TileSize, Global::TileSize);
                    sf::FloatRect collisionBounds = collision.bounds;
                    collisionBounds.left += position.x;
                    collisionBounds.top += position.y;
                    sf::FloatRect intersection;
                    if (tileBounds.intersects(collisionBounds, intersection))
                    {
                        auto normal = (sf::Vector2f(tileBounds.left, tileBounds.top) + CollisionOffset) - position;
                        collision.manifolds[collision.manifoldCount] = calcManifold(normal, intersection);
                        collision.manifolds[collision.manifoldCount++].ID = ManifoldID::Terrain;
                    }
                }
            }

            //log any water tiles for water calc below
            else if (testTile == TileType::Water)
            {              
                waterCount++;
            }
        }
    }


    //calculate water percentage
    auto currentTile = tileAt(x, y);
    collision.water = 0.f;
    if (IslandRect.contains(position))
    {
        if (currentTile == TileType::Water)
        {
            //we're actually in water
            collision.water = static_cast<float>(waterCount) / 24.f;

            /*std::array<float, 4> landTiles = {};
            std::size_t landTileCount = 0;

            if (tileAt(x - 1, y) != TileType::Water)
            {
                landTiles[landTileCount++] = xy::Util::Math::clamp(tilePosition.x / (Global::TileSize / 2.f), 0.f, 1.f);
            }*/          
        }
    }
    else
    {
        //out of the map
        collision.water = 1.f;
    }
}

void CollisionSystem::actorCollision(xy::Entity entity)
{
    if (entity.getComponent<xy::BroadphaseComponent>().getFilterFlags() == 0) return; //collision is disabled

    const auto& tx = entity.getComponent<xy::Transform>();
    auto position = tx.getWorldPosition();
    auto origin = tx.getOrigin();
    auto bounds = entity.getComponent<CollisionComponent>().bounds;
    auto queryBounds = bounds * 2.f;

    bounds.left += position.x;// +origin.x;
    bounds.top += position.y;// +origin.y;
        
    queryBounds.left += position.x;
    queryBounds.top += position.y;

    //query quad tree and do inter-sprite collision
    std::vector<xy::Entity> ents = getScene()->getSystem<xy::DynamicTreeSystem>().query(queryBounds, QuadTreeFilter::Collidable);

    for (auto otherEnt : ents)
    {
        if (otherEnt != entity)
        {
            auto otherPosition = otherEnt.getComponent<xy::Transform>().getWorldPosition();
            auto otherBounds = otherEnt.getComponent<CollisionComponent>().bounds;
            otherBounds.left += otherPosition.x;
            otherBounds.top += otherPosition.y;

            if (otherBounds.intersects(bounds))
            {
                m_collisions.insert(std::minmax(entity, otherEnt));
            }
        }
    }
}

void CollisionSystem::boatCollision(xy::Entity entity)
{
    auto position = entity.getComponent<xy::Transform>().getWorldPosition();
    auto bounds = BoatSearchArea;
    bounds.left += position.x;
    bounds.top += position.y;

    auto boats = getScene()->getSystem<xy::DynamicTreeSystem>().query(bounds, QuadTreeFilter::SpawnArea);

    for (auto boat : boats)
    {
        if (boat == entity) continue;

        auto boatPos = boat.getComponent<xy::Transform>().getPosition();
        auto direction = boatPos - position;
        auto dist = xy::Util::Vector::lengthSquared(direction);
        if (dist < BoatRadiusSqr)
        {
            float len = std::sqrt(dist);
            Manifold manifold;
            manifold.ID = ManifoldID::SpawnArea;
            manifold.normal = -(direction / len);
            manifold.penetration = Global::BoatRadius - len;
            manifold.otherEntity = boat;

            auto& collision = entity.getComponent<CollisionComponent>();
            XY_ASSERT(collision.manifoldCount < CollisionComponent::MaxManifolds, "");
            collision.manifolds[collision.manifoldCount++] = manifold;
        }
    }
}

void CollisionSystem::narrowPhase()
{
    for (auto c : m_collisions)
    {
        const auto& txA = c.first.getComponent<xy::Transform>();
        const auto& txB = c.second.getComponent<xy::Transform>();

        auto& collisionA = c.first.getComponent<CollisionComponent>();
        auto& collisionB = c.second.getComponent<CollisionComponent>();

        //we don't want to include scale in the collision bounds, just world pos
        auto posA = txA.getWorldPosition();
        auto posB = txB.getWorldPosition();

        //auto originA = txA.getOrigin() * txA.getScale();
        //auto originB = txB.getOrigin() * txB.getScale();

        auto boundsA = collisionA.bounds;
        boundsA.left += posA.x;// +originA.x;
        boundsA.top += posA.y;// +originA.y;
        auto boundsB = collisionB.bounds;
        boundsB.left += posB.x;// +originB.x;
        boundsB.top += posB.y;// +originB.y;

        auto normal = posB - posA;
        sf::FloatRect overlap;
        boundsA.intersects(boundsB, overlap);

        Manifold manifold = calcManifold(normal, overlap);
        manifold.ID = collisionB.ID;
        manifold.otherEntity = c.second;

        if (collisionA.manifoldCount < CollisionComponent::MaxManifolds)
        {
            collisionA.manifolds[collisionA.manifoldCount++] = manifold;
        }

        manifold.normal = -manifold.normal;
        manifold.ID = collisionA.ID;
        manifold.otherEntity = c.first;

        if (collisionB.manifoldCount < CollisionComponent::MaxManifolds)
        {
            collisionB.manifolds[collisionB.manifoldCount++] = manifold;
        }
    }
}

void CollisionSystem::narrowPhaseQuery(xy::Entity entity)
{
    for (auto& c : m_collisions)
    {
        xy::Entity first;
        xy::Entity second;
        if (entity == c.first)
        {
            first = c.first;
            second = c.second;
        }
        else
        {
            first = c.second;
            second = c.first;
        }

        const auto& txA = first.getComponent<xy::Transform>();
        const auto& txB = second.getComponent<xy::Transform>();

        auto& collisionA = first.getComponent<CollisionComponent>();
        const auto& collisionB = second.getComponent<CollisionComponent>();

        auto posA = txA.getWorldPosition();
        auto posB = txB.getWorldPosition();

        auto boundsA = collisionA.bounds;
        boundsA.left += posA.x;
        boundsA.top += posA.y;
        auto boundsB = collisionB.bounds;
        boundsB.left += posB.x;
        boundsB.top += posB.y;

        auto normal = posB - posA;
        sf::FloatRect overlap;
        boundsA.intersects(boundsB, overlap);

        Manifold manifold = calcManifold(normal, overlap);
        manifold.ID = collisionB.ID;
        manifold.otherEntity = second;

        if (collisionA.manifoldCount < CollisionComponent::MaxManifolds)
        {
            collisionA.manifolds[collisionA.manifoldCount++] = manifold;
        }
    }
}

std::uint32_t CollisionSystem::tileAt(std::size_t x, std::size_t y)
{
    if (x >= Global::TileCountX || y >= Global::TileCountY) return TileType::Water;
    return m_tileArray[y * Global::TileCountX + x] / 2;
}

Manifold CollisionSystem::calcManifold(sf::Vector2f normal, sf::FloatRect overlap)
{
    Manifold manifold;
    if (overlap.width < overlap.height)
    {
        manifold.normal.x = (normal.x < 0) ? 1.f : -1.f;
        manifold.penetration = overlap.width;
    }
    else
    {
        manifold.normal.y = (normal.y < 0) ? 1.f : -1.f;
        manifold.penetration = overlap.height;
    }
    return manifold;
}

void CollisionSystem::resolveAll()
{
    //resolves all manifold collisions
    //but leaves manifold data in tact in case other systems want to read it
    for (auto c : m_collisions)
    {
        auto& txA = c.first.getComponent<xy::Transform>();
        auto& txB = c.second.getComponent<xy::Transform>();

        const auto& collisionA = c.first.getComponent<CollisionComponent>();
        const auto& collisionB = c.second.getComponent<CollisionComponent>();

        for (auto i = 0u; i < collisionA.manifoldCount; ++i)
        {
            const auto& manifold = collisionA.manifolds[i];
            txA.move(manifold.normal * manifold.penetration);
        }

        for (auto i = 0u; i < collisionB.manifoldCount; ++i)
        {
            const auto& manifold = collisionB.manifolds[i];
            txB.move(manifold.normal * manifold.penetration);
        }

        //TODO static objects shouldn't move! Also if two dynamic objects they should
        //move only by penetration / 2.f
    }
}