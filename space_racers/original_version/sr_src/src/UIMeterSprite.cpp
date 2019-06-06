#include <Game/UIMeterSprite.h>

#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

using namespace Game;

//ctor
UIMeterSprite::UIMeterSprite(const sf::Texture& front, const sf::Texture& back, const sf::Texture& needle, sf::Uint8 itemCount)
	: m_frontTexture	(front),
	m_backTexture		(back),
	m_selectedIndex		(0u),
	m_destinationIndex	(0u),
	m_itemCount			(itemCount),
	m_enabledItems		(itemCount),
	m_needle			(sf::Vector2f(), sf::Vector2f(-static_cast<float>(needle.getSize().x), 0.f), static_cast<float>(needle.getSize().y)),
	m_destAngle			(0.f)
{
	m_backSprite.setTexture(back, true);
	m_backSprite.setOrigin(static_cast<sf::Vector2f>(back.getSize() / 2u));

	m_frontSprite.setTexture(front);
	sf::IntRect subRect(0, 0, front.getSize().x, front.getSize().y / 2u);
	m_subRects[State::MeterNormal] = subRect;
	subRect.top += subRect.height;
	m_subRects[State::MeterSelected] = subRect;
	m_frontSprite.setTextureRect(m_subRects[State::MeterNormal]);
	m_frontSprite.setOrigin(sf::Vector2f(static_cast<float>(subRect.width / 2), static_cast<float>(subRect.height / 2)));

	const float angleSize = 180.f / static_cast<float>(m_itemCount);
	m_needle.setRotation((angleSize * static_cast<float>(m_selectedIndex + 1)) - (angleSize / 2.f));
	m_needle.setTexture(needle);

	for(auto& b : m_enabledItems)
		b = true;
}

//public
void UIMeterSprite::Update(float dt)
{
	if(m_selectedIndex != m_destinationIndex)
	{
		//update animation
		const float currentRotation = m_needle.getRotation();
		const float remaining = std::abs(m_destAngle - currentRotation);
		const float speed = 10.f + (remaining * 4.f);
		float rotation = speed * dt;
		//set direction
		if(currentRotation > m_destAngle)
			rotation =- rotation;

		if(remaining > rotation)
			m_needle.rotate(rotation);
		else
			m_selectedIndex = m_destinationIndex;
	}
}

sf::Uint8 UIMeterSprite::SelectedIndex() const
{
	return m_selectedIndex;
}

bool UIMeterSprite::Selectable() const
{
	return true;
}

void UIMeterSprite::Select()
{
	UIComponent::Select();
	m_frontSprite.setTextureRect(m_subRects[State::MeterSelected]);
}

void UIMeterSprite::Deselect()
{
	UIComponent::Deselect();
	m_frontSprite.setTextureRect(m_subRects[State::MeterNormal]);
}

void UIMeterSprite::Activate()
{
	if(!Enabled()) return;
	if(m_destinationIndex == m_selectedIndex)
	{
		do
			m_destinationIndex = (m_destinationIndex + 1u) % m_itemCount;
		while(!m_enabledItems[m_destinationIndex]);

		m_SetDestinationAngle();
	}
}

void UIMeterSprite::HandleEvent(const sf::Event& e)
{

}

bool UIMeterSprite::Contains(const sf::Vector2f& point) const
{
	return getTransform().transformRect(m_frontSprite.getGlobalBounds()).contains(point);
}

sf::Vector2f UIMeterSprite::GetSize() const
{
	return sf::Vector2f(m_frontSprite.getLocalBounds().width, m_frontSprite.getLocalBounds().height);
}

void UIMeterSprite::SetIndexEnabled(sf::Uint8 index, bool enabled)
{
	m_enabledItems[index] = enabled;
}

void UIMeterSprite::SetNeedlePosition(const sf::Vector2f& pos)
{
	m_needle.setPosition(pos);
}

void UIMeterSprite::SelectIndex(sf::Uint8 index)
{
	if(m_enabledItems[index])
	{
		m_destinationIndex = index;
		m_SetDestinationAngle();
	}
}

//private
void UIMeterSprite::draw(sf::RenderTarget& rt, sf::RenderStates states) const
{
	states.transform *= getTransform();
	rt.draw(m_backSprite, states);
	rt.draw(m_needle, states);
	rt.draw(m_frontSprite, states);
}

void UIMeterSprite::m_SetDestinationAngle()
{
	const float angleSize = 180.f / static_cast<float>(m_itemCount);
	m_destAngle = (angleSize * static_cast<float>(m_destinationIndex + 1)) - (angleSize / 2.f);
}