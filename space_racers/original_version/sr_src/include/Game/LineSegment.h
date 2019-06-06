///class draws a line segment via a vertex array as a quad///
#ifndef LINE_SEG_H_
#define LINE_SEG_H_

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Transformable.hpp>
#include <SFML/Graphics/VertexArray.hpp>


class LineSegment: public sf::Drawable, public sf::Transformable
{
public:
	LineSegment(const sf::Vector2f& start, const sf::Vector2f& end, float thickness = 2.f, sf::Color colour = sf::Color::White);

	void setTexture(const sf::Texture& texture);
	void setThickness(float thickness);
	void setColour(sf::Color colour);

	//width / height bounding box
	sf::Vector2f getSize() const
	{
		return sf::Vector2f(m_endPoint - m_startPoint);
	}

private:
	const sf::Texture* m_texture;
	sf::VertexArray m_vertexArray;
	sf::Vector2f m_startPoint, m_endPoint;

	void draw(sf::RenderTarget& rt, sf::RenderStates states) const;
};

#endif