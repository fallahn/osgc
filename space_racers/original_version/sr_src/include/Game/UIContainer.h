///ui container component//
#ifndef UI_CONTAINER_H_
#define UI_CONTAINER_H_

#include <SFML/Graphics/RenderWindow.hpp>
#include <Game/UIComponent.h>
#include <vector>


namespace Game
{
	class UIContainer : public UIComponent
	{
	public:
		typedef std::shared_ptr<UIContainer> Ptr;

		UIContainer(const sf::RenderWindow& rw);

		virtual void Update(float dt);
		void AddComponent(UIComponent::Ptr component);

		virtual bool Selectable() const;
		virtual void HandleEvent(const sf::Event& e);

		void SetVisible(bool b = true);

	private:
		std::vector<UIComponent::Ptr> m_children;
		sf::Int16 m_selectedChild;
		bool m_visible;

		bool m_HasSelection() const;
		void m_Select(sf::Int16 index);
		void m_SelectNext();
		void m_SelectPrevious();

		const sf::RenderWindow& m_renderWindow;

		virtual void draw(sf::RenderTarget& rt, sf::RenderStates states) const;
	};
}

#endif