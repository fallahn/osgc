#include <Game/UIButton.h>
#include <helpers.h>

#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderStates.hpp>

using namespace Game;

//ctor
UIButton::UIButton(const sf::Font& font, const sf::Texture& texture)
	: m_texture			(texture),
	m_text				("", font, 20u),
	m_toggle			(false)
{
	sf::IntRect subrect(0, 0, texture.getSize().x, texture.getSize().y / 3);
	m_subrects[State::ButtonNormal] = subrect;
	subrect.top += subrect.height;
	m_subrects[State::ButtonSelected] = subrect;
	subrect.top += subrect.height;
	m_subrects[State::ButtonActive] = subrect;
	
	m_sprite.setTexture(m_texture);
	m_sprite.setTextureRect(m_subrects[State::ButtonNormal]);
	setOrigin(GetSize() / 2.f);

	sf::FloatRect bounds = m_sprite.getLocalBounds();
	m_text.setPosition(bounds.width / 2.f, bounds.height / 2.f);
}

//public
void UIButton::SetCallback(Callback callback)
{	
	m_callback = std::move(callback);
}

void UIButton::SetText(const std::string& text)
{
	m_text.setString(text);
	Helpers::Position::CentreOrigin(m_text);
}

void UIButton::SetToggle(bool b)
{
	m_toggle = b;
}

bool UIButton::Selectable() const
{
	return true;
}

void UIButton::Select()
{
	UIComponent::Select();
	m_sprite.setTextureRect(m_subrects[State::ButtonSelected]);
}

void UIButton::Deselect()
{
	UIComponent::Deselect();
	m_sprite.setTextureRect(m_subrects[State::ButtonNormal]);
}

void UIButton::Activate()
{
	UIComponent::Activate();
	if(m_toggle) m_sprite.setTextureRect(m_subrects[State::ButtonActive]);

	if(m_callback) m_callback();

	if(!m_toggle) Deactivate();
}

void UIButton::Deactivate()
{
	UIComponent::Deactivate();

	if(m_toggle)
	{
		if(Selected())
			m_sprite.setTextureRect(m_subrects[State::ButtonSelected]);
		else
			m_sprite.setTextureRect(m_subrects[State::ButtonNormal]);
	}
}

void UIButton::HandleEvent(const sf::Event& e)
{}

bool UIButton::Contains(const sf::Vector2f& point) const
{
	sf::FloatRect bounds = getTransform().transformRect(m_sprite.getGlobalBounds());
	return bounds.contains(point);
}

sf::Vector2f UIButton::GetSize() const
{
	return sf::Vector2f(static_cast<float>(m_subrects[0].width), static_cast<float>(m_subrects[0].height));
}

//private
void UIButton::draw(sf::RenderTarget& rt, sf::RenderStates states) const
{
	states.transform *= getTransform();
	rt.draw(m_sprite, states);
	rt.draw(m_text, states);
}