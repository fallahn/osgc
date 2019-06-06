///source for base animated class///
#include <Game/BaseAnimated.h>

using namespace Game;

BaseAnimated::BaseAnimated(sf::Vector2f position,
	sf::Texture* texture, sf::Uint16 frameCountHor, sf::Uint16 frameCountVert) :
BaseEntity(),
	m_moveSpeed(60.f) //units per second
{
	m_sprite.setPosition(position);
	m_sprite.setFrameRate(16.f);
}

void BaseAnimated::Update(float dt)
{
	m_sprite.update(); //update animation
	m_sprite.move(Helpers::Vectors::Normalize(m_velocity) * (m_moveSpeed * dt)); //update entity position
	
}

const sf::Vector2f& BaseAnimated::GetVelocity(void)
{
	return m_velocity;
}

void BaseAnimated::SetVelocity(sf::Vector2f& velocity)
{
	m_velocity = velocity;
}


//private methods
void BaseAnimated::m_Run(void)
{
	m_sprite.play(RUN);
}

void BaseAnimated::m_Idle(void)
{
	m_sprite.play(IDLE);
}
