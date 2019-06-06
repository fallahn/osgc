///header file UIButton component///
#ifndef UI_BUTTON_H_
#define UI_BUTTON_H_

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>

#include <Game/UIComponent.h>
#include <functional>
#include <array>

namespace Game
{
	class UIButton : public UIComponent
	{
	public:
		typedef std::function<void()> Callback;
		typedef std::shared_ptr<UIButton> Ptr;

		UIButton(const sf::Font& font, const sf::Texture& texture);

		void SetCallback(Callback callback);
		void SetText(const std::string& text);
		void SetToggle(bool b);

		virtual bool Selectable() const;
		virtual void Select();
		virtual void Deselect();

		virtual void Activate();
		virtual void Deactivate();

		virtual void HandleEvent(const sf::Event& e);

		virtual bool Contains(const sf::Vector2f& point) const;

		sf::Vector2f GetSize() const;
	private:
		enum State
		{
			ButtonNormal   = 0u,
			ButtonSelected = 1u,
			ButtonActive   = 2u
		};

		Callback m_callback;

		const sf::Texture& m_texture;
		sf::Sprite m_sprite;
		sf::Text m_text;
		std::array<sf::IntRect, 3> m_subrects;

		bool m_toggle;

		virtual void draw(sf::RenderTarget& rt, sf::RenderStates states) const;
	};
}

#endif