///source for class which creates a drawable for marking out split screen///
#include <Game/Hud.h>
#include <cassert>

using namespace Game;

namespace
{
	const float barThickness = 6.f;
	const sf::Color barColour = sf::Color(120u, 120u, 120u, 100u);
}

//public
HudManager::HudItemScreenBars::HudItemScreenBars()
	: m_vertices	(sf::Quads)
{

}
void HudManager::HudItemScreenBars::Create(sf::Uint8 screenCount, const sf::Vector2f& size)
{
	m_vertices.clear();

	if(screenCount > 1u)
	{
		//create vertical bar
		sf::Vector2f position((size.x - barThickness) / 2.f, 0.f);
		m_vertices.append(sf::Vertex(position, barColour));
		position.x += barThickness;
		m_vertices.append(sf::Vertex(position, barColour));
		position.y += size.y;
		m_vertices.append(sf::Vertex(position, barColour));
		position.x -= barThickness;
		m_vertices.append(sf::Vertex(position, barColour));

		//stop here if only 2 screens
		if(screenCount == 2) return;

		//else create horizontal bar
		position.x = 0.f;
		position.y = (size.y - barThickness) / 2.f;
		m_vertices.append(sf::Vertex(position, barColour));
		position.x = size.x;
		m_vertices.append(sf::Vertex(position, barColour));
		position.y += barThickness;
		m_vertices.append(sf::Vertex(position, barColour));
		position.x = 0.f;
		m_vertices.append(sf::Vertex(position, barColour));
	}
}

void HudManager::HudItemScreenBars::Move(const sf::Vector2f& distance)
{
	for(auto i = 0u; i < m_vertices.getVertexCount(); ++i)
	{
		m_vertices[i].position += distance;
	}
}

//private
void HudManager::HudItemScreenBars::draw(sf::RenderTarget& rt, sf::RenderStates states) const
{
	assert(m_vertices.getVertexCount());
	rt.draw(m_vertices, states);
}