#include <Game/UIContainer.h>

#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderStates.hpp>

#include <iostream>

using namespace Game;

UIContainer::UIContainer(const sf::RenderWindow& rw)
	: m_selectedChild	(-1),
	m_visible			(true),
	m_renderWindow		(rw)
{

}

//public
void UIContainer::Update(float dt)
{
	for(auto& c : m_children)
		c->Update(dt);
}

void UIContainer::AddComponent(UIComponent::Ptr component)
{
	m_children.push_back(component);

	if(!m_HasSelection() && component->Selectable())
		m_Select(m_children.size() - 1);
}

bool UIContainer::Selectable() const
{
	return false;
}

void UIContainer::HandleEvent(const sf::Event& e)
{
	if(!m_visible || !Enabled()) return;
	
	if(m_HasSelection() && m_children[m_selectedChild]->Active())
    {
		m_children[m_selectedChild]->HandleEvent(e);
    }
	//TODO handle controller input
    else if(e.type == sf::Event::KeyReleased)
    {
        if(e.key.code == sf::Keyboard::W || e.key.code == sf::Keyboard::Up)
        {
                m_SelectPrevious();
        }
        else if(e.key.code == sf::Keyboard::S || e.key.code == sf::Keyboard::Down)
        {
                m_SelectNext();
        }
        else if(e.key.code == sf::Keyboard::Return || e.key.code == sf::Keyboard::Space)
        {
            if(m_HasSelection())
				m_children[m_selectedChild]->Activate();
        }
    }

	//mouse input
	if(e.type == sf::Event::MouseMoved)
	{
		sf::Vector2f position = m_renderWindow.mapPixelToCoords(sf::Vector2i(e.mouseMove.x, e.mouseMove.y));
		for(auto i = 0u; i < m_children.size(); ++i)
		{
			if(m_children[i]->Contains(position))
				m_Select(i);
		}
	}

	if(e.type == sf::Event::MouseButtonReleased)
	{
		switch(e.mouseButton.button)
		{
		case sf::Mouse::Left:
			{
				sf::Vector2f position = m_renderWindow.mapPixelToCoords(sf::Mouse::getPosition(m_renderWindow));
				if(m_HasSelection() &&
					m_children[m_selectedChild]->Contains(position))
				{
					m_children[m_selectedChild]->Activate();
				}
			}
			break;
		case sf::Mouse::Middle:
			break;
		case sf::Mouse::Right:
			break;
		default: break;
		}
	}
}

void UIContainer::SetVisible(bool b)
{
	m_visible = b;
}

//private
bool UIContainer::m_HasSelection() const
{
	return (m_selectedChild >= 0);
}

void UIContainer::m_Select(sf::Int16 index)
{
	if(m_children[index]->Selectable())
	{
		if(m_HasSelection())
			m_children[m_selectedChild]->Deselect();

		m_children[index]->Select();
		m_selectedChild = index;
	}
}

void UIContainer::m_SelectNext()
{
	if(!m_HasSelection()) return;

	sf::Int16 next = m_selectedChild;
	do  //mod value so flips round to beginning
		next = (next + 1) % m_children.size();
	while
		(!m_children[next]->Selectable());

	m_Select(next);
}

void UIContainer::m_SelectPrevious()
{
	if(!m_HasSelection()) return;

	sf::Int16 prev = m_selectedChild;
	do
		prev = (prev + m_children.size() - 1) % m_children.size();
	while
		(!m_children[prev]->Selectable());

	m_Select(prev);
}

void UIContainer::draw(sf::RenderTarget& rt, sf::RenderStates states) const
{
	if(!m_visible) return;
	
	states.transform *= getTransform();

	for(const auto& c : m_children)
		rt.draw(*c, states);
}