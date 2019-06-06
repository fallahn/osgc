//text input box for UI//
#ifndef UI_TEXTBOX_H_
#define UI_TEXTBOX_H_

#include <Game/UIComponent.h>
#include <Game/LineSegment.h>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <string>

namespace Game
{
	class UITextBox : public UIComponent
	{
	public:
		typedef std::shared_ptr<UITextBox> Ptr;
		UITextBox(const sf::Font& font, const sf::Color& background = sf::Color::Black, const sf::Color& border = sf::Color::White);

		virtual void Update(float dt);

		virtual bool Selectable() const;
		virtual void Select();
		virtual void Deselect();

		virtual void Activate();
		virtual void Deactivate();

		virtual void HandleEvent(const sf::Event& e);

		virtual bool Contains(const sf::Vector2f& point) const;

		const std::string& GetText() const;
		void SetTexture(const sf::Texture& texture);
		void ShowBorder(bool b = true);
		void SetSize(const sf::Vector2f& size);
		void SetText(const std::string& text);
	private:

		std::string m_string;
		sf::Text m_text;
		sf::RectangleShape m_shape;
		LineSegment m_cursor;
		sf::Clock m_flashClock;
		bool m_showCursor;

		sf::Keyboard::Key m_lastKey;

		virtual void draw(sf::RenderTarget& rt, sf::RenderStates states) const;
	};
}

#endif