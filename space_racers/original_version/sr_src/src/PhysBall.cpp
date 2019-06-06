//source code for physball entity
#include <Game/PhysBall.h>

using namespace Game;

//ctor
PhysBall::PhysBall(float radius, sf::Color colour)
	: m_colour(colour)
{
	if(radius <= 2.f) radius = 2.f;
	m_radius = radius;
	
	//create shape components
	m_highlight.setRadius(radius * 0.05f);//TODO see what works best here
	m_highlight.setOrigin(m_highlight.getRadius(), m_highlight.getRadius());
	//m_highlight.setFillColor(sf::Color(255u, 255u, 255u, 190u));

	m_diffuse.setRadius(radius * 0.8f);
	m_diffuse.setOrigin(m_diffuse.getRadius(), m_diffuse.getRadius());
	m_diffuse.setFillColor(colour);

	sf::Uint8 r, g, b;
	const float reduction = 0.3f;
	r = colour.r - static_cast<sf::Uint8>(colour.r * reduction);
	g = colour.g - static_cast<sf::Uint8>(colour.g * reduction);
	b = colour.b - static_cast<sf::Uint8>(colour.b * reduction);

	m_shade.setRadius(radius);
	m_shade.setOrigin(radius, radius);
	m_shade.setFillColor(sf::Color(r, g, b));

	m_shadow.setRadius(radius);
	m_shadow.setOrigin(radius, radius);
	m_shadow.setFillColor(sf::Color(0u, 0u, 0u, 80u));

	m_shadowOffset = (radius * 1.5f) - radius;

	//create render texture (large enough for shadow offset)
	sf::Uint16 textureSize = static_cast<sf::Uint16>(radius + m_shadowOffset);
	textureSize *= 2u;
	m_renderTexture.create(textureSize, textureSize);
	m_renderTexture.setSmooth(true);

	//create sprite
	m_sprite.setTexture(m_renderTexture.getTexture());
	m_sprite.setFrameSize(textureSize, textureSize);
	m_sprite.setOrigin(static_cast<float>(textureSize / 2u), static_cast<float>(textureSize / 2u));

	m_highlight.setPosition(m_sprite.getOrigin());
	m_diffuse.setPosition(m_sprite.getOrigin());
	m_shade.setPosition(m_sprite.getOrigin());
	m_shadow.setPosition(m_sprite.getOrigin());

	//create shader
	m_shader.loadFromMemory(falloff, sf::Shader::Fragment);
	m_shader.setParameter("verticalResolution", static_cast<float>(textureSize));
}

//dtor

//public
void PhysBall::Update(float dt, const sf::Vector2f& lightPos)
{
	const float maxLength = 10.f;
	const float maxDistance = m_radius * maxLength;
	
	//update position of component shapes
	sf::Vector2f lightVector = lightPos - GetCentre();
	float distance = Helpers::Vectors::GetLength(lightVector);
	distance = std::min(distance, maxDistance); //clamp distance of light source
	const float ratio = distance / maxDistance;

	lightVector = Helpers::Vectors::Normalize(lightVector);

	const float multiplier = distance / maxLength;
	m_shadow.setPosition(m_sprite.getOrigin() - (lightVector * m_shadowOffset * ratio));
	m_diffuse.setPosition(m_sprite.getOrigin() + (lightVector * (multiplier - (m_diffuse.getRadius() * ratio))));
	m_highlight.setPosition(m_sprite.getOrigin() + (lightVector * (multiplier - ((m_diffuse.getRadius() / 2.f) * ratio))));
	//m_highlight.setRadius(m_radius * 0.1f * (1.f - ratio)); // make highligh bigger when nearer centre


	//TODO physics updates


	//update render texture (bottom up)
	m_renderTexture.clear(sf::Color::Transparent);
	m_shader.setParameter("colour", sf::Color::Black);
	m_shader.setParameter("radius", m_shadow.getRadius() * 1.05f);
	m_shader.setParameter("position", m_shadow.getPosition());
	m_renderTexture.draw(m_shadow, &m_shader);
	m_renderTexture.draw(m_shade);
	m_shader.setParameter("colour", m_colour);
	m_shader.setParameter("radius", m_diffuse.getRadius() * 1.05f);
	m_shader.setParameter("position", m_diffuse.getPosition());
	m_renderTexture.draw(m_diffuse, &m_shader);
	m_renderTexture.draw(m_highlight);
	m_renderTexture.display();
}


///private