///creates a 'score shape' which can be used in score items///
#include <Game/Hud.h>

using namespace Game;

HudManager::ScoreShape::ScoreShape(sf::Color colour, float radius)
{
	//TODO fix this being rotated in ScoreItem
	
	sf::Uint8 highestVal = 0u;
	if(colour .r > colour.g && colour.r > colour.b) highestVal = colour.r;
	else if(colour.g > colour.r && colour.g > colour.b) highestVal = colour.g;
	else if(colour.b > colour.r && colour.b > colour.g) highestVal = colour.b;

	sf::Uint8 transparency = (highestVal > 127u) ? 160u : 120u;
	m_drawRing = (highestVal > 127u);

	m_bodyShape.setRadius(radius);
	m_bodyShape.setOutlineThickness(-(radius * 0.15f));
	m_bodyShape.setFillColor(sf::Color::Black);
	m_bodyShape.setOutlineColor(colour);
	m_bodyShape.setOrigin(radius, radius);

	float highlightRadius = radius * 0.6f;
	m_highlightShape.setRadius(highlightRadius);
	m_highlightShape.setFillColor(sf::Color(255u, 255u, 255u, transparency));
	m_highlightShape.setOrigin(highlightRadius, highlightRadius * 2.1f);
	m_highlightShape.setScale(1.f, 0.4f);

	float ringRad = radius * 0.9f;
	m_ringShape.setRadius(ringRad);
	m_ringShape.setFillColor(sf::Color::Transparent);
	m_ringShape.setOutlineThickness(1.f);
	m_ringShape.setOutlineColor(sf::Color(255u, 255u, 255u, 160u));
	m_ringShape.setOrigin(ringRad, ringRad);
}

//public
void HudManager::ScoreShape::setPosition(const sf::Vector2f& position)
{
	setPosition(position.x, position.y);
}

void HudManager::ScoreShape::setPosition(float x, float y)
{
	m_bodyShape.setPosition(x, y);
	m_highlightShape.setPosition(x, y);
	m_ringShape.setPosition(x, y);
}

void HudManager::ScoreShape::move(float x, float y)
{
	m_bodyShape.move(x, y);
	m_highlightShape.move(x, y);
	m_ringShape.move(x, y);
}

//private

void HudManager::ScoreShape::draw(sf::RenderTarget& rt, sf::RenderStates states) const
{
	rt.draw(m_bodyShape);
	rt.draw(m_highlightShape);
	if(m_drawRing) rt.draw(m_ringShape);
}