#include <Game/UICheckBox.h>
#include <Helpers.h>
#include <SFML/Graphics/RenderTarget.hpp>

using namespace Game;

UICheckBox::UICheckBox(const sf::Font& font, const sf::Texture& texture)
	: m_texture		(texture),
	m_text			("", font, 22u),
	m_checked		(false),
	m_alignment		(Alignment::Left)
{
	sf::IntRect subrect(0, 0, texture.getSize().x, texture.getSize().y / 4);
	m_subrects[State::Normal] = subrect;
	subrect.top += subrect.height;
	m_subrects[State::Selectected] = subrect;
	subrect.top += subrect.height;
	m_subrects[State::CheckedNormal] = subrect;
	subrect.top += subrect.height;
	m_subrects[State::CheckedSelected] = subrect;

	m_sprite.setTexture(m_texture);
	m_sprite.setTextureRect(m_subrects[State::Normal]);
	Helpers::Position::CentreOrigin(m_sprite);

	m_AlignText();
}

//public
void UICheckBox::SetText(const std::string& text)
{
	m_text.setString(text);
	m_AlignText();
}

void UICheckBox::SetTextSize(sf::Uint8 size)
{
	m_text.setCharacterSize(size);
	m_AlignText();
}

bool UICheckBox::Checked() const
{
	return m_checked;
}

void UICheckBox::Check(bool b)
{
	m_checked = b;
	Select();
}

void UICheckBox::SetAlignment(UICheckBox::Alignment alignment)
{
	m_alignment = alignment;
	m_AlignText();
}

bool UICheckBox::Selectable() const
{
	return true;
}

void UICheckBox::Select()
{
	UIComponent::Select();
	(m_checked) ? 
		m_sprite.setTextureRect(m_subrects[State::CheckedSelected])
		:
		m_sprite.setTextureRect(m_subrects[State::Selectected]);
}

void UICheckBox::Deselect()
{
	UIComponent::Deselect();
	(m_checked) ?
		m_sprite.setTextureRect(m_subrects[State::CheckedNormal])
		:
		m_sprite.setTextureRect(m_subrects[State::Normal]);
}

void UICheckBox::Activate()
{
	UIComponent::Activate();
	m_checked = !m_checked;
	Select();
}

void UICheckBox::Deactivate()
{
	UIComponent::Deactivate();
}

void UICheckBox::HandleEvent(const sf::Event& e)
{}

bool UICheckBox::Contains(const sf::Vector2f& point) const
{
	return getTransform().transformRect(m_sprite.getGlobalBounds()).contains(point);
}

//private
void UICheckBox::draw(sf::RenderTarget& rt, sf::RenderStates states) const
{
	states.transform *= getTransform();
	rt.draw(m_sprite, states);
	rt.draw(m_text, states);
}

void UICheckBox::m_AlignText()
{
	const float padding = static_cast<float>(m_texture.getSize().x);
	Helpers::Position::CentreOrigin(m_text);
	
	if(m_alignment == Alignment::Right)
		m_text.setPosition(padding + m_text.getOrigin().x, 0.f);
	else
		m_text.setPosition(-(padding + m_text.getOrigin().x), 0.f);
}