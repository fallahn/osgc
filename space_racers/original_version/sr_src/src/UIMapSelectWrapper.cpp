#include <Game/MapSelect.h>
#include <Game/UIMapSelectWrapper.h>

#include <SFML/Window/Event.hpp>

using namespace Game;

//ctor
UIMapSelectWrapper::UIMapSelectWrapper(MapSelection& ms)
	: m_mapSelection	(ms)
{

}

//public
bool UIMapSelectWrapper::Selectable() const
{
	return true;
}

void UIMapSelectWrapper::Select()
{
	UIComponent::Select();

}

void UIMapSelectWrapper::Deselect()
{
	UIComponent::Deselect();
}

void UIMapSelectWrapper::Activate()
{
	UIComponent::Activate();
	m_mapSelection.Activate();
}

void UIMapSelectWrapper::Deactivate()
{
	UIComponent::Deactivate();
}

void UIMapSelectWrapper::HandleEvent(const sf::Event& e)
{

}

bool UIMapSelectWrapper::Contains(const sf::Vector2f& point) const
{
	return m_mapSelection.Contains(point);
}

//private
void UIMapSelectWrapper::draw(sf::RenderTarget& rt, sf::RenderStates states) const
{}