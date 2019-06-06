///source for starfield used in menu background///
#include <SFML/Window/Keyboard.hpp>

#include <Game/StarField.h>
#include <Game/Shaders/Planet.h>

using namespace Game;

//star ctor
StarField::Star::Star(float minSpeed, float maxSpeed)
	: m_minSpeed	(minSpeed),
	m_maxSpeed		(maxSpeed),
	m_minSize		(m_minSpeed / 100.f),
	m_maxSize		(m_maxSpeed / 100.f),
	m_speed			(0.f),
	m_size			(0.f),
	m_grow			(false)
{
	m_Reset();
}

void StarField::Star::Update(float dt, const sf::Vector2f& velocity)
{
	Shape.move(velocity * m_speed * dt);
	float change = Helpers::Vectors::GetLength(velocity) * dt;

	if(m_grow)
	{
		m_size += change;
		if(m_size > m_maxSize) m_grow = false;
	}
	else
	{
		m_size -= change;
		if(m_size < m_minSize) m_grow = true;
	}

	m_speed = m_size * 100.f;
	Shape.setSize(sf::Vector2f(m_size, m_size));

	float max = 255.f / m_maxSpeed;
	sf::Uint8 c = static_cast<sf::Uint8>(m_speed * max);
	Colour = sf::Color(c, c, c, 255u);
}

void StarField::Star::m_Reset()
{
	m_speed = Helpers::Random::Float(m_minSpeed, m_maxSpeed);
	m_size = m_speed / 100.f;
	Shape.setSize(sf::Vector2f(m_size, m_size));
	Shape.setPosition(Helpers::Random::Float(0.f, 1920.f), Helpers::Random::Float(0.f, 1080.f));

	m_grow = ((Helpers::Random::Int(2, 3) % 2) == 1) ? true : false;
}

//----starfield-----//

//ctor
StarField::StarField(TextureResource& textures)
	: m_acceleration(0.4f),
	m_randTime(Helpers::Random::Float(6.f, 15.f))
{
	for(auto i = 0u; i < 6u; i++)
		m_nearStars.push_back(std::shared_ptr<Star>(new Star(900.f, 1800.f)));

	for(auto i = 0u; i < 50u; ++i)
		m_farStars.push_back(std::shared_ptr<Star>(new Star(200.f, 750.f)));

	m_velocity.x = Helpers::Random::Float(-1.f, 1.f);
	m_velocity.y = Helpers::Random::Float(-1.f, 1.f);

	//background nebula
	m_nebulaTexture = textures.Get("assets/textures/environment/nebula.jpg");
	m_nebulaTexture.setSmooth(true);
	m_nebulaTexture.setRepeated(true);
	m_nebulaSprite.setTexture(m_nebulaTexture);
	m_nebulaSprite.setScale(2.f, 2.f);

	//planet textures
	m_planetDiffuse = textures.Get("assets/textures/environment/globe_diffuse.png");
	m_planetDiffuse.setRepeated(true);
	//m_planetDiffuse.setSmooth(true);
	m_planetCraterNormal = textures.Get("assets/textures/environment/crater_normal.jpg");
	m_planetCraterNormal.setRepeated(true);
	m_planetCraterNormal.setSmooth(true);
	m_planetGlobeNormal = textures.Get("assets/textures/environment/globe_normal.png");
	m_planetResolution.x = static_cast<float>(m_planetDiffuse.getSize().x);
	m_planetResolution.y = static_cast<float>(m_planetDiffuse.getSize().y);

	m_planetSprite.setTexture(m_planetDiffuse);
	m_planetSprite.setPosition(440.f, -25.f);
	m_planetSprite.setScale(2.f, 2.f);
	//m_planetSprite.rotate(47.f);

	m_planetShader.loadFromMemory(planetMap, sf::Shader::Fragment);
	m_planetShader.setParameter("normalTexture", m_planetCraterNormal);
	m_planetShader.setParameter("globeNormalTexture", m_planetGlobeNormal);

	//star set up
	m_starTexture = textures.Get("assets/textures/environment/star.png");
	float texWidth = static_cast<float>(m_starTexture.getSize().x);
	m_texCoords[1] = sf::Vector2f(texWidth, 0.f);
	m_texCoords[2] = sf::Vector2f(texWidth, texWidth);
	m_texCoords[3] = sf::Vector2f(0.f, texWidth);
}

