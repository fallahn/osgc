///source code for race starting lights///
#include <Game/Hud.h>

using namespace Game;


//ctor
HudManager::HudItemStartLights::HudItemStartLights(const sf::Texture& texture)
	: m_active		(false),
	m_finished		(false),
	m_endY			(0.f),
	m_texture		(texture)
{
	m_sprite = AniSprite(m_texture, m_texture.getSize().x, m_texture.getSize().y / 6u);
	m_sprite.setFrameRate(1.f);

}

//dtor


//public
bool HudManager::HudItemStartLights::Update(float dt, const sf::View& hudView)
{
	sf::Vector2f viewPos = hudView.getCenter();

	if(m_sprite.playing())
	{
		m_sprite.update();
	}
	else if(m_active)
	{
		//scroll off screen if still visible
		if(m_sprite.getPosition().y > m_endY)
		{
			m_sprite.move(sf::Vector2f(0.f, -1.f) * 200.f * dt);
		}
		else
		{
			m_active = false;
			m_finished = true;
		}
	}

	if(m_sprite.getCurrentFrame() > 3)
		return true; //return when we're ready to start
	else
		return false;
}

void HudManager::HudItemStartLights::StartCounting(const sf::View& hudView)
{
	if(m_finished) return;
	m_sprite.setPosition(hudView.getCenter().x -(m_texture.getSize().x / 2.f),
								hudView.getCenter().y - (hudView.getSize().y / 2.f));
	m_endY = m_sprite.getPosition().y - static_cast<float>(m_sprite.getFrameSize().y * 2);
	m_sprite.play(0, m_sprite.getFrameCount() - 1, false);
	m_active = true;
}

//private
