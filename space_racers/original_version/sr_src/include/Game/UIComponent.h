///base class for all UI components (buttons etc)///
#ifndef COMPONENT_H_
#define COMPONENT_H_

#include <SFML/System/NonCopyable.hpp>
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Transformable.hpp>

#include <memory>

//forward dec
namespace sf
{
	class Event;
}

namespace Game
{
	class UIComponent : public sf::Drawable, public sf::Transformable, private sf::NonCopyable
	{
	public:
		typedef std::shared_ptr<UIComponent> Ptr;

		UIComponent();
		virtual ~UIComponent();

		virtual void Update(float dt);

		virtual bool Selectable() const = 0;
		bool Selected() const;
		virtual void Select();
		virtual void Deselect();

		virtual bool Active() const;
		virtual void Activate();
		virtual void Deactivate();

		virtual void HandleEvent(const sf::Event& e) = 0;

		virtual bool Contains(const sf::Vector2f& point) const;

		void SetEnabled(bool b = true);
		bool Enabled() const;

	private:
		bool m_selected;
		bool m_active;
		bool m_enabled;
	};
}

#endif