///source for trail particle class///
#include <Game/TrailParticle.h>

using namespace Game;

namespace
{
	const sf::Vector2f size(48.f, 12.f);
	const float reductionAmount = 700.f;
	const float transparency = 190.f;
}

TrailParticle::TrailParticle(QuadVerts& verts)
	: BaseParticle	(verts),
	m_transparency	(transparency)
{	
	m_SetScale(1.f, 0.5f);
}

//public
void TrailParticle::Update(float dt)
{
	if(m_state == Alive)
	{
		m_transparency -= reductionAmount * dt;
		if(m_transparency < 0.f)
		{
			m_state = Dead;
			return;
		}

		sf::Uint8 t = static_cast<sf::Uint8>(m_transparency);
		sf::Uint8 c = 255u;
		m_SetColour(sf::Color(c, c, c, t));
	}
	else
	{
		m_transparency = transparency;
	}
}

//private