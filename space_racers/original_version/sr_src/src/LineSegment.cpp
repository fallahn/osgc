///source for line segment class///
#include <Game/LineSegment.h>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Texture.hpp>

LineSegment::LineSegment(const sf::Vector2f& start, const sf::Vector2f& end, float thickness, sf::Color colour)
	: m_texture		(nullptr),
	m_vertexArray	(sf::Quads, 4u),
	m_startPoint	(start),
	m_endPoint		(end)
{
	setThickness(thickness);
	setColour(colour);
}


//public
void LineSegment::setTexture(const sf::Texture& texture)
{
	m_texture = &texture;

	sf::Vector2f texSize = static_cast<sf::Vector2f>(texture.getSize());
	m_vertexArray[1].texCoords = sf::Vector2f(texSize.x, 0.f);
	m_vertexArray[2].texCoords = texSize;
	m_vertexArray[3].texCoords = sf::Vector2f(0.f, texSize.y);
}

void LineSegment::setThickness(float thickness)
{
	sf::Vector2f direction = m_endPoint - m_startPoint;
    sf::Vector2f unitDirection = direction / std::sqrt(direction.x * direction.x + direction.y * direction.y);
    sf::Vector2f unitPerpendicular(-unitDirection.y, unitDirection.x);

    float halfThick = thickness / 2.f;

	m_vertexArray[0].position = halfThick * unitPerpendicular;
	m_vertexArray[1].position = direction + halfThick * unitPerpendicular;
	m_vertexArray[2].position = direction - halfThick * unitPerpendicular;
	m_vertexArray[3].position = -halfThick * unitPerpendicular;

	setPosition(m_startPoint);
}

void LineSegment::setColour(sf::Color colour)
{
	for(auto i = 0u; i < m_vertexArray.getVertexCount(); ++i)
	{
		m_vertexArray[i].color = colour;
	}
}

//private
void LineSegment::draw(sf::RenderTarget& rt, sf::RenderStates states) const
{
	states.transform *= getTransform();
	if(m_texture) states.texture = m_texture;
	rt.draw(m_vertexArray, states);
}