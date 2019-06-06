///source for skid mark particle class///
#include <Game/SkidParticle.h>

using namespace Game;

namespace
{
	const float transparency = 100.f;
}

//ctor
SkidParticle::SkidParticle(QuadVerts& verts)
	: BaseParticle	(verts),
	m_transparency	(transparency)
{
	m_SetSize(sf::Vector2f(26.f, 10.f));
}

//public
void SkidParticle::Update(float dt)
{
	if(m_state == Alive)
	{
		m_transparency -= 25 * dt;
		if(m_transparency < 0.f) m_state = Dead;

		sf::Uint8 t = static_cast<sf::Uint8>(m_transparency);
		m_SetColour(sf::Color(0u, 0u, 0u, t));
	}
	else
	{
		m_transparency = transparency;
	}
}

//private