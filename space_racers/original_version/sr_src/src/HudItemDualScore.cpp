///hud item for 2 player races///
#include <Game/Hud.h>

using namespace Game;

namespace
{
	const float shapeRad = 20.f;
}

HudManager::HudItemDualScore::HudItemDualScore(const SharedData::GameData& gameData, sf::Uint8 size)
	: m_data		(gameData),
	m_size			(size),
	m_shapeRed		(PlayerColours::Light(0u), shapeRad),
	m_shapeBlue		(PlayerColours::Light(1u), shapeRad),
	m_flashIcon		(false),
	m_flashOn		(false),
	m_flashTime		(0.25f)
{
	//create render texture and set up sprite
	const sf::Uint16 segWidth = static_cast<sf::Uint16>(shapeRad + 2.f) * 2u;
	m_renderTexture.create(segWidth, segWidth * m_size);
	m_sprite.setTexture(m_renderTexture.getTexture());

	//save spacing for when we need to draw
	m_iconSpacing = static_cast<float>(segWidth);
}

void HudManager::HudItemDualScore::Update(float dt)
{
	//update flash signal
	if(m_flashClock.getElapsedTime().asSeconds() > m_flashTime)
	{
		m_flashClock.restart();
		m_flashOn = !m_flashOn;
	}
	//calc number of lights to show
	sf::Uint8 redCount = m_data.Players[0]->RoundWins;
	sf::Uint8 blueCount = m_data.Players[1]->RoundWins;

	if(m_flashIcon)
	{
		if(m_data.WinnerId == 0u)
		{
			if(m_flashOn)
			{
				redCount++;
				blueCount--;
			}
		}
		else
		{
			if(m_flashOn)
			{
				redCount--;
				blueCount++;
			}
		}
	}

	m_shapeRed.setPosition(m_iconSpacing / 2.f, m_iconSpacing / 2.f);
	m_renderTexture.clear(sf::Color::Transparent);
	//draw out icons based on player scores
	//if(!m_data.SuddenDeath)
	//{
		for(auto i = 0u; i < redCount; ++i)
		{
			m_renderTexture.draw(m_shapeRed);
			m_shapeRed.move(0.f, m_iconSpacing);
		}
		m_shapeBlue.setPosition(m_shapeRed.getPosition());
		
		for(auto i = 0u; i < blueCount; ++i)
		{
			m_renderTexture.draw(m_shapeBlue);
			m_shapeBlue.move(0.f, m_iconSpacing);
		}
	//}
	//else //else animate if sudden death
	//{

	//}
	m_renderTexture.display();
}