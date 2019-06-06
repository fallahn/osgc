///source for smoke particle class///
#include <Game/SmokeParticle.h>
#include <Helpers.h>

using namespace Game;

namespace
{
	const float colour = 255.f;
	const float radius = 1.f;
	const float transparencyReduction = 200.f;
	const float colourReduction = transparencyReduction * 0.9f;
	const float radiusIncrease = 90.f;
}

//ctor
SmokeParticle::SmokeParticle(QuadVerts& verts)
	: BaseParticle	(verts),
	m_colour		(colour),
	m_radius		(radius),
	m_transparency	(colour),
	m_rotationAmount(Helpers::Random::Float(180.f, 250.f))
{
	m_SetSize(sf::Vector2f(m_radius, m_radius));
}


//public
void SmokeParticle::Update(float dt)
{
	if(m_state == Alive)
	{
		m_colour -= colourReduction * dt;
		m_transparency -= transparencyReduction * dt;
		if(m_transparency < 0.f)
		{
			m_state = Dead;
			return;
		}

		m_radius += radiusIncrease * dt;
		m_SetSize(sf::Vector2f(m_radius, m_radius));

		sf::Uint8 c = static_cast<sf::Uint8>(m_colour);
		sf::Uint8 t = static_cast<sf::Uint8>(m_transparency);
		m_SetColour(sf::Color(c, c, c, t));
		m_Move(m_velocity * m_force * dt);
		m_Rotate(m_rotationAmount * dt);
	}
	else
	{
		//reset properties
		m_radius = radius;
		m_colour = m_transparency = colour;
		m_rotationAmount = Helpers::Random::Float(180.f, 250.f);
	}
}
