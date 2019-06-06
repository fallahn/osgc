///surce for UIComponent base class///
#include <Game/UIComponent.h>

using namespace Game;

//ctor/dtor
UIComponent::UIComponent()
	: m_selected	(false),
	m_active		(false),
	m_enabled		(true)
{

}

UIComponent::~UIComponent()
{

}

//public
void UIComponent::Update(float dt)
{

}

bool UIComponent::Selected() const
{
	return m_selected;
}

void UIComponent::Select()
{
	m_selected = true;
}

void UIComponent::Deselect()
{
	m_selected = false;
}

bool UIComponent::Active() const
{
	return m_active;
}

void UIComponent::Activate()
{
	m_active = true;
}

void UIComponent::Deactivate()
{
	m_active = false;
}

bool UIComponent::Contains(const sf::Vector2f& point) const
{
	return false;
}

void UIComponent::SetEnabled(bool b)
{
	m_enabled = b;
}

bool UIComponent::Enabled() const
{
	return m_enabled;
}