void StarField::Update(float dt, const sf::FloatRect& viewArea)
{
	//parse keyboard and set velocity
	if(sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
	{
		m_velocity.x = -1.f;
		m_acceleration = 1.f;
	}
	else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
	{
		m_velocity.x = 1.f;
		m_acceleration = 1.f;
	}

	if(sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
	{
		m_velocity.y = 1.f;
		m_acceleration = 1.f;
	}
	else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
	{
		m_velocity.y = -1.f;
		m_acceleration = 1.f;
	}
	
	m_velocity = Helpers::Vectors::Normalize(m_velocity) * m_acceleration;
	if(m_acceleration > 0.4f) m_acceleration -= (0.3f * dt);

	//randomise direction occasionally
	if(m_randClock.getElapsedTime().asSeconds() > m_randTime)
	{
		m_randClock.restart();
		m_randTime = Helpers::Random::Float(6.f, 15.f);

		float newX, newY;
		newX = Helpers::Random::Float(-0.5f, 0.6f);
		newY = Helpers::Random::Float(-0.6f, 0.7f);
		m_acceleration += std::abs((m_velocity.x - newX) + (m_velocity.y - newY)) / 2.f;

		m_velocity.x += newX;
		m_velocity.y += newY;
	}

	Update(dt, viewArea, m_velocity);

}

void StarField::Update(float dt, const sf::FloatRect& viewArea, const sf::Vector2f& velocity)
{
	sf::Vector2f currentVel;
	if(Helpers::Vectors::GetLength(velocity) == 0)
	{
		currentVel = m_lastVelocity;
	}
	else
	{
		m_lastVelocity = currentVel = velocity;
	}
	
	//update planet position (updates the shader light property, not position on screen)
	m_planetPosition -= currentVel * 200.f * dt;
	m_planetShader.setParameter("position", sf::Vector2f((m_planetPosition.x + m_planetResolution.y) / m_planetResolution.x, 1.f - ( m_planetPosition.y / m_planetResolution.y)));

	//make planet float about
	float width = static_cast<float>(m_planetDiffuse.getSize().x);
	float height = static_cast<float>(m_planetDiffuse.getSize().y);
	m_planetSprite.move(currentVel * 30.f * dt);
	if(m_planetSprite.getPosition().x < -width)
		m_planetSprite.move(viewArea.width + width, 0.f);
	else if(m_planetSprite.getPosition().x > viewArea.width)
		m_planetSprite.move(-(width + viewArea.width), 0.f);

	if(m_planetSprite.getPosition().y < -height)
		m_planetSprite.move(0.f, viewArea.height + height);
	else if(m_planetSprite.getPosition().y > viewArea.height)
		m_planetSprite.move(0.f, -(height + viewArea.height));

	//update near stars
	for(auto& star : m_nearStars)
	{
		star->Update(dt, currentVel);
		m_boundStar(star, viewArea);
	}

	//update far stars
	for(auto& star : m_farStars)
	{
		star->Update(dt, -currentVel);
		m_boundStar(star, viewArea);
	}
}

//private
void StarField::draw(sf::RenderTarget& rt, sf::RenderStates states) const
{
	//background
	rt.draw(m_nebulaSprite);
	
	//far stars
	sf::VertexArray farStars(sf::PrimitiveType::Quads);
	for(const auto& star : m_farStars)
	{
		for(auto i = 0u; i < star->Shape.getPointCount(); i++)
			farStars.append(sf::Vertex(star->Shape.getPoint(i) + star->Shape.getPosition(), star->Colour, m_texCoords[i]));
	}
	rt.draw(farStars, &m_starTexture);

	//planet / moon thing
	rt.draw(m_planetSprite, &m_planetShader);
	
	//near stars
	sf::VertexArray nearStars(sf::PrimitiveType::Quads);
	for(const auto& star : m_nearStars)
	{
		for(auto i = 0u; i < star->Shape.getPointCount(); i++)
			nearStars.append(sf::Vertex(star->Shape.getPoint(i) + star->Shape.getPosition(), star->Colour, m_texCoords[i]));
	}
	rt.draw(nearStars, &m_starTexture);
}

void StarField::m_boundStar(std::shared_ptr<Star>& star, const sf::FloatRect& viewArea)
{
		sf::Vector2f position = star->Shape.getPosition();
		if(position.x < 0) position.x += viewArea.width;
		else if(position.x > viewArea.width) position.x -= viewArea.width;

		if(position.y < 0) position.y += viewArea.height;
		else if(position.y > viewArea.height) position.y -= viewArea.height;

		star->Shape.setPosition(position);
}