///source for Hud item for drawing player scores///
#include <Game/Hud.h>
#include <sstream>

using namespace Game;

namespace
{
	const float shapeRadius = 20.f;
}

//ctor
HudManager::HudItemScore::HudItemScore(sf::Uint8 playerId, const std::shared_ptr<PlayerData>& playerData, sf::Uint8 size)
	: m_size		(size),
	m_data			(playerData),
	m_lightShape	(PlayerColours::Light(playerId),shapeRadius),
	m_darkShape		(PlayerColours::Dark(playerId), shapeRadius),
	m_flashIcon		(false),
	m_flashOn		(false),
	m_flashTime		(0.25f)
{

	//create render texture and set up sprite
	const sf::Uint16 segWidth = static_cast<sf::Uint16>(shapeRadius + 2.f) * 2u;
	m_renderTexture.create(segWidth, segWidth * m_size);
	//m_renderTexture.setSmooth(true);
	m_sprite.setTexture(m_renderTexture.getTexture());
	//m_sprite.setOrigin(static_cast<float>(m_renderTexture.getSize().x / 2u), static_cast<float>(m_renderTexture.getSize().y / 2u));

	//save spacing for when we need to draw
	m_iconSpacing = static_cast<float>(segWidth);

}

//dtor


//public
void HudManager::HudItemScore::Update(float dt)
{
	//update flash state
	if(m_flashClock.getElapsedTime().asSeconds() > m_flashTime)
	{
		m_flashClock.restart();
		m_flashOn = !m_flashOn;
	}
		
	//redraw render texture from player data
	sf::Uint8 lightCount = m_data->RoundWins;
	if(m_flashIcon && m_flashOn)
		lightCount++;

	const sf::Uint8 greyCount = m_size - lightCount;
	m_lightShape.setPosition(m_iconSpacing / 2.f, m_iconSpacing / 2.f);

	m_renderTexture.clear(sf::Color::Transparent);

	//draw light
	sf::Uint8 i;
	for(i = 0; i < lightCount; i++)
	{
		m_renderTexture.draw(m_lightShape);
		m_lightShape.move(0.f, m_iconSpacing);
	}

	//draw dark
	m_darkShape.setPosition(m_lightShape.getPosition());

	for(i = 0; i < greyCount; i++)
	{
		m_renderTexture.draw(m_darkShape);		
		m_darkShape.move(0.f, m_iconSpacing);
	}
	m_renderTexture.display();
}

//private