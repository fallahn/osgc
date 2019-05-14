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

#include <xyginext/ecs/System.hpp>
#include <xyginext/util/Const.hpp>

//roid component. Looks a bit complicated but only
//because we want to make sure the mass is accurate
//when the radius is set.
class Asteroid final
{
public:
    Asteroid() { updateMass(); }

    void setVelocity(sf::Vector2f v) { m_velocity = v; }
    void setSpeed(float s) { m_speed = s; }

    void setRadius(float radius)
    {
        m_radius = radius;
        m_radiusSqr = radius * radius;
        updateMass();
    }

    sf::Vector2f getVelocity() const { return m_velocity; }
    float getSpeed() const { return m_speed; }
    float getMass() const { return m_mass; }
    float getRadius() const { return m_radius; }
    float getRadiusSqr() const { return m_radiusSqr; }

private:
    sf::Vector2f m_velocity;
    float m_speed = 0.f;
    float m_mass = 1.f;
    float m_radius = 100.f;
    float m_radiusSqr = m_radius * m_radius;

    static constexpr float density = 0.001f; //kg/cm3
    void updateMass()
    {
        auto volume = (4.f / 3.f) * (xy::Util::Const::PI * std::pow(m_radius, 3.f));
        m_mass = volume * density;
    }
};

class AsteroidSystem final : public xy::System
{
public:
    explicit AsteroidSystem(xy::MessageBus& mb);

    void process(float) override;
    void setMapSize(sf::FloatRect ms) { m_mapSize = ms; }

    //TODO could possibly client-side predict these too

private:
    sf::FloatRect m_mapSize;
};