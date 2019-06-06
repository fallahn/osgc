#include <Game/UITextBox.h>
#include <SFML/Graphics/RenderTexture.hpp>

namespace
{
	const float borderThickness = 2.f;
	const sf::Vector2f defaultSize(320.f, 70.f);
	const float flashTime = 0.8f;
	const float padding = 20.f;
}

using namespace Game;

//ctor
UITextBox::UITextBox(const sf::Font& font, const sf::Color& background, const sf::Color& border)
	: m_text	("", font, 45u),
	m_cursor	(sf::Vector2f(), sf::Vector2f(0.f, defaultSize.y * 0.75f), 2.f, border),
	m_showCursor(false),
	m_lastKey	(sf::Keyboard::Unknown)
{
	m_shape.setFillColor(background);
	m_shape.setOutlineColor(border);
	m_shape.setOutlineThickness(borderThickness);
	m_shape.setSize(defaultSize);
	m_shape.setOrigin(defaultSize / 2.f);

	m_text.setColor(border);
	m_text.setPosition(-(defaultSize / 2.f));
	m_text.move(padding, 0.f);
}

//public
void UITextBox::Update(float dt)
{
	sf::FloatRect bounds = m_text.getGlobalBounds();
	m_cursor.setPosition(bounds.left + bounds.width, -(m_cursor.getSize().y / 2.f));


	m_text.setString(m_string);
}

bool UITextBox::Selectable() const
{
	return true;
}

void UITextBox::Select()
{
	UIComponent::Select();
	//TODO highlight somehow

}

void UITextBox::Deselect()
{
	UIComponent::Deselect();
	//Deactivate();
}

void UITextBox::Activate()
{
	UIComponent::Activate();
	//enable text input
	m_showCursor = true;
}

void UITextBox::Deactivate()
{
	UIComponent::Deactivate();
	//disable text input
	m_showCursor = false;
}

void UITextBox::HandleEvent(const sf::Event& e)
{
	//handle text input events
	if(m_showCursor)
	{
		if(e.type == sf::Event::KeyReleased)
		{
			switch(e.key.code)
			{
			case sf::Keyboard::BackSpace:
				if(m_string.size())
					m_string.pop_back();
				return;
			default: break;
			}
		}

		else if(e.type == sf::Event::TextEntered)
		{
			//handle ascii chars only skipping delete and backspace
			if(e.text.unicode > 31
				&& e.text.unicode < 127) //TODO handle scrolling of text input
			{
				m_string += static_cast<char>(e.text.unicode);
			}
		}
	}
}

bool UITextBox::Contains(const sf::Vector2f& point) const
{
	return getTransform().transformRect(m_shape.getGlobalBounds()).contains(point);
}

const std::string& UITextBox::GetText() const
{
	return m_string;
}

void UITextBox::SetTexture(const sf::Texture& texture)
{
	m_shape.setTexture(&texture);
}

void UITextBox::ShowBorder(bool b)
{
	if(b)
		m_shape.setOutlineThickness(borderThickness);
	else
		m_shape.setOutlineThickness(0.f);
}

void UITextBox::SetSize(const sf::Vector2f& size)
{
	m_shape.setSize(size);
	m_shape.setOrigin(size / 2.f);
	m_cursor.setScale(1.f, size.y / defaultSize.y);

	sf::Uint8 cs = static_cast<sf::Uint8>(size.y * 0.75f);
	m_text.setCharacterSize(cs);
	m_text.setPosition(-(size / 2.f));
	m_text.move(padding, 0.f);
}

void UITextBox::SetText(const std::string& text)
{
	m_string = text;
}

//private
void UITextBox::draw(sf::RenderTarget& rt, sf::RenderStates states) const
{
	states.transform *= getTransform();
	rt.draw(m_shape, states);
	rt.draw(m_text, states);
	if(m_showCursor) rt.draw(m_cursor, states);
}