///source for star particle///
#include <Game/StarParticle.h>

using namespace Game;

namespace
{
	const sf::Uint8 transparency = 100u;
}

//ctor
StarParticle::StarParticle(QuadVerts& verts)
	: BaseParticle	(verts),
	m_colour		(Helpers::Random::Float(0.f, 255.f)),
	m_scale			(1.f),
	m_transparency	(transparency)
{
	m_SetSize(sf::Vector2f(6.f, 6.f));
}

//public functions
void StarParticle::Update(float dt)
{
	if(m_state == Alive)
	{
		m_scale += 2.f * dt;
		if(m_scale > 3.f)
		{
			m_state = Dead;
			m_transparency = 0u;
		}
		m_SetScale(m_scale, m_scale);
		
		m_colour -= 180.f * dt;
		if(m_colour < 0.f) m_colour = 255.f - m_colour;

		sf::Uint8 r = static_cast<sf::Uint8>(m_colour);
		sf::Uint8 g = r - 85u;
		sf::Uint8 b = g - 85u;

		m_SetColour(sf::Color(r, g, b, m_transparency));
		m_Rotate(120.f * dt);
		m_Move(m_velocity * m_force * dt);
	}
	else
	{
		m_scale = 1.f;
		m_transparency = transparency;
	}
}