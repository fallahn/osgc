///source for track distance meter shown on HUD///
#include <Game/Hud.h>

using namespace Game;

//ctor
HudManager::HudItemLapMeter::HudItemLapMeter(const std::vector< std::shared_ptr<PlayerData>>& playerData)
	: m_playerData		(playerData),
	m_scale				(0.f),
	m_lapCount			(0u)
{


}

//public
void HudManager::HudItemLapMeter::Create(float screenWidth, float trackLength, sf::Uint8 totalLaps)
{
	//reset lap count
	m_lapCount = 0;
	
	//create shapes
	const float barHeight = 10.f;
	const float borderThickness = 2.f;
	const float shapeRadius = 10.f;

	m_barWidth = screenWidth;
	m_scale = screenWidth / trackLength;

	sf::Vector2f barSize(m_barWidth, barHeight);
	m_trackbar.setSize(barSize);
	m_trackbar.setFillColor(sf::Color::Black);
	m_trackbar.setOutlineThickness(borderThickness);
	m_trackbar.setPosition(0.f, (shapeRadius + 3.f) - 5.f); //TODO kill magic number

	m_lapBar.setFillColor(sf::Color(120u, 120u, 120u));
	m_lapBar.setSize(sf::Vector2f((m_barWidth / static_cast<float>(totalLaps)) - 1.f, barHeight - 2.f));
	m_lapBar.setPosition(m_trackbar.getPosition());
	m_lapBar.move(0.f, 1.f);

	for(auto i = 0u; i < m_playerData.size(); ++i)
	{
		sf::CircleShape c(shapeRadius);
		c.setOutlineThickness(-2.f);
		c.setOrigin(shapeRadius, shapeRadius);
		c.setFillColor(sf::Color::Black);
		c.setOutlineColor(PlayerColours::Light(i));
		m_playerShapes.push_back(c);
	}

	m_highlight.setRadius(shapeRadius * 0.5f);
	m_highlight.setOrigin(shapeRadius / 2.f, shapeRadius * 2.f);
	m_highlight.setScale(1.f, 0.4f);
	m_highlight.setFillColor(sf::Color(255u, 255u, 255u, 120u));

	//create render texture
	m_renderTexture.create(static_cast<unsigned>(m_barWidth), static_cast<unsigned>(shapeRadius + 3.f) * 2u);

	//create sprite
	m_sprite.setTexture(m_renderTexture.getTexture(), true);
	m_sprite.setOrigin(static_cast<float>(m_renderTexture.getSize().x / 2u), static_cast<float>(m_renderTexture.getSize().y / 2u));
	//must update texture rect if resizing render texture
	//m_sprite.setTextureRect(sf::IntRect(0, 0, m_renderTexture.getSize().x, m_renderTexture.getSize().y));
}

void HudManager::HudItemLapMeter::Update(float dt)
{
	float renderHeight = static_cast<float>(m_renderTexture.getSize().y) / 2.f;

	//draw out track bar
	m_renderTexture.clear(sf::Color::Transparent);
	m_renderTexture.draw(m_trackbar);

	//draw lap bars
	const float startPoint = m_lapBar.getPosition().x;
	for(auto i = 0u; i < m_lapCount; ++i)
	{
		m_renderTexture.draw(m_lapBar);
		m_lapBar.move(m_lapBar.getSize().x + 1.f, 0.f);
	}
	m_lapBar.move(startPoint - m_lapBar.getPosition().x, 0.f);

	//update and draw player positions
	for(auto i = 0u; i < m_playerData.size(); ++i)
	{
		m_playerShapes[i].setPosition(m_playerData[i]->CurrentDistance * m_scale, renderHeight);
		(m_playerData[i]->Active) ? m_playerShapes[i].setOutlineColor(PlayerColours::Light(i)) : m_playerShapes[i].setOutlineColor(PlayerColours::Dark(i));
		m_renderTexture.draw(m_playerShapes[i]);
		m_highlight.setPosition(m_playerShapes[i].getPosition());
		m_renderTexture.draw(m_highlight);
	}

	m_renderTexture.display();
}

//private