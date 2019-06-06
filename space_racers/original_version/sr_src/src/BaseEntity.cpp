///source for base object class///

#include <Game/BaseEntity.h>

using namespace Game;

BaseEntity::BaseEntity()
	: m_state	(Dead),
	Ident		(0u)
{
	m_sprite.setTexture(m_texture);
}

AniSprite& BaseEntity::GetSprite()
{
	return m_sprite;
}

const State BaseEntity::GetState() const
{
	return m_state;
}

void BaseEntity::SetState(State state)
{
	m_state = state;
}

const sf::Vector2f BaseEntity::GetCentre() const
{
	return m_sprite.getPosition() - m_sprite.getOrigin()
		+ sf::Vector2f(static_cast<float>(m_sprite.getTextureRect().width) / 2.f, static_cast<float>(m_sprite.getTextureRect().height) / 2.f);
}