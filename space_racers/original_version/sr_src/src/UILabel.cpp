///source for label class //
#include <Game/UILabel.h>
#include <SFML/Graphics/RenderTarget.hpp>

using namespace Game;

//ctor
UILabel::UILabel(const std::string& text, const sf::Font& font)
	: m_text	(text, font, 16u)
{

}

//public
bool UILabel::Selectable() const
{
	return false;
}

void UILabel::HandleEvent(const sf::Event& e)
{
}

void UILabel::SetText(const std::string& text)
{
	m_text.setString(text);
}

//private
void UILabel::draw(sf::RenderTarget& rt, sf::RenderStates states) const
{
	states.transform *= getTransform();
	rt.draw(m_text, states);
}