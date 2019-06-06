#include <Game/UISlider.h>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Window/Event.hpp>

#include <Helpers.h>
#include <cassert>

using namespace Game;

namespace
{
	const sf::Color border(160u, 160u, 160u);
	const float thickness = 4.f;
}

//ctor
UISlider::UISlider(const sf::Font& font, const sf::Texture& t, float length, float maxValue)
	: m_maxValue		(maxValue),
	m_direction			(Direction::Horizontal),
	m_slotShape			(sf::Vector2f(length, thickness)),
	m_handleSprite		(t),
	m_length			(length),
	m_font				(font),
	m_text				("", font, 20u),
	m_localBounds		(0.f, -static_cast<float>(t.getSize().y / 2), length, static_cast<float>(t.getSize().y))
{
	sf::IntRect subrect(0, 0, t.getSize().x, t.getSize().y / 2u);
	m_subrects[State::Normal] = subrect;
	subrect.top += subrect.height;
	m_subrects[State::Selected] = subrect;
	m_handleSprite.setTextureRect(m_subrects[State::Normal]);


	Helpers::Position::CentreOrigin(m_handleSprite);
	m_slotShape.setOrigin(0.f, thickness / 2.f);
	m_slotShape.setFillColor(sf::Color::Black);
	m_slotShape.setOutlineColor(border);
	m_slotShape.setOutlineThickness(3.f);
}

//public
bool UISlider::Selectable() const
{
	return true;
}

void UISlider::Select()
{
	UIComponent::Select();
	m_handleSprite.setTextureRect(m_subrects[State::Selected]);
}

void UISlider::Deselect()
{
	UIComponent::Deselect();
	m_handleSprite.setTextureRect(m_subrects[State::Normal]);
}

void UISlider::Activate()
{
	UIComponent::Activate();
}

void UISlider::Deactivate()
{
	UIComponent::Deactivate();
}

void UISlider::HandleEvent(const sf::Event& e)
{

}

bool UISlider::Contains(const sf::Vector2f& point) const
{
	m_mousePos = point;
	return getTransform().transformRect(m_handleSprite.getGlobalBounds()).contains(point);
}

void UISlider::Update(float dt)
{
	if(sf::Mouse::isButtonPressed(sf::Mouse::Left) &&
		UIComponent::Selected() && 
		getTransform().transformRect(m_localBounds).contains(m_mousePos))
	{
		sf::Vector2f relPos = m_mousePos - getPosition();
		if(m_direction == Direction::Horizontal)
		{
			if(relPos.x < 0.f) relPos.x = 0.f;
			else if(relPos.x > m_length) relPos.x = m_length;

			m_handleSprite.setPosition(relPos.x, m_handleSprite.getPosition().y);
		}
		else
		{
			if(relPos.y < 0.f) relPos.y = 0.f;
			else if(relPos.y > m_length) relPos.y = m_length;

			m_handleSprite.setPosition(m_handleSprite.getPosition().x, relPos.y);
		}

		//SetText(Helpers::String::ToString<float>(GetValue()));
	}
}

void UISlider::SetMaxValue(float value)
{
	m_maxValue = value;
}

void UISlider::SetDirection(Direction direction)
{
	if(m_direction != direction)
	{
		m_direction = direction;
	
		float newX = m_slotShape.getSize().y;
		float newY = m_slotShape.getSize().x;
		m_slotShape.setSize(sf::Vector2f(newX, newY));

		newX = m_handleSprite.getPosition().y;
		newY = m_handleSprite.getPosition().x;
		m_handleSprite.setPosition(newX, newY);

		newX = m_localBounds.height;
		newY = m_localBounds.width;
		m_localBounds.width = newX;
		m_localBounds.height = newY;
		
		newX = m_localBounds.top;
		newY = m_localBounds.left;
		m_localBounds.left = newX;
		m_localBounds.top = newY;
	}
}

void UISlider::SetSize(float length)
{
	sf::Vector2f s = m_slotShape.getSize();
	if(m_direction == Direction::Horizontal)
	{
		float pos = m_handleSprite.getPosition().x / s.x;
		pos *= length;
		m_handleSprite.setPosition(pos, m_handleSprite.getPosition().y);
		
		s.x = length;
		m_slotShape.setSize(s);
		m_localBounds.width = length;
	}
	else
	{
		float pos = m_handleSprite.getPosition().y / s.y;
		pos *= length;
		m_handleSprite.setPosition(m_handleSprite.getPosition().x, pos);
		
		s.y = length;
		m_slotShape.setSize(s);
		m_localBounds.height = length;
	}
	m_length = length;
}

float UISlider::GetValue() const
{
	if(m_direction == Direction::Horizontal)
	{
		return m_handleSprite.getPosition().x / m_length * m_maxValue;
	}
	else
	{
		return m_handleSprite.getPosition().y / m_length * m_maxValue;
	}
}

void UISlider::SetText(const std::string& text)
{
	m_text.setString(text);
	m_UpdateText();	
}

void UISlider::SetTextSize(sf::Uint16 size)
{
	m_text.setCharacterSize(size);
	m_UpdateText();
}

void UISlider::SetValue(float val)
{
	assert(val <= m_maxValue && val >= 0.f);
	sf::Vector2f pos = m_handleSprite.getPosition();
	if(m_direction == Direction::Horizontal)
	{
		pos.x = (m_length / m_maxValue) * val;
	}
	else
	{
		pos.y = (m_length / m_maxValue) * val;
	}
	m_handleSprite.setPosition(pos);
}
//private
void UISlider::draw(sf::RenderTarget& rt, sf::RenderStates states) const
{
	states.transform *= getTransform();
	states.blendMode = sf::BlendMultiply;
	rt.draw(m_slotShape, states);
	states.blendMode = sf::BlendAlpha;
	rt.draw(m_handleSprite, states);
	rt.draw(m_text, states);
}

void UISlider::m_UpdateText()
{
	Helpers::Position::CentreOrigin(m_text);
	m_text.setPosition(m_text.getOrigin().x, -m_text.getLocalBounds().height * 2.f);
}